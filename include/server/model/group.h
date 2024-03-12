#ifndef GROUP_H
#define GROUP_H

#include "groupuser.h"
#include<string>
#include<vector>
using namespace std;

// Group表的ORM类
class Group{
private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;

public:
    Group(int id=-1,string name="",string desc="")
    :id(id),name(name),desc(desc){};

    void setId(int id){this->id = id;}
    void setName(string name){this->name = name;}
    void setDesc(string desc){this->desc = desc;}

    int getId() const {return this->id;}
    string getName() const {return this->name;}
    string getDesc() const {return this->desc;}

    // 
    vector<GroupUser>& getUsers() {return this->users;} 

};
#endif