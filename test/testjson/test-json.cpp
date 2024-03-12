#include"json.hpp"
using json = nlohmann::json;

#include<iostream>
#include<vector>
#include<map>
#include<string>
using namespace std;

string func1(){
    json js;
    js["msg"] = "hello,how are u?";
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg_type"] = 2;
    cout << js << endl;
    string sendBuf = js.dump(); // js -> string
    cout << sendBuf.c_str() << endl; // string -> char*
    return sendBuf;
}
string func2(){
    json js;
    js["id"] = {1,2,3,4}; // key-value value为数组类型
    js["name"] = "zhangsan";
    js["msg"]["zhang san"] = "hello world"; // 数组写法
    js["msg"]["li si"] = "hello china";  // 等效于value为json类型
    js["msg"] = {{"zhang san","hello world"},{"li si","hello china"}}; // key不能重复，所以这一段把前面的msg内容给覆盖了
    cout << js << endl;
    return js.dump();
}
string func3(){
    json js;
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(5);
    // 序列化一个vector容器 -> 数组
    js["list"] = v;

    map<int,string> m;
    m.insert({1,"tom"});
    m.insert({2,"jack"});
    m.insert({3,"lucky"});
    // 序列化一个map容器 -> 嵌套数组
    js["path"] = m;

    cout << js << endl;

    string sendBuf = js.dump(); // json对象 -> json字符串 -> char* -> 发送

    return sendBuf;
}
void func4(){
    string recvBuf = func1();
    // json反序列化 json字符串->json对象(json类，json容器)
    json jsbuf = json::parse(recvBuf);
    cout << "func1:" << jsbuf["msg"] << endl;

    string recvBuf2 = func2();
    cout << "func2:" << json::parse(recvBuf2)["id"][0] << endl;
    
    string recvBuf3 = func3();
    vector<int> myvector= json::parse(recvBuf3)["list"];
    for(int &v : myvector){
        cout << v << " ";
    }
    cout << endl;
    map<int,string> mymap = json::parse(recvBuf3)["path"];
    for(auto &p : mymap){
        cout << p.first << " " << p.second;
    }
    cout << endl;
}

int main(){
    // func1();
    // func2();
    // func3();
    func4();
    return 0;
}


