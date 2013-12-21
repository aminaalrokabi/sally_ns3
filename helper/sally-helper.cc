/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * Authors: Pavel Boyko <boyko@iitp.ru>, written after OlsrHelper by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "sally-helper.h"
#include "ns3/sally-routing-protocol.h"
#include "ns3/node.h"
#include "ns3/aodv-helper.h"
#include "ns3/olsr-helper.h"

namespace ns3
{

SallyHelper::SallyHelper()
{
  OlsrHelper olsr;
  AodvHelper aodv;
  Add (olsr, 20);
  Add (aodv, 30);
}

SallyHelper::~SallyHelper()
{
}

Ptr<Ipv4RoutingProtocol>
Ipv4ListRoutingHelper::Create (Ptr<Node> node) const
{
  Ptr<ns3::sally::RoutingProtocol> list = CreateObject<ns3::sally::RoutingProtocol> ();
  for (std::list<std::pair<const Ipv4RoutingHelper *, int16_t> >::const_iterator i = m_list.begin ();
       i != m_list.end (); ++i)
    {
      Ptr<Ipv4RoutingProtocol> prot = i->first->Create (node);
      list->AddRoutingProtocol (prot,i->second);
    }
  return list;
}

}

