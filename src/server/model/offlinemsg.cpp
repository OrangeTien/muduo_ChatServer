#include "offlinemsgmodel.h"
#include "db.h"
#include <iostream>
using namespace std;

// 存储用户的离线消息
void OfflineMsgModel::insert(int &userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d,'%s')",userid, msg.c_str()); // 这里必须把string转为c_str(),不然数据库会报错

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 删除用户的离线消息
void OfflineMsgModel::remove(int &userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 查询用户的离线消息
vector<string> OfflineMsgModel::query(int &userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);

    vector<string> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        // 把用户的所有的离线消息放入vec中
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}