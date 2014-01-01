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
#include "ns3/node.h"

#include "ns3/aodv-routing-protocol.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/aodv-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/solsr-helper.h"
#include "ns3/sally-routing.h"
#include "ns3/enum.h"
#include "ns3/config.h"

namespace ns3
{

SallyHelper::SallyHelper():
		Ipv4ListRoutingHelper (), number_of_hybrid_nodes(0)
{
}

SallyHelper::~SallyHelper()
{
}

void
SallyHelper::SetNumberHybridNodes(int num) {
	number_of_hybrid_nodes = num;
}

SallyHelper::SallyHelper (const SallyHelper &o)
{
  SOlsrHelper olsr;
  AodvHelper aodv;
  std::list<std::pair<const Ipv4RoutingHelper *, int16_t> >::const_iterator i;
  for (i = o.m_list.begin (); i != o.m_list.end (); ++i)
    {
      m_list.push_back (std::make_pair (const_cast<const Ipv4RoutingHelper *> (i->first->Copy ()), i->second));
    }

  m_list.push_back (std::make_pair (const_cast<const ns3::SOlsrHelper *> (olsr.Copy ()), 20));
  m_list.push_back (std::make_pair (const_cast<const ns3::AodvHelper *> (aodv.Copy ()), 10));
  number_of_hybrid_nodes = o.number_of_hybrid_nodes;
}

SallyHelper*
SallyHelper::Copy (void) const
{
  return new SallyHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
SallyHelper::Create (Ptr<Node> node) const
{
  static int num_created = 0;
  Ptr<SallyRouting> list = CreateObject<SallyRouting> ();
  Config::SetDefault ("ns3::olsr::RoutingProtocol::HelloInterval", TimeValue (Seconds (4)));

  for (std::list<std::pair<const Ipv4RoutingHelper *, int16_t> >::const_iterator i = m_list.begin ();
		  i != m_list.end (); ++i)
  {
	  Ptr<Ipv4RoutingProtocol> prot = i->first->Create (node);
	  if (prot->GetInstanceTypeId().GetName() == "ns3::aodv::RoutingProtocol") {
		  if (num_created >= number_of_hybrid_nodes) {
			  continue;
		  }
	  }
	  list->AddRoutingProtocol (prot,i->second);
  }
  num_created++;
  return list;

}

}

