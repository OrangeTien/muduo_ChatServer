#ifndef GROUP_USER_H
#define GROUP_USER_H

#include <string>
#include <vector>
#include "user.h"
using namespace std;

// GroupUser表的ORM类
// GroupUser继承了User的所有成员变量和方法
class GroupUser : public User
{
private:
    string role;
public:
    void setRole(string role) {this->role = role;}
    string getRole() const {return this->role;}
};

#endif