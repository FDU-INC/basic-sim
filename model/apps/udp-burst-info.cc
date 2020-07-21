#include "udp-burst-info.h"

namespace ns3 {

    UdpBurstInfo::UdpBurstInfo(
            int64_t udp_burst_id,
            int64_t from_node_id,
            int64_t to_node_id,
            int64_t rate_byte_per_sec,
            int64_t start_time_ns,
            int64_t duration_ns,
            std::string additional_parameters,
            std::string metadata
    ) {
        m_udp_burst_id = udp_burst_id;
        m_from_node_id = from_node_id;
        m_to_node_id = to_node_id;
        m_rate_byte_per_sec = rate_byte_per_sec;
        m_start_time_ns = start_time_ns;
        m_duration_ns = duration_ns;
        m_additional_parameters = additional_parameters;
        m_metadata = metadata;
    }

    int64_t UdpBurstInfo::GetUdpBurstId() {
        return m_udp_burst_id;
    }

    int64_t UdpBurstInfo::GetFromNodeId() {
        return m_from_node_id;
    }

    int64_t UdpBurstInfo::GetToNodeId() {
        return m_to_node_id;
    }

    int64_t UdpBurstInfo::GetRateBytePerSec() {
        return m_rate_byte_per_sec;
    }

    int64_t UdpBurstInfo::GetStartTimeNs() {
        return m_start_time_ns;
    }

    int64_t UdpBurstInfo::GetDurationNs() {
        return m_duration_ns;
    }

    std::string UdpBurstInfo::GetAdditionalParameters() {
        return m_additional_parameters;
    }

    std::string UdpBurstInfo::GetMetadata() {
        return m_metadata;
    }

}