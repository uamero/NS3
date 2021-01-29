#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "applicationB.h"
#include "My-tag.h"

#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define END_CODE "\033[0m"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("CustomApplicationBnodes");
NS_OBJECT_ENSURE_REGISTERED (CustomApplicationBnodes);

TypeId
CustomApplicationBnodes::GetTypeId ()
{
  static TypeId tid =
      TypeId ("ns3::CustomApplicationBnodes")
          .SetParent<Application> ()
          .AddConstructor<CustomApplicationBnodes> ()
          .AddAttribute ("Interval", "Broadcast Interval", TimeValue (MilliSeconds (100)),
                         MakeTimeAccessor (&CustomApplicationBnodes::m_broadcast_time),
                         MakeTimeChecker ());
  return tid;
}

TypeId
CustomApplicationBnodes::GetInstanceTypeId () const
{
  return CustomApplicationBnodes::GetTypeId ();
}

CustomApplicationBnodes::CustomApplicationBnodes ()
{
  m_broadcast_time = MilliSeconds (100); //every 100ms
  m_time_limit = Seconds (5); //Tiempo limite para los nodos vecinos
  m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
}
CustomApplicationBnodes::~CustomApplicationBnodes ()
{
}
void
CustomApplicationBnodes::StartApplication ()
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
          dev->SetReceiveCallback (MakeCallback (&CustomApplicationBnodes::ReceivePacket, this));

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
                           &CustomApplicationBnodes::BroadcastInformation, this);
    }
  else
    {
      NS_FATAL_ERROR ("There's no WaveNetDevice in your node");
    }
  //We will periodically (every 1 second) check the list of neighbors, and remove old ones (older than 5 seconds)
  //Simulator::Schedule (Seconds (1), &CustomApplication::RemoveOldNeighbors, this);
}
void
CustomApplicationBnodes::SetBroadcastInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_broadcast_time = interval;
}

void
CustomApplicationBnodes::SetWifiMode (WifiMode mode)
{
  m_mode = mode;
}

void
CustomApplicationBnodes::BroadcastInformation ()
{
  NS_LOG_FUNCTION (this);
  if (m_Paquetes_A_Reenviar.size () != 0)
    {
      for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_A_Reenviar.begin ();
           it != m_Paquetes_A_Reenviar.end (); it++)
        {
          if (!it->Estado)
            {

              Ptr<Packet> packet = Create<Packet> (it->Tam_Paquete);
              CustomDataTag tag;
              // El timestamp se configrua dentro del constructor del tag
              tag.SetNodeId (GetNode ()->GetId ());
              tag.CopySEQNumber (it->numeroSEQ);
              tag.SetTimestamp(it->Tiempo_ultimo_envio);
              tag.SetTypeOfpacket(it->tipo_de_paquete);
              packet->AddPacketTag (tag);
              m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
              m_Paquetes_A_Reenviar.erase(it);
              break;
            }
        }
    }

  //Broadcast the packet as WSMP (0x88dc)
  //Schedule next broadcast
  Simulator::Schedule (m_broadcast_time, &CustomApplicationBnodes::BroadcastInformation, this);
}

bool
CustomApplicationBnodes::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,
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

  if (!BuscaSEQEnTabla (tag.GetSEQNumber ()) && tag.GetTypeOfPacket () != 2)
    { // Si el numero de secuencia no esta en la tabla lo guarda para reenviar
      Guarda_Paquete_reenvio (tag.GetSEQNumber (), tag.GetNodeId (), packet->GetSize (),
                              tag.GetTimestamp (), tag.GetTypeOfPacket ());
    }
  else if (BuscaSEQEnTabla (tag.GetSEQNumber ()) && tag.GetTypeOfPacket () != 2)
    { //El SEQ number ya se ha recibido previamente, hay que verificar
      //que no haya acuse de recibo por parte del sink
      if (!VerificaSEQRecibido (tag.GetSEQNumber ()))
        { //Se verifica si el estado del paquete no es entregado
          ST_ReenviosB NewP;
          NewP.ID_Creador = tag.GetNodeId ();
          NewP.numeroSEQ = tag.GetSEQNumber ();
          NewP.Tam_Paquete = packet->GetSize ();
          NewP.Tiempo_ultimo_envio = tag.GetTimestamp ();
          NewP.tipo_de_paquete = tag.GetTypeOfPacket ();
          NewP.Estado = false;
          m_Paquetes_A_Reenviar.push_back (NewP);
        }
    }
  else if (tag.GetTypeOfPacket () == 2)
    {
      ConfirmaEntrega (tag.GetSEQNumber ());
    }
  return true;
}

