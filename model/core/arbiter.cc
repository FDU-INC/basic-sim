#include <iostream>
#include <stdexcept>
#include <cstring> // For memset and strlen
#include <sys/socket.h> // For socket functions
#include <unistd.h>
#include <string.h>

#include "ns3/arbiter.h"
#include "ns3/log.h"
#include "ns3/socket-helper.h"
#include "ns3/message.h"

namespace ns3 {

// Arbiter result

ArbiterResult::ArbiterResult(bool failed, uint32_t out_if_idx, uint32_t gateway_ip_address) {
    m_failed = failed;
    m_out_if_idx = out_if_idx;
    m_gateway_ip_address = gateway_ip_address;
}

bool ArbiterResult::Failed() {
    return m_failed;
}

uint32_t ArbiterResult::GetOutIfIdx() {
    if (m_failed) {
        throw std::runtime_error("Cannot retrieve out interface index if the arbiter did not succeed in finding a next hop");
    }
    return m_out_if_idx;
}

uint32_t ArbiterResult::GetGatewayIpAddress() {
    if (m_failed) {
        throw std::runtime_error("Cannot retrieve gateway IP address if the arbiter did not succeed in finding a next hop");
    }
    return m_gateway_ip_address;
}

// Arbiter

NS_OBJECT_ENSURE_REGISTERED (Arbiter);
TypeId Arbiter::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::Arbiter")
            .SetParent<Object> ()
            .SetGroupName("BasicSim")
    ;
    return tid;
}

Arbiter::Arbiter(Ptr<Node> this_node, NodeContainer nodes) {
    m_node_id = this_node->GetId();
    m_nodes = nodes;
    m_tap_bridge_enable=false;
    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
    for (uint32_t i = 0; i < m_nodes.GetN(); i++) {
        for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
            m_ip_to_node_id.insert({m_nodes.Get(i)->GetObject<Ipv4>()->GetAddress(j, 0).GetLocal().Get(), i});
        }
    }
}

Arbiter::Arbiter(Ptr<Node> this_node, NodeContainer nodes, bool tap_bridge_enable) {
    m_node_id = this_node->GetId();
    m_nodes = nodes;
    m_tap_bridge_enable = tap_bridge_enable;
    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
    for (uint32_t i = 0; i < m_nodes.GetN(); i++) {
        for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
            m_ip_to_node_id.insert({m_nodes.Get(i)->GetObject<Ipv4>()->GetAddress(j, 0).GetLocal().Get(), i});
        }
    }

}

Arbiter::Arbiter(Ptr<Node> this_node, NodeContainer nodes,bool tap_bridge_enable, SocketHelper* socketHelper) {
    m_socketHelper = socketHelper;
    m_node_id = this_node->GetId();
    m_nodes = nodes;
    m_tap_bridge_enable = tap_bridge_enable;
    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
    for (uint32_t i = 0; i < m_nodes.GetN(); i++) {
        for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
            m_ip_to_node_id.insert({m_nodes.Get(i)->GetObject<Ipv4>()->GetAddress(j, 0).GetLocal().Get(), i});
        }
    }

}

uint32_t Arbiter::ResolveNodeIdFromIp(uint32_t ip) {
    if(m_tap_bridge_enable){
        if ((ip & 0xFF) != 1) {
            ip = (ip & 0xFFFFFF00) | 2; 
        }
    }
    
    m_ip_to_node_id_it = m_ip_to_node_id.find(ip);
    if (m_ip_to_node_id_it != m_ip_to_node_id.end()) {
        return m_ip_to_node_id_it->second;
    } else {
        std::ostringstream res;
        res << "IP address " << Ipv4Address(ip)  << " (" << ip << ") is not mapped to a node id";
        throw std::invalid_argument(res.str());
    }
}

ArbiterResult Arbiter::BaseDecide(Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {
    ArbiterMessageProMax m_arbiter_message_promax;
    m_arbiter_message_promax.source_ip = ipHeader.GetSource().Get();
    m_arbiter_message_promax.current_ip = m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal().Get();
    m_arbiter_message_promax.target_ip = ipHeader.GetDestination().Get();

    if ((m_arbiter_message_promax.current_ip & 0xFF) != 1) {
        m_arbiter_message_promax.current_ip = (m_arbiter_message_promax.current_ip & 0xFFFFFF00) | 2; 
    }
    if ((m_arbiter_message_promax.target_ip & 0xFF) != 1) {
        m_arbiter_message_promax.target_ip = (m_arbiter_message_promax.target_ip & 0xFFFFFF00) | 2; 
    }
    


    // ArbiterMessage m_arbiter_message;
    // m_arbiter_message.source_ip = ipHeader.GetSource().Get();
    // m_arbiter_message.target_ip = ipHeader.GetDestination().Get();
    // bool is_socket_request_for_source_ip = (m_arbiter_message.source_ip == 1717986918);

    // m_arbiter_message.source_ip = m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal().Get();
    
    // if ((m_arbiter_message.source_ip & 0xFF) != 1) {
    //     m_arbiter_message.source_ip = (m_arbiter_message.source_ip & 0xFFFFFF00) | 2; 
    // }
    // if ((m_arbiter_message.target_ip & 0xFF) != 1) {
    //     m_arbiter_message.target_ip = (m_arbiter_message.target_ip & 0xFFFFFF00) | 2; 
    // }
    // std::cout << "source_ip = " << m_arbiter_message.source_ip <<std::endl;
    // std::cout << "target_ip = " << m_arbiter_message.target_ip <<std::endl;
    std::cout << "开始发送消息 " << std::endl;

    std::cout << "source_ip = " << m_arbiter_message_promax.source_ip <<std::endl;
    std::cout << "current_ip = " << m_arbiter_message_promax.current_ip <<std::endl;
    std::cout << "target_ip = " << m_arbiter_message_promax.target_ip <<std::endl;
    

    this->m_socketHelper->sendMessage(&m_arbiter_message_promax);

    std::cout << "发送消息结束，开始接收消息!!! " << std::endl;

    Message* msg = this->m_socketHelper->receiveMessage();

    std::cout << "接收消息结束 " << std::endl;
    auto forwardingMsg = dynamic_cast<ForwardingMessage*>(msg);
    if(forwardingMsg->type =="ForwardingMessage"){
        std::cout << "interface1是" << forwardingMsg->interface1;
        std::cout << "interface2是" << forwardingMsg->interface2;
        std::cout << "next_hop_id是" << forwardingMsg->next_hop_id;
    }
    if (msg) {
        // std::cout << "接收到消息类型: " << msg->type << std::endl;
        if(auto forwardingMsg = dynamic_cast<ForwardingMessage*>(msg)){
            uint32_t result_interface = forwardingMsg->interface1;
            uint32_t result_next_ip =m_nodes.Get(forwardingMsg->next_hop_id)->GetObject<Ipv4>()->GetAddress(forwardingMsg->interface2 + 1, 0).GetLocal().Get();
            std::cout << "result_next_ip是" << result_next_ip;
            return ArbiterResult(false, result_interface + 1, result_next_ip);
        }
        else{
            std::cout << "接收到错误消息！" << std::endl;
            return ArbiterResult(true, 0, 0);
        }
    }
    std::cout << "未接收到消息！" << std::endl;
    return ArbiterResult(true, 0, 0);
}

}


