/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// my-ndn-simple.cpp
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/log.h"

// applications
#include "ndn-cxx-simple2/ndnAppStarterAgent.hpp"
#include "ndn-cxx-simple2/ndnAppStarterManager.hpp"
#include "ndn-cxx-simple2/ndnAppStarterConsumer2.hpp"

// random number generator
#include <stdlib.h>
#include <time.h>

NS_LOG_COMPONENT_DEFINE("my-ndn-simple2");
// NS_LOG=my-ndn-simple2 ./waf --run=my-ndn-simple --vis

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

enum ReturnCode {
  RC_ERROR   = -1,
  RC_SUCCESS = 0
};

struct _Prefix2NodeMapping {
  Ptr<Node> node;
  string prefix;
};
#define MAX_NODE_CNT 10000
_Prefix2NodeMapping _prefix_tbl[MAX_NODE_CNT];
static int _prefix_tbl_size = 0;

_Prefix2NodeMapping _nfd_tbl[MAX_NODE_CNT];
static int _nfd_tbl_size = 0;

int
addNfdTable(string nfd_name, Ptr<Node> node)
{
  if (_nfd_tbl_size == MAX_NODE_CNT)
    return RC_ERROR;
  _nfd_tbl[_nfd_tbl_size].node = node;
  _nfd_tbl[_nfd_tbl_size].prefix = nfd_name;
  ++_nfd_tbl_size;
  return RC_SUCCESS;
}

void
addAllNodesToNfdTable(NodeContainer& nc, string name)
{
  for (int i=0; i<(int)nc.GetN(); ++i)
  {
    addNfdTable(name + "[" + std::to_string(i) + "]", nc.Get(i));
  }
}

int
addPrefixTable(string prefix, Ptr<Node> node)
{
  if (_prefix_tbl_size == MAX_NODE_CNT)
    return RC_ERROR;
  _prefix_tbl[_prefix_tbl_size].node = node;
  _prefix_tbl[_prefix_tbl_size].prefix = prefix;
  ++_prefix_tbl_size;
  return RC_SUCCESS;
}

void
logNodeTables(std::string file_path)
{
  std::ofstream out(file_path);
  for (int i=0; i<_prefix_tbl_size; ++i)
  {
    out << _prefix_tbl[i].node->GetId() << " " << _prefix_tbl[i].prefix << endl;
  }
  for (int i=0; i<_nfd_tbl_size; ++i)
  {
    out << _nfd_tbl[i].node->GetId() << " " << _nfd_tbl[i].prefix << endl;
  }
  out.close();
}

void
AddAllOrigins(ndn::GlobalRoutingHelper& gh)
{
  for (int i=0; i<_prefix_tbl_size; ++i)
  {
    gh.AddOrigins(_prefix_tbl[i].prefix, _prefix_tbl[i].node);
  }
}

void AddAllRoute() {
  for (int i=0; i<_prefix_tbl_size; ++i)
  {
    if (_prefix_tbl[i].prefix != "/available")
      ndn::StrategyChoiceHelper::InstallAll(_prefix_tbl[i].prefix, "/localhost/nfd/strategy/best-route");
  }
  ndn::StrategyChoiceHelper::InstallAll("/available", "/localhost/nfd/strategy/multicast"); // BAD CODE
}


ndn::AppHelper _managerHelper("ndnAppStarterManager");
ndn::AppHelper _agentHelper("ndnAppStarterAgent");
ndn::AppHelper _consumerHelper("ndnAppStarterConsumer2");

void
installAgentApp(Ptr<Node> node,
    int manager_id,
    int agent_id,
    int exec_time,
    bool is_log_on)
{
  std::string prefix = "/Agent" + std::to_string(agent_id);
  std::string child_prefix = "/UnderAgent" + std::to_string(agent_id);
  addPrefixTable(prefix, node);
  addPrefixTable(child_prefix, node);
  addPrefixTable("/available", node);  // BAD CODE
  ndn::AppHelper& ah = _agentHelper;
  ah.SetAttribute("Prefix", StringValue(prefix));
  ah.SetAttribute("ChildPrefix", StringValue(child_prefix));
  ah.SetAttribute("ParentPrefix", StringValue("/Manager" + std::to_string(manager_id)));
  ah.SetAttribute("AgentID", IntegerValue(agent_id));
  ah.SetAttribute("IsLogOn", BooleanValue(is_log_on));
  ah.SetAttribute("ExecTime", IntegerValue(exec_time));
  ApplicationContainer ac = ah.Install(node);
  ac.Start(Seconds(5.0)); // application start after 5 seconds
  ac.Stop(Seconds(30.0));
}


