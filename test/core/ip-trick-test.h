/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ArbiterTest");

const std::string arbiter_test_dir = ".tmp-arbiter-test";

void prepare_arbiter_test_config() {
    mkdir_if_not_exists(arbiter_test_dir);
    std::ofstream config_file(arbiter_test_dir + "/config_ns3.properties");
    config_file << "simulation_end_time_ns=10000000000" << std::endl;
    config_file << "simulation_seed=123456789" << std::endl;
    config_file << "topology_ptop_filename=\"topology.properties.temp\"" << std::endl;
    config_file.close();
}

void prepare_arbiter_test_topology() {
    std::ofstream topology_file;
    topology_file.open (arbiter_test_dir + "/topology.properties.temp");
    topology_file << "num_nodes=3" << std::endl;
    topology_file << "num_undirected_edges=2" << std::endl;
    topology_file << "switches=set(1)" << std::endl;
    topology_file << "switches_which_are_tors=set(1)" << std::endl;
    topology_file << "servers=set(0, 2)" << std::endl;
    topology_file << "undirected_edges=set(0-1,1-2)" << std::endl;
    topology_file << "link_channel_delay_ns=10000" << std::endl;
    topology_file << "link_device_data_rate_megabit_per_s=100" << std::endl;
    topology_file << "link_device_queue=drop_tail(100p)" << std::endl;
    topology_file << "link_interface_traffic_control_qdisc=disabled" << std::endl;
    topology_file.close();
}

void prepare_arbiter_test() {
    prepare_arbiter_test_config();
    prepare_arbiter_test_topology();
}

void cleanup_arbiter_test() {
    remove_file_if_exists(arbiter_test_dir + "/config_ns3.properties");
    remove_file_if_exists(arbiter_test_dir + "/topology.properties.temp");
    remove_file_if_exists(arbiter_test_dir + "/logs_ns3/finished.txt");
    remove_file_if_exists(arbiter_test_dir + "/logs_ns3/timing_results.txt");
    remove_file_if_exists(arbiter_test_dir + "/logs_ns3/timing_results.csv");
    remove_dir_if_exists(arbiter_test_dir + "/logs_ns3");
    remove_dir_if_exists(arbiter_test_dir);
}

////////////////////////////////////////////////////////////////////////////////////////

class ArbiterRoutingTestCase : public TestCase
{
public:
    ArbiterRoutingTestCase () : TestCase ("Arbiter Routing Test Case") {};
    void DoRun () {
        prepare_arbiter_test();

        // Create nodes
        NodeContainer nodes;
        nodes.Create(3);

        // Create point-to-point links
        PointToPointHelper pointToPoint;
        pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
        pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));

        NetDeviceContainer devices;
        devices = pointToPoint.Install(nodes.Get(0), nodes.Get(1));
        devices = pointToPoint.Install(nodes.Get(1), nodes.Get(2));

        // Install Internet Stack
        InternetStackHelper stack;
        stack.Install(nodes);

        // Assign IP Addresses
        Ipv4AddressHelper address;
        address.SetBase("10.0.0.0", "255.255.255.0");
        Ipv4InterfaceContainer interfaces;
        interfaces = address.Assign(devices);

        address.SetBase("10.0.1.0", "255.255.255.0");
        interfaces = address.Assign(devices);

        // Add static routes
        Ipv4StaticRoutingHelper ipv4RoutingHelper;
        Ptr<Ipv4StaticRouting> staticRouting;
        
        // Node 1 routing
        staticRouting = ipv4RoutingHelper.GetStaticRouting(nodes.Get(1)->GetObject<Ipv4>());
        staticRouting->AddHostRouteTo(Ipv4Address("10.0.0.1"), Ipv4Address("10.0.1.1"), 1); // From 10.0.1.2 to 10.0.0.1

        // Create a packet sink to receive packets at node 0
        uint16_t sinkPort = 8080;
        Address sinkAddress(InetSocketAddress(Ipv4Address("10.0.0.1"), sinkPort));
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkAddress);
        ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(0));
        sinkApps.Start(Seconds(1.0));
        sinkApps.Stop(Seconds(10.0));

        // Create a TCP connection from node 2 to node 0
        Ptr<Socket> srcSocket = Socket::CreateSocket(nodes.Get(2), TcpSocketFactory::GetTypeId());
        srcSocket->Bind();
        srcSocket->Connect(InetSocketAddress(Ipv4Address("10.0.0.1"), sinkPort));

        // Send one packet from node 2 to node 0
        Ptr<Packet> packet = Create<Packet>(1024);
        srcSocket->Send(packet);

        // Run the simulation
        Simulator::Stop(Seconds(10.0));
        Simulator::Run();
        Simulator::Destroy();

        // Verify packet reception
        Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(0));
        NS_TEST_ASSERT_MSG_EQ(sink->GetTotalRx(), packet->GetSize(), "Packet not received correctly");

        cleanup_arbiter_test();
    }
};

////////////////////////////////////////////////////////////////////////////////////////

static class ArbiterTestSuite : public TestSuite
{
public:
    ArbiterTestSuite () : TestSuite ("arbiter-routing", UNIT) {
        AddTestCase (new ArbiterRoutingTestCase, TestCase::QUICK);
    }
} g_arbiterTestSuite;

////////////////////////////////////////////////////////////////////////////////////////