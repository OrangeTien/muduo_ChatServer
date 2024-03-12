#include "groupmodel.h"
#include "db.h"

bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname,groupdesc) values('%s','%s')", group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection())); // 这个groupid自动生成
            return true;
        }
    }
    return false;
}

// 加入群组userid
void GroupModel::addGroup(int &userid, int &groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values('%d','%d','%s')", groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户所在群组 -> 用户可能在多个群 -> 返回vector
vector<Group> GroupModel::queryGroups(int &userid)
{
    char sql[1024] = {0};

    // 把指定用户所在群组的详细信息id,name,desc全部查出来
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join \
            groupuser b on a.id = b.groupid where b.userid=%d",
            userid);

    vector<Group> groupVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 执行sql语句
        if (res != nullptr)
        {
            MYSQL_ROW row;                                  // 解析sql语句执行后的结果 row[0]为id字段值 row[1]为name字段的值
            while ((row = mysql_fetch_row(res)) != nullptr) // 一行一行的查询,查询到最后一行的下一行返回值为nullptr
            {
                // 把查询出来的row给映射成一个User对象并返回
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    for (Group &group : groupVec)
    { // 遍历groupINfo信息
        // 根据群组id查找该群组中用户的id,name,state,grouprole
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join \
                groupuser b on b.userid = a.id where b.groupid=%d",
                group.getId());

        MYSQL_RES *res = mysql.query(sql); // 执行sql语句
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) // 一行一行的查询,查询到最后一行的下一行返回值为nullptr
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 查询群组中的用户列表
vector<int> GroupModel::queryGroupUsers(int &userid, int &groupid)
{
    char sql[1024] = {0};
    // 查找指定群组的用户，排除查询用户自己
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d", groupid, userid);

    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 执行sql语句
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) // 一行一行的查询,查询到最后一行的下一行返回值为nullptr
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}
