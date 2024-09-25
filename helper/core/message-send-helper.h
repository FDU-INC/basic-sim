// message_sender.h

#ifndef MESSAGE_SENDER_H
#define MESSAGE_SENDER_H

#include <string>
#include "ns3/message.h"
#include "ns3/json.h"

class MessageSendHelper {
public:
    // 构造函数
    MessageSendHelper(int socket);

    // 发送消息
    bool sendMessage(Message* msg);

private:
    int sock_;  // socket 文件描述符
};

#endif // MESSAGE_SENDER_H
