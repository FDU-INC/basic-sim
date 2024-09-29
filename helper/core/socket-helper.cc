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
    closeConnection();
}

// 获取 socket1 描述符
int SocketHelper::getSocket_1() const {
    return sock_1;
}

// 获取 socket2 描述符
int SocketHelper::getSocket_2() const {
    return sock_2;
}

bool SocketHelper::isEstablished() const {
    return isEstablished_;
}

// 创建并连接 socket 的私有函数
void SocketHelper::createAndConnectSocket(const std::string& server_ip, int port) {
    struct sockaddr_in serv_addr1;

    // 创建 socket
    if ((sock_1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("Socket creation error");
    }

    // 设置服务器地址和端口
    serv_addr1.sin_family = AF_INET;
    serv_addr1.sin_port = htons(port);

    // 将 IP 地址转换为二进制形式并检查是否有效
    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr1.sin_addr) <= 0) {
        close(sock_1);
        throw std::runtime_error("Invalid address/ Address not supported");
    }

    // 连接服务器
    if (connect(sock_1, (struct sockaddr*)&serv_addr1, sizeof(serv_addr1)) < 0) {
        close(sock_1);
        throw std::runtime_error("Connection failed");
    }

    struct sockaddr_in serv_addr2;

    // 创建 socket
    if ((sock_2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("Socket creation error");
    }

    // 设置服务器地址和端口
    serv_addr2.sin_family = AF_INET;
    serv_addr2.sin_port = htons(port);

    // 将 IP 地址转换为二进制形式并检查是否有效
    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr2.sin_addr) <= 0) {
        close(sock_2);
        throw std::runtime_error("Invalid address/ Address not supported");
    }

    // 连接服务器
    if (connect(sock_2, (struct sockaddr*)&serv_addr2, sizeof(serv_addr2)) < 0) {
        close(sock_2);
        throw std::runtime_error("Connection failed");
    }

    isEstablished_ = true;
}

// 发送消息
bool SocketHelper::sendMessage(Message* msg) {
    json j;

    if (auto arbiterMsg = dynamic_cast<ArbiterMessage*>(msg)) {
        std::cout << "source_ip " << decimalToDottedDecimal(arbiterMsg->source_ip) << std::endl;
        std::cout << "target_ip " << decimalToDottedDecimal(arbiterMsg->target_ip) << std::endl;
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
    ssize_t sent = send(sock_1, &len, sizeof(len), 0);
    if (sent != sizeof(len)) {
        std::cerr << "发送数据长度失败" << std::endl;
        return false;
    }

    // 发送实际数据
    sent = send(sock_1, serialized.c_str(), serialized.size(), 0);
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
    ssize_t received = recv(sock_2, &dataLength, sizeof(dataLength), 0);
    if (received != sizeof(dataLength)) {
        std::cerr << "接收数据长度失败" << std::endl;
        return nullptr;
    }
    dataLength = ntohl(dataLength); // 转换为主机字节序

    // 接收实际数据
    std::string data(dataLength, '\0');
    received = recv(sock_2, &data[0], dataLength, 0);
    if (received != (ssize_t)dataLength) {
        std::cerr << "接收实际数据失败" << std::endl;
        return nullptr;
    }

    // 解析 JSON
    json j = json::parse(data);
    return Message::from_json(j);
}

void SocketHelper::closeConnection() {
    if (sock_1 != -1) {
        if (close(sock_1) == -1) {
            std::cout << "关闭 socket1 失败: " << std::endl;
        } else {
            std::cout << "Socket1 已成功关闭。" << std::endl;
        }
        sock_1 = -1;
        isEstablished_ = false;
    }
    if (sock_2 != -1) {
        if (close(sock_2) == -1) {
            std::cout << "关闭 socket2 失败: " << std::endl;
        } else {
            std::cout << "Socket2 已成功关闭。" << std::endl;
        }
        sock_2 = -1;
        isEstablished_ = false;
    }
}

std::string SocketHelper::decimalToDottedDecimal(uint32_t n) {
    // 使用位运算提取每个字节
    uint8_t byte1 = (n >> 24) & 0xFF;
    uint8_t byte2 = (n >> 16) & 0xFF;
    uint8_t byte3 = (n >> 8) & 0xFF;
    uint8_t byte4 = n & 0xFF;

    // 构建点分十进制字符串
    return std::to_string(byte1) + "." +
           std::to_string(byte2) + "." +
           std::to_string(byte3) + "." +
           std::to_string(byte4);
}