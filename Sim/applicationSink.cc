#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "applicationSink.h"
#include "My-tag.h"

#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define END_CODE "\033[0m"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("ApplicationSink");
NS_OBJECT_ENSURE_REGISTERED (ApplicationSink);

TypeId
ApplicationSink::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::ApplicationSink")
          .SetParent<Application> ()
          .AddConstructor<ApplicationSink> ()
          .AddAttribute ("Interval", "Broadcast Interval", TimeValue (MilliSeconds (100)),
                         MakeTimeAccessor (&ApplicationSink::m_broadcast_time), MakeTimeChecker ());
  return tid;
}

TypeId
ApplicationSink::GetInstanceTypeId () const
{
  return ApplicationSink::GetTypeId ();
}

ApplicationSink::ApplicationSink ()
{
  m_broadcast_time = MilliSeconds (100); //every 100ms
  m_packetSize = 10; //10 bytes
  m_Tiempo_de_reenvio = Seconds (1.0); //Tiempo para reenviar los paquetes ACK
  m_time_limit = Seconds (5); //Tiempo limite para los nodos vecinos
  m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
  m_semilla = 0; // controla los numeros de secuencia
  m_n_channels=8;
}
ApplicationSink::~ApplicationSink ()
{
}
void
ApplicationSink::StartApplication ()
{
  NS_LOG_FUNCTION (this);
  //Set A Receive callback
  Ptr<Node> n = GetNode ();
  for (uint32_t i = 0; i < n->GetNDevices (); i++)
    {
      Ptr<NetDevice> dev = n->GetDevice (i);
      if (dev->GetInstanceTypeId () == WifiNetDevice::GetTypeId ())
        {
          m_wifiDevice = DynamicCast<WifiNetDevice> (dev);
          //ReceivePacket will be called when a packet is received
          dev->SetReceiveCallback (MakeCallback (&ApplicationSink::ReceivePacket, this));

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
      Simulator::Schedule (m_broadcast_time + random_offset, &ApplicationSink::BroadcastInformation,
                           this);
    }
  else
    {
      NS_FATAL_ERROR ("There's no WaveNetDevice in your node");
    }
  //We will periodically (every 1 second) check the list of neighbors, and remove old ones (older than 5 seconds)
  //Simulator::Schedule (Seconds (1), &ApplicationSink::RemoveOldNeighbors, this);
}
void
ApplicationSink::SetBroadcastInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_broadcast_time = interval;
}

void
ApplicationSink::SetWifiMode (WifiMode mode)
{
  m_mode = mode;
}

void
ApplicationSink::BroadcastInformation ()
{
  NS_LOG_FUNCTION (this);
  
 //std::cout<<"Sink: "<<GetNode()->GetNDevices() << " n_chanels: "<<m_n_channels<<std::endl;
  if (m_Tabla_paquetes_ACK.size () != 0) //hay ACK pendientes
    {
      
      for (std::list<ST_Paquete_A_Enviar_sink>::iterator it = m_Tabla_paquetes_ACK.begin ();
          it != m_Tabla_paquetes_ACK.end (); it++)
        {
          Time Ultimo_Envio = Now () - it->Tiempo_ultimo_envio;
          
          if (it->NumeroDeEnvios < 3 && (Ultimo_Envio >= m_Tiempo_de_reenvio))
            {
              
              m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode()->GetDevice(m_n_channels));
              Ptr<Packet> packet = Create<Packet> (m_packetSize);
              CustomDataTag tag;
              // El timestamp se configura dentro del constructor del tag
              tag.SetNodeId (GetNode ()->GetId ());
              tag.CopySEQNumber (it->numeroSEQ);
              tag.SetTypeOfpacket (2);
              it->NumeroDeEnvios += 1;
              packet->AddPacketTag (tag);
              m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
              //Simulator::Stop();
              break;
            }
        }
    }

  //Broadcast the packet as WSMP (0x88dc)
  //Schedule next broadcast
  Simulator::Schedule (m_broadcast_time, &ApplicationSink::BroadcastInformation, this);
}

bool
ApplicationSink::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                const Address &sender)
{
  /**Hay dos tipos de paquetes que se pueden recibir
   * 1.- Un paquete que fue enviado por este nodo 
   * 2.- Un paquete que fue enviado por el Sink*/
  CustomDataTag tag;
  NS_LOG_FUNCTION (device << packet << protocol << sender);
  /*
        Packets received here only have Application data, no WifiMacHeader. 
        We created packets with 1000 bytes payload, so we'll get 1000 bytes of payload.
    */

  NS_LOG_INFO ("ReceivePacket() : Node " << GetNode ()->GetId () << " : Received a packet from "
                                         << sender << " Size:" << packet->GetSize ());
  packet->PeekPacketTag (tag);

  if (!BuscaSEQEnTabla (tag.GetSEQNumber ()) && tag.GetTypeOfPacket () != 2)
    { // Si el numero de secuencia no esta ne la tabla lo guarda para enviar su ACK
   std::cout<<"Recibi paquete " <<std::endl;
      Guarda_Paquete_para_ACK (tag.GetSEQNumber (), tag.GetNodeId (), packet->GetSize (),
                               tag.GetTimestamp (), tag.GetTypeOfPacket ());

      //ImprimeTabla ();
    }
  return true;
}

