/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of Kansas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Justin Rohrer <rohrej@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

/*
 * This example program allows one to run ns-3 DSDV, AODV, or OLSR under
 * a typical random waypoint mobility model.
 *
 * By default, the simulation runs for 200 simulated seconds, of which
 * the first 50 are used for start-up time.  The number of nodes is 50.
 * Nodes move according to RandomWaypointMobilityModel with a speed of
 * 20 m/s and no pause time within a 300x1500 m region.  The WiFi is
 * in ad hoc mode with a 2 Mb/s rate (802.11b) and a Friis loss model.
 * The transmit power is set to 7.5 dBm.
 *
 * It is possible to change the mobility and density of the network by
 * directly modifying the speed and the number of nodes.  It is also
 * possible to change the characteristics of the network by changing
 * the transmit power (as power increases, the impact of mobility
 * decreases and the effective density increases).
 *
 * By default, OLSR is used, but specifying a value of 2 for the protocol
 * will cause AODV to be used, and specifying a value of 3 will cause
 * DSDV to be used.
 *
 * By default, there are 10 source/sink data pairs sending UDP data
 * at an application rate of 2.048 Kb/s each.    This is typically done
 * at a rate of 4 64-byte packets per second.  Application data is
 * started at a random time between 50 and 51 seconds and continues
 * to the end of the simulation.
 *
 * The program outputs a few items:
 * - packet receptions are notified to stdout such as:
 *   <timestamp> <node-id> received one packet from <src-address>
 * - each second, the data reception statistics are tabulated and output
 *   to a comma-separated value (csv) file
 * - some tracing and flow monitor configuration that used to work is
 *   left commented inline in the program
 */

#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/sally-helper.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE ("manet-routing-compare");

class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (double txp);
  std::string CommandSetup (int argc, char **argv);

private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);
  void SetupRoutingPacketReceive (int nodeId,std::string protocolName);
  void ReceivePacket (Ptr<Socket> socket);
  void ReceiveOLSRPacket (std::string context, const PacketHeader &p, const MessageList &m);
  void SendOLSRPacket (std::string context, const PacketHeader &p, const MessageList &m);
  void ReceiveAODVPacket (std::string context, uint32_t i);
  void SendAODVPacket (std::string context, uint32_t i);

  int nSinks;
  int nNodes;
  int nPackets;
  int nControlPackets;
  std::string protocolName;
};

RoutingExperiment::RoutingExperiment ()
  : nSinks (5), nNodes(20), nPackets(0), nControlPackets(0), protocolName("SALLY")
{
}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet)
{
  SocketAddressTag tag;
  bool found;
  found = packet->PeekPacketTag (tag);
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (found)
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (tag.GetAddress ());
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void
RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  while ((packet = socket->Recv ()))
    {
	  nPackets++;
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet));
    }
}

void
RoutingExperiment::ReceiveOLSRPacket (std::string context, const PacketHeader &p, const MessageList &m)
{
	nControlPackets++;
}

void
RoutingExperiment::SendOLSRPacket (std::string context, const PacketHeader &p, const MessageList &m)
{
	nControlPackets++;
}

void
RoutingExperiment::ReceiveAODVPacket (std::string context, uint32_t i)
{
	nControlPackets++;
}

void
RoutingExperiment::SendAODVPacket (std::string context, uint32_t i)
{
	nControlPackets++;
}


void
RoutingExperiment::SetupRoutingPacketReceive (int nodeId, std::string protocolName)
{
  if (protocolName =="OLSR" || protocolName =="CHAINED" || protocolName =="SALLY") {
	  std::ostringstream oss;
	  oss << "/NodeList/" << nodeId << "/$ns3::olsr::RoutingProtocol/Rx";
	  Config::Connect (oss.str (), MakeCallback (&RoutingExperiment::ReceiveOLSRPacket, this));

	  std::ostringstream oss1;
	  oss1 << "/NodeList/" << nodeId << "/$ns3::olsr::RoutingProtocol/Tx";
	  Config::Connect (oss1.str (), MakeCallback (&RoutingExperiment::SendOLSRPacket, this));
  }
  if (protocolName =="AODV" || protocolName =="CHAINED" || protocolName =="SALLY") {
  	  std::ostringstream oss;
  	  oss << "/NodeList/" << nodeId << "/$ns3::aodv::RoutingProtocol/Rx";
  	  Config::Connect (oss.str (), MakeCallback (&RoutingExperiment::ReceiveAODVPacket, this));

  	  std::ostringstream oss1;
  	  oss1 << "/NodeList/" << nodeId << "/$ns3::aodv::RoutingProtocol/Tx";
  	  Config::Connect (oss1.str (), MakeCallback (&RoutingExperiment::SendAODVPacket, this));
  }


}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, 9);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

  return sink;
}