void
installManagerApp(Ptr<Node> node,
    int manager_id,
    bool is_log_on)
{
  std::string prefix = "/Manager" + std::to_string(manager_id);
  addPrefixTable(prefix, node);
  std::string child_prefix = "/UnderManager" + std::to_string(manager_id);
  addPrefixTable(child_prefix, node);
  ndn::AppHelper& ah = _managerHelper;
  ah.SetAttribute("Prefix", StringValue(prefix));
  ah.SetAttribute("ChildPrefix", StringValue(child_prefix));
  ah.SetAttribute("ManagerID", IntegerValue(manager_id));
  ah.SetAttribute("IsLogOn", BooleanValue(is_log_on));
  ApplicationContainer ac = ah.Install(node);
  ac.Start(Seconds(5.0)); // application start after 5 seconds
  ac.Stop(Seconds(30.0));
}


void
installConsumerApp(Ptr<Node> node)
{
  std::string prefix = "/Consumer";
  addPrefixTable(prefix, node);
  ndn::AppHelper& ah = _consumerHelper;
  ah.SetAttribute("Prefix", StringValue(prefix));
  ApplicationContainer ac = ah.Install(node);
  ac.Start(Seconds(10.0)); // application start after 5 seconds
  ac.Stop(Seconds(30.0));
}

void
connectAllNodes(NodeContainer& nc1, NodeContainer& nc2, PointToPointHelper& p2p) {
  for (int i=0; i<(int)nc1.GetN(); ++i)
    for (int j=0; j<(int)nc2.GetN(); ++j)
    {
        p2p.Install(nc1.Get(i), nc2.Get(j));
    }
}

void
connectNodesRandomly(NodeContainer& nc1, NodeContainer& nc2, PointToPointHelper& p2p, int percentage) {
  for (int i=0; i<(int)nc1.GetN(); ++i)
  {
    int n_link = 0;
    for (int j=0; j<(int)nc2.GetN(); ++j)
    {
      if ((std::rand() % 100) < percentage) // percentage% chance to connect
      {
        p2p.Install(nc1.Get(i), nc2.Get(j));
        n_link++;
      }
    }
    if (n_link == 0)
    {
      // prevent lonely node
      p2p.Install(nc1.Get(i), nc2.Get(std::rand() % (int)nc2.GetN()));
    }
  }


}