void
ApplicationSink::UpdateNeighbor (Mac48Address addr)
{
  bool found = false;
  //Go over all neighbors, find one matching the address, and updates its 'last_beacon' time.
  for (std::vector<NeighborInformation_sink>::iterator it = m_neighbors.begin ();
       it != m_neighbors.end (); it++)
    {
      if (it->neighbor_mac == addr)
        {
          it->last_beacon = Now ();
          found = true;
          break;
        }
    }
  if (!found) //If no node with this address exist, add a new table entry
    {
      NS_LOG_INFO (GREEN_CODE << Now () << " : Node " << GetNode ()->GetId ()
                              << " is adding a neighbor with MAC=" << addr << END_CODE);
      NeighborInformation_sink new_n;
      new_n.neighbor_mac = addr;
      new_n.last_beacon = Now ();
      m_neighbors.push_back (new_n);
    }
}
u_long
ApplicationSink::CalculaSeqNumber (u_long *sem)
{
  u_long m = 2147483648;
  u_long a = 314159269;
  u_long c = 453806247;
  *sem = (*sem * a + c) % m;
  return *sem;
}

void
ApplicationSink::Guarda_Paquete_para_ACK (u_long SEQ, uint32_t ID_Creador, uint32_t tam_del_paquete,
                                          Time timeStamp, int32_t type)
{
  ST_Paquete_A_Enviar_sink ACK;

  ACK.ID_Creador = ID_Creador;
  ACK.numeroSEQ = SEQ;
  ACK.Tam_Paquete = tam_del_paquete;
  ACK.Tiempo_ultimo_envio = timeStamp;
  ACK.tipo_de_paquete = type;
  ACK.NumeroDeEnvios = 0;
  m_Tabla_paquetes_ACK.push_back (ACK);
}

bool
ApplicationSink::BuscaSEQEnTabla (u_long SEQ)
{
  bool find = false;
  for (std::list<ST_Paquete_A_Enviar_sink>::iterator it = m_Tabla_paquetes_ACK.begin ();
       it != m_Tabla_paquetes_ACK.end (); it++)
    {
      if (it->numeroSEQ == SEQ)
        {
          find = true;
          break;
        }
    }
  return find;
}
void
ApplicationSink::ImprimeTabla ()
{
  std::cout << "\t ********Paquetes recibidos en el Nodo " << GetNode ()->GetId () << " *********"
            << std::endl;
  for (std::list<ST_Paquete_A_Enviar_sink>::iterator it = m_Tabla_paquetes_ACK.begin ();
       it != m_Tabla_paquetes_ACK.end (); it++)
    {
      std::cout << " SEQnumber: " << it->numeroSEQ << "\t PacketSize: " << it->Tam_Paquete
                << "\t Nodo que envia: " << it->ID_Creador << std::endl;
    }
}
void
ApplicationSink::PrintNeighbors ()
{
  std::cout << "Neighbor Info for Node: " << GetNode ()->GetId () << std::endl;
  for (std::vector<NeighborInformation_sink>::iterator it = m_neighbors.begin ();
       it != m_neighbors.end (); it++)
    {
      std::cout << "\tMAC: " << it->neighbor_mac << "\tLast Contact: " << it->last_beacon
                << std::endl;
    }
}

void
ApplicationSink::RemoveOldNeighbors ()
{
  //Go over the list of neighbors
  for (std::vector<NeighborInformation_sink>::iterator it = m_neighbors.begin ();
       it != m_neighbors.end (); it++)
    {
      //Get the time passed since the last time we heard from a node
      Time last_contact = Now () - it->last_beacon;

      if (last_contact >=
          Seconds (
              5)) //if it has been more than 5 seconds, we will remove it. You can change this to whatever value you want.
        {
          NS_LOG_INFO (RED_CODE << Now () << " Node " << GetNode ()->GetId ()
                                << " is removing old neighbor " << it->neighbor_mac << END_CODE);
          //Remove an old entry from the table
          m_neighbors.erase (it);
          break;
        }
    }
  //Check the list again after 1 second.
  Simulator::Schedule (Seconds (1), &ApplicationSink::RemoveOldNeighbors, this);
}

} // namespace ns3
