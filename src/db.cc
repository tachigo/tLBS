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
#include "table.h"

#include <sys/stat.h>
#include <fstream>

using namespace tLBS;

DEFINE_int32(db_num, 4, "数据库的数量");
DEFINE_string(db_root, "", "数据文件存放的路径");

std::vector<Db *> Db::dbs;

Db::Db(int id) {
    this->id = id;
    this->resetSaveParams();
    char buf[256];
    snprintf(buf, sizeof(buf), "node[%0.5d]db#%0.2d", atoi(FLAGS_tcp_port.c_str()), id);
    this->info = buf;
//    info("创建") << this->getInfo();
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
//    info("销毁") << this->getInfo();
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
    return FLAGS_db_root + this->getInfo() + "/";
}

void Db::init() {
    if (FLAGS_db_root.size() == 0) {
        // 如果没有配置过数据文件存放的根路径 配置为默认的根路径
        FLAGS_db_root = getAbsolutePath("../data/");
    }
    const char *dbDataRoot = FLAGS_db_root.c_str();
    if (access(dbDataRoot, F_OK) != 0) {
        // 不存在
        if (mkdir(dbDataRoot, S_IXUSR | S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) {
            fatal("无法创建数据库目录: ") << strerror(errno);
        }
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
    // 加载数据
    loadAll();
}

void Db::free() {
    saveAll();
    for (int i = 0; i < FLAGS_db_num; i++) {
        Db *db = dbs[i];
        delete db;
    }
    dbs.clear();
}

std::vector<Db *> Db::getDbs() {
    return dbs;
}

void* Db::threadProcess(void *arg) {
    UNUSED(arg);
    while (true) {
        sleep(1);
        Server *server = Server::getInstance();
        for (int i = 0; i < FLAGS_db_num; i++) {
            Db *db = dbs[i];
            std::vector<SaveParam *> dbSaveParams = db->getSaveParams();
            std::map<std::string, Table *> tables = db->getTables();
            bool needSave = false;
            for (auto mapIter = tables.begin(); mapIter != tables.end(); mapIter++) {
                std::string tableName = mapIter->first;
                Table *tableObj = mapIter->second;
                for (int j = 0; j < dbSaveParams.size(); j++) {
                    SaveParam *sp = dbSaveParams[j];
                    if (tableObj->getDirty() >= sp->getChanges() &&
                        server->getUnixTime() - tableObj->getLastSave() > sp->getSeconds() ) {
                        warning(tableObj->getInfo()) << " " << sp->getSeconds() << "秒内有"
                                               << sp->getChanges() << "数据变化，即将开始保存数据...";
                        needSave = true;
                        break;
                    }
                }
            }
            if (needSave) {
                db->save();
            }
        }
    }
}

void Db::saveAll() {
    warning("保存所有db数据到磁盘");
    for (int i = 0; i < FLAGS_db_num; i++) {
        dbs[i]->save();
    }
}

void Db::loadAll() {
    warning("从磁盘加载所有db数据");
    for (int i = 0; i < FLAGS_db_num; i++) {
        dbs[i]->load();
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


int Db::execDb(Connection *conn, std::vector<std::string> args) {
    if (args.size() == 2) {
        std::string arg = args[1];
        int dbId = atoi(arg.c_str());
        if (dbId < dbs.size()) {
            conn->setDb(dbs[dbId]);
            info(conn->getInfo()) << "选择数据库#" << dbId;
            conn->success(Json::createSuccessStringJsonObj("OK"));
        }
        else {
            conn->fail(Json::createErrorJsonObj(ERRNO_EXEC_DB_SELECT_ERR, ERROR_EXEC_DB_SELECT_ERR));
        }
    }
    else {
        Json* json = Json::createSuccessNumberJsonObj();
        json->get("data").SetInt(conn->getDb()->getId());
        conn->success(json);
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

std::map<std::string, Table *> Db::getTables() {
    return this->tables;
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

std::string Db::getTmpFile() {
    char tmpFile[256];
    snprintf(tmpFile, sizeof(tmpFile), "node[%0.5d]db#%0.2d-%d.tmp", atoi(FLAGS_tcp_port.c_str()), this->getId(), ::getpid());
    return tmpFile;
}

std::string Db::getDatFile() {
    char tmpFile[256];
    snprintf(tmpFile, sizeof(tmpFile), "node[%0.5d]db#%0.2d.dat", atoi(FLAGS_tcp_port.c_str()), this->getId());
    return tmpFile;
}


void Db::load() {
    std::string datFile = FLAGS_db_root + this->getDatFile();
    std::ifstream ifs(datFile);
    if (!ifs) {
        if (errno == ENOENT) {
            // 文件不存在
//            error("无法打开" + this->getInfo() + "磁盘数据文件" + datFile)
//                    << ": " << strerror(errno) << "(" << errno << ")";
        }
        else {
            fatal("无法打开" + this->getInfo() + "磁盘数据文件" + datFile)
                    << ": " << strerror(errno) << "(" << errno << ")";
        }
    }
    else {
        std::string line;
        while (std::getline(ifs, line)) {
            Table *table = Table::parseMetadata(line);
            this->tableAdd(table->getName(), table);
            table->callLoaderHandler(this->getDataPath());
        }
        ifs.close();
    }

}

void Db::save() {
    // 先保存db的table的metadata在db_root目录下 再保存各个table里面的数据
    std::string tmpFile = FLAGS_db_root + this->getTmpFile();
    std::string datFile = FLAGS_db_root + this->getDatFile();
    std::ofstream ofs;
    ofs.open(tmpFile, std::ios::out | std::ios::trunc);
    for (auto mapIter = this->tables.begin(); mapIter != this->tables.end(); mapIter++) {
        std::string tableName = mapIter->first;
        Table *tableObj = mapIter->second;
        ofs << tableObj->getMetadata() << std::endl;
    }
    ofs.close();
    if (rename(tmpFile.c_str(), datFile.c_str()) == -1) {
        error("将临时文件") << tmpFile << "移动到最终文件" << datFile << "失败!";
        unlink(tmpFile.c_str());
        return;
    }
    for (auto mapIter = this->tables.begin(); mapIter != this->tables.end(); mapIter++) {
        std::string tableName = mapIter->first;
        Table *tableObj = mapIter->second;
        if (tableObj->callSaverHandler(this->getDataPath()) != C_OK) {
            // 保存失败了
            error(tableObj->getInfo()) << "保存失败";
        }
    }
}