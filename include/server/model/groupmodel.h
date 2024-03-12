#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.h"
#include <string>
#include <vector>

using namespace std;

class GroupModel{
private:

public:
    // 创建群组
    bool createGroup(Group&);

    // 加入群组
    void addGroup(int& userid,int& groupid,string role);

    // 查询用户所在群组 -> 用户可能在多个群 -> 返回vector
    vector<Group> queryGroups(int& userid);

    // 查询群组中的用户列表 -> 主要用户群聊
    vector<int> queryGroupUsers(int& userid,int& groupid);
};

#endif