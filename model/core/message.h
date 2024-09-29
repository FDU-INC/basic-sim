#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <memory>
#include "json.h"

using json = nlohmann::json;

// 基类 Message
class Message {
public:
    std::string type;

    virtual json to_json() const = 0;
    virtual ~Message() = default;

    static Message* from_json(const json& j);
    // static std::unique_ptr<Message> from_json(const json& j);
};

// 具体消息类 TextMessage
class CreateMappingMessage : public Message {
public:
    int key;
    int value;

    CreateMappingMessage() {
        type = "CreateMappingMessage";
    }

    json to_json() const override {
        return json{
            {"type", type},
            {"key", key},
            {"value", value}
        };
    }
};

// 具体消息类 ImageMessage
class AskingMappingMessage : public Message {
public:
    int key;

    AskingMappingMessage() {
        type = "AskingMappingMessage";
    }

    json to_json() const override {
        return json{
            {"type", type},
            {"key", key}
        };
    }
};

// 具体消息类 ImageMessage
class ArbiterMessage : public Message {
public:
    uint32_t source_ip;
    uint32_t target_ip;

    ArbiterMessage() {
        type = "ArbiterMessage";
    }

    json to_json() const override {
        return json{
            {"type", type},
            {"source_ip", source_ip},
            {"target_ip", target_ip}
        };
    }
};

class ForwardingMessage : public Message {
public:
    int interface1;
    int interface2;
    int next_hop_id;

    ForwardingMessage() {
        type = "ForwardingMessage";
    }

    json to_json() const override {
        return json{
            {"type", type},
            {"interface1", interface1},
            {"interface2", interface2},
            {"next_hop_id", next_hop_id}
        };
    }
};

#endif // MESSAGE_HPP