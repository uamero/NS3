#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "applicationPrimarios.h"
#include "TagPrimarios.h"

#include <bitset>
#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define END_CODE "\033[0m"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("CustomApplicationPnodes");
NS_OBJECT_ENSURE_REGISTERED (CustomApplicationPnodes);

TypeId
CustomApplicationPnodes::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::CustomApplicationPnodes")
          .SetParent<Application> ()
          .AddConstructor<CustomApplicationPnodes> ()
          .AddAttribute ("Interval", "Broadcast Interval", TimeValue (MilliSeconds (100)),
                         MakeTimeAccessor (&CustomApplicationPnodes::m_broadcast_time),
                         MakeTimeChecker ());
  return tid;
}

TypeId
CustomApplicationPnodes::GetInstanceTypeId () const
{
  return CustomApplicationPnodes::GetTypeId ();
}

CustomApplicationPnodes::CustomApplicationPnodes ()
{
  m_broadcast_time = Seconds (10); //every 101ms
  m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
  m_n_channels = 8;
  m_porcentaje_Ch_disp = 10; //numero de 0% a 100%
}
CustomApplicationPnodes::~CustomApplicationPnodes ()
{
}
void
CustomApplicationPnodes::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  //Set A Receive callback
  Ptr<Node> n = GetNode ();
  //Ptr<NetDevice> dev = n->GetDevice (m_n_channels +1);
  //m_wifiDevice = DynamicCast<WifiNetDevice> (dev);
  for (uint32_t i = 0; i < n->GetNDevices (); i++)
    {
      Ptr<NetDevice> dev = n->GetDevice (i);
      //std::cout<<"config: "<<i<<std::endl;
      if (dev->GetInstanceTypeId () == WifiNetDevice::GetTypeId ())
        {
          m_wifiDevice = DynamicCast<WifiNetDevice> (dev);
          //ReceivePacket will be called when a packet is received
          // dev->SetReceiveCallback (MakeCallback (&CustomApplicationPnodes::ReceivePacket, this));
          /*
            If you want promiscous receive callback, connect to this trace. 
            For every packet received, both functions ReceivePacket & PromiscRx will be called. with PromicRx being called first!
           */
          //break;
        }
    }
  if (m_wifiDevice)
    {

      //Let's create a bit of randomness with the first broadcast packet time to avoid collision
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      Time random_offset = MicroSeconds (rand->GetValue (50, 200));
      Simulator::Schedule (random_offset, &CustomApplicationPnodes::BroadcastInformation, this);
    }
  else
    {
      NS_FATAL_ERROR ("There's no WifiNetDevice in your node");
    }
  //We will periodically (every 1 second) check the list of neighbors, and remove old ones (older than 5 seconds)
  //Simulator::Schedule (Seconds (1), &CustomApplication::RemoveOldNeighbors, this);
}
void
CustomApplicationPnodes::SetBroadcastInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_broadcast_time = interval;
}

void
CustomApplicationPnodes::SetWifiMode (WifiMode mode)
{
  m_mode = mode;
}

void
CustomApplicationPnodes::BroadcastInformation ()
{
  NS_LOG_FUNCTION (this);
  GetCanales ();
  //std::cout<<"Primarios: "<<GetNode()->GetNDevices() << " n_chanels: "<<m_n_channels<<std::endl;
  m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (m_n_channels));
  //uint8_t *buffer = new uint8_t[std::strlen(m_chs.c_str ())];

  //packet->CopyData (buffer, packet->GetSize ());
  //std::string ruta = std::string (buffer, buffer + packet->GetSize ());
  //Ptr<Packet> PacketToReSend = Create<Packet> ((uint8_t *) m_chs.c_str (), m_chs.length ());

  Ptr<Packet> packet = Create<Packet> ((uint8_t *) m_chs.c_str (), m_chs.length ());
  // Ptr<Packet> packet = Create<Packet> ();
  PrimariosDataTag tag;
  // El timestamp se configrua dentro del constructor del tag
  tag.SetNodeId (GetNode ()->GetId ());
  //tag.SetChanels(0);
  packet->AddPacketTag (tag);
  //std::bitset<8> ocu(tag.GetChanels());
  //std::cout << "La ocupación enviada por el primario ID: "<<GetNode()->GetId()<<" es: "<<ocu<<std::endl;
  m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
  //Broadcast the packet as WSMP (0x88dc)
  //Schedule next broadcast

  Simulator::Schedule (m_broadcast_time, &CustomApplicationPnodes::BroadcastInformation, this);
}

/*bool
CustomApplicationPnodes::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,
                                        uint16_t protocol, const Address &sender)
{
  Hay dos tipos de paquetes que se pueden recibir
   * 1.- Un paquete que fue enviado por este nodo 
   * 2.- Un paquete que fue enviado por el Sink*
  NS_LOG_FUNCTION (device << packet << protocol << sender);
  
       Packets received here only have Application data, no WifiMacHeader. 
        We created packets with 1000 bytes payload, so we'll get 1000 bytes of payload.
    *

  NS_LOG_INFO ("ReceivePacket() : Node " << GetNode ()->GetId () << " : Received a packet from "
                                         << sender << " Size:" << packet->GetSize ());

  return true;
}
*/

void
CustomApplicationPnodes::GetCanales ()
{
  //se calcula el numero de canales que seran ocupados
  uint32_t N_chs = (m_porcentaje_Ch_disp / 100.0) * m_n_channels;
  if (N_chs == 0)
    {
      N_chs = 1;
    } //como minimo un canal se ocupa

  uint32_t CD[m_n_channels];
  m_chs = "";
  for (uint32_t i = 0; i < m_n_channels; i++)
    {
      CD[i] = 0;
    }

  //srand (time (NULL));
  uint32_t i = 0;
  while (i != N_chs)
    {
      uint32_t disp = rand () % (m_n_channels -
                                 1); //se genera un numero aleatorio entre 0 y numero de canales -1
      if (CD[disp] == 0)
        {
          CD[disp] = 1;
          i++;
        }
    }

  for (uint32_t i = 0; i < m_n_channels; i++)
    {
      m_chs = m_chs + std::to_string (CD[i]);
    }
}
void
CustomApplicationPnodes::SetChannels (uint32_t channels)
{
  m_n_channels = channels;
}
} // namespace ns3
