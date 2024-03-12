#ifndef USER_H
#define USER_H

#include<string>
using namespace std;

// 映射数据库中的user表与变量
class User
{
private:
    int id;
    string name;
    string password;
    string state;
public:
    User(int id=-1,string name="",string pwd="",string state="offline")
    :id(id),name(name),password(pwd),state(state){}

    void setId(int id) {this->id =id; }
    void setName(string name) {this->name = name;}
    void setPwd(string pwd) {this->password = pwd;}
    void setState(string state) {this->state = state;}

    int getId() const {return id; }
    string getName() const {return name;}
    string getPwd() const {return password;}
    string getState() const {return state;}
};


#endif