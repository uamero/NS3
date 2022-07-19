#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "applicationB.h"
#include "SecundariosTag.h"
#include "SinkTag.h"
#include "TagPrimarios.h"
#include "ns3/mobility-building-info.h"

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
  m_broadcast_time = Seconds (20); //every 100ms
  m_time_limit = Seconds (5); //Tiempo limite para los nodos vecinos
  //m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
  m_mode = WifiMode ("OfdmRate54Mbps");
  m_n_channels = 8;
  m_Batery = 100.0;
  m_retardo_acumulado = 0.0;
  m_collissions = 0;

  //iniciaCanales();
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
  //Simulator::Schedule (Seconds (1), &CustomApplicationBnodes::RemoveOldNeighbors, this);
  //Simulator::Schedule (Seconds (0.1), &CustomApplicationBnodes::CheckBuffer, this);
  Simulator::Schedule (Seconds (0.1), &CustomApplicationBnodes::SendPacket, this);
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
  //std::cout<<"APPB en funcionamiento time: "<<m_Canales_Para_Utilizar.size()<<" | Batery: "<<m_Batery<<"% "<<" ID:"<< GetNode()->GetId()<<std::endl;
  /*if(Now().GetSeconds()>=21)
    this->SetStopTime(Seconds(.2));*/
  //std::cout << "Entro a  broadcast information B" << std::endl;
  NS_LOG_FUNCTION (this);
  //std::cout<<"Nodos B: "<<GetNode()->GetNDevices() << " n_chanels: "<<m_n_channels<<std::endl;
  //std::cout << "Aqui me quedo 9B" << std::endl;
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  if (m_Paquetes_A_Reenviar.size () != 0)
    {
      // std::cout<<"Nodos B: "<<GetNode()->GetId()<<" Paquetes en memoria>> "<<m_Paquetes_A_Reenviar.size ()<<std::endl;
      for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_A_Reenviar.begin ();
           it != m_Paquetes_A_Reenviar.end (); it++)
        {
          Time Ultimo_Envio = Now () - it->Tiempo_ultimo_envio;
          it->retardo = it->retardo + Ultimo_Envio;
          if (m_Canales_Para_Utilizar.size () != 0 && Ultimo_Envio >= m_broadcast_time)
            {
              it->Tiempo_ultimo_envio = Now ();
              SecundariosDataTag etiqueta;
              it->m_packet->PeekPacketTag (
                  etiqueta); //Este paquete proviene de la lectura del buffer por lo que ya contiene una etiqueta
              etiqueta.SetTimestamp (etiqueta.GetTimestamp () + Ultimo_Envio);
              etiqueta.SetSL (rand->GetValue (0, 1));

              etiqueta.SetNodeIdPrev (GetNode ()->GetId ());
              m_retardo_acumulado += Ultimo_Envio.GetSeconds ();
              for (std::list<uint32_t>::iterator _it = m_Canales_Para_Utilizar.begin ();
                   _it != m_Canales_Para_Utilizar.end (); _it++)
                {
                  etiqueta.SetChanels (*_it);
                  //etiqueta.SetSL(it->retardo.GetSeconds());//aqui se debe involucrar el retardo

                  //etiqueta.SetSL (etiqueta.GetTimestamp ().GetSeconds () /
                  //                m_retardo_acumulado); //aqui se debe involucrar el retardo

                  /*std::cout << "**El retardo en el nodo B" << GetNode ()->GetId ()
                            << " Es: " << etiqueta.GetTimestamp ().GetSeconds () << std::endl;*/
                  it->m_packet->ReplacePacketTag (etiqueta);
                  uint32_t NIC = 0;
                  for (std::list<ST_bufferOfCannelsB>::iterator buffer = m_bufferB.begin ();
                       buffer != m_bufferB.end (); buffer++)
                    { //se itera por los canales y se guarda el paquete en el buffer correspondiente
                      if (NIC == *_it)
                        { // se localiza el buffer en el que se guardara el paquete
                          ST_PacketInBufferB newPacket;
                          newPacket.m_packet = it->m_packet->Copy ();
                          newPacket.m_TimeTosavedOnBuffer = Now ();
                          newPacket.m_Send = true;
                          buffer->m_PacketAndTime.push_back (newPacket);

                          /*std::cout << "#BufferB" << GetNode ()->GetId () << " : " << NIC
                                    << " Size: " << buffer->m_PacketAndTime.size ()
                                    << " Time: " << Now ().GetSeconds ()
                                    << " Paquetes a reenviar: " << m_Paquetes_A_Reenviar.size ()
                                    << std::endl;*/
                          break; // este break rompe la iteración para los canales
                        }
                      NIC++;
                    }
                  //CheckBuffer();
                  //uint8_t *buffer = new uint8_t[it->m_packet->GetSize ()];

                  //it->m_packet->CopyData (buffer, it->m_packet->GetSize ());

                  //std::string ruta = std::string (buffer, buffer + it->m_packet->GetSize ());
                  //std::cout<<ruta<<std::endl;
                  //m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (*_it));
                  //m_wifiDevice->Send (it->m_packet->Copy (), Mac48Address::GetBroadcast (), 0x88dc);
                  //std::cout<<"Salgo B broadcast"<<std::endl;
                  //std::cout << "Salgo B broadcast: " << Now ().GetSeconds () << std::endl;
                }

              //std::cout << "El canal en B es : " << std::to_string (ch) << std::endl;
              break;
            }
        }
    }
  else
    {
      //std::cout<<" Memoria vacia nodo B | Id: " << GetNode()->GetId() <<std::endl;
    }
  /*std::cout << GetNode ()->GetId () << " | Ruta en reeenvio>>>>>>  " << Now ().GetSeconds ()
            << std::endl;*/

  //std::cout<<" Tiempo de reenvio: "<<(m_broadcast_time).GetSeconds()<<std::endl;
  //Broadcast the packet as WSMP (0x88dc)
  //Schedule next broadcast
  //std::cout << "Aqui me quedo 10B" << std::endl;
  /*Simulator::Schedule (m_broadcast_time,
                       &CustomApplicationBnodes::ReadPacketOnBuffer, this);*/
  // std::cout << "Salgo de  broadcast information B" << std::endl;

  Simulator::Schedule (m_broadcast_time, &CustomApplicationBnodes::BroadcastInformation, this);
}

