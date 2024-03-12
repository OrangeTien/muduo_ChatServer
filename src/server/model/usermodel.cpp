#include "usermodel.h"
#include "db.h"
#include <iostream>
using namespace std;

// User表的增加插入方法
bool UserModel::insert(User &user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name,password,state) values('%s','%s','%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    // 2.
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 查询
User UserModel::query(int &id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id=%d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 执行sql语句
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res); // 解析sql语句执行后的结果 row[0]为id字段值 row[1]为name字段的值
            if (row != nullptr)
            {
                // 把查询出来的row给映射成一个User对象并返回
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
        else
        {
            return User(); // 返回User对象的默认构造,id为-1
        }
    }
}

bool UserModel::updateState(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
        return false;
    }
}

void UserModel::resetState(){
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}