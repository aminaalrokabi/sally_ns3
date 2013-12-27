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
 */

#ifndef SALLY_ROUTING_H
#define SALLY_ROUTING_H

#include <list>
#include "ns3/ipv4-list-routing.h"
#include "ns3/simulator.h"

namespace ns3 {

/**
 * \ingroup internet
 * \defgroup ipv4ListRouting Ipv4 List Routing
 */
/**
 * \ingroup ipv4ListRouting
 *
 * This class is a specialization of Ipv4RoutingProtocol that allows
 * other instances of Ipv4RoutingProtocol to be inserted in a
 * prioritized list.  Routing protocols in the list are consulted one
 * by one, from highest to lowest priority, until a routing protocol
 * is found that will take the packet (this corresponds to a non-zero
 * return value to RouteOutput, or a return value of true to RouteInput).
 * The order by which routing protocols with the same priority value
 * are consulted is undefined.
 *
 */
class SallyRouting : public Ipv4ListRouting
{
public:
  static TypeId GetTypeId (void);

  // Below are from Ipv4RoutingProtocol
  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

  virtual bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                           UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                           LocalDeliverCallback lcb, ErrorCallback ecb);
};

} // namespace ns3

#endif /* IPV4_LIST_ROUTING_H */
