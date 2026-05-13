#include "chatservice.hpp"
#include "public.hpp"
#include "password_utils.hpp"
#include <muduo/base/Logging.h>
#include <mutex>
#include <vector>
using namespace std;
using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({PING_MSG, std::bind(&ChatService::ping, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online状态的用户，设置成offline
    _userModel.resetState();
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务  id  pwd   pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    // 输入校验
    if (id <= 0 || !isValidPassword(pwd))
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "invalid id or password format!";
        conn->send(response.dump());
        return;
    }

    User user = _userModel.query(id);
    if (user.getId() == id && verifyPassword(pwd, user.getPwd()))
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            {
                unique_lock<shared_mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
                _lastActiveMap[id] = time;
            }

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id); 

            // 登录成功，更新用户状态信息 state offline=>online
            user.setState("online");
            _userModel.updateState(user);
            _redis.setUserState(id, "online");  // Redis 状态缓存

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在，用户存在但是密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

// 处理注册业务  name  password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    // 输入校验
    if (!isValidName(name) || !isValidPassword(pwd))
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "invalid name or password format! name: 2-20 chars, password: 6-100 chars";
        conn->send(response.dump());
        return;
    }

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        unique_lock<shared_mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);
    _redis.delUserState(userid);  // Redis 清除状态

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        unique_lock<shared_mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道和状态
    _redis.unsubscribe(user.getId());
    _redis.delUserState(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    string msg = js.value("msg", "");

    if (toid <= 0 || !isValidMessage(msg)) return;

    // 消息去重检查（基于 msg_uuid）
    string msgUuid = js.value("msg_uuid", "");
    if (!msgUuid.empty() && !_redis.setnxWithExpire("msg:" + msgUuid))
    {
        // 重复消息，仅回复 ACK
        json ack;
        ack["msgid"] = MSG_ACK;
        ack["msg_uuid"] = msgUuid;
        ack["status"] = "duplicate";
        conn->send(ack.dump());
        return;
    }

    // 更新发送者活跃时间
    int fromid = js["id"].get<int>();
    {
        unique_lock<shared_mutex> lock(_connMutex);
        _lastActiveMap[fromid] = time;
    }

    {
        shared_lock<shared_mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息   服务器主动推送消息给toid用户
            it->second->send(js.dump());
            // 回复 ACK
            json ack;
            ack["msgid"] = MSG_ACK;
            ack["msg_uuid"] = msgUuid;
            ack["status"] = "ok";
            conn->send(ack.dump());
            return;
        }
    }

    // 查询toid是否在线（优先查 Redis 缓存，未命中回源 MySQL）
    string toState = _redis.getUserState(toid);
    if (toState.empty())
    {
        User user = _userModel.query(toid);
        toState = user.getState();
        if (toState == "online")
            _redis.setUserState(toid, "online");
    }
    if (toState == "online")
    {
        _redis.publish(toid, js.dump());
        json ack;
        ack["msgid"] = MSG_ACK;
        ack["msg_uuid"] = msgUuid;
        ack["status"] = "ok";
        conn->send(ack.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
    // 回复 ACK（离线存储成功）
    json ack;
    ack["msgid"] = MSG_ACK;
    ack["msg_uuid"] = msgUuid;
    ack["status"] = "offline";
    conn->send(ack.dump());
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    if (friendid <= 0) return;

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    if (!isValidGroupName(name) || !isValidGroupDesc(desc)) return;

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    if (groupid <= 0) return;
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    string msg = js.value("msg", "");
    if (groupid <= 0 || !isValidMessage(msg)) return;

    // 消息去重检查
    string msgUuid = js.value("msg_uuid", "");
    if (!msgUuid.empty() && !_redis.setnxWithExpire("msg:" + msgUuid))
    {
        json ack;
        ack["msgid"] = MSG_ACK;
        ack["msg_uuid"] = msgUuid;
        ack["status"] = "duplicate";
        conn->send(ack.dump());
        return;
    }

    // 更新发送者活跃时间
    {
        unique_lock<shared_mutex> lock(_connMutex);
        _lastActiveMap[userid] = time;
    }
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    shared_lock<shared_mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询用户是否在线（优先查 Redis，未命中回源 MySQL）
            string state = _redis.getUserState(id);
            if (state.empty())
            {
                User user = _userModel.query(id);
                state = user.getState();
                if (state == "online")
                    _redis.setUserState(id, "online");
            }
            if (state == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }

    // 回复群聊 ACK
    json ack;
    ack["msgid"] = MSG_ACK;
    ack["msg_uuid"] = msgUuid;
    ack["status"] = "ok";
    conn->send(ack.dump());
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    shared_lock<shared_mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}

// 处理心跳请求
void ChatService::ping(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        unique_lock<shared_mutex> lock(_connMutex);
        _lastActiveMap[userid] = time;
    }
    // 回复 PONG
    json response;
    response["msgid"] = PONG_MSG;
    conn->send(response.dump());
}

// 定时检查空闲连接（超过 90 秒无活动则断开）
void ChatService::checkIdleConnections()
{
    Timestamp now = Timestamp::now();
    vector<int> staleUsers;

    {
        unique_lock<shared_mutex> lock(_connMutex);
        for (auto it = _lastActiveMap.begin(); it != _lastActiveMap.end();)
        {
            if (timeDifference(now, it->second) > 90.0)
            {
                staleUsers.push_back(it->first);
                it = _lastActiveMap.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    for (int userid : staleUsers)
    {
        // 清理连接并更新状态
        {
            unique_lock<shared_mutex> lock(_connMutex);
            auto it = _userConnMap.find(userid);
            if (it != _userConnMap.end())
            {
                it->second->shutdown();
                _userConnMap.erase(it);
            }
        }
        _redis.unsubscribe(userid);
        _redis.delUserState(userid);
        User user(userid, "", "", "offline");
        _userModel.updateState(user);
        LOG_INFO << "heartbeat timeout, user " << userid << " disconnected";
    }
}