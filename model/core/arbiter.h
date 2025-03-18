#ifndef ARBITER_H
#define ARBITER_H

#include <map>
#include <cinttypes>
#include <zlib.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/hash.h"

// 添加gRPC相关头文件
#include <grpcpp/grpcpp.h>
#include <ns3/NS3service.grpc.pb.h>
#include <ns3/NS3service.pb.h>

// 添加cache相关头文件
#include <unordered_map>

// 定义自定义结构体
struct CacheKey {
    unsigned int source_ip;
    unsigned int current_ip;
    unsigned int target_ip;

    // 重载==运算符，确保可以在unordered_map中比较键
    bool operator==(const CacheKey& other) const {
        return source_ip == other.source_ip &&
               current_ip == other.current_ip &&
               target_ip == other.target_ip;
    }
};

// 定义缓存结构，缓存内容包括 forwardingMsg 和 缓存时间
struct CacheEntry {
    NS3::ForwardingMessage response;
    int64_t time;  // 时间戳（单位：纳秒）
};

namespace std {
    template <>
    struct hash<CacheKey> {
        size_t operator()(const CacheKey& key) const {
            size_t h1 = hash<unsigned int>{}(key.source_ip);
            size_t h2 = hash<unsigned int>{}(key.current_ip);
            size_t h3 = hash<unsigned int>{}(key.target_ip);
            // 合并哈希值
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

namespace ns3 {

class ArbiterResult {

public:
    ArbiterResult(bool failed, uint32_t out_if_idx, uint32_t gateway_ip_address);
    bool Failed();
    uint32_t GetOutIfIdx();
    uint32_t GetGatewayIpAddress();

private:
    bool m_failed;
    uint32_t m_out_if_idx;
    uint32_t m_gateway_ip_address;

};

class Arbiter : public ns3::Object
{

public:
    static TypeId GetTypeId (void);

    Arbiter(Ptr<Node> this_node, NodeContainer nodes);
    Arbiter(Ptr<Node> this_node, NodeContainer nodes, bool tap_bridge_enable);
    
    // 替换SocketHelper为NS3::NS3Service::Stub
    Arbiter(Ptr<Node> this_node, NodeContainer nodes, bool tap_bridge_enable, std::shared_ptr<NS3::NS3Service::Stub> ns3_service_stub);
    Arbiter(Ptr<Node> this_node, NodeContainer nodes, bool tap_bridge_enable, std::shared_ptr<NS3::NS3Service::Stub> ns3_service_stub, bool time_selection_enable, uint64_t* current_time, int64_t dynamicStateUpdateIntervalNs, bool ns3_cache_flag);
    
    uint32_t ResolveNodeIdFromIp(uint32_t ip);
    ArbiterResult BaseDecide(Ptr<const Packet> pkt, Ipv4Header const &ipHeader);

    /**
     * Decide what should be done with the result.
     *
     * @param source_node_id                    Node where the packet originated from
     * @param target_node_id                    Node where the packet has to go to
     * @param pkt                               Packet
     * @param ipHeader                          IP header of the packet
     * @param is_socket_request_for_source_ip   True iff there is not actually forwarding being done, but it is only
     *                                          a dummy packet sent by the socket to decide the source IP address.
     *                                          Most importantly, this means that THERE IS NO OTHER HEADER IN THE
     *                                          PACKET, IT IS EMPTY (EVEN IF THE PROTOCOL FIELD IS SET IN THE IP
     *                                          HEADER).
     *
     * @return Routing arbiter result.
     *
     *         If it is a socket request for source IP AND you signal it failed, it is not a drop but it will send
     *         a direct signal to the socket that there is no route, likely terminating the socket directly.
     *         In other cases, when you set it failed, it leads to a drop.
     *
     *         A TCP socket first asks for source IP, and then subsequently the tcp-l4 layer does another
     *         call with the full header to get the real decision.
     *
     *         A UDP source only asks for source IP, and does not do another subsequent call in the udp-l4 layer.
     */
    virtual ArbiterResult Decide(
            int32_t source_node_id,
            int32_t target_node_id,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv4Header const &ipHeader,
            bool is_socket_request_for_source_ip
    ) = 0;

    /**
     * Convert the forwarding state (i.e., routing table) to a string representation.
     *
     * @return String representation
     */
    virtual std::string StringReprOfForwardingState() = 0;

protected:
    std::map<uint32_t, uint32_t>::iterator m_ip_to_node_id_it;
    std::map<uint32_t, uint32_t> m_ip_to_node_id;
    uint32_t m_node_id;
    NodeContainer m_nodes;
    bool m_tap_bridge_enable = false;
    bool m_time_selection_enable = false;
    uint64_t* m_current_time_ptr;
    
    // 替换SocketHelper为NS3Service::Stub
    std::shared_ptr<NS3::NS3Service::Stub> m_ns3_service_stub;

    // 添加缓存
    bool m_ns3_cache_flag; // 缓存开关
    int64_t m_dynamicStateUpdateIntervalNs; // 更新间隔
    std::unordered_map<CacheKey, CacheEntry> m_cache;   // 缓存
};

}

#endif //ARBITER_H
