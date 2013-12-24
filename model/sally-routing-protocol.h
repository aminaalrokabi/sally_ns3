#ifndef SALLY_AGENT_IMPL_H
#define SALLY_AGENT_IMPL_H

#include "ns3/ipv4-list-routing.h"

namespace ns3 {
namespace sally {

///
/// \ingroup sally
///
/// \brief SALLY routing protocol for IPv4
///
class RoutingProtocol: public Ipv4ListRouting
{
	public:
	  static TypeId GetTypeId (void);

	  // Below are from Ipv4RoutingProtocol
	  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

	  virtual bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
							   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
							   LocalDeliverCallback lcb, ErrorCallback ecb);


};

}
}  // namespace ns3

#endif /* SALLY_AGENT_IMPL_H */
