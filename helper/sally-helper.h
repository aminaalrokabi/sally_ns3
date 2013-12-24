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

#ifndef SALLY_HELPER_H
#define SALLY_HELPER_H

#include "ns3/ipv4-list-routing-helper.h"

namespace ns3
{

class SallyHelper : public Ipv4ListRoutingHelper
{
public:
  /*
   * Construct an Ipv4ListRoutingHelper used to make installing routing
   * protocols easier.
   */
	SallyHelper ();

  /*
   * \internal
   * Destroy an Ipv4ListRoutingHelper.
   */
  virtual ~SallyHelper ();
  SallyHelper (const SallyHelper &);
    /**
     * \internal
     * \returns pointer to clone of this Ipv4ListRoutingHelper
     *
     * This method is mainly for internal use by the other helpers;
     * clients are expected to free the dynamic memory allocated by this method
     */
  SallyHelper* Copy (void) const;
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
  void SetNumberHybridNodes(int num);
private:
  /**
   * \internal
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   */
  SallyHelper &operator = (const SallyHelper &o);
  std::list<std::pair<const Ipv4RoutingHelper *,int16_t> > m_list;
  int number_of_hybrid_nodes;
};
}

#endif /* AODV_HELPER_H */
