#include "chatserver.hpp"
#include "chatservice.hpp"
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
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    // 端口校验
    if (port <= 0 || port > 65535)
    {
        cerr << "invalid port: " << argv[2] << " (must be 1-65535)" << endl;
        exit(-1);
    }

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();

    // 每 30 秒检查一次空闲连接（超过 90 秒无活动则断开）
    loop.runEvery(30.0, []() {
        ChatService::instance()->checkIdleConnections();
    });

    loop.loop();

    return 0;
}