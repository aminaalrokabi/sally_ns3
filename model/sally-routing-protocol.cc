///
/// \brief Implementation of SALLY agent and related classes.
///
/// This is the main file of this software because %SALLY's behaviour is
/// implemented here.
///

#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/node.h"
#include "ns3/ipv4-static-routing.h"
#include "sally-routing-protocol.h"

namespace ns3 {
namespace sally {

NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

TypeId RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::sally::RoutingProtocol")
	.SetParent<Ipv4ListRouting> ()
    .AddConstructor<RoutingProtocol> ()
  ;
  return tid;
}

RoutingProtocol::RoutingProtocol ()
{
  NS_LOG_FUNCTION (this);
}

RoutingProtocol::~RoutingProtocol ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, enum Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << p << header.GetDestination () << header.GetSource () << oif << sockerr);
  Ptr<Ipv4Route> route;
  int16_t priority;
  Ptr<Ipv4RoutingProtocol> protocol1 = GetRoutingProtocol(0, priority);

  route = protocol1->RouteOutput (p, header, oif, sockerr);
  if (route) {
	  sockerr = Socket::ERROR_NOTERROR;
	  return route;
  }
  sockerr = Socket::ERROR_NOROUTETOHOST;
  return 0;
}

bool
RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
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
  // Next, try to find a route
  // If we have already delivered a packet locally (e.g. multicast)
  // we suppress further downstream local delivery by nulling the callback
  LocalDeliverCallback downstreamLcb = lcb;
  if (retVal == true)
    {
      downstreamLcb = MakeNullCallback<void, Ptr<const Packet>, const Ipv4Header &, uint32_t > ();
    }
  int16_t priority;
  Ptr<Ipv4RoutingProtocol> protocol1 = GetRoutingProtocol(0, priority);
  if (protocol1->RouteInput (p, header, idev, ucb, mcb, downstreamLcb, ecb)) {
	  return true;
  }
  return retVal;
}

} // namespace olsr
} // namespace ns3