int
main(int argc, char* argv[])
{
  std::srand (std::time(NULL));

  NS_LOG_DEBUG("start main");
  // setting default parameters for PointToPoint links and channels
  //Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("400Mbps"));
  //Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("2ms"));
  //Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("2048"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Install p2p connection on all nodes
  PointToPointHelper p2p_node; // for connecting nodes
  p2p_node.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p_node.SetChannelAttribute ("Delay", StringValue ("5ms"));

  PointToPointHelper p2p_router; // for connecting routers
  p2p_router.SetDeviceAttribute ("DataRate", StringValue ("400Mbps"));
  p2p_router.SetChannelAttribute ("Delay", StringValue ("1ms"));

  PointToPointHelper p2p_area; // for connecting areas
  p2p_area.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  p2p_area.SetChannelAttribute ("Delay", StringValue ("20ms"));


  // Creating nodes and connecting them
  NS_LOG_DEBUG("creating network topo");
  /*
    create nodes and randomly connect them together
  */

  // create node containers for routers and servers for different NFD settings
  NodeContainer nc_nfd_noCache;
  NodeContainer nc_nfd_withCache;

  // create node containers for routers and diff types of servers
  const int n_areas = 8; // 4 areas
  NodeContainer nc_routers[n_areas];
  NodeContainer nc_servers_high[n_areas];
  NodeContainer nc_servers_mid[n_areas];
  NodeContainer nc_servers_low[n_areas];
  const int n_routers = 4;
  const int n_servers_high = 2;
  const int n_servers_mid = 8;
  const int n_servers_low = 4;
  const int exec_time_servers_high = 8;
  const int exec_time_servers_mid = 80;
  const int exec_time_servers_low = 800;


  // create manager node
  NS_LOG_DEBUG("create manager");
  NodeContainer nc_managers;
  nc_managers.Create(1);
  nc_nfd_noCache.Add(nc_managers);
  int manager_id = 0;
  installManagerApp(nc_managers.Get(0), manager_id, true);

  // create and link nodes inside each area
  NS_LOG_DEBUG("create devices and agents");
  int agent_id = 0;
  for (int i=0; i<n_areas; i++)
  {
    nc_routers[i].Create(n_routers);
    nc_nfd_withCache.Add(nc_routers[i]);
    addAllNodesToNfdTable(nc_routers[i], "nc_routers[" + std::to_string(i) + "]");
    connectNodesRandomly(nc_routers[i], nc_routers[i], p2p_router, 70);
    // connect manager to all routers
    // connectAllNodes(nc_manager, nc_routers[i], p2p);

    nc_servers_high[i].Create(n_servers_high);
    nc_nfd_noCache.Add(nc_servers_high[i]);
    addAllNodesToNfdTable(nc_servers_high[i], "nc_servers_high[" + std::to_string(i) + "]");
    connectNodesRandomly(nc_servers_high[i], nc_routers[i], p2p_node, 25);
    for (int j=0; j<n_servers_high; j++)
    {
      installAgentApp(nc_servers_high[i].Get(j), manager_id, agent_id++, true, exec_time_servers_high);
    }


    nc_servers_mid[i].Create(n_servers_mid);
    nc_nfd_noCache.Add(nc_servers_mid[i]);
    addAllNodesToNfdTable(nc_servers_mid[i], "nc_servers_mid[" + std::to_string(i) + "]");
    connectNodesRandomly(nc_servers_mid[i], nc_routers[i], p2p_node, 25);
    for (int j=0; j<n_servers_mid; j++)
    {
      installAgentApp(nc_servers_mid[i].Get(j), manager_id, agent_id++, true, exec_time_servers_mid);
    }

    nc_servers_low[i].Create(n_servers_low);
    nc_nfd_noCache.Add(nc_servers_low[i]);
    addAllNodesToNfdTable(nc_servers_low[i], "nc_servers_low[" + std::to_string(i) + "]");
    connectNodesRandomly(nc_servers_low[i], nc_routers[i], p2p_node, 25);
    for (int j=0; j<n_servers_low; j++)
    {
      installAgentApp(nc_servers_low[i].Get(j), manager_id, agent_id++, true, exec_time_servers_low);
    }
  }
  NS_LOG_DEBUG("number of agents=" << agent_id);

  // connect area and area
  for (int i=0; i<n_areas-1; i++)
  {
    p2p_area.Install(nc_routers[i].Get(0), nc_routers[i+1].Get(0));
  }

  // create 1 consumer
  NS_LOG_DEBUG("create consumer");
  NodeContainer nc_consumers;
  nc_consumers.Create(1);
  nc_nfd_noCache.Add(nc_consumers);
  installConsumerApp(nc_consumers.Get(0));
  // connect manager and consumer
  p2p_node.Install(nc_managers.Get(0), nc_routers[0].Get(0));
  p2p_node.Install(nc_consumers.Get(0), nc_routers[n_areas-1].Get(0));

  // Install NDN stack on all nodes after all nodes are connected (otherwise will fail)
  ndn::StackHelper ndnNFD;
  //ndnNFD.InstallAll();
  NS_LOG_DEBUG("Install NDN stack: nc_nfd_noCache");
  ndnNFD.SetOldContentStore("ns3::ndn::cs::Nocache");
  ndnNFD.Install(nc_nfd_noCache);
  NS_LOG_DEBUG("Install NDN stack: nc_nfd_withCache");
  ndnNFD.SetOldContentStore("ns3::ndn::cs::Freshness::Lru", "MaxSize", "10000");
  ndnNFD.Install(nc_nfd_withCache);
  NS_LOG_DEBUG("Install NDN stack: done");


  // Add /prefix origins to ndn::GlobalRouter
  // Installing global routing interface on all nodes
  NS_LOG_DEBUG("add origins and routing");
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  AddAllOrigins(ndnGlobalRoutingHelper);
  // Choosing forwarding strategy
  AddAllRoute();
  ndn::GlobalRoutingHelper::CalculateRoutes();

  ndn::CsTracer::InstallAll("mylog/cs-trace.txt", MilliSeconds(100)); // sampling rate: every 0.1 second
  L2RateTracer::InstallAll("mylog/l2-drop-trace.txt", MilliSeconds(100));
  ndn::L3RateTracer::InstallAll("mylog/l3-rate-trace.txt", MilliSeconds(100));
  logNodeTables("mylog/Prefix2NodeMapping.txt");
  ndn::AppDelayTracer::InstallAll("mylog/app-delays-trace.txt");

  Simulator::Stop(Seconds(33.0));
  NS_LOG_DEBUG("Simulator::Run");
  Simulator::Run();
  Simulator::Destroy();

  NS_LOG_DEBUG("end main");

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