bool
CustomApplicationBnodes::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,
                                        uint16_t protocol, const Address &sender)
{
  //std::cout << "Entro a received packet B" << std::endl;

  /**Hay dos tipos de paquetes que se pueden recibir
   * 1.- Un paquete que fue enviado por este nodo 
   * 2.- Un paquete que fue enviado por el Sink*/

  NS_LOG_FUNCTION (device << packet << protocol << sender);
  /*
        Packets received here only have Application data, no WifiMacHeader. 
        We created packets with 1000 bytes payload, so we'll get 1000 bytes of payload.
    */
  //std::cout << "Aqui me quedo 7B" << std::endl;
  NS_LOG_INFO ("ReceivePacket() : Node " << GetNode ()->GetId () << " : Received a packet from "
                                         << sender << " Size:" << packet->GetSize ());

  //uint32_t size;
  uint32_t NIC = 0;

  for (std::list<ST_bufferOfCannelsB>::iterator it = m_bufferB.begin (); it != m_bufferB.end ();
       it++)
    {
      if (NIC == device->GetIfIndex ())
        {
          // std::cout << "Aqui me quedo 3######################################> "
          //       << device->GetIfIndex () << " | " << NIC << std::endl;

          SecundariosDataTag sec;
          ST_PacketInBufferB newPacket;
          newPacket.m_packet = packet->Copy ();
          newPacket.m_TimeTosavedOnBuffer = Now ();
          newPacket.m_Send =
              false; //este paquete aun debe ser leido por el nodo para ver si es reenviado o desechado.
          packet->PeekPacketTag (sec);
          if (!VerificaCanal (sec.GetChanels ()) && NIC < m_n_channels)
            { //si el canal no esta disponible es una colision
              m_collissions++;
              //std::cout <<GetNode()->GetId()<<" número de colisiones: "<<m_collissions<<" NIC: "<<NIC<<std::endl;
              break;
            }
          Simulator::Schedule (Seconds ((8.0 * 1024.0) / (m_mode.GetDataRate (20))),
                               &CustomApplicationBnodes::CheckBuffer, this);
          it->m_PacketAndTime.push_back (newPacket);
          /* std::cout << "Nodo: " << GetNode ()->GetId () << " Recibo paquete correctamente B"
                    << Now ().GetSeconds () << "SEQN: " << sec.GetSEQNumber () << std::endl;*/
          /*std::cout << "Aqui se guarda el retardo es: " << sec.GetTimestamp ().GetSeconds ()
                    << std::endl;*/
          //Let's see if this packet is intended to this node
          //Mac48Address destination = hdr.GetAddr1 ();
          // Mac48Address source = hdr.GetAddr2 ();
          Mac48Address source = Mac48Address::ConvertFrom (sender);
          ST_VecinosB NV;
          NV.NodoID = sec.GetNodeIdPrev ();
          NV.SL_canal = sec.GetSL ();
          NV.MacaddressNB = source;
          NV.canal = NIC;
          //UpdateNeighbor (NV);
          break;
        }
      NIC++;
    }

  //CheckBuffer ();
  //ImprimeTabla();
  //Imprimebuffers();
  //uint8_t *buffer = new uint8_t[packet->GetSize ()];
  //packet->CopyData (buffer, packet->GetSize ());
  //std::string ruta = std::string (buffer, buffer + packet->GetSize ());
  //std::cout << "Received:" << ruta << " Size: " << size << std::endl;
  //packet->PeekPacketTag (tag);

  //std::cout << "El data rate es en B " <<(8.0 * 1000.0) /m_mode.GetDataRate (10)<< std::endl;

  //std::cout << "El indice es: " << device->GetIfIndex () << std::endl;

  // std::cout << "Aqui me quedo 8B" << std::endl;
  // std::cout << "Salgo received packet B" << std::endl;
  return true;
}

