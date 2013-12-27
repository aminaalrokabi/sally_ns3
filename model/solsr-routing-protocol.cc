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
#include "solsr-routing-protocol.h"

NS_LOG_COMPONENT_DEFINE ("SOlsrRouting");
#define OLSR_MAX_SEQ_NUM        65535

namespace ns3 {
namespace sally {

NS_OBJECT_ENSURE_REGISTERED (SOlsrRoutingProtocol);

TypeId SOlsrRoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::sally::SOlsrRoutingProtocol")
        .SetParent<ns3::olsr::RoutingProtocol> ()
    .AddConstructor<SOlsrRoutingProtocol> ()
  ;
  return tid;
}

void
SOlsrRoutingProtocol::SendTc()
{
}

void
SOlsrRoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);
  NS_LOG_DEBUG ("Created olsr::RoutingProtocol");
  m_helloTimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
  m_tcTimer.SetFunction (&RoutingProtocol::TcTimerExpire, this);
  m_midTimer.SetFunction (&RoutingProtocol::MidTimerExpire, this);
  m_hnaTimer.SetFunction (&RoutingProtocol::HnaTimerExpire, this);
  m_queuedMessagesTimer.SetFunction (&RoutingProtocol::SendQueuedMessages, this);

  m_packetSequenceNumber = OLSR_MAX_SEQ_NUM;
  m_messageSequenceNumber = OLSR_MAX_SEQ_NUM;
  m_ansn = OLSR_MAX_SEQ_NUM;

  m_linkTupleTimerFirstTime = true;

  m_ipv4 = ipv4;

  m_hnaRoutingTable->SetIpv4 (ipv4);
}
} // namespace olsr
} // namespace ns3

