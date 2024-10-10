// message.cpp
#include <iostream>
#include "message.h"

Message* Message::from_json(const json& j) {
    if (!j.contains("type")) {
        std::cout << "JSON 数据中缺少 'type' 字段。" << std::endl;
        return nullptr;
    }
    std::string type = j.at("type").get<std::string>();

    if (type == "CreateMappingMessage") {
        auto* msg = new CreateMappingMessage();
        msg->key = j.at("key").get<int>();
        msg->value = j.at("value").get<int>();
        return msg;
    }
    else if (type == "AskingMappingMessage") {
        auto* msg = new AskingMappingMessage();
        msg->key = j.at("key").get<int>();
        return msg;
    }
    else if (type == "ArbiterMessage") {
        auto* msg = new ArbiterMessage();
        msg->source_ip = j.at("source_ip").get<uint32_t>();
        msg->target_ip = j.at("target_ip").get<uint32_t>();
        return msg;
    }
    else if (type == "ArbiterMessageProMax") {
        auto* msg = new ArbiterMessageProMax();
        msg->source_ip = j.at("source_ip").get<uint32_t>();
        msg->current_ip = j.at("current_ip").get<uint32_t>();
        msg->target_ip = j.at("target_ip").get<uint32_t>();
        return msg;
    }
    else if (type == "ForwardingMessage") {
        auto* msg = new ForwardingMessage();
        msg->interface1 = j.at("interface1").get<int>();
        msg->interface2 = j.at("interface2").get<int>();
        msg->next_hop_id = j.at("next_hop_id").get<int>();
        return msg;
    }
    else {
        // 未知类型，可以选择抛出异常或返回 nullptr
        return nullptr;
    }
}