void
CustomApplicationBnodes::SendPacket ()
{
  //if(VerificaVisitados()) {ReiniciaVisitados();}
  //std::cout << "Aqui me quedoSend1B" << std::endl;
  // std::cout << "Entro a send packet buffer B" << std::endl;
  uint32_t NIC = 0;
  for (std::list<ST_bufferOfCannelsB>::iterator it = m_bufferB.begin (); it != m_bufferB.end ();
       it++) //Se itera sobre cada buffer de canal para identificar si hay paquetes a enviar
    {
      Ptr<Packet> packet;
      Time TimeInThisNode;
      SecundariosDataTag SecundariosTag;

      if (it->m_PacketAndTime.size () != 0 &&
          !(*it)
               .m_visitado_Enviar) // Se evalua si el buffer visitado esta vacio o bien si no ha sido visitado
        {
          /*if (NIC == m_n_channels)
            {
              ReiniciaVisitados_Enviar ();
              break;
            }*/
          (*it).m_visitado_Enviar = true;
          //std::cout << "Aqui me quedo 1" << std::endl;
          bool find = false;
          for (std::list<ST_PacketInBufferB>::iterator pck = it->m_PacketAndTime.begin ();
               pck != it->m_PacketAndTime.end ();
               pck++) //Este for itera sobre los paquetes del buffer visitado
            { // este for es para obtener solo un paquete dentro del buffer en el canal n
              if (pck->m_Send)
                {
                  find = true;
                  packet = pck->m_packet->Copy ();
                  TimeInThisNode = Now () - pck->m_TimeTosavedOnBuffer;
                  /* std::cout << "El retardo en el nodo B" << GetNode ()->GetId ()
                               << " Es: " << TimeInThisNode.GetMilliSeconds()<< std::endl;*/

                  //std::cout << "El paquete tiene un : " << pck->m_Send << std::endl;
                  it->m_PacketAndTime.erase (pck);
                  break;
                }
            }
          if (!find)
            {

              break;
            }
          //std::cout << "Aqui me quedo 2" << std::endl;

          if (packet->PeekPacketTag (SecundariosTag))
            {
              if (NIC != m_n_channels && NIC != m_n_channels + 1 &&
                  VerificaCanal (SecundariosTag.GetChanels ()))
                {
                  // std::cout << "Aqui me quedo 3" << std::endl;
                  //std::cout << "AppB::SendPacket1:"<<Now().GetSeconds() << std::endl;
                  /*uint8_t *buffer = new uint8_t[packet->GetSize ()];
                  packet->CopyData (buffer, packet->GetSize ());*/
                  std::string ruta = std::string (SecundariosTag.GetBufferRoute (),
                                                  SecundariosTag.GetBufferRoute () +
                                                      SecundariosTag.GetSizeBufferRoute ()) +
                                     "," + std::to_string (GetNode ()->GetId ());
                  /*std::string ruta = std::string (buffer, buffer + packet->GetSize ()) + "," +
                                     std::to_string (GetNode ()->GetId ());*/

                  /*Ptr<Packet> PacketToReSend =
                      Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());*/
                  //SecundariosTag.Print (std::cout<<"1> ");
                  // std::cout << "Aqui me quedoBB1:" << std::endl;
                  SecundariosTag.SetTimestamp (TimeInThisNode + SecundariosTag.GetTimestamp ());
                  SecundariosTag.SetBufferRoute ((uint8_t *) ruta.c_str ());
                  SecundariosTag.SetSizeBufferRoute (ruta.length ());
                  //std::cout <<"Salgo de aqui"<<std::endl;
                  /*std::cout << Now ().GetSeconds () << " | "
                            << "Nodo: " << GetNode ()->GetId ()
                            << " El retardo de este paquete enviado es: "
                            << SecundariosTag.GetTimestamp ().GetSeconds ()
                            << " Disponibilidad: " << m_Canales_Para_Utilizar.size ()
                            << "SEQN: " << SecundariosTag.GetSEQNumber () << std::endl;*/
                  //SecundariosTag.Print (std::cout);
                  // packet->RemoveAllPacketTags();
                  packet->ReplacePacketTag (SecundariosTag);
                  //std::cout << "Aqui me quedoBB2:" << std::endl;

                  //PacketToReSend->AddPacketTag (SecundariosTag);
                  if (Entregado (SecundariosTag.GetSEQNumber (), SecundariosTag.GetNodeId (),
                                 SecundariosTag.GetcopiaID ()))
                    {
                      break; //Si el paquete ya está entregado no se envia el paquete
                    }
                  m_wifiDevice = DynamicCast<WifiNetDevice> (
                      GetNode ()->GetDevice (SecundariosTag.GetChanels ()));
                  m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (),
                                      0x88dc); //aqui se hace el broadcast en un canal
                  //std::cout << "Aqui me quedo Fin de SendPacket " << std::endl;
                 //std::cout << "AppB::SendPacket2:"<< std::endl;
                  break;
                }
            }
        }
      else
        {
          (*it).m_visitado_Enviar = true;
        }
      // std::cout << "Aqui me quedo " <<NIC<< std::endl;

      NIC++;
    }
  if (VerificaVisitados_Enviar ()) // se verifica si todos los buffers fueron visitados
    {
      /*std::cout << "El nodo es: " << GetNode ()->GetId () << " Aqui me quedo CheckBuffer2"
                << std::endl;*/
      ReiniciaVisitados_Enviar (); //si ya fueron visitados se reinician de nuevo las visitas al primer canal (es de forma circular)
    }
  // std::cout << "Aqui me quedoSend2B" << std::endl;
  
  Simulator::Schedule (Seconds ((8.0 * 1024.0) / (m_mode.GetDataRate (20))),
                       &CustomApplicationBnodes::SendPacket, this);
  // std::cout << "Salgo de send packet buffer B" << std::endl;
}

