//
// Created by liuliwu on 2020-06-18.
//

#include "t_hashmap.h"

#include "common.h"
#include "log.h"
#include "connection.h"
#include "db.h"
#include "table.h"

#include <fstream>
#include <sstream>

using namespace tLBS;

HashMap::StringStringHashMap::StringStringHashMap() {
    this->map.clear();
}

HashMap::StringStringHashMap::~StringStringHashMap() {
    this->map.clear();
}

uint64_t HashMap::StringStringHashMap::getSize() {
    return this->map.size();
}

std::string HashMap::StringStringHashMap::getDatFile(std::string table, int shard) {
    char datFile[1024];
    snprintf(datFile, sizeof(datFile),"ss_hashmap<%s>-%0.2d.dat", table.c_str(), shard);
    return datFile;
}


std::string HashMap::StringStringHashMap::getTmpFile(std::string table, int shard) {
    char tmpFile[1024];
    snprintf(tmpFile, sizeof(tmpFile), "ss_hashmap<%s>-%0.2d-%d.tmp", table.c_str(), shard, ::getpid());
    return tmpFile;
}

int HashMap::StringStringHashMap::dumper(std::string dataRootPath, std::string table, int shards,
                                         void *ptr) {
    auto data = (StringStringHashMap *)ptr;
    return data->dump(dataRootPath, table, shards);
}

int HashMap::StringStringHashMap::loader(std::string dataRootPath, std::string table, int shards,
                                         void *ptr) {
    auto data = (StringStringHashMap *)ptr;
    return data->load(dataRootPath, table, shards);
}

int HashMap::StringStringHashMap::sender(std::string dataRootPath, std::string table, int shards, void *ptr,
                                         std::string prefix, tLBS::Connection *conn) {
    auto data = (StringStringHashMap *)ptr;
    return data->send(dataRootPath, table, shards, prefix, conn);
}

int HashMap::StringStringHashMap::receiver(void *ptr, std::string line) {
    auto data = (StringStringHashMap *)ptr;
    return data->receive(line);
}


int HashMap::StringStringHashMap::dump(std::string dataRootPath, std::string table, int shards) {
    UNUSED(shards);
    int shard = 0;
    std::ofstream ofs;
    std::string tmpFile = dataRootPath + this->getTmpFile(table, shard);
    ofs.open(tmpFile, std::ios::out | std::ios::trunc);

    for (auto mapIter = this->map.begin(); mapIter != this->map.end(); mapIter++) {
        std::string key = mapIter->first;
        std::string value = mapIter->second;
        ofs << key << '\31' << value << std::endl;
    }

    std::string datFile = dataRootPath + this->getDatFile(table, shard);
    if (rename(tmpFile.c_str(), datFile.c_str()) == -1) {
        error("将临时文件") << tmpFile << "移动到最终文件" << datFile << "失败!";
        unlink(tmpFile.c_str());
        goto err;
    }
    return C_OK;

err:
    ofs.close();
    unlink(tmpFile.c_str());
    return C_ERR;
}


int HashMap::StringStringHashMap::load(std::string dataRootPath, std::string table, int shards) {
    UNUSED(shards);
    int shard = 0;
    std::ifstream ifs;
    std::string datFile = dataRootPath + this->getDatFile(table, shard);
    ifs.open(datFile, std::ios::in);
    if (!ifs) {
        if (errno == ENOENT) {
            // 文件不存在
            error("无法打开磁盘数据文件" + datFile)
                    << ": " << strerror(errno) << "(" << errno << ")";
        }
        else {
            fatal("无法打开磁盘数据文件" + datFile)
                    << ": " << strerror(errno) << "(" << errno << ")";
        }
    }
    else {
        std::string line;
        while (getline(ifs, line)) {
            std::vector<std::string> v = splitString(line, '\31');
            std::string key = v[0];
            std::string value = v[1];
            this->map[key] = value;
        }
        ifs.close();
    }
    return C_OK;
}

int HashMap::StringStringHashMap::send(std::string dataRootPath, std::string table, int shards,
                                       std::string prefix, tLBS::Connection *conn) {
    UNUSED(shards);
    int shard = 0;
    std::ifstream ifs;
    std::string datFile = dataRootPath + this->getDatFile(table, shard);
    ifs.open(datFile, std::ios::in);
    if (!ifs) {
        if (errno == ENOENT) {
            // 文件不存在
            error("无法打开磁盘数据文件" + datFile)
                    << ": " << strerror(errno) << "(" << errno << ")";
        }
        else {
            fatal("无法打开磁盘数据文件" + datFile)
                    << ": " << strerror(errno) << "(" << errno << ")";
        }
    }
    else {
        std::string line;
        while (getline(ifs, line)) {
            line = prefix + " " + line + '\n'; // 每行增加换行符
            conn->write(line.c_str(), line.size());
        }
        ifs.close();
    }
    return C_OK;
}


