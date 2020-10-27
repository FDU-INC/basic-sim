/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/traffic-control-layer.h"

////////////////////////////////////////////////////////////////////////////////////////

const std::string ptop_tc_qdisc_test_dir = ".tmp-ptop-tc-qdisc-test";

void prepare_ptop_tc_qdisc_test_config() {
    mkdir_if_not_exists(ptop_tc_qdisc_test_dir);
    std::ofstream config_file(ptop_tc_qdisc_test_dir + "/config_ns3.properties");
    config_file << "simulation_end_time_ns=10000000000" << std::endl;
    config_file << "simulation_seed=123456789" << std::endl;
    config_file << "topology_ptop_filename=\"topology.properties.temp\"" << std::endl;
    config_file.close();
}

void cleanup_ptop_tc_qdisc_test() {
    remove_file_if_exists(ptop_tc_qdisc_test_dir + "/config_ns3.properties");
    remove_file_if_exists(ptop_tc_qdisc_test_dir + "/topology.properties.temp");
    remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/finished.txt");
    remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/timing_results.txt");
    remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/timing_results.csv");
    remove_dir_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3");
    remove_dir_if_exists(ptop_tc_qdisc_test_dir);
}

////////////////////////////////////////////////////////////////////////////////////////

class PtopTcQdiscRedValidTestCase : public TestCase
{
public:
    PtopTcQdiscRedValidTestCase () : TestCase ("ptop-tc-qdisc-red valid") {};
    void DoRun () {
        prepare_ptop_tc_qdisc_test_config();

        // Desired qdisc configurations
        std::vector<std::tuple<std::string, int64_t, int64_t, bool, double, int64_t, int64_t, int64_t, double, bool, bool>> desired_configs;
        desired_configs.push_back(std::make_tuple("disabled", 0, 2, true, 1.0, 0, 0, 0, 0, true, true));
        desired_configs.push_back(std::make_tuple("simple_red(drop; 1.0; 1; 2; 3; 0.5; wait; not_gentle)", 2, 0, false, 1.0, 1, 2, 3, 0.5, true, false));
        desired_configs.push_back(std::make_tuple("simple_red(ecn; 0.5; 0; 100; 100; 0.1; no_wait; not_gentle)", 2, 3, true, 0.5, 0, 100, 100, 0.1, false, false));
        desired_configs.push_back(std::make_tuple("simple_red(ecn; 0.001; 10; 10; 20; 1.0; wait; gentle)", 3, 2, true, 0.001, 10, 10, 20, 1.0, true, true));
        desired_configs.push_back(std::make_tuple("simple_red(drop; 0.4; 10; 10; 10; 0.9; no_wait; not_gentle)", 1, 2, false, 0.4, 10, 10, 10, 0.9, false, false));
        desired_configs.push_back(std::make_tuple("simple_red(ecn; 0.9; 0; 0; 3; 0.01; wait; gentle)", 2, 1, true, 0.9, 0, 0, 3, 0.01, true, true));

        // Create string for topology encoding
        std::string link_interface_traffic_control_qdisc_str = "map(";
        size_t i = 0;
        std::map<std::pair<int64_t, int64_t>, std::tuple<std::string, bool, double, int64_t, int64_t, int64_t, double, bool, bool>> pair_to_desired_config;
        for (std::tuple<std::string, int64_t, int64_t, bool, double, int64_t, int64_t, int64_t, double, bool, bool> c : desired_configs) {
            pair_to_desired_config.insert(std::make_pair(std::make_pair(std::get<1>(c), std::get<2>(c)), std::make_tuple(std::get<0>(c), std::get<3>(c), std::get<4>(c), std::get<5>(c), std::get<6>(c), std::get<7>(c), std::get<8>(c), std::get<9>(c), std::get<10>(c))));
            if (i != 0) {
                link_interface_traffic_control_qdisc_str += ",";
            }
            link_interface_traffic_control_qdisc_str += std::to_string(std::get<1>(c)) + "->" + std::to_string(std::get<2>(c)) + ": " + std::get<0>(c);
            i++;
        }
        link_interface_traffic_control_qdisc_str += ")";
        std::cout << link_interface_traffic_control_qdisc_str << std::endl;

        // Create mapping

        // Write topology
        std::ofstream topology_file;
        topology_file.open (ptop_tc_qdisc_test_dir + "/topology.properties.temp");
        topology_file << "num_nodes=4" << std::endl;
        topology_file << "num_undirected_edges=3" << std::endl;
        topology_file << "switches=set(2)" << std::endl;
        topology_file << "switches_which_are_tors=set(2)" << std::endl;
        topology_file << "servers=set(0,1,3)" << std::endl;
        topology_file << "undirected_edges=set(0-2,1-2,2-3)" << std::endl;
        topology_file << "all_nodes_are_endpoints=true" << std::endl;
        topology_file << "link_channel_delay_ns=10000" << std::endl;
        topology_file << "link_net_device_data_rate_megabit_per_s=10" << std::endl;
        topology_file << "link_net_device_queue=drop_tail(60p)" << std::endl;
        topology_file << "link_net_device_receive_error_model=none" << std::endl;
        topology_file << "link_interface_traffic_control_qdisc=" << link_interface_traffic_control_qdisc_str << std::endl;
        topology_file.close();

        Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>(ptop_tc_qdisc_test_dir);
        Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());