void
CustomApplicationBnodes::ReadPacketOnBuffer ()
{
  //std::cout<<"Entro aqui B read buff"<<std::endl;
  /*Ptr<MobilityBuildingInfo> MB = GetNode()->GetObject<MobilityBuildingInfo> ();
          Ptr<MobilityModel> mm = GetNode()->GetObject<MobilityModel> ();
          MB->MakeConsistent (mm);*/
  //std::cout << "Entro a read buffer B" << std::endl;
  std::list<ST_bufferOfCannelsB>::iterator SinkAndPrimariosIterator = m_bufferB.end ();
  uint32_t NIC = 0;
  for (std::list<ST_bufferOfCannelsB>::iterator it = m_bufferB.begin (); it != m_bufferB.end ();
       it++) //Se itera sobre cada buffer de canal para identificar si hay paquetes a enviar
    {
      //std::cout << "Aqui me quedo11B" << std::endl;
      Ptr<Packet> packet;
      Time TimeInThisNode;
      SecundariosDataTag SecundariosTag;
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      bool send;
      if (it->m_PacketAndTime.size () != 0 &&
          !(*it)
               .m_visitado_Guardar) // Se evalua si el buffer visitado esta vacio o bien si no ha sido visitado
        {
          if (NIC == m_n_channels)
            {
              SinkAndPrimariosIterator = it;
              break;
            }
          (*it).m_visitado_Guardar = true;
          /*std::cout << "BufferB" << GetNode ()->GetId () << " : " << NIC
                    << " Size: " << it->m_PacketAndTime.size () << " Time: " << Now ().GetSeconds ()
                    << std::endl;*/

          for (std::list<ST_PacketInBufferB>::iterator pck = it->m_PacketAndTime.begin ();
               pck != it->m_PacketAndTime.end ();
               pck++) //Este for itera sobre los paquetes del buffer visitado
            { // este for es para obtener solo un paquete dentro del buffer en el canal n
              packet = pck->m_packet->Copy ();

              TimeInThisNode = Now () - pck->m_TimeTosavedOnBuffer;
              //std::cout << "El retardo en el nodo B" << GetNode ()->GetId ()
              //             << " Es: " << TimeInThisNode.GetMilliSeconds()<< std::endl;
              send = pck->m_Send;
              //std::cout << "El paquete tiene un : " << pck->m_Send << std::endl;
              if (!send)
                {
                  it->m_PacketAndTime.erase (
                      pck); //se obtiene el primer paquete que esta en la posicion 0
                }
              break;
            }
          if (packet->PeekPacketTag (SecundariosTag))
            { //Se verifica el paquete recibido sea de nodos secundarios
              if (NIC != m_n_channels && NIC != m_n_channels + 1 &&
                  VerificaCanal (SecundariosTag.GetChanels ()))
                {
                  /*std::cout << "#BufferB" << GetNode ()->GetId () << " : " << NIC
                            << " Size: " << it->m_PacketAndTime.size ()
                            << " Time: " << Now ().GetSeconds () << std::endl;*/
                  //std::cout<<"######Aquii "<<TimeInThisNode.GetMilliSeconds()<<std::endl;
                  /*uint8_t *buffer = new uint8_t[packet->GetSize ()];

                  packet->CopyData (buffer, packet->GetSize ());*/

                  /*if (send)
                    { // se verifica si el paquete ya está listo para ser enviado o se debe leer
                      std::string ruta = std::string (buffer, buffer + packet->GetSize ()) + "," +
                                         std::to_string (GetNode ()->GetId ());
                      Ptr<Packet> PacketToReSend =
                          Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());

                      SecundariosTag.SetTimestamp (TimeInThisNode + SecundariosTag.GetTimestamp ());
                      std::cout << Now ().GetSeconds () << " | "
                                << "Nodo: " << GetNode ()->GetId ()
                                << " El retardo de este paquete enviado es: "
                                << SecundariosTag.GetTimestamp ().GetSeconds ()
                                << " Disponibilidad: " << m_Canales_Para_Utilizar.size ()
                                << std::endl;
                      PacketToReSend->AddPacketTag (SecundariosTag);
                      if (Entregado (SecundariosTag.GetSEQNumber ()))
                        {
                          break; //Si el paquete ya está entregado no se envia el paquete
                        }
                      m_wifiDevice = DynamicCast<WifiNetDevice> (
                          GetNode ()->GetDevice (SecundariosTag.GetChanels ()));
                      m_wifiDevice->Send (PacketToReSend, Mac48Address::GetBroadcast (),
                                          0x88dc); //aqui se hace el broadcast en un canal
                    }*/
                  if (!send)
                    {
                     // std::cout << "AppB::ReadPacketOnBuffer1:"<<Now().GetSeconds() << std::endl;
                      std::string ruta = std::string (SecundariosTag.GetBufferRoute (),
                                                      SecundariosTag.GetBufferRoute () +
                                                          SecundariosTag.GetSizeBufferRoute ());

                      for (std::list<uint32_t>::iterator FreeChIt =
                               m_Canales_Para_Utilizar.begin ();
                           FreeChIt != m_Canales_Para_Utilizar.end (); FreeChIt++)
                        {
                          /*Ptr<Packet> PacketToReSend =
                              Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());*/

                          SecundariosTag.SetChanels ((*FreeChIt));

                          SecundariosTag.SetTimestamp (TimeInThisNode +
                                                       SecundariosTag.GetTimestamp ());
                          /*std::cout << "El retardo en el nodo B" << GetNode ()->GetId ()
                                    << " Es: " << SecundariosTag.GetTimestamp ().GetSeconds ()
                                    << std::endl;*/
                          //SecundariosTag.SetSL(m_satisfaccionL);
                          SecundariosTag.SetSL (rand->GetValue (0, 1));
                          SecundariosTag.SetNodeIdPrev (GetNode ()->GetId ());
                          //std::cout << "Aqui me quedoB1:" << std::endl;
                          //PacketToReSend->AddPacketTag (SecundariosTag);
                          packet->ReplacePacketTag (SecundariosTag);
                          //std::cout << "Aqui me quedoB2:" << std::endl;
                          if (Entregado (SecundariosTag.GetSEQNumber (),
                                         SecundariosTag.GetNodeId (), SecundariosTag.GetcopiaID ()))
                            {
                              break; //Si el paquete ya está entregado no se envia el paquete
                            }
                          else if (!BuscaSEQEnTabla (SecundariosTag.GetSEQNumber (),
                                                     SecundariosTag.GetNodeId (),
                                                     SecundariosTag.GetcopiaID ()))
                            { // Se busca si este paquete no se ha guardado en el nodo para ser reenviado
                              m_retardo_acumulado += TimeInThisNode.GetSeconds () +
                                                     SecundariosTag.GetTimestamp ().GetSeconds ();
                              // std::cout << "Aqui me quedo2B" << std::endl;

                              //std::cout << "El retardo en el nodo B" << GetNode ()->GetId ()
                              //        << " Es: " << m_retardo_acumulado << std::endl;
                              Guarda_Paquete_reenvio (
                                  packet,
                                  TimeInThisNode); //En este punto se almacena en memoria el paquete
                            }
                          else
                            {
                              break;
                            }

                          uint32_t _NIC = 0;
                          for (std::list<ST_bufferOfCannelsB>::iterator buff = m_bufferB.begin ();
                               buff != m_bufferB.end (); buff++)
                            { //se itera por los canales y se guarda el paquete en el buffer correspondiente
                              /* std::cout << "######Aquii " << m_retardo_acumulado << "| "
                                        << (*FreeChIt) << "| " << _NIC << std::endl;*/
                              if (_NIC == (*FreeChIt))
                                { // se localiza el buffer en el que se guardara el paquete
                                  ST_PacketInBufferB newPacket;
                                  newPacket.m_packet = packet->Copy ();
                                  newPacket.m_TimeTosavedOnBuffer = Now ();
                                  newPacket.m_Send =
                                      true; //se coloca en true para enviar el paquete
                                  buff->m_PacketAndTime.push_back (newPacket);

                                  //(*buffer).m_PacketAndTime.push_back(newPacket);
                                  /*std::cout
                                      << "##BufferB" << GetNode ()->GetId () << " : " << (*FreeChIt)
                                      << " size: " << buff->m_PacketAndTime.size ()
                                      << " Time: " << Now ().GetSeconds ()
                                      << " Disponibilidad: " << m_Canales_Para_Utilizar.size ()
                                      << std::endl;*/

                                  break;
                                }
                              else
                                {
                                  _NIC++;
                                }
                              //break;
                            }
                          //Imprimebuffers ();
                        }
                      //break;
                      /*Simulator::Schedule (Seconds ((8.0 * 1024.0) / (m_mode.GetDataRate (20))),
                           &CustomApplicationBnodes::CheckBuffer, this);  */
                      //Imprimebuffers();
                      //este break rompe el for que itera sobre los buffer de los canales
                      //m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (NIC));
                      // m_wifiDevice->Send (PacketToReSend->Copy (),
                      //                    Mac48Address::GetBroadcast (), 0x88dc);
                      //std::cout << "Aqui me quedo Envio paquete2" << std::endl;
                    }
                  // SecundariosTag.Print(std::cout<<SecundariosTag.GetSerializedSize());
                  // std::cout << "Aqui me quedo2B" << std::endl;

                  break; //este break rompe el for que itera sobre los buffer de los canales
                }
            }
        }
      else
        {
          (*it).m_visitado_Guardar = true;
        }

      NIC++; //es el device correspondiente
    }
  //std::cout << "Aqui me quedo3B" << std::endl;
  NIC = m_n_channels;
  for (std::list<ST_bufferOfCannelsB>::iterator it = SinkAndPrimariosIterator;
       it != m_bufferB.end ();
       it++) //Se itera sobre cada buffer de canal para identificar si hay paquetes a enviar
    {
      SinkDataTag SinkTag;
      PrimariosDataTag PrimariosTag;
      Ptr<Packet> packet;
      Time TimeInThisNode;

      if (it->m_PacketAndTime.size () != 0 &&
          !(*it)
               .m_visitado_Guardar) // Se evalua si el buffer visitado esta vacio o bien si no ha sido visitado
        {
          (*it).m_visitado_Guardar = true;
          for (std::list<ST_PacketInBufferB>::iterator pck = it->m_PacketAndTime.begin ();
               pck != it->m_PacketAndTime.end ();
               pck++) //Este for itera sobre los paquetes del buffer visitado
            { // este for es para obtener solo un paquete dentro del buffer en el canal n
              packet = pck->m_packet->Copy ();

              TimeInThisNode = Now () - pck->m_TimeTosavedOnBuffer;
              //std::cout << "El retardo en el nodo B" << GetNode ()->GetId ()
              //             << " Es: " << TimeInThisNode.GetMilliSeconds()<< std::endl;
              //std::cout << "El paquete tiene un : " << pck->m_Send << std::endl;
              it->m_PacketAndTime.erase (pck);
              break;
            }
          if (packet->PeekPacketTag (SinkTag))
            {
              if (NIC == m_n_channels)
                {
                  //std::cout << "Si confirmo la entregaB" << std::endl;
                  //Imprimebuffers ();
                  std::string ruta =
                      std::string (SinkTag.GetBufferRoute (),
                                   SinkTag.GetBufferRoute () + SinkTag.GetSizeBufferRoute ());
                  ConfirmaEntrega (SinkTag.GetSEQNumber (), SinkTag.GetNodeId (),
                                   SinkTag.GetcopiaID ());
                  //std::cout << "Aqui me quedo3" << std::endl;

                  //Imprimebuffers();
                  break;
                }
            }
          else if (packet->PeekPacketTag (PrimariosTag))
            {
              if (NIC == m_n_channels + 1)
                {
                  uint8_t *buffer = new uint8_t[packet->GetSize ()];
                  packet->CopyData (buffer, packet->GetSize ());
                  std::string canalesDP = std::string (buffer, buffer + packet->GetSize ());

                  BuscaCanalesID (canalesDP, PrimariosTag.GetnodeID (), Now ());
                  CanalesDisponibles ();
                  break;
                }
            }
        }
      else
        {
          (*it).m_visitado_Guardar = true;
        }
      NIC++;
    }
   // std::cout << "AppB::ReadPacketOnBuffer2" << std::endl;
  //CheckBuffer();
  //std::cout<<"Si salgo de aqui B read buff"<<std::endl;
  //CheckBuffer();
  /* Simulator::Schedule (Seconds ((8.0 * 1024.0) / (m_mode.GetDataRate (20))),
                           &CustomApplicationBnodes::SendPacket, this);*/
  if (VerificaVisitados_Guardar ()) // se verifica si todos los buffers fueron visitados
    {
      /*std::cout << "El nodo es: " << GetNode ()->GetId () << " Aqui me quedo CheckBuffer2"
                << std::endl;*/
      ReiniciaVisitados_Guardar (); //si ya fueron visitados se reinician de nuevo las visitas al primer canal (es de forma circular)
    }
  /*if (BuscaPaquete ())
    {
      Simulator::Schedule (m_broadcast_time,
                           &CustomApplicationBnodes::ReadPacketOnBuffer, this);
    }*/
  //std::cout << "Salgo de Readbuffer en B" << std::endl;
}

