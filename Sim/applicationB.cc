#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "applicationB.h"
#include "My-tag.h"

#include <bitset>
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
  m_broadcast_time = Seconds (1); //every 100ms
  m_time_limit = Seconds (5); //Tiempo limite para los nodos vecinos
  m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
  m_n_channels = 8;
  m_Batery=100.0;
  iniciaCanales();
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
      //std::cout<<"config: "<<i<<std::endl;
      if (dev->GetInstanceTypeId () == WifiNetDevice::GetTypeId ())
        {
          m_wifiDevice = DynamicCast<WifiNetDevice> (dev);
          //ReceivePacket will be called when a packet is received
          dev->SetReceiveCallback (MakeCallback (&CustomApplicationBnodes::ReceivePacket, this));

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
      Simulator::Schedule (m_broadcast_time + random_offset,
                           &CustomApplicationBnodes::BroadcastInformation, this);
      
      //Simulator::Schedule (MilliSeconds (200), &CustomApplicationBnodes::CanalesDisponibles, this);
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
  //std::cout<< "El nodo Tiene un intervalo de broadcast de: "<< interval.GetSeconds()<<std::endl;
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
  std::cout<<"APPB en funcionamiento time: "<<m_Canales_Para_Utilizar.size()<<" | Batery: "<<m_Batery<<"% "<<" ID:"<< GetNode()->GetId()<<std::endl;
  /*if(Now().GetSeconds()>=21)
    this->SetStopTime(Seconds(.2));*/
  NS_LOG_FUNCTION (this);
  //std::cout<<"Nodos B: "<<GetNode()->GetNDevices() << " n_chanels: "<<m_n_channels<<std::endl;
  
  if (m_Paquetes_A_Reenviar.size () != 0)
    {
      for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_A_Reenviar.begin ();
           it != m_Paquetes_A_Reenviar.end (); it++)
        {
          if (!it->Estado && m_Canales_Para_Utilizar.size () != 0)
            {
              for (std::list<uint32_t>::iterator _it = m_Canales_Para_Utilizar.begin ();
                   _it != m_Canales_Para_Utilizar.end (); _it++)
                {
                  Ptr<Packet> packet = Create<Packet> (it->Tam_Paquete);
                  CustomDataTag tag;
                  // El timestamp se configrua dentro del constructor del tag
                  tag.SetNodeId (it->ID_Creador);
                  tag.CopySEQNumber (it->numeroSEQ);
                  //std::cout << "SEQQQ en BBBBB"<<it->numeroSEQ <<std::endl;
                  tag.SetTimestamp (it->Tiempo_ultimo_envio);
                  tag.SetTypeOfpacket (it->tipo_de_paquete);
                  tag.SetChanels (*_it);
                  packet->AddPacketTag (tag);
                  //std::cout<<"channel ->>>>>>>>>>>>>> "<<*_it <<std::endl;
                  m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (*_it));
                  m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
                }

              //std::cout << "El canal en B es : " << std::to_string (ch) << std::endl;
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
  uint64_t ch = tag.GetChanels ();
 // std::cout << "SEQ en B" <<tag.GetSEQNumber() << std::endl; 
 //std::cout << "pAQUETE RECIBIDO en BBBBB   SEQ: "  << tag.GetSEQNumber()<<" | "<<device->GetIfIndex () <<std::endl;
  

  if (!BuscaSEQEnTabla (tag.GetSEQNumber ()) &&
      (tag.GetTypeOfPacket () == 0 || tag.GetTypeOfPacket () == 1) &&
      device->GetIfIndex () != m_n_channels && VerificaCanal (ch) &&
      device->GetIfIndex () != m_n_channels + 1)
    { // Si el numero de secuencia no esta en la tabla lo guarda para reenviar
      
      Guarda_Paquete_reenvio (tag.GetSEQNumber (), tag.GetNodeId (), packet->GetSize (),
                              tag.GetTimestamp (), tag.GetTypeOfPacket ());
    }
  else if (BuscaSEQEnTabla (tag.GetSEQNumber ()) && tag.GetTypeOfPacket () != 2 &&
           device->GetIfIndex () != m_n_channels && device->GetIfIndex () != m_n_channels + 1 &&
           VerificaCanal (ch) && tag.GetTypeOfPacket () != 3)
    { //El SEQ number ya se ha recibido previamente, hay que verificar
      //que no haya acuse de recibo por parte del sink
      //std::cout <<"Aquiweeeeeee"<<std::endl;
      if (!VerificaSEQRecibido (tag.GetSEQNumber ()))
        { //Se verifica si el estado del paquete no es entregado
          //std::cout << "B El canal por donde recibo es es ch: " << std::to_string(ch) << std::endl;
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
  else if (tag.GetTypeOfPacket () == 2 && device->GetIfIndex () == m_n_channels)
    { // paquete proveniente del sink para confirmar entrega de un paquete
      ConfirmaEntrega (tag.GetSEQNumber ());
    }
  else if (tag.GetTypeOfPacket () == 3 && device->GetIfIndex () == m_n_channels + 1)
    { //Es un paquete que proviene de los primarios
      //std::bitset<8> ocu (ch);
      // std::cout << "B -> La ocupación recibida en "<<GetNode()->GetId()<<" es: "<<ocu<<std::endl;
      BuscaCanalesID (tag.GetChanels (), tag.GetNodeId (), Now ());
      CanalesDisponibles ();
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

  if (m_Paquetes_A_Reenviar.size () != 0)
    {
      for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_A_Reenviar.begin ();
           it != m_Paquetes_A_Reenviar.end (); it++)
        {
          if (it->numeroSEQ == SEQ)
            {
              m_Paquetes_A_Reenviar.erase (it);
              break;
            }
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
  std::cout << "\t ********Paquetes recibidos en el Nodo*********" << GetNode ()->GetId ()
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
Time
CustomApplicationBnodes::GetBroadcastInterval ()
{
  return m_broadcast_time;
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
void
CustomApplicationBnodes::iniciaCanales(){
  for(u_int32_t i = 0; i < m_n_channels; i++)
  {
    m_Canales_Para_Utilizar.push_back(i);
  }
  
}
void
CustomApplicationBnodes::CanalesDisponibles ()
{
  uint64_t canales = 0;
  m_Canales_Para_Utilizar.clear();
  for (std::list<ST_CanalesB>::iterator it = m_Canales_disponibles.begin ();
       it != m_Canales_disponibles.end (); it++)
    {
      canales = it->m_chanels | canales;
    }
  std::bitset<64> x (canales);
  std::string cadena = x.to_string ();
  std::string disp = std::string (cadena.rbegin (), cadena.rend ());

  for (uint8_t i = 0; i < m_n_channels; i++)
    {
      if (disp[i] == '0')
        {
          m_Canales_Para_Utilizar.push_back (i);
        }
    }
  //Simulator::Schedule (MilliSeconds (200), &CustomApplicationBnodes::CanalesDisponibles, this);
}

bool
CustomApplicationBnodes::VerificaCanal (uint8_t ch)
{
  uint64_t canales = 0;
  bool find = false;
  if (m_Canales_disponibles.size () == 0)
    {
      canales = 0;
    }
  else
    {
      for (std::list<ST_CanalesB>::iterator it = m_Canales_disponibles.begin ();
           it != m_Canales_disponibles.end (); it++)
        {
          canales = it->m_chanels | canales;
        }
    }

  std::bitset<64> x (canales);
  std::string cadena = x.to_string ();
  std::string disp = std::string (cadena.rbegin (), cadena.rend ());

  if (disp[ch] == '0')
    {
      find = true;
    }
  return find;
}

bool
CustomApplicationBnodes::BuscaCanalesID (uint8_t ch, uint32_t ID, Time tim)
{
  bool find = false;
  for (std::list<ST_CanalesB>::iterator it = m_Canales_disponibles.begin ();
       it != m_Canales_disponibles.end (); it++)
    {
      if (it->ID_Persive == ID)
        {
          it->m_chanels = ch;
          it->Tiempo_ultima_actualizacion = Now ();
          find = true;
          break;
        }
    }
  if (!find)
    {
      ST_CanalesB Nch;
      Nch.ID_Persive = ID;
      Nch.m_chanels = ch;
      Nch.Tiempo_ultima_actualizacion = Now ();
    }
  return find;
}
} // namespace ns3
