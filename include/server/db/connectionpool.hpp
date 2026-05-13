#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

class ConnectionPool
{
public:
    static ConnectionPool *instance();

    // 从池中获取连接（阻塞等待直到有可用连接）
    MYSQL *acquire();
    // 归还连接到池中
    void release(MYSQL *conn);
    // 初始化连接池
    void init(const std::string &host, int port,
              const std::string &user, const std::string &password,
              const std::string &dbname, int poolSize = 8);

private:
    ConnectionPool() = default;
    ~ConnectionPool();
    ConnectionPool(const ConnectionPool &) = delete;
    ConnectionPool &operator=(const ConnectionPool &) = delete;

    std::queue<MYSQL *> _pool;
    std::mutex _mutex;
    std::condition_variable _cv;
    int _poolSize = 0;

    std::string _host;
    int _port = 3306;
    std::string _user;
    std::string _password;
    std::string _dbname;

    MYSQL *createConnection();
};

#endif
