#include "socket-helper.h"
#include <iostream>
#include <stdexcept>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// SocketHelper 构造函数：创建并连接 socket
SocketHelper::SocketHelper(const std::string& server_ip, int port) {
    createAndConnectSocket(server_ip, port);
}

// SocketHelper 析构函数：关闭 socket
SocketHelper::~SocketHelper() {
    if (sock_ != -1) {
        close(sock_);
    }
}

// 获取 socket 描述符
int SocketHelper::getSocket() const {
    return sock_;
}

bool SocketHelper::isEstablished() const {
    return isEstablished_;
}

// 创建并连接 socket 的私有函数
void SocketHelper::createAndConnectSocket(const std::string& server_ip, int port) {
    struct sockaddr_in serv_addr;

    // 创建 socket
    if ((sock_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("Socket creation error");
    }

    // 设置服务器地址和端口
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // 将 IP 地址转换为二进制形式并检查是否有效
    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        close(sock_);
        throw std::runtime_error("Invalid address/ Address not supported");
    }

    // 连接服务器
    if (connect(sock_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock_);
        throw std::runtime_error("Connection failed");
    }

    isEstablished_ = true;
}

// 发送消息
bool SocketHelper::sendMessage(Message* msg) {
    json j;

    if (auto arbiterMsg = dynamic_cast<ArbiterMessage*>(msg)) {
        j = arbiterMsg->to_json();
    } else if (auto createMappingMsg = dynamic_cast<CreateMappingMessage*>(msg)) {
        j = createMappingMsg->to_json();
    } else if (auto askingMappingMsg = dynamic_cast<AskingMappingMessage*>(msg)) {
        j = askingMappingMsg->to_json();
    } else {
        std::cerr << "未知消息类型!" << std::endl;
        return false;
    }

    std::string serialized = j.dump();

    // 发送数据长度（4字节，网络字节序）
    uint32_t len = htonl(serialized.size());
    ssize_t sent = send(sock_, &len, sizeof(len), 0);
    if (sent != sizeof(len)) {
        std::cerr << "发送数据长度失败" << std::endl;
        return false;
    }

    // 发送实际数据
    sent = send(sock_, serialized.c_str(), serialized.size(), 0);
    if (sent != (ssize_t)serialized.size()) {
        std::cerr << "发送实际数据失败" << std::endl;
        return false;
    }

    std::cout << "发送消息: " << serialized << std::endl;

    return true;
}

// 接收消息
Message* SocketHelper::receiveMessage() {
    // 接收数据长度（4字节，网络字节序）
    uint32_t dataLength;
    ssize_t received = recv(sock_, &dataLength, sizeof(dataLength), 0);
    if (received != sizeof(dataLength)) {
        std::cerr << "接收数据长度失败" << std::endl;
        return nullptr;
    }
    dataLength = ntohl(dataLength); // 转换为主机字节序

    // 接收实际数据
    std::string data(dataLength, '\0');
    received = recv(sock_, &data[0], dataLength, 0);
    if (received != (ssize_t)dataLength) {
        std::cerr << "接收实际数据失败" << std::endl;
        return nullptr;
    }

    // 解析 JSON
    json j = json::parse(data);
    return Message::from_json(j);
}
