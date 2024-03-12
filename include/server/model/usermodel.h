#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.h"

// user表的数据操作方法全定义在这
class UserModel
{
private:
public:
    // 向表中插入新数据
    bool insert(User &);

    // 查询数据
    User query(int &);

    // 更新用户状态信息
    bool updateState(User &);

    // 重置用户的状态信息
    void resetState();
};

#endif