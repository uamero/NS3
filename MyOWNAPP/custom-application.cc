#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "custom-application.h"
#include "custom-data-tag.h"
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
  m_time_limit = Seconds (5);
  m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
  m_NextData = "";
  m_SenderOrSink = 0;
  m_NumberOfPacketsOK = 0;
  m_sms1 = "Ayuda! edificio caido mensaje 1! ";
  m_sms2 = "Ayuda! incendio grave mensaje 2! ";
  m_sms3 = "Ayuda! inundación mensaje 3! ";
  m_sms4 = "Ayuda! exploción con multiples heridos mensaje 4! ";
  m_ControlPakets._NPQ_WAS_Created = 0;
  m_ControlPakets._NPQ_WAS_Duplicated = 0;
  m_ControlPakets._NPQ_WAS_Received = 0;
  m_ControlPakets._NPQ_WAS_ReSend = 0;
  m_ControlPakets.delayOnNode = MilliSeconds (0);
  m_IDSink = 4;
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
          m_wifiDevice->SetReceiveCallback (MakeCallback (&CustomApplication::ReceivePacket, this));
          //dev->SetReceiveCallback (MakeCallback (&CustomApplication::ReceivePacket, this));
          /*
            If you want promiscous receive callback, connect to this trace. 
            For every packet received, both functions ReceivePacket & PromiscRx will be called. with PromicRx being called first!
            */
          m_Channels.push_back (m_wifiDevice);
          Ptr<WifiPhy> phy =
              m_wifiDevice->GetPhy (); //default, there's only one PHY in a WaveNetDevice
          phy->TraceConnectWithoutContext ("MonitorSnifferRx",
                                           MakeCallback (&CustomApplication::PromiscRx, this));
          //break;
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
  Simulator::Schedule (Seconds (1), &CustomApplication::RemoveOldNeighbors, this);
  Simulator::Schedule (Seconds (1), &CustomApplication::RemoveOldTags, this);
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
  //Setup transmission parameters
  TxInfo tx;
  tx.channelNumber = CCH;
  tx.priority = 7; //highest priority.
  tx.txPowerLevel = 7;
  tx.dataRate = m_mode;
  tx.preamble = WIFI_PREAMBLE_LONG;
  Ptr<Packet> packet;
  Ptr<Packet> packet_n;
  CustomDataTag tag;
  
  
  //std::cout << "BRODCASt_ NDATA: " << m_NextData << std::endl;
  //Ptr<Packet> packet = Create<Packet> (m_packetSize);
  //std::cout <<"BRODCASt: "<<   m_Buffer[0] << std::endl;
  /*int i=0;
  while(m_Buffer[i]!='\0' && i<=50 && m_Buffer != NULL){
    std::cout <<"BRODCASt: "<< i<<  m_Buffer[i] << std::endl;
    i++;    
  }*/
  Ptr<UniformRandomVariable> NumberOfChannel = CreateObject<UniformRandomVariable> ();
  uint32_t channelrnd=(uint32_t) NumberOfChannel->GetValue(0,GetNode()->GetNDevices());
  Ptr<NetDevice> dev = GetNode()->GetDevice (channelrnd);
  m_wifiDevice= DynamicCast<WifiNetDevice> (dev);

  std::stringstream msg;
  std::string message;
  std::cout << "Soy el nodo " << GetNode ()->GetId () << " Tengo " << m_buffer.size ()
            << " mensajes en el buffer"
            << "Tengo " << m_Channels.size () << " canales de comunicación" << std::endl;
  if (m_SenderOrSink == 2)
    { //Nodo sink
      std::cout << "soy el sink: " << GetNode ()->GetId () << " Type: " << 2
                << " Paquetes recibidos correctamente: " << m_NumberOfPacketsOK << std::endl;
      /*Aqui se programa los paquetes que debe enviar el Sink*/
    }
  else if (m_SenderOrSink == 1 || m_SenderOrSink == 0)
    { //Nodo que envia un nuevo mensaje  o lo reenvi
      /*Genera paquetes*/

      if (m_SenderOrSink == 1) /*Nodo que crea un nuevo mensaje*/
        {
          if (m_buffer.size () == 0)
            //if (m_NextData == "") /*Hay un paquete pendiente para enviar?*/
            { 
              std::cout << "soy el nodo: " << GetNode ()->GetId () << " Type:" << m_SenderOrSink
                        << std::endl;
              //std::cout << "Broadcast de un nuevo mensaje: " << m_NextData << std::endl;
              //std::cout<<"##############Comienza envio de paquete###############"<< std::endl;
              message = m_sms1;
              char const *cWord = message.c_str ();
              uint8_t *data = new uint8_t[message.size () + 1];
              std::memcpy (data, cWord, message.size () + 1);
              packet = Create<Packet> (data, message.size () + 1);
              /*Al generar un nuevo paquete se agrega un nuevo tag*/
              
              tag.SetNodeId (GetNode ()->GetId ());
              tag.SetNodeSinkId (m_IDSink);
              tag.SetSEQNumber (Now ().GetMilliSeconds ());
              tag.SetTimestamp (Now ());
              tag.SetChannel(channelrnd);

              /*Se agrega el tag al paquete generado*/
              
              packet->AddPacketTag (tag);
              /*Se almacena el tag en la tabla de paquetes analizados previamente para evitar copias*/
              
              FindTags (tag.GetSEQNumber (), Now (), tag.GetNodeId ());
              m_ControlPakets._NPQ_WAS_Created++;
              m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
              //std::cout<<"*************Termina envio de paquete*****************"<< std::endl;
              
            }
          else /*Se crean dos paquetes, uno es el que reenviamos y el otro es el que queremos enviar al Sink*/
            {
              /*Si hay un dato pendiente y ademas soy un nodo que envia mensajes hacia el sink debo crear dos paquetes*/
              std::cout << "soy el nodo: " << GetNode ()->GetId () << " Type:  " << m_SenderOrSink
                        << " Nuevo + reenvio " << std::endl;
              //std::cout<<"##############Comienza envio de paquete###############"<< std::endl;
              /*Se obtiene el mensaje a reenviar del buffer*/
              std::vector<MessagesToBuffer>::iterator DataonBuffer = GetMessageOnBuffer ();
              /*Se crea el paquete con el paquete del buffer*/
              message = DataonBuffer->Message;
              char const *cWord = message.c_str ();
              uint8_t *data = new uint8_t[message.size () + 1];
              std::memcpy (data, cWord, message.size () + 1);
              packet = Create<Packet> (data, message.size () + 1);

              m_ControlPakets._NPQ_WAS_ReSend++;
              /*Termina la creación del primer paquete*/

              /*Creación del paquete de auxilio a enviar */
              message = m_sms4;
              cWord = message.c_str ();
              data = new uint8_t[message.size () + 1];
              std::memcpy (data, cWord, message.size () + 1);
              packet_n = Create<Packet> (data, message.size () + 1);
             //if(DataonBuffer->SourcePacketID==GetNode()->GetId()){
              m_ControlPakets._NPQ_WAS_Created++;
             //}
              
              /*Termina creación del segundo paquete*/

              tag.SetNodeId (DataonBuffer->SourcePacketID);
              tag.SetNodeSinkId (m_IDSink);
              tag.CopySEQNumber (DataonBuffer->SEQnumberTAG);
              tag.SetTimestamp (DataonBuffer->TimeStamp);
              tag.SetChannel(channelrnd);
              packet->AddPacketTag (tag);
    
              tag.SetSEQNumber (Now ().GetMilliSeconds ());
              tag.SetNodeSinkId(m_IDSink);
              tag.SetNodeId (GetNode ()->GetId ());
              tag.SetChannel(channelrnd);
              tag.SetTimestamp(Now());
              packet_n->AddPacketTag (tag);
              m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
              m_wifiDevice->Send (packet_n, Mac48Address::GetBroadcast (), 0x88dc);
              //std::cout<<"*************Termina envio de paquete*****************"<< std::endl;
            }
        }
      else /*Nodo que no genera mensajes nuevo*/
        {

          if (m_buffer.size () == 0) /*No hay paquetes para retransmitir*/
            {
              std::cout << "soy el nodo: " << GetNode ()->GetId () << " Type:  " << m_SenderOrSink
                        << std::endl;
              std::cout << "No hay paquete a reenviar" << std::endl;
            }
          else
            {
              std::vector<MessagesToBuffer>::iterator DataonBuffer = GetMessageOnBuffer ();
              std::cout << "soy el nodo: " << GetNode ()->GetId () << " Type: " << m_SenderOrSink
                        << " Reenvio de mensaje" << std::endl;
              message = DataonBuffer->Message;
              m_ControlPakets._NPQ_WAS_ReSend++;
              char const *cWord = message.c_str ();
              uint8_t *data = new uint8_t[message.size () + 1];
              std::memcpy (data, cWord, message.size () + 1);
              packet = Create<Packet> (data, message.size () + 1);
              //std::cout << "Retransmision de un mensaje: " << m_NextData << std::endl;
              tag.SetChannel(channelrnd);
              tag.SetNodeId (DataonBuffer->SourcePacketID);
              tag.SetNodeSinkId (m_IDSink);
              tag.SetTimestamp(DataonBuffer->TimeStamp);
              tag.CopySEQNumber (DataonBuffer->SEQnumberTAG);
              packet->AddPacketTag (tag);
              m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
            }
        }
    }
  //Schedule next broadcast
  Simulator::Schedule (m_broadcast_time, &CustomApplication::BroadcastInformation, this);
}

