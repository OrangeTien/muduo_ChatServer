#ifndef OFFLINEMSGMODEL_H
#define OFFLINEMSGMODEL_H

#include<string>
#include<vector>
using namespace std;

// offlinemessage的数据操作方法
class OfflineMsgModel
{
private:
    
public:
    // 存储用户的离线消息
    void insert(int&,string);
    // 删除用户的离线消息
    void remove(int&);
    // 查询用户的离线消息
    vector<string> query(int&);
};


#endif