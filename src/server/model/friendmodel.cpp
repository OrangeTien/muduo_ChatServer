#include "friendmodel.h"
#include "db.h"

// 添加好友关系
void FriendModel::insert(int &userid, int &friendid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values('%d','%d')", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 返回好友列表
vector<User> FriendModel::query(int &userid)
{
    char sql[1024] = {0};
    // 拿userid的friend的id,name,state
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d ", userid);

    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 执行sql语句
        if (res != nullptr)
        {
            MYSQL_ROW row;                                  // 解析sql语句执行后的结果 row[0]为id字段值 row[1]为name字段的值
            while((row = mysql_fetch_row(res)) != nullptr) // 一行一行的查询,查询到最后一行的下一行返回值为nullptr
            {
                // 把查询出来的row给映射成一个User对象并返回
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }

        return vec; 
    }
}