std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd;
  cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=SALLY,5=DSR", protocolName);
  cmd.AddValue ("numNodes", "Number of nodes", nNodes);
  cmd.AddValue ("numSinks", "Number of sinks", nSinks);
  cmd.Parse (argc, argv);
  return protocolName;
}

int
main (int argc, char *argv[])
{
  RoutingExperiment experiment;
  double txp = 7.5;
  experiment.CommandSetup (argc,argv);
  experiment.Run (txp);
}

void
RoutingExperiment::Run (double txp)
{
  Packet::EnablePrinting ();

  double TotalTime = 100.0;
  std::string rate ("64kbps");
  std::string phyMode ("DsssRate11Mbps");
  std::string tr_name ("manet-routing-compare");
  double nodeSpeed = 1.5;
  int nodePause = 10;

  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer adhocNodes;
  adhocNodes.Create (nNodes);

  // setting up wifi phy and channel using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);

  MobilityHelper mobilityAdhoc;
  int64_t streamIndex = 0; // used to get consistent mobility across scenarios

  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
  mobilityAdhoc.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                                  "Speed", StringValue (ssSpeed.str ()),
                                  "Pause", StringValue (ssPause.str ()));
  mobilityAdhoc.Install (adhocNodes);
  streamIndex += mobilityAdhoc.AssignStreams (adhocNodes, streamIndex);

  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;
  DsrHelper dsr;

  DsrMainHelper dsrMain;
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

    if (protocolName== "OLSR") {
      list.Add (olsr, 100);
      internet.SetRoutingHelper (list);
	  internet.Install (adhocNodes);
    } else if (protocolName == "AODV") {
      list.Add (aodv, 100);
      internet.SetRoutingHelper (list);
      internet.Install (adhocNodes);
    } else if (protocolName == "CHAINED") {
      list.Add (olsr, 200);
      list.Add (aodv, 100);
      internet.SetRoutingHelper (list);
      internet.Install (adhocNodes);
    } else if (protocolName =="SALLY") {
      SallyHelper sally;
      sally.SetNumberHybridNodes(nNodes);
      internet.SetRoutingHelper (sally);
      internet.Install (adhocNodes);
    } else {
      NS_FATAL_ERROR ("No such protocol:");
    }


  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);

  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

  for (int i=0; i< nNodes; i++) {
	  SetupRoutingPacketReceive (i, protocolName);
  }
  for (int i = 0; i <= nSinks - 1; i++)
    {
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i), adhocNodes.Get (i));

      AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (i), 9));
      onoff1.SetAttribute ("Remote", remoteAddress);

      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      ApplicationContainer temp = onoff1.Install (adhocNodes.Get (i + nSinks));
      temp.Start (Seconds (var->GetValue (50.0,51.0)));
      temp.Stop (Seconds (TotalTime));
    }

  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();

  NS_LOG_INFO ("Run Simulation.");

  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();
  std::ostringstream filename;
  filename << protocolName << ".flomonitor." << nNodes;
  flowmon->SerializeToXmlFile(filename.str().c_str(), false, false);

  std::ostringstream filename2;
  filename2 << protocolName << ".custom." << nNodes;
  std::ofstream os (filename2.str().c_str(), std::ios::out|std::ios::binary);
  os << "<?xml version=\"1.0\" ?>\n";
  os << "<CustomStats>\n<RoutingStats numPackets=\"" << nPackets << "\" numControlPackets=\"" << nControlPackets << "\" />\n</CustomStats>";
  os.close();
  Simulator::Destroy ();
}

