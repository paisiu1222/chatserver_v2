#include "config.hpp"
#include <fstream>
#include <iostream>

Config *Config::instance()
{
    static Config config;
    return &config;
}

bool Config::load(const std::string &configFile)
{
    std::ifstream ifs(configFile);
    if (!ifs.is_open())
    {
        std::cerr << "config file not found: " << configFile
                  << ", using defaults" << std::endl;
        return false;
    }

    try
    {
        _config = json::parse(ifs);
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "config parse error: " << e.what() << std::endl;
        return false;
    }
}

std::string Config::serverIp() const
{
    return _config.value("server", json::object()).value("ip", "0.0.0.0");
}
int Config::serverPort() const
{
    return _config.value("server", json::object()).value("port", 6000);
}
int Config::serverThreads() const
{
    return _config.value("server", json::object()).value("threads", 4);
}

std::string Config::mysqlHost() const
{
    return _config.value("mysql", json::object()).value("host", "127.0.0.1");
}
int Config::mysqlPort() const
{
    return _config.value("mysql", json::object()).value("port", 3306);
}
std::string Config::mysqlUser() const
{
    return _config.value("mysql", json::object()).value("user", "root");
}
std::string Config::mysqlPassword() const
{
    return _config.value("mysql", json::object()).value("password", "");
}
std::string Config::mysqlDatabase() const
{
    return _config.value("mysql", json::object()).value("database", "chat");
}
int Config::mysqlPoolSize() const
{
    return _config.value("mysql", json::object()).value("pool_size", 8);
}

std::string Config::redisHost() const
{
    return _config.value("redis", json::object()).value("host", "127.0.0.1");
}
int Config::redisPort() const
{
    return _config.value("redis", json::object()).value("port", 6379);
}

std::string Config::logLevel() const
{
    return _config.value("log", json::object()).value("level", "INFO");
}