        // Basic sizes
        ASSERT_EQUAL(topology->GetNumNodes(), 4);
        ASSERT_EQUAL(topology->GetNumUndirectedEdges(), 3);
        ASSERT_EQUAL(topology->GetSwitches().size(), 1);
        ASSERT_EQUAL(topology->GetSwitchesWhichAreTors().size(), 1);
        ASSERT_EQUAL(topology->GetServers().size(), 3);
        ASSERT_EQUAL(topology->GetUndirectedEdges().size(), 3);
        ASSERT_EQUAL(topology->GetUndirectedEdgesSet().size(), 3);
        ASSERT_EQUAL(topology->GetAllAdjacencyLists().size(), 4);
        std::set<int64_t> endpoints = topology->GetEndpoints();
        ASSERT_EQUAL(endpoints.size(), 4);

        // Test all the queueing disciplines installed
        for (const std::pair<int64_t, int64_t>& edge : topology->GetUndirectedEdges()) {
            Ptr<PointToPointNetDevice> deviceAtoB = topology->GetSendingNetDeviceForLink(edge);
            Ptr<PointToPointNetDevice> deviceBtoA = topology->GetSendingNetDeviceForLink(std::make_pair(edge.second, edge.first));
            std::vector<std::pair<std::pair<int64_t, int64_t>, Ptr<PointToPointNetDevice>>> links_with_devices;
            links_with_devices.push_back(std::make_pair(edge, deviceAtoB));
            links_with_devices.push_back(std::make_pair(std::make_pair(edge.second, edge.first), deviceBtoA));
            for (std::pair<std::pair<int64_t, int64_t>, Ptr<PointToPointNetDevice>> link_and_device : links_with_devices) {

                // Under investigation
                std::pair<int64_t, int64_t> link = link_and_device.first;
                Ptr<PointToPointNetDevice> device = link_and_device.second;

                // Traffic control queueing discipline
                Ptr<QueueDisc> queueDisc = topology->GetNodes().Get(link.first)->GetObject<TrafficControlLayer>()->GetRootQueueDiscOnDevice(device);
                std::tuple<std::string, bool, double, int64_t, int64_t, int64_t, double, bool, bool> desired_config = pair_to_desired_config.at(link);
                if (std::get<0>(desired_config) != "disabled") {

                    // simple_red
                    Ptr<RedQueueDisc> realDisc = queueDisc->GetObject<RedQueueDisc>();
                    ASSERT_NOT_EQUAL(realDisc, 0);

                    // Whether to mark ECN
                    BooleanValue use_ecn_att;
                    realDisc->GetAttribute ("UseEcn", use_ecn_att);
                    ASSERT_EQUAL(use_ecn_att.Get(), std::get<1>(desired_config));

                    // Whether to drop
                    BooleanValue use_hard_drop_att;
                    realDisc->GetAttribute ("UseHardDrop", use_hard_drop_att);
                    ASSERT_EQUAL(use_hard_drop_att.Get(), !std::get<1>(desired_config));

                    // Queue weight for EWMA average queue size estimate
                    DoubleValue qw_att;
                    realDisc->GetAttribute ("QW", qw_att);
                    ASSERT_EQUAL(qw_att.Get(), std::get<2>(desired_config));

                    // Mean packet size we set to 1500 byte always
                    UintegerValue mean_pkt_size_att;
                    realDisc->GetAttribute ("MeanPktSize", mean_pkt_size_att);
                    ASSERT_EQUAL(mean_pkt_size_att.Get(), 1500);

                    // RED minimum threshold (packets)
                    DoubleValue min_th_att;
                    realDisc->GetAttribute ("MinTh", min_th_att);
                    ASSERT_EQUAL(min_th_att.Get(), std::get<3>(desired_config));

                    // RED maximum threshold (packets)
                    DoubleValue max_th_att;
                    realDisc->GetAttribute ("MaxTh", max_th_att);
                    ASSERT_EQUAL(max_th_att.Get(), std::get<4>(desired_config));

                    // Maximum queue size (packets)
                    QueueSizeValue max_size_att;
                    realDisc->GetAttribute ("MaxSize", max_size_att);
                    QueueSize max_size_queue_size = max_size_att.Get();
                    ASSERT_EQUAL(max_size_queue_size.GetValue(), std::get<5>(desired_config));

                    // Max probability
                    DoubleValue l_interm_att;
                    realDisc->GetAttribute ("LInterm", l_interm_att);
                    ASSERT_EQUAL(l_interm_att.Get(), 1.0 / std::get<6>(desired_config));

                    // Gentle
                    BooleanValue wait_att;
                    realDisc->GetAttribute ("Wait", wait_att);
                    ASSERT_EQUAL(wait_att.Get(), std::get<7>(desired_config));

                    // Gentle
                    BooleanValue gentle_att;
                    realDisc->GetAttribute ("Gentle", gentle_att);
                    ASSERT_EQUAL(gentle_att.Get(), std::get<8>(desired_config));

                } else {
                    // disabled
                    ASSERT_EQUAL(queueDisc, 0);
                }

                // Check if the node identifiers here and on the other side match up
                ASSERT_EQUAL(device->GetNode()->GetId(), link.first);
                int64_t node_id_one = device->GetChannel()->GetObject<PointToPointChannel>()->GetDevice(0)->GetNode()->GetId();
                int64_t node_id_two = device->GetChannel()->GetObject<PointToPointChannel>()->GetDevice(1)->GetNode()->GetId();
                ASSERT_EQUAL(device->GetChannel()->GetObject<PointToPointChannel>()->GetNDevices(), 2);
                ASSERT_TRUE((node_id_one == link.first && node_id_two == link.second) || (node_id_one == link.second && node_id_two == link.first));

            }

        }

