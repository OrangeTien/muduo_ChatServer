#include "chatservice.h"
#include "user.h"
#include "usermodel.h"
#include "group.h"
#include "public.h"
#include <functional>
#include <muduo/base/Logging.h>
#include <vector>
#include <map>

using namespace std;
using namespace placeholders;

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 解耦
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)}); // 这里是insert({key,value})的形式
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriends, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGIN_OUT_MSG, std::bind(&ChatService::loginOut, this, _1, _2, _3)});

    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
    
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid); // 返回迭代器
    if (it == _msgHandlerMap.end())
    { // msgid没有对应的处理器Handler
        // 返回一个空处理器
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid: " << msgid << " con not find handler.";
        };
    }
    else
    {
        return _msgHandlerMap[msgid]; // map下表查key,返回value
    }
}
// 解耦业务层和数据层
// 处理登陆业务 -> id password
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string pwd = js["password"];
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 用户已经登陆
            json resp;
            resp["msgid"] = LOGIN_MSG_ACK;
            resp["errno"] = 2;
            resp["errmsg"] = "user already online"; // error number 为 0 说明无错误
            resp["id"] = user.getId();              //
            resp["name"] = user.getName();
            conn->send(resp.dump()); // json->string 发送给client
        }
        else
        {
            // 登陆成功 更新用户状态 offline->online
            user.setState("online");
            _userModel.updateState(user);

            // 记录用户连接 这样服务器才可以把消息推送给每个用户
            // 由于存在多个用户同时登陆 同时向map中插入信息 所以考虑线程安全问题
            // 自动加锁解锁 lock_guard 构造时加锁 析构时解锁 所以加个{}限定作用域
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnnMap.insert({id, conn});
            }

            // 向redis订阅channel
            _redis.subscribe(id);

            // 局部变量和数据库操作不需要考虑多线程问题 -> 局部变量在栈区，每个线程有自己的栈区，隔离的；数据库的多线程是由mysql平台保证的
            json resp;
            resp["msgid"] = LOGIN_MSG_ACK;
            resp["errno"] = 0;         // error number 为 0 说明无错误
            resp["id"] = user.getId(); //
            resp["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> offlinemsg = _offlineMsgModel.query(id);
            if (!offlinemsg.empty())
            {
                resp["offlinemsg"] = offlinemsg;
                // 查询完离线消息后 及时删除
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            // json.hpp不可以序列化自定义类型的容器 比如vector<User> 所以我们把vector<User>的内容手动转化为string
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
                resp["friends"] = vec2;
            }
            else
            {
                // resp["friends"] = "no friend.";
            }

            // 查询用户的群组信息
            // groupid name desc vector<GroupUser>
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if(!groupuserVec.empty()){
                vector<string> groupVec;
                for(Group& group : groupuserVec){
                    json groupjs;
                    groupjs["id"] = group.getId();
                    groupjs["groupname"] = group.getName();
                    groupjs["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for(GroupUser &user : group.getUsers()){
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    groupjs["users"] = userV;
                    groupVec.push_back(groupjs.dump());
                }
                resp["groups"] = groupVec;
            }
            // 发送
            conn->send(resp.dump()); // json->string 发送给client
        }
    }
    else
    {
        // 用户不存在或者密码错误
        json resp;
        resp["msgid"] = LOGIN_MSG_ACK;
        resp["errno"] = 1;                                   // error number 为 0 说明无错误
        resp["errmsg"] = "user not exist or password wrong"; // error number 为 0 说明无错误
        string error_info = "login failed.";
        conn->send(error_info); // json->string 发送给client
    }
}
// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    // 向user表中插入用户数据，插入方法中会给调用user对象的setId()方法
    bool state = _userModel.insert(user);
    if (state)
    {
        json resp;
        resp["msgid"] = REG_MSG_ACK;
        resp["errno"] = 0;         // error number 为 0 说明无错误
        resp["id"] = user.getId(); //
        conn->send(resp.dump());   // json->string 发送给client
    }
    else
    {
        json resp;
        resp["msgid"] = REG_MSG_ACK;
        resp["errno"] = 1; // error number 为 1 说明有错误
        conn->send(resp.dump()); 
    }
}
// 客户端异常退出 删除conn 并 改变state
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);

        // 遍历map，删除指定map
        for (auto it = _userConnnMap.begin(); it != _userConnnMap.end(); it++)
        {
            if (it->second == conn)
            {
                // 删除
                user.setId(it->first);
                _userConnnMap.erase(it);
                break;
            }
        }
    }
    // 取消订阅
    _redis.unsubscribe(user.getId()); 
    // 更新用户的状态
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnnMap.find(toid);
        if (it != _userConnnMap.end())
        {
            // toid在线 转发消息 服务器推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }
    // 查一下toid是否在线，在线就说明在其他的server上
    User user = _userModel.query(toid);
    if (user.getState() == "online"){
        _redis.publish(toid, js.dump());
        return;
    }

    // toid不在线 存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}
// 服务器异常 业务重置
void ChatService::reset()
{
    // 把online状态的用户设置为offline
    _userModel.resetState();
}

// 添加好友
void ChatService::addFriends(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    js["msgid"] = ADD_FRIEND_MSG;
    int userid = js["id"].get<int>();
    int friendid = js["friend"].get<int>();

    _friendModel.insert(userid, friendid);
    conn->send(js.dump());
}
// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["desc"];

    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 创建群组成功后 把创建的用户加入到这个群 并且role设定为creator
        int groupid = group.getId();
        _groupModel.addGroup(userid, groupid, "creator");
    }
}
// 加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    _groupModel.addGroup(userid, groupid, "normal");
}
// 群组聊天
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    vector<int> userIdVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for (int &userId : userIdVec)
    {
        auto userConnIter = _userConnnMap.find(userId);
        if (userConnIter != _userConnnMap.end())
        {
            // 在线
            userConnIter->second->send(js.dump());
        }
        else
        {   
            // 在其他服务器上
            User user = _userModel.query(userId);
            if (user.getState() == "online"){
                _redis.publish(userId, js.dump());
            }
            else{
                // 不在线
            _offlineMsgModel.insert(userId, js.dump());
            }

        }
    }
}

void ChatService::loginOut(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnnMap.find(userid);
        if(it != _userConnnMap.end()){
            _userConnnMap.erase(it);
        }
    }

    // 用户注销，在redis中取消通道
    _redis.unsubscribe(userid);

    // 更新用户的状态
    User user(userid,"","","offline");
    _userModel.updateState(user);
    
    // 让client的接收线程读取msgid,退出线程,结束阻塞
    conn->send(js.dump());
}

// 这个就是notify -> 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int channel,string msg){
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnnMap.find(channel);
    if(it != _userConnnMap.end()){
        it->second->send(msg);
        return;
    }
    // 在从redis获取消息的时候 client下线
    _offlineMsgModel.insert(channel,msg);
}

// 用户登陆 {"msgid":1,"id":22,"password":"666999"} {"msgid":1,"id":19,"password":"123456"}
// {"msgid":5,"id":22,"from":"jin cheng","to":19,"msg":"hello,pi pi!"} {"msgid":5,"id":19,"from":"pi pi","to":22,"msg":"hello,jin cheng!"}
// {"msgid":5,"id":22,"from":"jin cheng","to":19,"msg":"hello,pi pi 2!"}
// 添加好友 {"msgid":6,"id":22,"friend":19}