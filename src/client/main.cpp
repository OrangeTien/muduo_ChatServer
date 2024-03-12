#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "group.h"
#include "user.h"
#include "public.h"

// 记录系统登陆的用户信息
User g_currentUser;
// 记录当前登陆用户的好友列表
vector<User> g_currentUserFriendList;
// 记录当前登陆用户的群组列表
vector<Group> g_currentUserGroupList;
// mainMeun
bool isMainMenuRunning = false;
// 现实当前登陆成功用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);
// 获取时间
string getCurrentTime();
// 主聊天页面
void mainMenu(int clientfd);

// 主线程是发送线程，子线程作为接收线程 -> 发送数据和接收数据是并行的
int main(int argc, char **agrv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.01 6000";
        exit(-1);
    }

    // 解析命令行函数 获取server的ip和port
    char *ip = agrv[1];
    uint16_t port = atoi(agrv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 定义server端的sock address
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 
    for(;;)
    {
        cout << "=============" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.quit" << endl;
        cout << "=============" << endl;
        cout << "choice:";
        int choice = 0;

        // 接收用户输入的选择
        cin >> choice;
        cin.get(); // cin会记录下回车 cin.get()可以去掉回车

        switch (choice)
        {
        case 1:
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get();
            cout << "password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string req = js.dump();

            int len = send(clientfd, req.c_str(), strlen(req.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login message error: " << req << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "receive login message error" << endl;
                }
                else
                {
                    json resp = json::parse(buffer);
                    if (0 != resp["errno"].get<int>())
                    {
                        cerr << resp["errmsg"] << endl;
                    }
                    else
                    {
                        // 在g_cuurentUser变量中记录从服务器获取的用户信息
                        g_currentUser.setId(resp["id"].get<int>());
                        g_currentUser.setName(resp["name"]);

                        if (resp.contains("friends"))
                        { // 有friends字段就说明有朋友,找出朋友列表
                            vector<string> friendVec = resp["friends"];
                            g_currentUserFriendList.clear();
                            for (string &friendstr : friendVec)
                            {                                     // firends对应的value是vector序列化之后的数据
                                json js = json::parse(friendstr); // 序列化
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        if (resp.contains("groups"))
                        {                                         // 反序列化一下
                            vector<string> vec1 = resp["groups"]; // groups的value是由一个vector对象转的
                            g_currentUserGroupList.clear();
                            for (string &groupstr : vec1)
                            {
                                json groupjs = json::parse(groupstr);
                                Group group;
                                group.setId(groupjs["id"].get<int>());
                                group.setName(groupjs["groupname"]);
                                group.setDesc(groupjs["groupdesc"]);

                                vector<string> vec2 = groupjs["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }
                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        showCurrentUserData();

                        if (resp.contains("offlinemsg"))
                        {
                            vector<string> vec = resp["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                // time + id + name + said + xxx
                                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                                {
                                    cout << js["time"] << " [" << js["id"] << "]"
                                         << "[" << js["name"].get<string>() << "]"
                                         << " said: " << js["msg"] << endl;
                                    continue;
                                }
                                else // 群组消息
                                {    // "2024-3-10" In group[5] [22][jin cheng] said: "1111"
                                    cout << js["time"] << " In group"
                                         << "[" << js["groupid"] << "] "
                                         << "[" << js["id"] << "]"
                                         << "[" << js["name"].get<string>() << "]"
                                         << " said: " << js["msg"] << endl;
                                }
                            }
                        }


                        std::thread readTask(readTaskHandler, clientfd);
                        readTask.detach();


                        isMainMenuRunning = true;
                        mainMenu(clientfd);
                        

                        // mainMenu结束后这个进入下一次循环，可是开启的线程处于阻塞状态
                    }
                }
            }

            break;
        }
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50); // "cin >>"不支持空格字符 比如"zhang san"这样的用户名
            cout << "password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string req = js.dump();

            // send()发送 string数据
            int len = send(clientfd, req.c_str(), strlen(req.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send register message error: " << req << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "receive register message error" << endl;
                }
                else
                {
                    json resp = json::parse(buffer);
                    if (0 != resp["errno"].get<int>())
                    {
                        cerr << "register error -> database update error" << endl;
                    }
                    else
                    {
                        cout << name << " register success, userid is " << resp["id"] << endl;
                    }
                }
            }
            break;
        }
        case 3:
            close(clientfd);
            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
}

void showCurrentUserData()
{
    cout << "=======login user=======" << endl;
    cout << "id:" << g_currentUser.getId() << " -> "
         << "name:" << g_currentUser.getName() << endl;
    cout << "-------Friend List-------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " -> " << user.getName() << " -> " << user.getState() << endl;
        }
    }
    cout << "-------Group List-------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << "[" << group.getId() << "]"
                 << " -> " << group.getName() << " -> " << group.getDesc() << endl;
        }
    }
    cout << "========================" << endl;
}