void
CustomApplicationBnodes::UpdateNeighbor (ST_VecinosB neighbor)
{
  bool found = false;
  //Go over all neighbors, find one matching the address, and updates its 'last_beacon' time.
  for (std::list<ST_VecinosB>::iterator it = m_vecinos_list.begin (); it != m_vecinos_list.end ();
       it++)
    {
      if (it->MacaddressNB == neighbor.MacaddressNB && it->canal == neighbor.canal)
        {
          it->last_beacon = Now ();

          found = true;
          break;
        }
    }
  if (!found) //If no node with this address exist, add a new table entry
    {
      NS_LOG_INFO (GREEN_CODE << Now () << " : Node " << GetNode ()->GetId ()
                              << " is adding a neighbor with ID=" << neighbor.MacaddressNB
                              << END_CODE);
      ST_VecinosB new_n;
      new_n.MacaddressNB = neighbor.MacaddressNB;
      new_n.canal = neighbor.canal;
      new_n.last_beacon = Now ();
      //m_vecinos_list.push_back (new_n);
      /*Se inserta ordenado en la tabla de vecinos con respecto a la SL*/
      if (m_vecinos_list.size () == 0)
        {
          //si no hay vecinos registrados
          m_vecinos_list.push_back (new_n);
        }
      else
        {
          for (std::list<ST_VecinosB>::iterator it = m_vecinos_list.begin ();
               it != m_vecinos_list.end (); it++)
            {
              if (it == m_vecinos_list.end ())
                {
                  m_vecinos_list.insert (it, new_n);
                }
              else if (neighbor.SL_canal == it->SL_canal || neighbor.SL_canal > it->SL_canal)
                {
                  m_vecinos_list.insert (it, new_n);
                  break;
                }
            }
        }
    }
}

