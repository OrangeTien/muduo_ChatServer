#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include "user.h"
#include<vector>
using namespace std;


class FriendModel{
private:

public:
    // 添加好友关系
    void insert(int& ,int&);
    // 返回好友列表
    vector<User> query(int&);
};

#endif