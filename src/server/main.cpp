#include "chatserver.hpp"
#include "chatservice.hpp"
#include "config.hpp"
#include <muduo/base/Logging.h>
#include <iostream>
#include <signal.h>
using namespace std;

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    // 加载配置文件
    string configFile = (argc >= 2) ? argv[1] : "config/server.conf";
    Config::instance()->load(configFile);

    auto *cfg = Config::instance();
    string ip = cfg->serverIp();
    uint16_t port = cfg->serverPort();

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();

    // 每 30 秒检查一次空闲连接（超过 90 秒无活动则断开）
    loop.runEvery(30.0, []() {
        ChatService::instance()->checkIdleConnections();
    });

    LOG_INFO << "ChatServer started on " << ip << ":" << port;
    loop.loop();

    return 0;
}