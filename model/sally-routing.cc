/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
 */

#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/node.h"
#include "ns3/ipv4-static-routing.h"
#include "sally-routing.h"
#include "ns3/solsr-routing-protocol.h"

NS_LOG_COMPONENT_DEFINE ("SallyRouting");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SallyRouting);

TypeId
SallyRouting::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SallyRouting")
    .SetParent<Ipv4ListRouting> ()
    .AddConstructor<SallyRouting> ()
  ;
  return tid;
}

Ptr<Ipv4Route>
SallyRouting::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, enum Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << p << header.GetDestination () << header.GetSource () << oif << sockerr);
  Ptr<Ipv4Route> route;

  bool useAodv = false;

  for (Ipv4RoutingProtocolList::const_iterator i = m_routingProtocols.begin ();
       i != m_routingProtocols.end (); i++)
    {
	  if ((*i).second->GetInstanceTypeId ().GetName() == "ns3::sally::SOlsrRoutingProtocol") {
		  Ptr<sally::SOlsrRoutingProtocol> olsr_prot = DynamicCast<sally::SOlsrRoutingProtocol> ((*i).second);
		  useAodv = olsr_prot->m_state.GetMprSelectors ().size () > 0;
	  }
	  if ((*i).second->GetInstanceTypeId ().GetName() == "ns3::aodv::RoutingProtocol" && !useAodv) {
		  continue;
	  }
      NS_LOG_LOGIC ("Checking protocol " << (*i).second->GetInstanceTypeId () << " with priority " << (*i).first);
      NS_LOG_LOGIC ("Requesting source address for destination " << header.GetDestination ());
      route = (*i).second->RouteOutput (p, header, oif, sockerr);
      if (route)
        {
          NS_LOG_LOGIC ("Found route " << route);
          sockerr = Socket::ERROR_NOTERROR;
          return route;
        }
    }
  NS_LOG_LOGIC ("Done checking " << GetTypeId ());
  NS_LOG_LOGIC ("");
  sockerr = Socket::ERROR_NOROUTETOHOST;
  return 0;
}

// Patterned after Linux ip_route_input and ip_route_input_slow
bool
SallyRouting::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
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
  bool useAodv = false;
  for (Ipv4RoutingProtocolList::const_iterator rprotoIter =
         m_routingProtocols.begin ();
       rprotoIter != m_routingProtocols.end ();
       rprotoIter++)
    {
	  if ((*rprotoIter).second->GetInstanceTypeId ().GetName() == "ns3::sally::SOlsrRoutingProtocol") {
		  Ptr<sally::SOlsrRoutingProtocol> olsr_prot = DynamicCast<sally::SOlsrRoutingProtocol> ((*rprotoIter).second);
		  useAodv = olsr_prot->m_state.GetMprSelectors ().size () > 0;
	  }
	  if ((*rprotoIter).second->GetInstanceTypeId ().GetName() == "ns3::aodv::RoutingProtocol" && !useAodv) {
		  continue;
	  }

      if ((*rprotoIter).second->RouteInput (p, header, idev, ucb, mcb, downstreamLcb, ecb))
        {
          NS_LOG_LOGIC ("Route found to forward packet in protocol " << (*rprotoIter).second->GetInstanceTypeId ().GetName ());
          return true;
        }
    }
  // No routing protocol has found a route.
  return retVal;
}

} // namespace ns3

