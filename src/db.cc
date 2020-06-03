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
}

Db::~Db() {

}

int Db::getId() {
    return this->id;
}

void Db::init() {
    if (FLAGS_db_root.size() == 0) {
        // 如果没有配置过数据文件存放的根路径 配置为默认的根路径
        FLAGS_db_root = getAbsolutePath("../data/");
    }
    warning("db root: ") << FLAGS_db_root;
    for (int i = 0; i < FLAGS_db_num; i++) {
        dbs.push_back(new Db(i));
        char dbDataDir[256];
        const char *fmt = (FLAGS_db_root + "db#%0.2d").c_str();
        snprintf(dbDataDir, sizeof(dbDataDir), fmt, i);
        if (access((const char *)dbDataDir, F_OK) != 0) {
            // 不存在
            if (mkdir(dbDataDir, S_IXUSR | S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) {
                fatal("无法创建数据库目录: ") << strerror(errno);
            }
        }
    }
}

void Db::free() {

}

void Db::save() {

}

Db* Db::getDb(int id) {
    if (id <= dbs.size()) {
        return dbs[id];
    }
    else {
        return nullptr;
    }
}


int Db::dbSelect(tLBS::Client *client) {
    std::string arg = client->arg(1);
    int dbId = atoi(arg.c_str());
    if (dbId < dbs.size()) {
        client->setDb(dbs[dbId]);
        client->success();
        info(client->getInfo()) << "选择数据库#" << dbId;
    }
    else {
        client->fail(ERRNO_CMD_DB_SELECT_ERR, ERROR_CMD_DB_SELECT_ERR);
    }
    return C_OK;
}

int Db::db(tLBS::Client *client) {
    if (client->getFormat() == ClientFormat::CLIENT_FORMAT_LEGACY) {
        char msg[1024];
        snprintf(msg, sizeof(msg), "%d", client->getDb()->getId());
        client->success(msg);
    }
    else {
        Json* json = Json::createCmdSuccessNumberJsonObj();
        json->get("data")->SetInt(client->getDb()->getId());
        client->success(json);
    }
    return C_OK;
}


Object* Db::lookupKey(std::string key, int flags) {
    UNUSED(flags);
    auto mapIter = this->table.find(key);
    if (mapIter != this->table.end()) {
        return mapIter->second;
    }
    return nullptr;
}

bool Db::tableExists(std::string key) {
    auto mapIter = this->table.find(key);
    return mapIter != this->table.end();
}

void Db::tableAdd(std::string key, tLBS::Object *data) {
    this->table[key] = data;
}

void Db::tableRemove(std::string key) {
    auto mapIter = this->table.find(key);
    if (mapIter != this->table.end()) {
        this->table.erase(key);
    }
}

Object* Db::lookupKeyRead(std::string key) {
    return this->lookupKeyReadWithFlags(key, DB_FLAGS_LOOKUP_NONE);
}

Object* Db::lookupKeyWrite(std::string key) {
    return this->lookupKeyWriteWithFlags(key, DB_FLAGS_LOOKUP_NONE);
}

Object* Db::lookupKeyReadWithFlags(std::string key, int flags) {
    // 添加读的额外flags
    return this->lookupKey(key, flags);
}

Object* Db::lookupKeyWriteWithFlags(std::string key, int flags) {
    // 添加写的额外flags
    return this->lookupKey(key, flags);
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