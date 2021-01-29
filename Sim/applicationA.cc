#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "applicationA.h"
#include "My-tag.h"

#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define END_CODE "\033[0m"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("CustomApplication");
NS_OBJECT_ENSURE_REGISTERED (CustomApplication);

TypeId
CustomApplication::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::CustomApplication")
          .SetParent<Application> ()
          .AddConstructor<CustomApplication> ()
          .AddAttribute ("Interval", "Broadcast Interval", TimeValue (MilliSeconds (100)),
                         MakeTimeAccessor (&CustomApplication::m_broadcast_time),
                         MakeTimeChecker ());
  return tid;
}

TypeId
CustomApplication::GetInstanceTypeId () const
{
  return CustomApplication::GetTypeId ();
}

CustomApplication::CustomApplication ()
{
  m_broadcast_time = MilliSeconds (100); //every 100ms
  m_packetSize = 1000; //1000 bytes
  m_Tiempo_de_reenvio = Seconds (3.0); //Tiempo para reenviar los paquetes
  m_time_limit = Seconds (5); //Tiempo limite para los nodos vecinos
  m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
  m_semilla = 0; // controla los numeros de secuencia
  
}
CustomApplication::~CustomApplication ()
{
}
void
CustomApplication::StartApplication ()
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
          dev->SetReceiveCallback (MakeCallback (&CustomApplication::ReceivePacket, this));

          /*
            If you want promiscous receive callback, connect to this trace. 
            For every packet received, both functions ReceivePacket & PromiscRx will be called. with PromicRx being called first!
            */
          break;
        }
    }
  if (m_wifiDevice)
    {

      //Let's create a bit of randomness with the first broadcast packet time to avoid collision
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      Time random_offset = MicroSeconds (rand->GetValue (50, 200));
      Simulator::Schedule (m_broadcast_time + random_offset,
                           &CustomApplication::BroadcastInformation, this);
    }
  else
    {
      NS_FATAL_ERROR ("There's no WaveNetDevice in your node");
    }
  //We will periodically (every 1 second) check the list of neighbors, and remove old ones (older than 5 seconds)
  //Simulator::Schedule (Seconds (1), &CustomApplication::RemoveOldNeighbors, this);
}
void
CustomApplication::SetBroadcastInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_broadcast_time = interval;
}

void
CustomApplication::SetWifiMode (WifiMode mode)
{
  m_mode = mode;
}

void
CustomApplication::BroadcastInformation ()
{
  NS_LOG_FUNCTION (this);
  
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
      Time Ultimo_Envio = Now () - it->Tiempo_ultimo_envio;

      if (!it->Estado)
        {
          
          Ptr<Packet> packet = Create<Packet> (m_packetSize);
          CustomDataTag tag;
          // El timestamp se configrua dentro del constructor del tag
          tag.SetNodeId (GetNode ()->GetId ());
          tag.CopySEQNumber (it->numeroSEQ);

          if (it->NumeroDeEnvios == 0)
            {

              tag.SetTypeOfpacket (0);
            }
          else if (Ultimo_Envio >= m_Tiempo_de_reenvio)
            {
              tag.SetTypeOfpacket (1);
            }
          it->NumeroDeEnvios += 1;
          packet->AddPacketTag (tag);
          m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
          break;
        }
      else if (m_Paquetes_A_Reenviar.size () != 0)
        {
          CustomDataTag tag;
          std::list<ST_Reenvios>::iterator it = GetReenvio ();
          tag.SetNodeId (it->ID_Creador);
          tag.CopySEQNumber (it->numeroSEQ);
          tag.SetTimestamp (it->Tiempo_ultimo_envio);
          tag.SetTypeOfpacket (it->tipo_de_paquete);
          Ptr<Packet> packet = Create<Packet> (it->Tam_Paquete);
          packet->AddPacketTag (tag);
          m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
        }
    }

  //Broadcast the packet as WSMP (0x88dc)
  //Schedule next broadcast
  Simulator::Schedule (m_broadcast_time, &CustomApplication::BroadcastInformation, this);
}

