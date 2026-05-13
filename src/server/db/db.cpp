#include "db.h"
#include "connectionpool.hpp"
#include "config.hpp"
#include <muduo/base/Logging.h>

static bool poolInitialized = false;

// 初始化数据库连接
MySQL::MySQL() : _conn(nullptr) {}

// 释放数据库连接资源（归还连接到池，而非关闭）
MySQL::~MySQL()
{
    if (_conn != nullptr)
        ConnectionPool::instance()->release(_conn);
}

// 连接数据库（从连接池获取连接）
bool MySQL::connect()
{
    if (!poolInitialized)
    {
        auto *cfg = Config::instance();
        ConnectionPool::instance()->init(
            cfg->mysqlHost(), cfg->mysqlPort(),
            cfg->mysqlUser(), cfg->mysqlPassword(),
            cfg->mysqlDatabase(), cfg->mysqlPoolSize());
        poolInitialized = true;
    }

    _conn = ConnectionPool::instance()->acquire();
    return _conn != nullptr;
}

// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!";
        return false;
    }

    return true;
}

// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    
    return mysql_use_result(_conn);
}

// 转义字符串，防止 SQL 注入
string MySQL::escape(const string &str)
{
    if (_conn == nullptr) return "";
    char *buf = new char[str.length() * 2 + 1];
    mysql_real_escape_string(_conn, buf, str.c_str(), str.length());
    string result(buf);
    delete[] buf;
    return result;
}

// 获取连接
MYSQL* MySQL::getConnection()
{
    return _conn;
}