void
CustomApplicationBnodes::PrintNeighbors ()
{
  std::cout << "Neighbor Info for Node: " << GetNode ()->GetId () << std::endl;
  for (std::list<ST_VecinosB>::iterator it = m_vecinos_list.begin (); it != m_vecinos_list.end ();
       it++)
    {
      std::cout << "\tSL: " << it->SL_canal << "\tID: " << it->NodoID
                << "\tMAC: " << it->MacaddressNB << "\tCanal: " << it->canal
                << "\tLast Contact: " << it->last_beacon.GetSeconds () << std::endl;
    }
}

void
CustomApplicationBnodes::RemoveOldNeighbors ()
{
  //Go over the list of neighbors
  for (std::list<ST_VecinosB>::iterator it = m_vecinos_list.begin (); it != m_vecinos_list.end ();
       it++)
    {
      //Get the time passed since the last time we heard from a node
      Time last_contact = Now () - it->last_beacon;

      if (last_contact >=
          Seconds (
              5)) //if it has been more than 5 seconds, we will remove it. You can change this to whatever value you want.
        {
          NS_LOG_INFO (RED_CODE << Now ().GetSeconds () << " Node " << GetNode ()->GetId ()
                                << " is removing old neighbor " << it->MacaddressNB << END_CODE);
          //Remove an old entry from the table
          m_vecinos_list.erase (it);
          break;
        }
    }
  //Check the list again after 1 second.
  Simulator::Schedule (m_broadcast_time, &CustomApplicationBnodes::RemoveOldNeighbors, this);
}

bool
CustomApplicationBnodes::BuscaPaquete ()
{
  bool find = false;
  for (std::list<ST_bufferOfCannelsB>::iterator it = m_bufferB.begin (); it != m_bufferB.end ();
       it++)
    {
      /* std::cout << "El nodo es: " << GetNode ()->GetId ()
                << " | El tamaño del buffer es: " << it->m_PacketAndTime.size ()
                << " y su estado es: " << it->m_visitado << std::endl;*/
      if (it->m_PacketAndTime.size () != 0 && !it->m_visitado_Enviar)
        {
          find = true;

          break;
        }
    }
  // std::cout << "Aqui me quedo BuscaPaquete(): " <<find<< std::endl;

  return find;
}

void
CustomApplicationBnodes::Guarda_Paquete_reenvio (Ptr<Packet> paquete, Time TimeBuff)
{

  ST_ReenviosB reenvio;
  reenvio.m_packet = paquete->Copy ();
  reenvio.Tiempo_ultimo_envio = Now ();
  reenvio.retardo = TimeBuff;
  m_Paquetes_A_Reenviar.push_back (reenvio);
}

