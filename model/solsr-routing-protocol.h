#ifndef SALLY_AGENT_IMPL_H
#define SALLY_AGENT_IMPL_H

#include "ns3/olsr-routing-protocol.h"

namespace ns3 {
namespace sally {

///
/// \ingroup sally
///
/// \brief SALLY routing protocol for IPv4
///
class SOlsrRoutingProtocol: public ns3::olsr::RoutingProtocol
{
        public:
         static TypeId GetTypeId (void);
	     virtual void SetIpv4 (Ptr<Ipv4> ipv4);
         virtual void SendTc ();

};

}
} // namespace ns3

#endif /* SALLY_AGENT_IMPL_H */
