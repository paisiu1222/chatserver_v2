#ifndef PUBLIC_H
#define PUBLIC_H

#include <string>
using namespace std;

/*
server和client的公共文件
*/
enum EnMsgType
{
    LOGIN_MSG = 1, // 登录消息
    LOGIN_MSG_ACK, // 登录响应消息
    LOGINOUT_MSG, // 注销消息
    REG_MSG, // 注册消息
    REG_MSG_ACK, // 注册响应消息
    ONE_CHAT_MSG, // 聊天消息
    ADD_FRIEND_MSG, // 添加好友消息

    CREATE_GROUP_MSG, // 创建群组
    ADD_GROUP_MSG, // 加入群组
    GROUP_CHAT_MSG, // 群聊天
    PING_MSG, // 心跳请求
    PONG_MSG, // 心跳响应
    MSG_ACK, // 消息确认
};

// 输入校验常量
const int MIN_NAME_LEN = 2;
const int MAX_NAME_LEN = 20;
const int MIN_PWD_LEN = 6;
const int MAX_PWD_LEN = 100;
const int MAX_MSG_LEN = 5000;
const int MAX_GROUP_NAME_LEN = 50;
const int MAX_GROUP_DESC_LEN = 200;

// 输入校验工具函数
inline bool isValidName(const string &name)
{
    if (name.length() < MIN_NAME_LEN || name.length() > MAX_NAME_LEN) return false;
    for (char c : name)
    {
        if (!isalnum(c) && c != '_' && (c & 0x80) == 0) // 允许中文（高字节）
            return false;
    }
    return true;
}

inline bool isValidPassword(const string &pwd)
{
    return pwd.length() >= MIN_PWD_LEN && pwd.length() <= MAX_PWD_LEN;
}

inline bool isValidMessage(const string &msg)
{
    return !msg.empty() && msg.length() <= MAX_MSG_LEN;
}

inline bool isValidGroupName(const string &name)
{
    return !name.empty() && name.length() <= MAX_GROUP_NAME_LEN;
}

inline bool isValidGroupDesc(const string &desc)
{
    return desc.length() <= MAX_GROUP_DESC_LEN;
}

#endif