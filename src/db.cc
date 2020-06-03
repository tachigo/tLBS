//
// Created by 刘立悟 on 2020/5/18.
//

#include "db.h"
#include "config.h"
#include "common.h"
#include "log.h"

#include <sys/stat.h>

using namespace tLBS;

DEFINE_int32(db_num, 4, "数据库的数量");
DEFINE_string(db_root, "", "数据文件存放的路径");

std::vector<Db *> Db::dbs;

Db::Db(int id) {
    this->id = id;
}

Db::~Db() {

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
//        info(dbDataDir);
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