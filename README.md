# muduo_ChatServer

基于muduo的nginx负载均衡集群聊天服务器

平台工具：CentOS7部署 Vscode远程开发 Cmake构建 Git版本控制

项目内容：
1.使用muduo库提供高并发网络IO服务，网络层数据层业务层模块解耦设计

2.配置nginx基于tcp的负载均衡，实现服务器集群；使用redis发布订阅实现跨服务器的消息通信

3.使用Mysql数据库管理项目数据；使用json序列化反序列化消息作为私有通信协议

4.使用jemter测试服务器的并发量

## 编译

### 一键编译脚本

`git clone`

`./autobuild.sh`

### 手动编译

`rm -rf muduo_ChatServer/build/*`

`cd muduo_ChatServer/build/`

`cmake ..`

`make`

## 运行

服务端 `./bin/ChatServer ip port`

客户端 `./bin/ChatClient ip port`



