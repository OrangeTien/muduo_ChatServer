#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <mutex>
#include <unordered_map>
#include <functional>
#include "json.hpp"
#include "offlinemsgmodel.h"
#include "friendmodel.h"
#include "groupmodel.h"
#include "redis.h"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

#include "usermodel.h"
using MsgHandler = std::function<void(const TcpConnectionPtr &, json &, Timestamp)>;

// 业务模块 单例 构造函数私有化
class ChatService
{
private:
    ChatService();
    // 存储消息id和对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;
    // 数据操作类对象
    UserModel _userModel;
    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnnMap;
    // 定义互斥锁
    mutex _connMutex;
    // 离线消息模型 这个类里面定义了offlinemsg的方法
    OfflineMsgModel _offlineMsgModel;
    // 
    FriendModel _friendModel;
    // 
    GroupModel _groupModel;
    // redis操作对象
    Redis _redis;
    

public:
    // 获取单例对象的接口函数
    static ChatService *instance();
    // 处理登陆业务
    void login(const TcpConnectionPtr &, json &, Timestamp);
    // 处理注册业务
    void reg(const TcpConnectionPtr &, json &, Timestamp);
    // 获取消息对应的处理器
    MsgHandler getHandler(int);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &, json &, Timestamp);
    // 服务器异常,业务重置
    void reset();
    // 添加好友业务
    void addFriends(const TcpConnectionPtr &, json &, Timestamp);
    // 创建群组
    void createGroup(const TcpConnectionPtr &, json &, Timestamp);
    // 加入群组
    void addGroup(const TcpConnectionPtr &, json &, Timestamp);
    // 群组聊天
    void groupChat(const TcpConnectionPtr &, json &, Timestamp);
    // 处理注销业务
    void loginOut(const TcpConnectionPtr &, json &, Timestamp);
    // 
    void handleRedisSubscribeMessage(int,string);

};

#endif