        basicSimulation->Finalize();
        cleanup_ptop_tc_qdisc_test();

    }
};

////////////////////////////////////////////////////////////////////////////////////////

class PtopTcQdiscFqCodelValidTestCase : public TestCase
{
public:
    PtopTcQdiscFqCodelValidTestCase () : TestCase ("ptop-tc-qdisc fq-codel-valid") {};
    void DoRun () {
        prepare_ptop_tc_qdisc_test_config();
        
        std::ofstream topology_file;
        topology_file.open (ptop_tc_qdisc_test_dir + "/topology.properties.temp");
        topology_file << "num_nodes=8" << std::endl;
        topology_file << "num_undirected_edges=7" << std::endl;
        topology_file << "switches=set(4)" << std::endl;
        topology_file << "switches_which_are_tors=set(4)" << std::endl;
        topology_file << "servers=set(0,1,2,3,5,6,7)" << std::endl;
        topology_file << "undirected_edges=set(0-4,1-4,2-4,3-4,4-5,4-6,4-7)" << std::endl;
        topology_file << "all_nodes_are_endpoints=false" << std::endl;
        topology_file << "link_channel_delay_ns=map(0-4: 400,1-4: 500,2-4: 600,3-4: 700,4-5: 900,4-6: 10000,4-7: 11000)" << std::endl;
        topology_file << "link_net_device_data_rate_megabit_per_s=map(0->4: 2.8,1->4: 3.1,2->4: 3.4,3->4: 3.7,4->5: 4.7,4->6: 5.4,4->7: 6.1,4->0: 1.2,4->1: 1.9,4->2: 2.6,4->3: 3.3,5->4: 4.3,6->4: 4.6,7->4: 4.9)" << std::endl;
        topology_file << "link_net_device_queue=map(0->4: drop_tail(4p),1->4: drop_tail(4B),2->4: drop_tail(4p),3->4: drop_tail(4B),4->5: drop_tail(5p),4->6: drop_tail(6p),4->7: drop_tail(7p),4->0: drop_tail(77p),4->1: drop_tail(1p),4->2: drop_tail(2p),4->3: drop_tail(3p),5->4: drop_tail(4B),6->4: drop_tail(4p),7->4: drop_tail(4B))" << std::endl;
        topology_file << "link_net_device_receive_error_model=map(0->4: iid_uniform_random_pkt(0.0), 1->4: iid_uniform_random_pkt(0.1), 2->4: iid_uniform_random_pkt(0.2), 3->4: iid_uniform_random_pkt(0.3), 5->4:iid_uniform_random_pkt(0.5), 6->4:iid_uniform_random_pkt(0.6), 7->4:iid_uniform_random_pkt(0.7), 4->0: iid_uniform_random_pkt(0.4), 4->1: none, 4->2: iid_uniform_random_pkt(0.4), 4->3: none, 4->5: none, 4->6: iid_uniform_random_pkt(0.400000), 4->7: iid_uniform_random_pkt(1.0))" << std::endl;
        topology_file << "link_interface_traffic_control_qdisc=map(0->4: default, 1->4: fq_codel_better_rtt, 2->4: disabled, 3->4: default, 5->4:disabled, 6->4:default, 7->4:fq_codel_better_rtt, 4->0: disabled, 4->1: fq_codel_better_rtt, 4->2: default, 4->3: disabled, 4->5: default, 4->6: disabled, 4->7: fq_codel_better_rtt)" << std::endl;
        topology_file.close();
        
        Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>(ptop_tc_qdisc_test_dir);
        Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());

