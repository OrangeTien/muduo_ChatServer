#include "chatserver.h"
#include "chatservice.h"
#include <iostream>
#include <signal.h>
using namespace std;

// 重置user的状态
void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc,char* *argv) 
{   
    if(argc < 3){
        cerr << "comand invalid.example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port= atoi(argv[2]);
    // 监测到ctrl+c,调用resetHandler函数
    signal(SIGINT,resetHandler);
    EventLoop loop;
    InetAddress const addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();
}