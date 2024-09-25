#ifndef MESSAGE_RECEIVE_HELPER_H
#define MESSAGE_RECEIVE_HELPER_H

#include <sys/socket.h>
#include "ns3/message.h"
#include "ns3/json.h"

class MessageReceiveHelper {
public:
    MessageReceiveHelper(int socket);
    Message* receiveMessage();

private:
    int sock_;
};

#endif // MESSAGE_RECEIVE_HELPER_H