void
CustomApplicationBnodes::ConfirmaEntrega (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia)
{

  if (m_Paquetes_A_Reenviar.size () != 0)
    {
      for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_A_Reenviar.begin ();
           it != m_Paquetes_A_Reenviar.end (); it++)
        {
          SecundariosDataTag SecTAg;
          it->m_packet->PeekPacketTag (SecTAg);
          if (SecTAg.GetSEQNumber () == SEQ && SecTAg.GetcopiaID () == IDCopia &&
              SecTAg.GetNodeId () == IDcreador)
            {
              m_Paquetes_Recibidos.push_back ((*it));
              m_Paquetes_A_Reenviar.erase (it);
              break;
            }
        }
    }
  for (std::list<ST_bufferOfCannelsB>::iterator buff = m_bufferB.begin (); buff != m_bufferB.end ();
       buff++)
    {

      for (std::list<ST_PacketInBufferB>::iterator pck = buff->m_PacketAndTime.begin ();
           pck != buff->m_PacketAndTime.end ();
           pck++) //Este for itera sobre los paquetes del buffer visitado
        {
          Ptr<Packet> packet;
          SecundariosDataTag SecundariosTag;

          packet = pck->m_packet->Copy ();

          if (packet->PeekPacketTag (SecundariosTag))
            {
              //std::cout << "Aqui toy***************************************|| " << SecundariosTag.GetSEQNumber ()<<std::endl;
              if (SEQ == SecundariosTag.GetSEQNumber ())
                {
                  if (pck != buff->m_PacketAndTime.end ())
                    {
                      pck = buff->m_PacketAndTime.erase (pck);
                    }
                }
            }
        }
    }
  //Imprimebuffers ();
}

bool
CustomApplicationBnodes::BuscaSEQEnTabla (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia)
{
  bool find = false;
  for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_A_Reenviar.begin ();
       it != m_Paquetes_A_Reenviar.end (); it++)
    {
      SecundariosDataTag SecTAg;
      it->m_packet->PeekPacketTag (SecTAg);
      if (SecTAg.GetSEQNumber () == SEQ && SecTAg.GetcopiaID () == IDCopia &&
          SecTAg.GetNodeId () == IDcreador)
        {
          find = true;
          break;
        }
    }
  return find;
}
bool
CustomApplicationBnodes::Entregado (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia)
{
  bool find = false;
  for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_Recibidos.begin ();
       it != m_Paquetes_Recibidos.end (); it++)
    {
      SecundariosDataTag SecTAg;
      it->m_packet->PeekPacketTag (SecTAg);
      if (SecTAg.GetSEQNumber () == SEQ && SecTAg.GetcopiaID () == IDCopia &&
          SecTAg.GetNodeId () == IDcreador)
        {
          find = true;
          break;
        }
    }
  return find;
}
void
CustomApplicationBnodes::Imprimebuffers ()
{
  std::cout << "\t ********Paquetes en el buffer del Nodo " << GetNode ()->GetId () << "********"
            << std::endl;
  uint32_t bf = 0;
  for (std::list<ST_bufferOfCannelsB>::iterator buff = m_bufferB.begin (); buff != m_bufferB.end ();
       buff++)
    {
      uint32_t pk = 1;
      for (std::list<ST_PacketInBufferB>::iterator pck = buff->m_PacketAndTime.begin ();
           pck != buff->m_PacketAndTime.end ();
           pck++) //Este for itera sobre los paquetes del buffer visitado
        {
          std::cout << "Buffer " << bf << ": Paquete " << pk << ": | " << pck->m_Send << std::endl;
          pk++;
        }
      bf++;
    }

  std::cout << "\t ************************* Termina Impresion ***************************"
            << std::endl;
}
void
CustomApplicationBnodes::ImprimeTabla ()
{
  std::cout << "\t ********Paquetes recibidos en el Nodo*********" << GetNode ()->GetId ()
            << std::endl;
  for (std::list<ST_ReenviosB>::iterator it = m_Paquetes_A_Reenviar.begin ();
       it != m_Paquetes_A_Reenviar.end (); it++)
    {
      SecundariosDataTag SecTAg;
      it->m_packet->PeekPacketTag (SecTAg);
      std::cout << "\t SEQnumber: " << SecTAg.GetSEQNumber () << std::endl;
    }
}

Time
CustomApplicationBnodes::GetBroadcastInterval ()
{
  return m_broadcast_time;
}

void
CustomApplicationBnodes::iniciaCanales ()
{
  for (u_int32_t i = 0; i < m_n_channels; i++)
    {
      m_Canales_Para_Utilizar.push_back (i);
    }
}
std::string
CustomApplicationBnodes::operacionORString (std::string str1, std::string str2)
{
  for (uint32_t i = 0; i < str1.length (); i++)
    {
      if (str2[i] == '1')
        {
          str1[i] = '1';
        }
    }
  return str1;
}
void
CustomApplicationBnodes::CanalesDisponibles ()
{
  std::string canales = "";
  for (uint32_t i = 0; i < m_n_channels; i++)
    {
      canales = canales + std::to_string (0);
    }
  m_Canales_Para_Utilizar.clear ();
  for (std::list<ST_CanalesB>::iterator it = m_Canales_disponibles.begin ();
       it != m_Canales_disponibles.end (); it++)
    {
      canales = operacionORString (canales, it->m_chanels);
    }
  //std::bitset<5> x (canales);
  //std::string cadena = x.to_string ();
  //std::string disp = std::string (cadena.rbegin (), cadena.rend ());
  //std::cout << "Canales disponibles: " << disp << " No. Primarios " << m_Canales_disponibles.size ()
  //         << std::endl;
  for (uint32_t i = 0; i < m_n_channels; i++)
    {
      if (canales[i] == '0')
        {
          m_Canales_Para_Utilizar.push_back (i);
        }
    }
  //Simulator::Schedule (MilliSeconds (200), &CustomApplicationBnodes::CanalesDisponibles, this);
}

