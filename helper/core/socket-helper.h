#ifndef SOCKET_HELPER_H
#define SOCKET_HELPER_H

#include <string>
#include <sys/socket.h>
#include "ns3/message.h"
#include "ns3/json.h"

class SocketHelper {
public:
    // 构造函数：接受 IP 地址和端口号并自动连接
    SocketHelper(const std::string& server_ip, int port);

    // 析构函数：确保在对象销毁时关闭 socket
    ~SocketHelper();

    // 获取 socket 描述符
    int getSocket() const;

    bool isEstablished() const;

    // 发送消息
    bool sendMessage(Message* msg);

    // 接收消息
    Message* receiveMessage();

private:
    int sock_ = -1;  // socket 文件描述符
    bool isEstablished_ = false;

    // 创建并连接 socket 的私有函数
    void createAndConnectSocket(const std::string& server_ip, int port);
};

#endif // SOCKET_HELPER_H