bool
CustomApplication::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,
                                  uint16_t protocol, const Address &sender)
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
  
  if (!BuscaSEQEnTabla (tag.GetSEQNumber ()))
    { // Si el numero de secuencia no esta ne la tabla lo guar para reenviar
      Guarda_Paquete_reenvio (tag.GetSEQNumber (), tag.GetNodeId (), packet->GetSize (),
                              tag.GetTimestamp (), tag.GetTypeOfPacket ());
    }
  else if (tag.GetTypeOfPacket () == 2)
    {
      ConfirmaEntrega (tag.GetSEQNumber ());
    }
  return true;
}

void
CustomApplication::UpdateNeighbor (Mac48Address addr)
{
  bool found = false;
  //Go over all neighbors, find one matching the address, and updates its 'last_beacon' time.
  for (std::vector<NeighborInformation>::iterator it = m_neighbors.begin ();
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
      NeighborInformation new_n;
      new_n.neighbor_mac = addr;
      new_n.last_beacon = Now ();
      m_neighbors.push_back (new_n);
    }
}
u_long
CustomApplication::CalculaSeqNumber (u_long *sem)
{
  u_long m = 2147483648;
  u_long a = 314159269;
  u_long c = 453806247;
  *sem = (*sem * a + c) % m;
  return *sem;
}
void
CustomApplication::IniciaTabla (uint32_t PQts_A_enviar, uint32_t ID)
{

  for (uint32_t i = 0; i < PQts_A_enviar; i++)
    {
      ST_Paquete_A_Enviar Paquete;
      
      Paquete.ID_Creador = ID;
      
      Paquete.numeroSEQ = CalculaSeqNumber (&m_semilla);
      Paquete.Tam_Paquete = m_packetSize;
      Paquete.Tiempo_ultimo_envio = Now ();
      Paquete.NumeroDeEnvios = 0;
      Paquete.Estado = false;
      m_Tabla_paquetes_A_enviar.push_back (Paquete);
    }
  // std::cout << "Inicia tabla "<<m_Tabla_paquetes_A_enviar.size()<<std::endl;
}
void
CustomApplication::Guarda_Paquete_reenvio (u_long SEQ, uint32_t ID_Creador,
                                           uint32_t tam_del_paquete, Time timeStamp, int32_t type)
{
  ST_Reenvios reenvio;
  reenvio.ID_Creador = ID_Creador;
  reenvio.numeroSEQ = SEQ;
  reenvio.Tam_Paquete = tam_del_paquete;
  reenvio.Tiempo_ultimo_envio = timeStamp;
  reenvio.tipo_de_paquete = type;
  m_Paquetes_A_Reenviar.push_back (reenvio);
}
void
CustomApplication::ConfirmaEntrega (u_long SEQ)
{
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
      if (it->numeroSEQ == SEQ)
        {
          it->Estado = true;
          break;
        }
    }
}
std::list<ST_Reenvios>::iterator
CustomApplication::GetReenvio ()
{
  std::list<ST_Reenvios>::iterator it = m_Paquetes_A_Reenviar.begin ();
  m_Paquetes_A_Reenviar.erase (it);
  return it;
}
bool
CustomApplication::BuscaSEQEnTabla (u_long SEQ)
{
  bool find = false;
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
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
CustomApplication::ImprimeTabla ()
{
  std::cout << "\t ********Paquetes generados en el Nodo*********" << GetNode ()->GetId ()
            << std::endl;
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
      std::cout << "\t SEQnumber: " << it->numeroSEQ << "\t PacketSize: " << it->Tam_Paquete
                << "\t N. Veces enviado: " << it->NumeroDeEnvios << "\t Estado: " << it->Estado
                << std::endl;
    }
}
void
CustomApplication::PrintNeighbors ()
{
  std::cout << "Neighbor Info for Node: " << GetNode ()->GetId () << std::endl;
  for (std::vector<NeighborInformation>::iterator it = m_neighbors.begin ();
       it != m_neighbors.end (); it++)
    {
      std::cout << "\tMAC: " << it->neighbor_mac << "\tLast Contact: " << it->last_beacon
                << std::endl;
    }
}

void
CustomApplication::RemoveOldNeighbors ()
{
  //Go over the list of neighbors
  for (std::vector<NeighborInformation>::iterator it = m_neighbors.begin ();
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
  Simulator::Schedule (Seconds (1), &CustomApplication::RemoveOldNeighbors, this);
}

} // namespace ns3