int HashMap::StringStringHashMap::receive(std::string line) {
    std::vector<std::string> v = splitString(line, '+');
    std::string key = v[0];
    std::string value = v[1];
    this->map[key] = value;
    return C_OK;
}

std::string HashMap::StringStringHashMap::get(std::string key) {
    auto mapIter = this->map.find(key);
    if (mapIter != this->map.end()) {
        return mapIter->second;
    }
    return nullptr;
}

void HashMap::StringStringHashMap::clear() {
    this->map.clear();
}

int HashMap::StringStringHashMap::set(std::string key, std::string value) {
    this->map[key] = value;
    return C_OK;
}

void HashMap::StringStringHashMap::del(std::string key) {
    auto mapIter = this->map.find(key);
    if (mapIter != this->map.end()) {
        this->map.erase(mapIter);
    }
}

// sshmset table key value
int HashMap::execSSHashMapSet(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 4) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];
    std::string key = args[2];
    std::string value = args[3];

    tableObj = conn->getDb()->lookupTableWrite(table);
    if (tableObj == nullptr) {
        tableObj = Table::createSSHashMapTable(
                conn->getDb()->getId(),
                table);
        conn->getDb()->tableAdd(table, tableObj);
    }
    if (tableObj->getType() != OBJ_TYPE_HASH_MAP) {
        return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
    }
    else if (tableObj->getEncoding() != OBJ_ENCODING_STRING_STRING_HASH_MAP) {
        return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
    }
    auto obj = (HashMap::StringStringHashMap *)tableObj->getData();
    obj->set(key, value);
    tableObj->incrDirty(1);
    return conn->success();
}

// sshmget table key
int HashMap::execSSHashMapGet(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 3) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];
    std::string key = args[2];

    tableObj = conn->getDb()->lookupTableRead(table);
    if (tableObj == nullptr) {
        return conn->success();
    }
    else {
        if (tableObj->getType() != OBJ_TYPE_HASH_MAP) {
            return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
        }
        else if (tableObj->getEncoding() != OBJ_ENCODING_STRING_STRING_HASH_MAP) {
            return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
        }
        auto obj = (HashMap::StringStringHashMap *)tableObj->getData();
        std::string value = obj->get(key);
        return conn->success(Json::createSuccessStringJsonObj(value.c_str()));
    }
    return C_OK; // never reached
}

// sshmdel table key
int HashMap::execSSHashMapDel(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 3) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];
    std::string key = args[2];

    tableObj = conn->getDb()->lookupTableWrite(table);
    if (tableObj == nullptr) {
        return conn->success();
    }
    else {
        if (tableObj->getType() != OBJ_TYPE_HASH_MAP) {
            return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
        }
        else if (tableObj->getEncoding() != OBJ_ENCODING_STRING_STRING_HASH_MAP) {
            return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
        }
        auto obj = (HashMap::StringStringHashMap *)tableObj->getData();
        obj->del(key);
        tableObj->incrDirty(1);
        return conn->success();
    }
    return C_OK; // never reached
}

// sshmclear table
int HashMap::execSSHashMapClear(tLBS::Exec *exec, tLBS::Connection *conn, std::vector<std::string> args) {
    UNUSED(exec);
    if (args.size() != 2) {
        return conn->fail(ERRNO_EXEC_SYNTAX_ERR, ERROR_EXEC_SYNTAX_ERR);
    }
    Table *tableObj;
    std::string table = args[1];

    tableObj = conn->getDb()->lookupTableWrite(table);
    if (tableObj == nullptr) {
        return conn->success();
    }
    else {
        if (tableObj->getType() != OBJ_TYPE_HASH_MAP) {
            return conn->fail(ERRNO_EXEC_TABLE_TYPE_ERR, ERROR_EXEC_TABLE_TYPE_ERR);
        }
        else if (tableObj->getEncoding() != OBJ_ENCODING_STRING_STRING_HASH_MAP) {
            return conn->fail(ERRNO_EXEC_TABLE_ENCODING_ERR, ERROR_EXEC_TABLE_ENCODING_ERR);
        }
        auto obj = (HashMap::StringStringHashMap *)tableObj->getData();
        obj->clear();
        tableObj->incrDirty(1);
        return conn->success();
    }
    return C_OK; // never reached
}