bool
CustomApplication::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,
                                  uint16_t protocol, const Address &sender)
{
  //This function is in the aplication layer
  NS_LOG_FUNCTION (device << packet << protocol << sender);
  //NS_LOG_UNCOND(device << packet << protocol << sender);

  /*
        Packets received here only have Application data, no WifiMacHeader. 
        We created packets with 1000 bytes payload, so we'll get 1000 bytes of payload.
    */
  //NS_LOG_INFO ("ReceivePacket() : Node " << GetNode()->GetId() << " : Received a packet from " << sender << " Size:" << packet->GetSize());
  /*NS_LOG_UNCOND ("ReceivePacket() : Node " << GetNode ()->GetId () << " : Received a packet from "
                                           << sender << " Size:" << packet->GetSize ());*/
  /*Se lee el tag del paquete recibido y si almacena si el nodo receptor no es el destino*/
  CustomDataTag AppTag;
  CustomDataTag tag;
  m_ControlPakets._NPQ_WAS_Received++;


  uint8_t *buffer = new uint8_t[packet->GetSize ()];
  packet->CopyData (buffer, packet->GetSize ());

  packet->PeekPacketTag (AppTag);
  m_ControlPakets.delayOnNode+=(Now()-AppTag.GetTimestamp()+m_ControlPakets.delayOnNode)/m_ControlPakets._NPQ_WAS_Received;
  
  
  /*Se almacena el contenido del Tag en el nodo para posteriormete enviarlo*/
  //m_PrevData.m_nodeId = AppTag.GetNodeId ();
  //m_PrevData.m_nodeSinkId = AppTag.GetNodeSinkId ();
  //m_PrevData.m_SEQNumber = AppTag.GetSEQNumber ();
  // m_PrevData.m_timestamp = AppTag.GetTimestamp ();
 /* NS_LOG_UNCOND ("ReceivePacket() : Node " << GetNode ()->GetId () << " : Received a packet from "
                                           << AppTag.GetNodeId ()
                                           << " Size:" << packet->GetSize ());*/
  /*Se pregunta si el paquete es para el Sink*/
  
    if (GetNode ()->GetId () == AppTag.GetNodeSinkId ())
    {
      if (FindTags (AppTag.GetSEQNumber (), Now (), AppTag.GetNodeId ()))
        /*Si el sink ya tiene un numero de SEQ identico al recibido elimina el paquete*/
        {
          m_ControlPakets._NPQ_WAS_Duplicated++;
          m_NextData = "";
        }
      else /*Si es el nodo Sink es el que recibe ya no retransmite el paquete y acumula la información del paquete*/
        {
          PrintPrevSEQnumbers ();
          m_NextData = "";
          m_NumberOfPacketsOK ++;
          std::cout << "Paquetes recibidos correctamente: " << m_NumberOfPacketsOK << std::endl;
        }
    }
  else /*Nodo distinto al Sink que debe reenviar el paquete si este no es una copia*/
    {
      /*Se busca una copia previa del paquete dentro de la tabla de SEQnumbers*/
      if (FindTags (AppTag.GetSEQNumber (), Now (), AppTag.GetNodeId ()))
        {
          m_NextData = "";
          m_ControlPakets._NPQ_WAS_Duplicated++;
        }
      else
        {
          /*if(m_SenderOrSink==1){
            tag.SetSEQNumber(Now().GetMilliSeconds());
            tag.SetNodeId(GetNode()->GetId());
             std::string message= m_sms4;
             char const *cWord = message.c_str ();
             uint8_t *data = new uint8_t[message.size () + 1];
             std::memcpy (data, cWord, message.size () + 1);
             FindTags (tag.GetSEQNumber (), Now (), tag.GetNodeId ());
             SaveMessageOnBuffer ((char *) (buffer), GetNode()->GetId(),tag.GetSEQNumber(),Now());
          }*/
          SaveMessageOnBuffer ((char *) (buffer), AppTag.GetNodeId (), AppTag.GetSEQNumber (),AppTag.GetTimestamp());
          //std::cout << "El paquete no es una copia ## " << (char *) (buffer) << " Node: "<<GetNode()->GetId() << std::endl;
        }
    }

  
  if (packet->PeekPacketTag (tag))
    {

      /*NS_LOG_INFO ("\tFrom Node Id: " << tag.GetNodeId() << " at " << tag.GetPosition() 
                        << "\tPacket Timestamp: " << tag.GetTimestamp() << " delay="<< Now()-tag.GetTimestamp());*/
      /*NS_LOG_UNCOND ("\tFrom Node Id: " << tag.GetNodeId () << " at "
                                        << " Sink_ID:" << tag.GetNodeSinkId ()
                                        << "\tPacket Timestamp: " << tag.GetTimestamp ()
                                        << " delay= " << Now () - tag.GetTimestamp ()
                                        << "SEQNumber: " << tag.GetSEQNumber ());*/
    }

  return true;
}

