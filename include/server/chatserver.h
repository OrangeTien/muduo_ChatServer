/*
chatserver.h chatserver.cpp main.cpp
本质上是把test/muduo_server.cpp里面的内容给拆开写
写成声明+定义+调用的形式
*/
#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer
{
private:
    TcpServer _server;
    EventLoop *_loop;
    void onConnection(const TcpConnectionPtr &);
    void onMessage(const TcpConnectionPtr &, // 连接
                   Buffer *,               // 缓冲区
                   Timestamp);               // 接收到数据的时间信息
public:
    ChatServer(EventLoop *,               // 事件循环
               const InetAddress &, // IP+Port
               const string &);
    void start();
};


#endif


