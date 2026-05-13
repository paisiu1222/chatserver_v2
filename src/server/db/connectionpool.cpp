#include "connectionpool.hpp"
#include <muduo/base/Logging.h>
#include <iostream>

ConnectionPool *ConnectionPool::instance()
{
    static ConnectionPool pool;
    return &pool;
}

ConnectionPool::~ConnectionPool()
{
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_pool.empty())
    {
        mysql_close(_pool.front());
        _pool.pop();
    }
}

MYSQL *ConnectionPool::createConnection()
{
    MYSQL *conn = mysql_init(nullptr);
    if (conn == nullptr) return nullptr;

    MYSQL *p = mysql_real_connect(conn, _host.c_str(), _user.c_str(),
                                   _password.c_str(), _dbname.c_str(),
                                   _port, nullptr, 0);
    if (p != nullptr)
    {
        mysql_query(conn, "set names gbk");
        LOG_INFO << "connection pool: new connection created";
        return conn;
    }

    mysql_close(conn);
    return nullptr;
}

void ConnectionPool::init(const std::string &host, int port,
                           const std::string &user, const std::string &password,
                           const std::string &dbname, int poolSize)
{
    _host = host;
    _port = port;
    _user = user;
    _password = password;
    _dbname = dbname;
    _poolSize = poolSize;

    for (int i = 0; i < _poolSize; ++i)
    {
        MYSQL *conn = createConnection();
        if (conn)
            _pool.push(conn);
    }

    LOG_INFO << "connection pool initialized, size: " << _pool.size();
}

MYSQL *ConnectionPool::acquire()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait(lock, [this] { return !_pool.empty(); });

    MYSQL *conn = _pool.front();
    _pool.pop();
    return conn;
}

void ConnectionPool::release(MYSQL *conn)
{
    if (conn == nullptr) return;

    std::lock_guard<std::mutex> lock(_mutex);
    _pool.push(conn);
    _cv.notify_one();
}