void
CustomApplication::PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx,
                              MpduInfo mpdu, SignalNoiseDbm sn)
{
  //This is a promiscous trace. It will pick broadcast packets, and packets not directed to this node's MAC address.
  /*
        Packets received here have MAC headers and payload.
        If packets are created with 1000 bytes payload, the size here is about 38 bytes larger. 
    */
  NS_LOG_DEBUG (Now () << " PromiscRx() : Node " << GetNode ()->GetId ()
                       << " : ChannelFreq: " << channelFreq << " Mode: " << tx.GetMode ()
                       << " Signal: " << sn.signal << " Noise: " << sn.noise
                       << " Size: " << packet->GetSize () << " Mode " << tx.GetMode ());
  /*NS_LOG_UNCOND (Now () << " PromiscRx() : Node " << GetNode()->GetId() << " : ChannelFreq: " << channelFreq << " Mode: " << tx.GetMode()
                 << " Signal: " << sn.signal << " Noise: " << sn.noise << " Size: " << packet->GetSize()
                 << " Mode " << tx.GetMode ()
                 );*/
  CustomDataTag AppTag;
  WifiMacHeader hdr;
  packet->PeekPacketTag (AppTag);
  if (packet->PeekHeader (hdr))
    {
      //Let's see if this packet is intended to this node
      Mac48Address destination = hdr.GetAddr1 ();
      Mac48Address source = hdr.GetAddr2 ();

      UpdateNeighbor (source);

      //Mac48Address myMacAddress = m_waveDevice->GetMac (CCH)->GetAddress ();
      //A packet is intened to me if it targets my MAC address, or it's a broadcast message
      if (destination == Mac48Address::GetBroadcast ())
        {
          NS_LOG_DEBUG ("\tFrom: " << source << "\n\tSeq. No. " << hdr.GetSequenceNumber ());
          //std::cout<<context<<std::endl;
          /*std::cout << "Recv. Packet from node: " << AppTag.GetNodeId () << " Signal= " << sn.signal
                    << " Noise= " << sn.noise << std::endl;*/
          //Do something for this type of packets
        }
      else //Well, this packet is not intended for me
        {
          //Maybe record some information about neighbors
        }
    }
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
      NS_LOG_DEBUG (GREEN_CODE << Now () << " : Node " << GetNode ()->GetId ()
                               << " is adding a neighbor with MAC=" << addr << END_CODE);

      NeighborInformation new_n;
      new_n.neighbor_mac = addr;
      new_n.last_beacon = Now ();
      m_neighbors.push_back (new_n);
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
CustomApplication::PrintPrevSEQnumbers ()
{
  std::cout << "Prev Tags Info for Node: " << GetNode ()->GetId () << std::endl;
  for (std::vector<PreviousTags>::iterator it = m_previusTags.begin (); it != m_previusTags.end ();
       it++)
    {
      std::cout << "\t SEQnumber : " << it->SEQnumberTAG << " \t SourceID: " << it->SourcePacketID
                << "\tLast Contact: " << it->lastBeacon << std::endl;
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

      if (last_contact >=m_time_limit) //if it has been more than 5 seconds, we will remove it. You can change this to whatever value you want.
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
bool
CustomApplication::FindTags (u_long PrevSEQNumber, Time lastBeacon, u_long addr)
{
  //Go over all neighbors, find one matching the address, and updates its 'last_beacon' time.
  bool found = false;

  for (std::vector<PreviousTags>::iterator it = m_previusTags.begin (); it != m_previusTags.end ();
       it++)
    {
      if (it->SEQnumberTAG == PrevSEQNumber)
        {
          found = true;
          //NS_LOG_UNCOND ("Packet Duplicated Tag: " << PrevSEQNumber);
          break;
        }
    }
  if (!found)
    { //si no hay ninguna replica de este SEQNumber se agrega como una nueva entrada
      PreviousTags new_n;
      new_n.SEQnumberTAG = PrevSEQNumber;
      new_n.SourcePacketID = addr;
      new_n.lastBeacon = Now ().GetNanoSeconds();
      m_previusTags.push_back (new_n);
    }
  return found;
}
void
CustomApplication::SaveMessageOnBuffer (std::string message, u_long SourcePacketID,
                                        u_long SEQnumberTAG, Time Timestamp)
{

  MessagesToBuffer new_message;
  new_message.Message = message;
  new_message.SourcePacketID = SourcePacketID;
  new_message.SEQnumberTAG = SEQnumberTAG;
  new_message.TimeStamp=Timestamp;
  m_buffer.push_back (new_message);
}
std::vector<MessagesToBuffer>::iterator
CustomApplication::GetMessageOnBuffer ()
{

  std::vector<MessagesToBuffer>::iterator it = m_buffer.begin ();
  m_buffer.erase (it);
  return it;
}
void
CustomApplication::printMessageOnBuffer ()
{
  for (std::vector<MessagesToBuffer>::iterator it = m_buffer.begin (); it != m_buffer.end (); it++)
    {
      std::cout << "\t Mensaje: " << it->Message << " | SourceID: " << it->SourcePacketID
                << " | SEQNumber: " << it->SEQnumberTAG << std::endl;
    }
}
void
CustomApplication::RemoveOldTags ()
{
  for (std::vector<PreviousTags>::iterator it = m_previusTags.begin (); it != m_previusTags.end ();
       it++)
    {
      int64_t last_SeqNumber = Now ().GetNanoSeconds() - it->lastBeacon;
      if (last_SeqNumber >=  Seconds(2).GetNanoSeconds())
        {
          m_previusTags.erase (it);
          break;
        }
    }

  Simulator::Schedule (Seconds (1), &CustomApplication::RemoveOldTags, this);
}

void
CustomApplication::setSenderOrSink (u_short value)
{
  m_SenderOrSink = value;
}
u_short
CustomApplication::GetSenderOrSink ()
{
  return m_SenderOrSink;
}
void
CustomApplication::SetSinkID (u_short SinkID)
{
  m_IDSink = SinkID;
}
u_short
CustomApplication::GetSinkID ()
{
  return m_IDSink;
}

} // namespace ns3
