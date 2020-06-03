//
// Created by 刘立悟 on 2020/5/18.
//

#include "db.h"
#include "config.h"
#include "common.h"
#include "log.h"
#include "client.h"
#include "object.h"
#include "json.h"
#include "server.h"

#include <sys/stat.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace tLBS;

DEFINE_int32(db_num, 4, "数据库的数量");
DEFINE_string(db_root, "", "数据文件存放的路径");

std::vector<Db *> Db::dbs;

Db::Db(int id) {
    this->id = id;
    this->dirty = 0;
    this->resetSaveParams();
    char buf[256];
    snprintf(buf, sizeof(buf), "db#%0.2d", id);
    this->info = buf;
    this->lastSave = time(nullptr);
    info("创建") << this->getInfo();
}

Db::SaveParam::SaveParam(time_t seconds, int changes) {
    this->seconds = seconds;
    this->changes = changes;
}

time_t Db::SaveParam::getSeconds() {
    return this->seconds;
}

int Db::SaveParam::getChanges() {
    return this->changes;
}

Db::~Db() {
    info("销毁") << this->getInfo();
}

void Db::appendSaveParam(time_t seconds, int changes) {
    this->saveParams.push_back(new Db::SaveParam(seconds, changes));
}

void Db::resetSaveParams() {
    this->saveParams.clear();
}

std::vector<Db::SaveParam *> Db::getSaveParams() {
    return this->saveParams;
}

int Db::getId() {
    return this->id;
}

std::string Db::getInfo() {
    return this->info;
}

std::string Db::getDataPath() {
    return FLAGS_db_root + this->getInfo();
}

void Db::init() {
    if (FLAGS_db_root.size() == 0) {
        // 如果没有配置过数据文件存放的根路径 配置为默认的根路径
        FLAGS_db_root = getAbsolutePath("../data/");
    }
    warning("db root: ") << FLAGS_db_root;
    for (int i = 0; i < FLAGS_db_num; i++) {
        Db *db = new Db(i);
        dbs.push_back(db);
        db->appendSaveParam(60 * 60,1); // 1小时1次修改
        db->appendSaveParam(5 * 60,100); // 5分钟100次修改
        db->appendSaveParam(60,10000); // 1分钟10000次修改
        // 测试用
        db->appendSaveParam(10,1); // 10秒钟1次修改
        const char *dbDataPath = db->getDataPath().c_str();
        if (access(dbDataPath, F_OK) != 0) {
            // 不存在
            if (mkdir(dbDataPath, S_IXUSR | S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) {
                fatal("无法创建数据库目录: ") << strerror(errno);
            }
        }
    }
}

void Db::free() {
    saveAll();
    for (int i = 0; i < FLAGS_db_num; i++) {
        Db *db = dbs[i];
        delete db;
    }
    dbs.clear();
}

time_t Db::getLastSave() {
    return this->lastSave;
}

void Db::save() {
    warning("准备保存") << this->getInfo() << "数据到磁盘";
    if (this->getDirty() > 0) {
        // 有数据发生变化
        info(this->getInfo()) << "有数据变化, 开始保存";
        for (auto mapIter = this->tables.begin(); mapIter != this->tables.end(); mapIter++) {
            std::string tableName = mapIter->first;
            Table *tableObj = mapIter->second;

        }
        this->lastSave = time(nullptr);
    }
    else {
        info(this->getInfo()) << "无数据变化, 保存结束";
    }
}

void Db::saveAll() {
    warning("保存所有db数据到磁盘");
    for (int i = 0; i < FLAGS_db_num; i++) {
        dbs[i]->save();
    }
}

Db* Db::getDb(int id) {
    if (id <= dbs.size()) {
        return dbs[id];
    }
    else {
        return nullptr;
    }
}


int Db::cmdDb(tLBS::Client *client) {
    if (client->getArgs().size() == 2) {
        std::string arg = client->arg(1);
        int dbId = atoi(arg.c_str());
        if (dbId < dbs.size()) {
            client->setDb(dbs[dbId]);
            info(client->getInfo()) << "选择数据库#" << dbId;
            client->success();
        }
        else {
            client->fail(Json::createCmdErrorJsonObj(ERRNO_CMD_DB_SELECT_ERR, ERROR_CMD_DB_SELECT_ERR));
        }
    }
    else {
        Json* json = Json::createCmdSuccessNumberJsonObj();
        json->get("data")->SetInt(client->getDb()->getId());
        client->success(json);
    }
    return C_OK;
}


Table* Db::lookupTable(std::string key, int flags) {
    UNUSED(flags);
    auto mapIter = this->tables.find(key);
    if (mapIter != this->tables.end()) {
        return mapIter->second;
    }
    return nullptr;
}

bool Db::tableExists(std::string key) {
    auto mapIter = this->tables.find(key);
    return mapIter != this->tables.end();
}

void Db::tableAdd(std::string key, tLBS::Table *table) {
    this->tables[key] = table;
}

void Db::tableRemove(std::string key) {
    auto mapIter = this->tables.find(key);
    if (mapIter != this->tables.end()) {
        this->tables.erase(key);
    }
}

Table* Db::lookupTableRead(std::string key) {
    return this->lookupTableReadWithFlags(key, DB_FLAGS_LOOKUP_NONE);
}

Table* Db::lookupTableWrite(std::string key) {
    return this->lookupTableWriteWithFlags(key, DB_FLAGS_LOOKUP_NONE);
}

Table* Db::lookupTableReadWithFlags(std::string key, int flags) {
    // 添加读的额外flags
    return this->lookupTable(key, flags);
}

Table* Db::lookupTableWriteWithFlags(std::string key, int flags) {
    // 添加写的额外flags
    return this->lookupTable(key, flags);
}

void Db::incrDirty(int incr) {
    this->dirty += incr;
}

void Db::decrDirty(int decr) {
    this->dirty -= decr;
}

void Db::resetDirty() {
    this->dirty = 0;
}

int Db::getDirty() {
    return this->dirty;
}


std::string Db::getTmpFile() {
    char tmpFile[256];
    snprintf(tmpFile, sizeof(tmpFile), "tmp-%d-%d.dat", this->getId(), ::getpid());
    return tmpFile;
}

std::string Db::getDatFile() {
    char tmpFile[256];
    snprintf(tmpFile, sizeof(tmpFile), "db#%0.2d.dat", this->getId());
    return tmpFile;
}


void Db::cron(long long id, void *data) {
    Server *server = Server::getInstance();
    UNUSED(id);
    UNUSED(data);
    // 保存数据
    for (int i = 0; i < FLAGS_db_num; i++) {
        Db *db = dbs[i];
        std::vector<SaveParam *> dbSaveParams = db->getSaveParams();
        for (int j = 0; j < dbSaveParams.size(); j++) {
            SaveParam *sp = dbSaveParams[i];
            if (db->getDirty() >= sp->getChanges() &&
                server->getUnixTime() - db->getLastSave() > sp->getSeconds() ) {
                warning(sp->getChanges()) << " changes in " << sp->getSeconds() << " seconds."
                    << db->getInfo() << " going AsyncSaving...";
                // 异步保存数据
                break;
            }
        }
    }
}