bool
CustomApplicationBnodes::VerificaCanal (uint32_t ch)
{
  std::string canales = "";
  for (uint32_t i = 0; i < m_n_channels; i++)
    {
      canales = canales + std::to_string (0);
    }
  bool find = false;
  if (m_Canales_disponibles.size () != 0)
    {
      for (std::list<ST_CanalesB>::iterator it = m_Canales_disponibles.begin ();
           it != m_Canales_disponibles.end (); it++)
        {
          canales = operacionORString (canales, it->m_chanels);
        }
    }

  //std::bitset<64> x (canales);
  //std::string cadena = x.to_string ();
  //std::string disp = std::string (cadena.rbegin (), cadena.rend ());

  if (canales[ch] == '0')
    {
      find = true;
    }
  return find;
}

bool
CustomApplicationBnodes::BuscaCanalesID (std::string ch, uint32_t ID, Time tim)
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
      m_Canales_disponibles.push_back (Nch);
    }
  return find;
}
void
CustomApplicationBnodes::CreaBuffersCanales ()
{

  for (uint32_t i = 0; i < m_n_channels + 2; i++) //dos mas 1 por los primarios y otro para el sink
    {
      ST_bufferOfCannelsB newBufferCH;
      newBufferCH.m_visitado_Enviar = false;
      newBufferCH.m_visitado_Guardar = false;
      m_bufferB.push_back (newBufferCH);
    }
}
void
CustomApplicationBnodes::CheckBuffer ()
{
  /*std::cout << "Aqui me quedo CheckBuffer: " << Now ().GetSeconds ()
            << " | Tbuff= " << (m_broadcast_time).GetSeconds () << " | Nodo "
            << GetNode ()->GetId () << std::endl;*/

  if (BuscaPaquete ())
    {
      //std::cout << "Aqui me quedo CheckBuffer " << Now ().GetSeconds () << std::endl;

      ReadPacketOnBuffer ();
      Simulator::Schedule (Seconds ((8.0 * 1024.0) / (m_mode.GetDataRate (20))),
                           &CustomApplicationBnodes::CheckBuffer, this);
    }
  else
    {
      BroadcastInformation ();
      //ReadPacketOnBuffer ();
      // std::cout << "Aqui me quedo CheckBuffer2" << std::endl;
      //Simulator::Schedule (m_broadcast_time, &CustomApplicationBnodes::CheckBuffer, this);
    }
  //Imprimebuffers();
  // Simulator::Schedule (m_broadcast_time + Seconds ((8.0 * 10000.0) / (m_mode.GetDataRate (10))),
  //                     &CustomApplicationBnodes::CheckBuffer, this);
  //std::cout << "Aqui me quedo CheckBuffer2##" << std::endl;
}

bool
CustomApplicationBnodes::VerificaVisitados_Guardar ()
{
  bool find = true;
  uint32_t buff = 0;
  for (std::list<ST_bufferOfCannelsB>::iterator it = m_bufferB.begin (); it != m_bufferB.end ();
       it++)
    {
      if ((*it).m_visitado_Guardar == false && buff < m_n_channels)
        { //si se encuentra un false en algun buffer es por que no ha sido visitado, por lo tanto aun no se debe reiniciar el orden de visita
          /* std::cout << "Buffer: " << buff << "El estado del buffer es: " << (*it).m_visitado
                    << std::endl;*/
          find = false;
          break;
        }
      buff++;
    }
  //std::cout << "Aqui me quedo VerificaVisitados() " << std::to_string (Now ().GetSeconds ())
  //         << std::endl;
  return find;
}
bool
CustomApplicationBnodes::VerificaVisitados_Enviar ()
{
  bool find = true;
  uint32_t buff = 0;
  for (std::list<ST_bufferOfCannelsB>::iterator it = m_bufferB.begin (); it != m_bufferB.end ();
       it++)
    {
      if ((*it).m_visitado_Enviar == false && buff < m_n_channels)
        { //si se encuentra un false en algun buffer es por que no ha sido visitado, por lo tanto aun no se debe reiniciar el orden de visita
          /* std::cout << "Buffer: " << buff << "El estado del buffer es: " << (*it).m_visitado
                    << std::endl;*/
          find = false;
          break;
        }
      buff++;
    }
  //std::cout << "Aqui me quedo VerificaVisitados() " << std::to_string (Now ().GetSeconds ())
  //         << std::endl;
  return find;
}
void
CustomApplicationBnodes::ReiniciaVisitados_Guardar ()
{
  for (std::list<ST_bufferOfCannelsB>::iterator it = m_bufferB.begin (); it != m_bufferB.end ();
       it++)
    {
      (*it).m_visitado_Guardar = false;
    }
  // std::cout << "Aqui me quedo Reinicia : " << std::to_string (Now ().GetSeconds ()) << std::endl;
}
void
CustomApplicationBnodes::ReiniciaVisitados_Enviar ()
{
  for (std::list<ST_bufferOfCannelsB>::iterator it = m_bufferB.begin (); it != m_bufferB.end ();
       it++)
    {
      (*it).m_visitado_Enviar = false;
    }
  // std::cout << "Aqui me quedo Reinicia : " << std::to_string (Now ().GetSeconds ()) << std::endl;
}

} // namespace ns3
