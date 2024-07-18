#include "ns3/arbiter.h"
#include "ns3/log.h"
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

Arbiter::Arbiter(Ptr<Node> this_node, NodeContainer nodes, bool tap_bridge_enable, int socket) {
    m_socket = socket;
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

// int Arbiter::createAndConnectSocket(const char* server_ip, int port) {
//         int sock = 0;
//         struct sockaddr_in serv_addr;

//         if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//             throw std::runtime_error("Socket creation error");
//         }

//         serv_addr.sin_family = AF_INET;
//         serv_addr.sin_port = htons(port);

//         if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
//             close(sock);
//             throw std::runtime_error("Invalid address/ Address not supported");
//         }

//         if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
//             close(sock);
//             throw std::runtime_error("Connection failed");
//         }

//         return sock;
//     }

ArbiterResult Arbiter::BaseDecide(Ptr<const Packet> pkt, Ipv4Header const &ipHeader) {

    // Retrieve the source node id
    uint32_t source_ip = ipHeader.GetSource().Get();
    uint32_t source_node_id;

    // Ipv4Address default constructor has IP 0x66666666 = 102.102.102.102 = 1717986918,
    // which is set by TcpSocketBase::SetupEndpoint to discover its actually source IP.
    bool is_socket_request_for_source_ip = source_ip == 1717986918;

    // const char* server_ip = "127.0.0.1";
    //     int port = 6001;
    //     int sock = createAndConnectSocket(server_ip, port);
    //     if (sock == -1) {
    //         throw std::runtime_error("Socket creation failed");
    //     }

    // If it is a request for source IP, the source node id is just the current node.
    if (is_socket_request_for_source_ip) {
        source_node_id = m_node_id;
    } else {
        if ((source_ip & 0xFF) != 1) {
            source_ip = (source_ip & 0xFFFFFF00) | 2; 
        }
        std::cerr << "source_ip" << source_ip << std::endl;

        std::string message = std::to_string(source_ip);
        message = "this is info of asking map " + message +"\0";
        std::cerr << "message" << message << std::endl;
        if (send(this->m_socket, message.c_str(), message.size(), 0) < 0) {
            close(this->m_socket);
            throw std::runtime_error("Send failed");
        }
        
        std::cerr << "sending out the fucking message" << std::endl;
        // 接收服务器的回复
        char server_reply[2000];
        if (recv(this->m_socket, server_reply, 2000, 0) < 0) {
            std::cerr << "Recv failed" << std::endl;
        }
        std::cerr << "receive the fucking message" << std::endl;

        std::cout << "Server reply: " << server_reply << std::endl;
        uint32_t parameter = std::stoi(server_reply);
        std::cerr << "Server reply parameter" << parameter << std::endl;
        source_node_id = parameter;

        // std::string stop_instruction = "stop";
        // send(this->m_socket, stop_instruction.c_str(), stop_instruction.size(), 0);
    }
    uint32_t second_parameter = ipHeader.GetDestination().Get();
    if ((second_parameter & 0xFF) != 1) {
        second_parameter = (second_parameter & 0xFFFFFF00) | 2; 
    }
    std::cerr << "second_parameter" << second_parameter << std::endl;
    std::string message = std::to_string(second_parameter);
    message = "this is info of asking map " + message + "\0";
    if (send(this->m_socket, message.c_str(), message.size(), 0) < 0) {
        close(this->m_socket);
        throw std::runtime_error("Send failed");
    }
    // 接收服务器的回复
    char server_reply2[2000];
    if (recv(this->m_socket, server_reply2, 2000, 0) < 0) {
        std::cerr << "Recv failed" << std::endl;
    }
    // std::cout << "Server reply: " << server_reply << std::endl;
    uint32_t parameter = std::stoi(server_reply2);
    std::cerr << "parameter2" << parameter << std::endl;
    uint32_t target_node_id = parameter;

    // std::string stop_instruction = "stop";
    // send(this->m_socket, stop_instruction.c_str(), stop_instruction.size(), 0);

    // Decide the next node
    return Decide(
                source_node_id,
                target_node_id,
                pkt,
                ipHeader,
                is_socket_request_for_source_ip
    );

}

}
