// // message_sender.cc

// #include "message-send-helper.h"
// #include <iostream>
// #include <cstring>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <unistd.h>

// using json = nlohmann::json;

// // 构造函数
// MessageSendHelper::MessageSendHelper(int socket) : sock_(socket) {}

// // 发送消息函数
// bool MessageSendHelper::sendMessage(Message* msg) {
//     // 序列化消息到 JSON 字符串
//     // std::string send="";
//     json j ;
//     if(msg->type=="ArbiterMessage"){
//         ArbiterMessage* tmp=dynamic_cast<ArbiterMessage*>(msg);
//         j = tmp->to_json();
//     }
//     else if(msg->type=="CreateMappingMessage"){
//         std::cout<<"call create_mapping"<<std::endl;
//         CreateMappingMessage* tmp=dynamic_cast<CreateMappingMessage*>(msg);
//         std::cout<<"dynamic cast"<<std::endl;
//         j = tmp->to_json();
//         std::cout<<"json complete"<<std::endl;
//         std::cout<< j <<std::endl;
//     }
//     else if(msg->type=="AskingMappingMessage"){
//         AskingMappingMessage* tmp=dynamic_cast<AskingMappingMessage*>(msg);
//         j = tmp->to_json();
//     }
//     else{
//         std::cout << "shit!!!" << std::endl;;
//     }
//     std::string serialized = j.dump();

//     // 发送数据长度（4字节，网络字节序）
//     uint32_t len = htonl(serialized.size());
//     ssize_t sent = send(sock_, &len, sizeof(len), 0);
//     if (sent != sizeof(len)) {
//         std::cerr << "发送数据长度失败" << std::endl;
//         return false;
//     }

//     // 发送实际数据
//     sent = send(sock_, serialized.c_str(), serialized.size(), 0);
//     if (sent != (ssize_t)serialized.size()) {
//         std::cout << "发送实际数据失败" << std::endl;
//         return false;
//     }

//     std::cout << "发送消息: " << serialized << std::endl;

//     return true;
// }


// message_sender.cc

#include "message-send-helper.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno> // 确保包含这个头文件

using json = nlohmann::json;

// 构造函数
MessageSendHelper::MessageSendHelper(int socket) : sock_(socket) {}

// 发送消息函数
bool MessageSendHelper::sendMessage(Message* msg) {
    json j;

    if (auto arbiterMsg = dynamic_cast<ArbiterMessage*>(msg)) {
        j = arbiterMsg->to_json();
    }
    else if (auto createMappingMsg = dynamic_cast<CreateMappingMessage*>(msg)) {
        j = createMappingMsg->to_json();
    }
    else if (auto askingMappingMsg = dynamic_cast<AskingMappingMessage*>(msg)) {
        j = askingMappingMsg->to_json();
    }
    else {
        std::cout << "未知消息类型!" << std::endl;
        return false; // 或者其他合适的错误处理
    }

    std::string serialized = j.dump();

    // 发送数据长度（4字节，网络字节序）
    uint32_t len = htonl(serialized.size());
    ssize_t sent = send(sock_, &len, sizeof(len), 0);
    if (sent != sizeof(len)) {
        std::cout << "发送数据长度失败" << std::endl;
        return false;
    }

    // 发送实际数据
    sent = send(sock_, serialized.c_str(), serialized.size(), 0);
    if (sent != static_cast<ssize_t>(serialized.size())) {
        std::cout << "发送实际数据失败" << std::endl;
        return false;
    }

    std::cout << "发送消息: " << serialized << std::endl;

    return true;
}