        // And now we are going to go test all the network devices installed and their channels in-between
        for (const std::pair<int64_t, int64_t>& edge : topology->GetUndirectedEdges()) {
            Ptr<PointToPointNetDevice> deviceAtoB = topology->GetSendingNetDeviceForLink(edge);
            Ptr<PointToPointNetDevice> deviceBtoA = topology->GetSendingNetDeviceForLink(std::make_pair(edge.second, edge.first));
            std::vector<std::pair<std::pair<int64_t, int64_t>, Ptr<PointToPointNetDevice>>> links_with_devices;
            links_with_devices.push_back(std::make_pair(edge, deviceAtoB));
            links_with_devices.push_back(std::make_pair(std::make_pair(edge.second, edge.first), deviceBtoA));
            for (std::pair<std::pair<int64_t, int64_t>, Ptr<PointToPointNetDevice>> link_and_device : links_with_devices) {

                // Under investigation
                std::pair<int64_t, int64_t> link = link_and_device.first;
                Ptr<PointToPointNetDevice> device = link_and_device.second;

                // Traffic control queueing discipline (based on formula (a * 2 + b * 7) % 3 = {0, 1, 2} = {fq_codel_better_rtt, default, disabled)
                Ptr<QueueDisc> queueDisc = topology->GetNodes().Get(link.first)->GetObject<TrafficControlLayer>()->GetRootQueueDiscOnDevice(device);
                if ((link.first * 2 + link.second * 7) % 3 == 0) {
                    // fq_codel_better_rtt
                    Ptr<FqCoDelQueueDisc> realDisc = queueDisc->GetObject<FqCoDelQueueDisc>();
                    ASSERT_NOT_EQUAL(realDisc, 0);

                    // Improved interval (= RTT estimate)
                    StringValue interval_att;
                    realDisc->GetAttribute ("Interval", interval_att);
                    ASSERT_EQUAL(interval_att.Get(), std::to_string(topology->GetWorstCaseRttEstimateNs()) + "ns");

                    // Improved target (= RTT estimate / 20)
                    StringValue target_att;
                    realDisc->GetAttribute ("Target", target_att);
                    ASSERT_EQUAL(target_att.Get(), std::to_string(topology->GetWorstCaseRttEstimateNs() / 20) + "ns");

                } else if ((link.first * 2 + link.second * 7) % 3 == 1) {
                    // default (currently, fq codel is default)
                    ASSERT_NOT_EQUAL(queueDisc->GetObject<FqCoDelQueueDisc>(), 0);

                } else {
                    // disabled
                    ASSERT_EQUAL(queueDisc, 0);
                }

                // Check if the node identifiers here and on the other side match up
                ASSERT_EQUAL(device->GetNode()->GetId(), link.first);
                int64_t node_id_one = device->GetChannel()->GetObject<PointToPointChannel>()->GetDevice(0)->GetNode()->GetId();
                int64_t node_id_two = device->GetChannel()->GetObject<PointToPointChannel>()->GetDevice(1)->GetNode()->GetId();
                ASSERT_EQUAL(device->GetChannel()->GetObject<PointToPointChannel>()->GetNDevices(), 2);
                ASSERT_TRUE((node_id_one == link.first && node_id_two == link.second) || (node_id_one == link.second && node_id_two == link.first));

            }

        }