void
CustomApplicationBnodes::UpdateNeighbor (Mac48Address addr)
{
  bool found = false;
  //Go over all neighbors, find one matching the address, and updates its 'last_beacon' time.
  for (std::vector<NeighborInformationB>::iterator it = m_neighbors.begin ();
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
      NeighborInformationB new_n;
      new_n.neighbor_mac = addr;
      new_n.last_beacon = Now ();
      m_neighbors.push_back (new_n);
    }
}

void
CustomApplicationBnodes::Guarda_Paquete_reenvio (u_long SEQ, uint32_t ID_Creador,
                                                 uint32_t tam_del_paquete, Time timeStamp,
                                                 int32_t type)
{
  ST_ReenviosB reenvio;
  reenvio.ID_Creador = ID_Creador;
  reenvio.numeroSEQ = SEQ;
  reenvio.Tam_Paquete = tam_del_paquete;
  reenvio.Tiempo_ultimo_envio = timeStamp;
  reenvio.tipo_de_paquete = type;
  reenvio.Estado = false;
  m_Paquetes_Recibidos.push_back (reenvio);
  m_Paquetes_A_Reenviar.push_back (reenvio);
}
bool
CustomApplicationBnodes::VerificaSEQRecibido (u_long SEQ)
{
  bool find = false;
  for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_Recibidos.begin ();
       it != m_Paquetes_Recibidos.end (); it++)
    {
      if (it->numeroSEQ == SEQ)
        {
          find = it->Estado;
          break;
        }
    }
  return find;
}
void
CustomApplicationBnodes::ConfirmaEntrega (u_long SEQ)
{
  for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_Recibidos.begin ();
       it != m_Paquetes_Recibidos.end (); it++)
    {
      if (it->numeroSEQ == SEQ)
        {
          it->Estado = true;
          break;
        }
    }
}
std::list<ST_ReenviosB>::iterator
CustomApplicationBnodes::GetReenvio ()
{
  std::list<ST_ReenviosB>::iterator it = m_Paquetes_A_Reenviar.begin ();
  m_Paquetes_A_Reenviar.erase (it);
  return it;
}
bool
CustomApplicationBnodes::BuscaSEQEnTabla (u_long SEQ)
{
  bool find = false;
  for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_Recibidos.begin ();
       it != m_Paquetes_Recibidos.end (); it++)
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
CustomApplicationBnodes::ImprimeTabla ()
{
  std::cout << "\t ********Paquetes generados en el Nodo*********" << GetNode ()->GetId ()
            << std::endl;
  for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_Recibidos.begin ();
       it != m_Paquetes_Recibidos.end (); it++)
    {
      std::cout << "\t SEQnumber: " << it->numeroSEQ << "\t PacketSize: " << it->Tam_Paquete
                << "\t Estado: " << it->Estado << std::endl;
    }
}
void
CustomApplicationBnodes::PrintNeighbors ()
{
  std::cout << "Neighbor Info for Node: " << GetNode ()->GetId () << std::endl;
  for (std::vector<NeighborInformationB>::iterator it = m_neighbors.begin ();
       it != m_neighbors.end (); it++)
    {
      std::cout << "\tMAC: " << it->neighbor_mac << "\tLast Contact: " << it->last_beacon
                << std::endl;
    }
}

void
CustomApplicationBnodes::RemoveOldNeighbors ()
{
  //Go over the list of neighbors
  for (std::vector<NeighborInformationB>::iterator it = m_neighbors.begin ();
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
  Simulator::Schedule (Seconds (1), &CustomApplicationBnodes::RemoveOldNeighbors, this);
}

} // namespace ns3
