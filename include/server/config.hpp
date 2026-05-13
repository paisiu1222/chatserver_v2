#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include "json.hpp"
using json = nlohmann::json;

// 配置管理单例
class Config
{
public:
    static Config *instance();

    // 加载配置文件
    bool load(const std::string &configFile);

    // 服务端配置
    std::string serverIp() const;
    int serverPort() const;
    int serverThreads() const;

    // MySQL 配置
    std::string mysqlHost() const;
    int mysqlPort() const;
    std::string mysqlUser() const;
    std::string mysqlPassword() const;
    std::string mysqlDatabase() const;
    int mysqlPoolSize() const;

    // Redis 配置
    std::string redisHost() const;
    int redisPort() const;

    // 日志配置
    std::string logLevel() const;

private:
    Config() = default;
    json _config;
};

#endif
