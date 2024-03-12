#include "redis.h"
#include <iostream>
using namespace std;

bool Redis::connect()
{
    // 负责publish发布消息的上下文
    _publish_context = redisConnect("127.0.0.1", 6379);
    if(_publish_context == nullptr){
        cerr << "connect redis error" << endl;
        return false;
    }
    // 负责subscribe发布消息的上下文
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if(_subcribe_context == nullptr){
        cerr << "connect redis error" << endl;
        return false;   
    }
    // 在单独的线程中观察订阅消息 -> 因为subscribe后阻塞了
    thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis server success" << endl;
    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    // 相当于在命令行里面敲了一个publish命令
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if(reply == nullptr){
        cerr << "publish message error" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // 函数内部组装一个redis subscribe的命令但是不马上执行
    // 函数只负责把这条命令写入到buffer里面，这样就是让redis执行subscrive命令但是这里不等待
    if(redisAppendCommand(_subcribe_context, "SUBSCRIBE %d", channel) != REDIS_OK){
        cerr << "subscribe channel error" << endl;
        return false;
    }

    int done = 0;
    while (!done)   
    {
        if(redisBufferWrite(this->_subcribe_context,&done) == REDIS_ERR){
            cerr << "wirte to redis error" << endl;
            return false;
        }
    }

    return true;
    
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if(redisAppendCommand(_subcribe_context, "UNSUBSCRIBE %d", channel) != REDIS_OK){
        cerr << "unsubscribe channel error" << endl;
        return false;
    }

    int done = 0;
    while (!done)   
    {
        if(redisBufferWrite(this->_subcribe_context,&done) == REDIS_ERR){
            cerr << "wirte to redis error" << endl;
            return false;
        }
    }

    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while(redisGetReply(_subcribe_context, (void**)&reply) == REDIS_OK){
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr){
            // reply->element[2]是message element[1]是channel
            _notify_message_handler(atoi(reply->element[1]->str), string(reply->element[2]->str));
        }
        freeReplyObject(reply);
    }
    cerr << "redis subscribe thread exit" << endl;
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}
