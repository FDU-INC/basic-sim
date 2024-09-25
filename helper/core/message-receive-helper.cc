#include "message-receive-helper.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <cerrno>

using json = nlohmann::json;

// 构造函数
MessageReceiveHelper::MessageReceiveHelper(int socket) : sock_(socket) {}

Message* MessageReceiveHelper::receiveMessage() {
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

// // 接收消息函数
// Message* MessageReceiveHelper::receiveMessage() {
//     uint32_t len;
//     ssize_t received = recv(sock_, &len, sizeof(len), 0);
//     if (received <= 0) {
//         std::cout << "接收数据长度失败" << std::endl;
//         return nullptr;
//     }
//     len = ntohl(len); // 网络字节序转主机字节序

//     std::string data(len, '\0');
//     received = recv(sock_, &data[0], len, 0);
//     if (received <= 0) {
//         std::cout << "接收实际数据失败" << std::endl;
//         return nullptr;
//     }

//     json msg_dict = json::parse(data);
//     std::string msg_type = msg_dict["type"];
    
//     Message* msg = nullptr;

//     if (msg_type == "CreateMappingMessage") {
//         msg = new CreateMappingMessage();
//         static_cast<CreateMappingMessage*>(msg)->key = msg_dict["key"];
//         static_cast<CreateMappingMessage*>(msg)->value = msg_dict["value"];
//         std::cout << "接收到 CreateMappingMessage: key = " << static_cast<CreateMappingMessage*>(msg)->key 
//                   << ", value = " << static_cast<CreateMappingMessage*>(msg)->value << std::endl;
//         return msg;
//     }
//     else if (msg_type == "AskingMappingMessage") {
//         msg = new AskingMappingMessage();
//         static_cast<AskingMappingMessage*>(msg)->key = msg_dict["key"];
//         std::cout << "接收到 AskingMappingMessage: key = " << static_cast<AskingMappingMessage*>(msg)->key << std::endl;
//         return msg;
//     }
//     else if (msg_type == "ArbiterMessage") {
//         msg = new ArbiterMessage();
//         static_cast<ArbiterMessage*>(msg)->source_ip = msg_dict["source_ip"];
//         static_cast<ArbiterMessage*>(msg)->target_ip = msg_dict["target_ip"];
//         std::cout << "接收到 ArbiterMessage: source_ip = " << static_cast<ArbiterMessage*>(msg)->source_ip 
//                   << ", target_ip = " << static_cast<ArbiterMessage*>(msg)->target_ip << std::endl;
//         return msg;
//     }
//     else {
//         std::cout << "接收到未知类型的消息" << std::endl;
//         return nullptr;
//     }

//     // delete msg; // 确保释放内存
// }
