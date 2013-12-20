///
/// \brief Implementation of SALLY agent and related classes.
///
/// This is the main file of this software because %SALLY's behaviour is
/// implemented here.
///

#include "sally-routing-protocol.h"

#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/node.h"
#include "ns3/ipv4-static-routing.h"

namespace ns3 {
namespace sally {

NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// Willingness for forwarding packets from other nodes: never.
#define OLSR_WILL_NEVER         0
/// Willingness for forwarding packets from other nodes: low.
#define OLSR_WILL_LOW           1
/// Willingness for forwarding packets from other nodes: medium.
#define OLSR_WILL_DEFAULT       3
/// Willingness for forwarding packets from other nodes: high.
#define OLSR_WILL_HIGH          6
/// Willingness for forwarding packets from other nodes: always.
#define OLSR_WILL_ALWAYS        7

TypeId RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::sally::RoutingProtocol")
	.SetParent<Ipv4RoutingProtocol> ()
    .AddConstructor<RoutingProtocol> ()
         .AddAttribute ("HelloInterval aodv", "HELLO messages emission interval.",
                       TimeValue (Seconds (1)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::HelloInterval),
                       MakeTimeChecker ())
        .AddAttribute ("RreqRetries", "Maximum number of retransmissions of RREQ to discover a route",
                       UintegerValue (2),
                       MakeUintegerAccessor (&aodv::RoutingProtocol::RreqRetries),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("RreqRateLimit", "Maximum number of RREQ per second.",
                       UintegerValue (10),
                       MakeUintegerAccessor (&aodv::RoutingProtocol::RreqRateLimit),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("RerrRateLimit", "Maximum number of RERR per second.",
                       UintegerValue (10),
                       MakeUintegerAccessor (&aodv::RoutingProtocol::RerrRateLimit),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("NodeTraversalTime", "Conservative estimate of the average one hop traversal time for packets and should include "
                       "queuing delays, interrupt processing times and transfer times.",
                       TimeValue (MilliSeconds (40)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::NodeTraversalTime),
                       MakeTimeChecker ())
        .AddAttribute ("NextHopWait", "Period of our waiting for the neighbour's RREP_ACK = 10 ms + NodeTraversalTime",
                       TimeValue (MilliSeconds (50)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::NextHopWait),
                       MakeTimeChecker ())
        .AddAttribute ("ActiveRouteTimeout", "Period of time during which the route is considered to be valid",
                       TimeValue (Seconds (3)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::ActiveRouteTimeout),
                       MakeTimeChecker ())
        .AddAttribute ("MyRouteTimeout", "Value of lifetime field in RREP generating by this node = 2 * max(ActiveRouteTimeout, PathDiscoveryTime)",
                       TimeValue (Seconds (11.2)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::MyRouteTimeout),
                       MakeTimeChecker ())
        .AddAttribute ("BlackListTimeout", "Time for which the node is put into the blacklist = RreqRetries * NetTraversalTime",
                       TimeValue (Seconds (5.6)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::BlackListTimeout),
                       MakeTimeChecker ())
        .AddAttribute ("DeletePeriod", "DeletePeriod is intended to provide an upper bound on the time for which an upstream node A "
                       "can have a neighbor B as an active next hop for destination D, while B has invalidated the route to D."
                       " = 5 * max (HelloInterval, ActiveRouteTimeout)",
                       TimeValue (Seconds (15)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::DeletePeriod),
                       MakeTimeChecker ())
        .AddAttribute ("NetDiameter", "Net diameter measures the maximum possible number of hops between two nodes in the network",
                       UintegerValue (35),
                       MakeUintegerAccessor (&aodv::RoutingProtocol::NetDiameter),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("NetTraversalTime", "Estimate of the average net traversal time = 2 * NodeTraversalTime * NetDiameter",
                       TimeValue (Seconds (2.8)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::NetTraversalTime),
                       MakeTimeChecker ())
        .AddAttribute ("PathDiscoveryTime", "Estimate of maximum time needed to find route in network = 2 * NetTraversalTime",
                       TimeValue (Seconds (5.6)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::PathDiscoveryTime),
                       MakeTimeChecker ())
        .AddAttribute ("MaxQueueLen", "Maximum number of packets that we allow a routing protocol to buffer.",
                       UintegerValue (64),
                       MakeUintegerAccessor (&aodv::RoutingProtocol::SetMaxQueueLen,
                                             &aodv::RoutingProtocol::GetMaxQueueLen),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("MaxQueueTime", "Maximum time packets can be queued (in seconds)",
                       TimeValue (Seconds (30)),
                       MakeTimeAccessor (&aodv::RoutingProtocol::SetMaxQueueTime,
                                         &aodv::RoutingProtocol::GetMaxQueueTime),
                       MakeTimeChecker ())
        .AddAttribute ("AllowedHelloLoss", "Number of hello messages which may be loss for valid link.",
                       UintegerValue (2),
                       MakeUintegerAccessor (&aodv::RoutingProtocol::AllowedHelloLoss),
                       MakeUintegerChecker<uint16_t> ())
        .AddAttribute ("GratuitousReply", "Indicates whether a gratuitous RREP should be unicast to the node originated route discovery.",
                       BooleanValue (true),
                       MakeBooleanAccessor (&aodv::RoutingProtocol::SetGratuitousReplyFlag,
                                            &aodv::RoutingProtocol::GetGratuitousReplyFlag),
                       MakeBooleanChecker ())
        .AddAttribute ("DestinationOnly", "Indicates only the destination may respond to this RREQ.",
                       BooleanValue (false),
                       MakeBooleanAccessor (&aodv::RoutingProtocol::SetDesinationOnlyFlag,
                                            &aodv::RoutingProtocol::GetDesinationOnlyFlag),
                       MakeBooleanChecker ())
        .AddAttribute ("EnableHello", "Indicates whether a hello messages enable.",
                       BooleanValue (true),
                       MakeBooleanAccessor (&aodv::RoutingProtocol::SetHelloEnable,
                                            &aodv::RoutingProtocol::GetHelloEnable),
                       MakeBooleanChecker ())
        .AddAttribute ("EnableBroadcast", "Indicates whether a broadcast data packets forwarding enable.",
                       BooleanValue (true),
                       MakeBooleanAccessor (&aodv::RoutingProtocol::SetBroadcastEnable,
                                            &aodv::RoutingProtocol::GetBroadcastEnable),
                       MakeBooleanChecker ())
        .AddAttribute ("UniformRv",
                       "Access to the underlying UniformRandomVariable",
                       StringValue ("ns3::UniformRandomVariable"),
                       MakePointerAccessor (&aodv::RoutingProtocol::m_uniformRandomVariable),
                       MakePointerChecker<UniformRandomVariable> ())
        ////OLSR
		   .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
							  TimeValue (Seconds (2)),
							  MakeTimeAccessor (&olsr::RoutingProtocol::m_helloInterval),
							  MakeTimeChecker ())
		   .AddAttribute ("TcInterval", "TC messages emission interval.",
						  TimeValue (Seconds (5)),
						  MakeTimeAccessor (&olsr::RoutingProtocol::m_tcInterval),
						  MakeTimeChecker ())
		   .AddAttribute ("MidInterval", "MID messages emission interval.  Normally it is equal to TcInterval.",
						  TimeValue (Seconds (5)),
						  MakeTimeAccessor (&olsr::RoutingProtocol::m_midInterval),
						  MakeTimeChecker ())
		   .AddAttribute ("HnaInterval", "HNA messages emission interval.  Normally it is equal to TcInterval.",
						  TimeValue (Seconds (5)),
						  MakeTimeAccessor (&olsr::RoutingProtocol::m_hnaInterval),
						  MakeTimeChecker ())
		   .AddAttribute ("Willingness", "Willingness of a node to carry and forward traffic for other nodes.",
						  EnumValue (OLSR_WILL_DEFAULT),
						  MakeEnumAccessor (&olsr::RoutingProtocol::m_willingness),
						  MakeEnumChecker (OLSR_WILL_NEVER, "never",
										   OLSR_WILL_LOW, "low",
										   OLSR_WILL_DEFAULT, "default",
										   OLSR_WILL_HIGH, "high",
										   OLSR_WILL_ALWAYS, "always"))
		   .AddTraceSource ("Rx", "Receive OLSR packet.",
							MakeTraceSourceAccessor (&RoutingProtocol::m_rxPacketTrace))
		   .AddTraceSource ("Tx", "Send OLSR packet.",
							MakeTraceSourceAccessor (&RoutingProtocol::m_txPacketTrace))
		   .AddTraceSource ("RoutingTableChanged", "The OLSR routing table has changed.",
							MakeTraceSourceAccessor (&RoutingProtocol::m_routingTableChanged))
  ;
  return tid;
}

RoutingProtocol::RoutingProtocol ()
  : m_ipv4 (0)
{
  NS_LOG_FUNCTION (this);
}

void
RoutingProtocol::DoInitialize (void)
{
	olsr::RoutingProtocol::DoInitialize();
	aodv::RoutingProtocol::DoInitialize();
	Ipv4RoutingProtocol::DoInitialize ();
}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, enum Socket::SocketErrno &sockerr)
{
	Ptr<Ipv4Route> route = olsr::RoutingProtocol::RouteOutput(p, header, oif, sockerr);
	if (route) {
		sockerr = Socket::ERROR_NOTERROR;
		return route;
	} else {
		Ptr<Ipv4Route> route_2 = aodv::RoutingProtocol::RouteOutput(p, header, oif, sockerr);
		if (route_2) {
			sockerr = Socket::ERROR_NOTERROR;
			return route_2;
		}
	}
	sockerr = Socket::ERROR_NOROUTETOHOST;
    return 0;
}

bool
RoutingProtocol::RouteInput  (Ptr<const Packet> p,
                                   const Ipv4Header &header, Ptr<const NetDevice> idev,
                                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                   LocalDeliverCallback lcb, ErrorCallback ecb)
{

	NS_LOG_FUNCTION (this << p << header << idev << &ucb << &mcb << &lcb << &ecb);
	  bool retVal = false;
	  NS_LOG_LOGIC ("RouteInput logic for node: " << m_ipv4->GetObject<Node> ()->GetId ());

	  NS_ASSERT (m_ipv4 != 0);
	  // Check if input device supports IP
	  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
	  uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);

	  retVal = m_ipv4->IsDestinationAddress (header.GetDestination (), iif);
	  if (retVal == true)
	    {
	      NS_LOG_LOGIC ("Address "<< header.GetDestination () << " is a match for local delivery");
	      if (header.GetDestination ().IsMulticast ())
	        {
	          Ptr<Packet> packetCopy = p->Copy ();
	          lcb (packetCopy, header, iif);
	          retVal = true;
	          // Fall through
	        }
	      else
	        {
	          lcb (p, header, iif);
	          return true;
	        }
	    }
	  // Check if input device supports IP forwarding
	  if (m_ipv4->IsForwarding (iif) == false)
	    {
	      NS_LOG_LOGIC ("Forwarding disabled for this interface");
	      ecb (p, header, Socket::ERROR_NOROUTETOHOST);
	      return false;
	    }

	  LocalDeliverCallback downstreamLcb = lcb;
	    if (retVal == true)
	      {
	        downstreamLcb = MakeNullCallback<void, Ptr<const Packet>, const Ipv4Header &, uint32_t > ();
	      }

	if (olsr::RoutingProtocol::RouteInput(p, header, idev, ucb, mcb, downstreamLcb, ecb)) {
		return true;
	} else if (aodv::RoutingProtocol::RouteInput(p, header, idev, ucb, mcb, downstreamLcb, ecb)) {
		return true;
	} else {
		return retVal;
	}
}

void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
	olsr::RoutingProtocol::NotifyInterfaceUp(i);
	aodv::RoutingProtocol::NotifyInterfaceUp(i);
}
void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
	olsr::RoutingProtocol::NotifyInterfaceDown(i);
	aodv::RoutingProtocol::NotifyInterfaceDown(i);
}
void
RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
	olsr::RoutingProtocol::NotifyAddAddress(interface, address);
	aodv::RoutingProtocol::NotifyAddAddress(interface, address);
}
void
RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
	olsr::RoutingProtocol::NotifyRemoveAddress(interface, address);
	aodv::RoutingProtocol::NotifyRemoveAddress(interface, address);
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
	olsr::RoutingProtocol::SetIpv4(ipv4);
	aodv::RoutingProtocol::SetIpv4(ipv4);

	m_ipv4 = ipv4;
}

void
RoutingProtocol::PrintRoutingTable(Ptr<OutputStreamWrapper> stream) const
{
	olsr::RoutingProtocol::PrintRoutingTable(stream);
	aodv::RoutingProtocol::PrintRoutingTable(stream);
}

void RoutingProtocol::DoDispose ()
{
	olsr::RoutingProtocol::DoDispose();
	aodv::RoutingProtocol::DoDispose();
	m_ipv4 = 0;
}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  olsr::RoutingProtocol::m_uniformRandomVariable->SetStream (stream);
  aodv::RoutingProtocol::m_uniformRandomVariable->SetStream (stream);
  return 1;
}

} // namespace olsr
} // namespace ns3


