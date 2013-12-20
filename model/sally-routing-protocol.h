#ifndef SALLY_AGENT_IMPL_H
#define SALLY_AGENT_IMPL_H

#include "ns3/aodv-routing-protocol.h"
#include "ns3/olsr-routing-protocol.h"

namespace ns3 {
namespace sally {

///
/// \ingroup sally
///
/// \brief SALLY routing protocol for IPv4
///
class RoutingProtocol : public olsr::RoutingProtocol, public aodv::RoutingProtocol
{
  public:
	RoutingProtocol ();
	static TypeId GetTypeId (void);
	virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p,
	                                      const Ipv4Header &header,
	                                      Ptr<NetDevice> oif,
	                                      Socket::SocketErrno &sockerr);
	  virtual bool RouteInput (Ptr<const Packet> p,
	                           const Ipv4Header &header,
	                           Ptr<const NetDevice> idev,
	                           UnicastForwardCallback ucb,
	                           MulticastForwardCallback mcb,
	                           LocalDeliverCallback lcb,
	                           ErrorCallback ecb);
	  virtual void NotifyInterfaceUp (uint32_t interface);
	  virtual void NotifyInterfaceDown (uint32_t interface);
	  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
	  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
	  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
	  void DoInitialize();
	  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;

	  void DoDispose ();
	  int64_t AssignStreams (int64_t stream);
	  Ptr<Ipv4> m_ipv4;

};

}
}  // namespace ns3

#endif /* SALLY_AGENT_IMPL_H */
