#include <iostream>
#include <stdexcept>
#include <cstring> // For memset and strlen
#include <sys/socket.h> // For socket functions
#include <unistd.h>
#include <string.h>

#include "ns3/arbiter.h"
#include "ns3/log.h"
#include <grpcpp/grpcpp.h>
#include <ns3/NS3service.grpc.pb.h>
#include <ns3/NS3service.pb.h>

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

Arbiter::Arbiter(Ptr<Node> this_node, NodeContainer nodes, bool tap_bridge_enable, std::shared_ptr<NS3::NS3Service::Stub> ns3_service_stub) {
    m_node_id = this_node->GetId();
    m_nodes = nodes;
    m_tap_bridge_enable = tap_bridge_enable;
    m_ns3_service_stub = ns3_service_stub;
    // Store IP address to node id (each interface has an IP address, so multiple IPs per node)
    for (uint32_t i = 0; i < m_nodes.GetN(); i++) {
        for (uint32_t j = 1; j < m_nodes.Get(i)->GetObject<Ipv4>()->GetNInterfaces(); j++) {
            m_ip_to_node_id.insert({m_nodes.Get(i)->GetObject<Ipv4>()->GetAddress(j, 0).GetLocal().Get(), i});
        }
    }
}

Arbiter::Arbiter(Ptr<Node> this_node, NodeContainer nodes, bool tap_bridge_enable, std::shared_ptr<NS3::NS3Service::Stub> ns3_service_stub, bool time_selection_enable, uint64_t* current_time, int64_t dynamicStateUpdateIntervalNs, bool ns3_cache_flag) {
    m_node_id = this_node->GetId();
    m_nodes = nodes;
    m_tap_bridge_enable = tap_bridge_enable;
    m_ns3_service_stub = ns3_service_stub;
    m_time_selection_enable = time_selection_enable;
    m_current_time_ptr = current_time;
    m_dynamicStateUpdateIntervalNs = dynamicStateUpdateIntervalNs;
    m_ns3_cache_flag = ns3_cache_flag;
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
    // 启用cache
    if (m_ns3_cache_flag) {
        // 构建缓存的 key
        CacheKey key;
        key.source_ip = ipHeader.GetSource().Get();
        key.current_ip = m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal().Get();
        key.target_ip = ipHeader.GetDestination().Get();

        // 查找缓存
        int64_t now = Simulator::Now().GetNanoSeconds();

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // 如果缓存存在并且未过期
            int64_t cache_time = it->second.time;
            std::cout << "缓存时间是" << cache_time << std::endl;
            std::cout << "当前时间是" << now << std::endl;
            std::cout << "阈值是" << m_dynamicStateUpdateIntervalNs << std::endl;

            // if (now - cache_time < m_dynamicStateUpdateIntervalNs + 10000000) {
            if (now - cache_time < 1000000000 + 10000000) {
                // 返回缓存结果
                std::cout << "缓存命中" << std::endl;
                NS3::ForwardingMessage cache_response = it->second.response;
                uint32_t result_interface = cache_response.interface1();
                uint32_t result_next_ip = m_nodes.Get(cache_response.next_hop_id())->GetObject<Ipv4>()->GetAddress(cache_response.interface2() + 1, 0).GetLocal().Get();
                return ArbiterResult(false, result_interface + 1, result_next_ip);
            } else {
                // 缓存过期，删除缓存
                std::cout << "缓存过期" << std::endl;
                m_cache.erase(it);
            }
        }
    }
    
    // 缓存未命中，发送gRPC请求
    NS3::ArbiterMessageProMax request;
    request.set_source_ip(ipHeader.GetSource().Get());
    request.set_current_ip(m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal().Get());
    request.set_target_ip(ipHeader.GetDestination().Get());
    
    if (m_time_selection_enable && m_current_time_ptr != nullptr) {
        request.set_current_time_ns(*m_current_time_ptr);
    }
    
    std::cout << "初始化信息" << std::endl;
    std::cout << "source_ip = " << request.source_ip() << std::endl;
    std::cout << "current_ip = " << request.current_ip() << std::endl;
    std::cout << "target_ip = " << request.target_ip() << std::endl;
    
    if ((request.source_ip() & 0xFF) != 1) {
        request.set_source_ip((request.source_ip() & 0xFFFFFF00) | 2);
    }
    if ((request.current_ip() & 0xFF) != 1) {
        request.set_current_ip((request.current_ip() & 0xFFFFFF00) | 2); 
    }
    if ((request.target_ip() & 0xFF) != 1) {
        request.set_target_ip((request.target_ip() & 0xFFFFFF00) | 2); 
    }
    
    std::cout << "我的id是:" << m_node_id << std::endl;
    std::cout << "开始发送gRPC请求" << std::endl;
    
    grpc::ClientContext context;
    NS3::ForwardingMessage response;
    
    grpc::Status status = m_ns3_service_stub->ArbiterProMax(&context, request, &response);
    
    std::cout << "发送请求结束" << std::endl;
    
    if (status.ok()) {
        std::cout << "interface1是" << response.interface1() << std::endl;
        std::cout << "interface2是" << response.interface2() << std::endl;
        std::cout << "next_hop_id是" << response.next_hop_id() << std::endl;
        
        uint32_t result_interface = response.interface1();
        uint32_t result_next_ip = m_nodes.Get(response.next_hop_id())->GetObject<Ipv4>()->GetAddress(response.interface2() + 1, 0).GetLocal().Get();
        std::cout << "result_next_ip是" << result_next_ip << std::endl;

        // 更新缓存
        if (m_ns3_cache_flag) {
            std::cout << "更新缓存" << std::endl;
            CacheKey key;
            key.source_ip = ipHeader.GetSource().Get();
            key.current_ip = m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal().Get();
            key.target_ip = ipHeader.GetDestination().Get();

            CacheEntry entry;
            entry.response = response;
            entry.time = Simulator::Now().GetNanoSeconds();
            m_cache[key] = entry;
        }
        return ArbiterResult(false, result_interface + 1, result_next_ip);
    }
    else {
        std::cout << "未收到服务响应！" << std::endl;
        return ArbiterResult(true, 0, 0);
    }
}

}