// 这个子线程专门来接收数据用的
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); // 开启后进入阻塞了
        if (-1 == len || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer); // 反序列化 json数据变json对象
        if (ONE_CHAT_MSG == js["msgid"].get<int>())
        {
            cout << js["time"] << " [" << js["id"] << "]"
                 << "[" << js["name"].get<string>() << "]"
                 << " said: " << js["msg"] << endl;
            continue;
        }
        else if (ADD_FRIEND_MSG == js["msgid"].get<int>())
        {
            cout << "You add friend"
                 << " [" << js["friend"] << "] "
                 << "success" << endl;
            continue;
        }
        else if (GROUP_CHAT_MSG == js["msgid"].get<int>())
        { // "2024-3-10" In group[5] [22][jin cheng] said: "1111"
            cout << js["time"] << " In group"
                 << "[" << js["groupid"] << "] "
                 << "[" << js["id"] << "]"
                 << "[" << js["name"].get<string>() << "]"
                 << " said: " << js["msg"] << endl;
        }
        else if (LOGIN_OUT_MSG == js["msgid"].get<int>())
        { 
           return;
        }
    }
}

string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

void help(int fd = 0, string str = "");

void chat(int, string);

void addfriend(int, string);

void creategroup(int, string);

void addgroup(int, string);

void groupchat(int, string);

void quit(int, string);

void loginout(int, string);

unordered_map<string, string> commandMap = {
    {"Help Manual", "help"},
    {"One Chat One", "chat:friend_id:message"},
    {"Add Friend", "addfriend:friend_id"},
    {"Create Group", "creategroup:groupname:groupdesc"},
    {"Join Group,", "addgroup:groupid"},
    {"Group Chat", "groupchat:groupid:message"},
    {"LoginOut", "loginout"}};

unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024); // cin.getline(char *) string(char *) 类型转换
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(":");
        if (-1 == idx) // idx下标为-1时 说明用户输入没有冒号
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx); // 找冒号前面的那个子串
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
        // second 指的是Map里面的对应函数
        // chat:friend_id:message -> chat(clientfd,"friend_id:message")
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

void help(int, string)
{
    cout << "show command list >>> " << endl;
    for (auto &p : commandMap)
    { // 打印map
        cout << p.first << ":" << p.second << endl;
    }
    cout << endl;
}

void chat(int clientfd, string str)
{
    int index = str.find(":"); // creategroup:groupname:groupdesc
    if (index == -1)
    {
        cerr << "chat command invalid." << endl;
        return;
    }

    int friendid = atoi(str.substr(0, index).c_str());          // 取冒号前一个字符串，然后转化为整数
    string message = str.substr(index + 1, str.size() - index); // 取冒号后一个字符串
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send chat msg error: " << buffer << endl;
    }
}

void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str()); // addfriend:friend_id str是firend_id
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friend"] = friendid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addfriend msg error: " << buffer << endl;
    }
}

void creategroup(int clientfd, string str)
{
    int index = str.find(":"); // creategroup:groupname:groupdesc
    if (index == -1)
    {
        cerr << "creategroup command invalid." << endl;
        return;
    }

    string groupname = str.substr(0, index);
    string groupdesc = str.substr(index + 1, str.size() - index);
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["desc"] = groupdesc;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send create group msg error: " << buffer << endl;
    }
}

void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addgroup msg error: " << buffer << endl;
    }
}

void groupchat(int clientfd, string str)
{
    int index = str.find(":"); // groupchat:groupid:message
    if (index == -1)
    {
        cerr << "groupchat command invalid." << endl;
        return;
    }

    int groupid = atoi(str.substr(0, index).c_str());
    string message = str.substr(index + 1, str.size() - index);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addgroup msg error: " << buffer << endl;
    }
}

void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGIN_OUT_MSG;
    js["id"] = g_currentUser.getId();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send loginout msg error: " << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}