        basicSimulation->Finalize();
        cleanup_ptop_tc_qdisc_test();

    }
};

////////////////////////////////////////////////////////////////////////////////////////

class PtopTcQdiscInvalidTestCase : public TestCase
{
public:
    PtopTcQdiscInvalidTestCase () : TestCase ("ptop-tc-qdisc invalid") {};
    void DoRun () {

        std::ofstream topology_file;
        Ptr<BasicSimulation> basicSimulation;

        std::vector<std::pair<std::string, std::string>> qdisc_and_expected_what;
        qdisc_and_expected_what.push_back(std::make_pair("some_non_existent_qdisc(100)", "Invalid traffic control qdisc value: some_non_existent_qdisc(100)"));
        qdisc_and_expected_what.push_back(std::make_pair("fifo(100)", "Invalid maximum FIFO queue size value: 100"));
        qdisc_and_expected_what.push_back(std::make_pair("fifo(100b)", "Invalid maximum FIFO queue size value: 100b"));
        qdisc_and_expected_what.push_back(std::make_pair("fifo(100P)", "Invalid maximum FIFO queue size value: 100P"));
        qdisc_and_expected_what.push_back(std::make_pair("fifo(-1p)", "Negative int64 value not permitted: -1"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(abc; 1.0; 10; 20; 30; 1.0; no_wait; gentle)", "Invalid RED action: abc"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(ecn; 0.5; 10; 9; 30; 0.9; no_wait; not_gentle)", "RED minimum threshold cannot exceed maximum threshold"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(drop; 0.001; 10; 20; 19; 0.2; no_wait; not_gentle)", "RED maximum threshold cannot exceed maximum queue size"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(drop; 0.9; -4; 20; 30; 0.7; no_wait; gentle)", "Negative int64 value not permitted: -4"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(drop; 1.0; 10; 20; 30; 0.0; wait; not_gentle)", "Maximum probability must be in range (0, 1.0]"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(ecn; 0.1; 10; 20; 30; 1.0001; no_wait; gentle)", "Maximum probability must be in range (0, 1.0]"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(ecn; 0.0; 10; 20; 30; 1.0001; no_wait; not_gentle)", "Queue weight must be in range (0, 1.0]"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(drop; -0.3; 10; 20; 30; 1.0001; wait; gentle)", "Queue weight must be in range (0, 1.0]"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(drop; 1.1; 10; 20; 30; 1.0001; wait; not_gentle)", "Queue weight must be in range (0, 1.0]"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(drop; 1.0; 10; 20; 30; 1.0; no_wait; true)", "Invalid RED gentle: true"));
        qdisc_and_expected_what.push_back(std::make_pair("simple_red(drop; 1.0; 10; 20; 30; 1.0; abcd; not_gentle)", "Invalid RED wait: abcd"));

        // Check each invalid case
        for (std::pair<std::string, std::string> scenario : qdisc_and_expected_what) {
            prepare_ptop_tc_qdisc_test_config();
            basicSimulation = CreateObject<BasicSimulation>(ptop_tc_qdisc_test_dir);
            topology_file.open(ptop_tc_qdisc_test_dir + "/topology.properties.temp");
            topology_file << "num_nodes=4" << std::endl;
            topology_file << "num_undirected_edges=3" << std::endl;
            topology_file << "switches=set(0,1,2,3)" << std::endl;
            topology_file << "switches_which_are_tors=set(0,3)" << std::endl;
            topology_file << "servers=set()" << std::endl;
            topology_file << "undirected_edges=set(0-1,1-2,2-3)" << std::endl;
            topology_file << "link_channel_delay_ns=10000" << std::endl;
            topology_file << "link_net_device_data_rate_megabit_per_s=100" << std::endl;
            topology_file << "link_net_device_queue=drop_tail(100p)" << std::endl;
            topology_file << "link_net_device_receive_error_model=none" << std::endl;
            topology_file << "link_interface_traffic_control_qdisc=" << scenario.first << std::endl;
            topology_file.close();
            ASSERT_EXCEPTION_MATCH_WHAT(
                    CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper()),
                    scenario.second
            );
            basicSimulation->Finalize();
            cleanup_ptop_tc_qdisc_test();
        }

    }
};

////////////////////////////////////////////////////////////////////////////////////////

class PtopTcQdiscRedEcnAndDropMarkingTestCase : public TestCaseWithLogValidators
{
public:
    PtopTcQdiscRedEcnAndDropMarkingTestCase () : TestCaseWithLogValidators ("ptop-tc-qdisc-red ecn-and-drop-marking") {};
    void DoRun () {
        mkdir_if_not_exists(ptop_tc_qdisc_test_dir);

        // Write configuration
        std::ofstream config_file(ptop_tc_qdisc_test_dir + "/config_ns3.properties");
        config_file << "simulation_end_time_ns=1950000000" << std::endl;
        config_file << "simulation_seed=123456789" << std::endl;
        config_file << "topology_ptop_filename=\"topology.properties.temp\"" << std::endl;
        config_file << "enable_link_net_device_queue_tracking=true" << std::endl;
        config_file << "link_net_device_queue_tracking_enable_for_links=all" << std::endl;
        config_file << "enable_link_interface_tc_qdisc_queue_tracking=true" << std::endl;
        config_file << "link_interface_tc_qdisc_queue_tracking_enable_for_links=all" << std::endl;
        config_file << "enable_udp_burst_scheduler=true" << std::endl;
        config_file << "udp_burst_schedule_filename=\"udp_burst_schedule.csv\"" << std::endl;
        config_file.close();

        // Write topology
        std::ofstream topology_file;
        topology_file.open (ptop_tc_qdisc_test_dir + "/topology.properties.temp");
        topology_file << "num_nodes=3" << std::endl;
        topology_file << "num_undirected_edges=2" << std::endl;
        topology_file << "switches=set(0)" << std::endl;
        topology_file << "switches_which_are_tors=set(0)" << std::endl;
        topology_file << "servers=set(1,2)" << std::endl;
        topology_file << "undirected_edges=set(0-1, 0-2)" << std::endl;
        topology_file << "all_nodes_are_endpoints=true" << std::endl;
        topology_file << "link_channel_delay_ns=10000" << std::endl;
        topology_file << "link_net_device_data_rate_megabit_per_s=10" << std::endl;
        topology_file << "link_net_device_queue=map(0->1: drop_tail(1p), 1->0: drop_tail(100p), 0->2: drop_tail(100p), 2->0: drop_tail(100p))" << std::endl;
        topology_file << "link_net_device_receive_error_model=none" << std::endl;
        topology_file << "link_interface_traffic_control_qdisc=map(0->1: simple_red(drop; 1.0; 100; 500; 4000; 0.1; no_wait; gentle), 1->0: fifo(100p), 0->2: fifo(100p), 2->0: fifo(100p))" << std::endl;
        topology_file.close();

        // Write UDP burst file
        std::ofstream udp_burst_schedule_file;
        udp_burst_schedule_file.open (ptop_tc_qdisc_test_dir + "/udp_burst_schedule.csv");
        // 1000 Mbit/s for 6000000 ns = 500 packets
        // 1000 Mbit/s for 12000000 ns = 1000 packets
        // 2000 Mbit/s for 12000000 ns = 2000 packets
        udp_burst_schedule_file << "0,0,1,1000,0,12000000,," << std::endl;
        topology_file.close();

        // Create simulation environment
        Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>(ptop_tc_qdisc_test_dir);

        // Create topology
        Ptr<TopologyPtop> topology = CreateObject<TopologyPtop>(basicSimulation, Ipv4ArbiterRoutingHelper());
        ArbiterEcmpHelper::InstallArbiters(basicSimulation, topology);

        // Schedule UDP bursts
        UdpBurstScheduler udpBurstScheduler(basicSimulation, topology);

        // Install link net-device queue trackers
        PtopLinkNetDeviceQueueTracking netDeviceQueueTracking = PtopLinkNetDeviceQueueTracking(basicSimulation, topology); // Requires enable_link_net_device_queue_tracking=true

        // Install link interface traffic-control qdisc queue trackers
        PtopLinkInterfaceTcQdiscQueueTracking tcQdiscQueueTracking = PtopLinkInterfaceTcQdiscQueueTracking(basicSimulation, topology); // Requires enable_link_interface_tc_qdisc_queue_tracking=true

        // Run simulation
        basicSimulation->Run();

        // Write UDP bursts results
        udpBurstScheduler.WriteResults();

        // Write link net-device queue results
        netDeviceQueueTracking.WriteResults();

        // Write link interface traffic-control qdisc queue results
        tcQdiscQueueTracking.WriteResults();

        // Finalize the simulation
        basicSimulation->Finalize();

        // Links
        std::vector<std::pair<int64_t, int64_t>> links;
        links.push_back(std::make_pair(0, 1));
        links.push_back(std::make_pair(1, 0));
        links.push_back(std::make_pair(0, 2));
        links.push_back(std::make_pair(2, 0));

        // Get the link net-device queue development
        std::map<std::pair<int64_t, int64_t>, std::vector<std::tuple<int64_t, int64_t, int64_t>>> link_net_device_queue_pkt;
        std::map<std::pair<int64_t, int64_t>, std::vector<std::tuple<int64_t, int64_t, int64_t>>> link_net_device_queue_byte;
        validate_link_net_device_queue_logs(ptop_tc_qdisc_test_dir, links, link_net_device_queue_pkt, link_net_device_queue_byte);

        // Get the link interface traffic-control qdisc queue development
        std::map<std::pair<int64_t, int64_t>, std::vector<std::tuple<int64_t, int64_t, int64_t>>> link_interface_tc_qdisc_queue_pkt;
        std::map<std::pair<int64_t, int64_t>, std::vector<std::tuple<int64_t, int64_t, int64_t>>> link_interface_tc_qdisc_queue_byte;
        validate_link_interface_tc_qdisc_queue_logs(ptop_tc_qdisc_test_dir, links, link_interface_tc_qdisc_queue_pkt, link_interface_tc_qdisc_queue_byte);

        // Re-used for convenience
        std::vector<std::tuple<int64_t, int64_t, int64_t>> qdisc_queue_intervals = link_interface_tc_qdisc_queue_pkt.at(std::make_pair(0, 1));
        int64_t largest_queue_size_pkt = -1;

        // It got 1000 packets put into its queue at a rate of 1000 Mbit/s
        // In the time of the burst, it drained 10 Mbit/s, which is 10 packets
        // As such, 990 total packets would be the maximum queue size for first-in-first-out

        // 0 -> 1 has a 1 packets link net-device queue and simple_red(drop; 1.0; 100; 500; 4000; 0.2; no_wait; gentle) as its qdisc
        int queue_size_analysis = 0;
        double prob = 0.1;
        Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
        x->SetAttribute ("Min", DoubleValue (0.0));
        x->SetAttribute ("Max", DoubleValue (1.0));
        int count = 0;
        for (int i = 0; i < 1000; i++) {
            if (queue_size_analysis < 100) {
                queue_size_analysis += 1;
            } else {
                double p_b;
                count += 1;
                if (queue_size_analysis < 500) {
                    double p_a = prob * ((queue_size_analysis - 100.0) / (500.0 - 100.0));
                    p_b = p_a / (1.0 - count * p_a);
                } else {
                    double p_a = prob + (1.0 - prob) * ((queue_size_analysis - 500.0) / (1000.0 - 500.0));
                    p_b = p_a / (1.0 - count * p_a);
                }
                if (x->GetValue() >= p_b) {
                    queue_size_analysis += 1;
                } else {
                    count = 0;
                }
            }
        }
        qdisc_queue_intervals = link_interface_tc_qdisc_queue_pkt.at(std::make_pair(0, 1));
        largest_queue_size_pkt = -1;
        for (std::tuple<int64_t, int64_t, int64_t> interval : qdisc_queue_intervals) {
            int64_t interval_num_pkt = std::get<2>(interval);
            largest_queue_size_pkt = std::max(largest_queue_size_pkt, interval_num_pkt);
        }
        queue_size_analysis = queue_size_analysis - 10; // For the 10 Mbit/s sending out 10 packets over the queueing
        ASSERT_EQUAL_APPROX(largest_queue_size_pkt, queue_size_analysis, 10);

        // Template:
        // std::vector<std::tuple<int64_t, int64_t, int64_t>> qdisc_queue_intervals = link_interface_tc_qdisc_queue_pkt.at(std::make_pair(0, 1));
        // int64_t largest_queue_size_pkt = -1;
        // for (std::tuple<int64_t, int64_t, int64_t> interval : qdisc_queue_intervals) {
        //     int64_t interval_start_ns = std::get<0>(interval);
        //     int64_t interval_end_ns = std::get<1>(interval);
        //     int64_t interval_num_pkt = std::get<2>(interval);
        //     largest_queue_size_pkt = std::max(largest_queue_size_pkt, interval_num_pkt);
        // }
        // ASSERT_EQUAL_APPROX(largest_queue_size_pkt, AMOUNT, DEVIATION);

        // Clean up
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/config_ns3.properties");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/topology.properties.temp");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/udp_burst_schedule.csv");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/finished.txt");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/timing_results.txt");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/timing_results.csv");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/udp_bursts_incoming.csv");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/udp_bursts_incoming.txt");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/udp_bursts_outgoing.csv");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/udp_bursts_outgoing.txt");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/link_net_device_queue_byte.csv");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/link_net_device_queue_pkt.csv");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/link_interface_tc_qdisc_queue_byte.csv");
        remove_file_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3/link_interface_tc_qdisc_queue_pkt.csv");
        remove_dir_if_exists(ptop_tc_qdisc_test_dir + "/logs_ns3");
        remove_dir_if_exists(ptop_tc_qdisc_test_dir);

    }
};

////////////////////////////////////////////////////////////////////////////////////////
