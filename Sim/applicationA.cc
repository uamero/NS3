#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "applicationA.h"
#include "SecundariosTag.h"
#include "SinkTag.h"
#include "TagPrimarios.h"
#include "ns3/drop-tail-queue.h"

#include <bitset>
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

CustomApplication::CustomApplication () : m_sendEvent ()
{
  m_broadcast_time = Seconds (20); //every 1s
  m_packetSize = 512; //1000 bytes
  m_Tiempo_de_reenvio = Seconds (20); //Tiempo para reenviar los paquetes
  m_time_limit = Seconds (5); //Tiempo limite para los nodos vecinos
  //m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
  m_mode = WifiMode ("OfdmRate54Mbps");
  m_n_channels = 8;
  m_satisfaccionL = 1;
  m_satisfaccionG = 1; //partimos de un sistema satisfecho
  m_retardo_acumulado = 0;
  m_collissions = 0;
  m_termina = false;

  //iniciaCanales();
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
          // break;
        }
    }
  if (m_wifiDevice)
    {
      //Let's create a bit of randomness with the first broadcast packet time to avoid collision
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      Time random_offset = MicroSeconds (rand->GetValue (10, 200));
      m_sendEvent = Simulator::Schedule (Seconds (.2) + random_offset,
                                         &CustomApplication::BroadcastInformation, this);
      //Simulator::Schedule (Seconds (.2) + random_offset, &CustomApplication::ReenviaPaquete, this);
      //Simulator::Schedule (m_broadcast_time + random_offset,
      //                    &CustomApplication::BroadcastInformation, this);
    }
  else
    {
      NS_FATAL_ERROR ("There's no WifiNetDevice in your node");
    }
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  Time random_offset = MicroSeconds (rand->GetValue (50, 200));
  //We will periodically (every 1 second) check the list of neighbors, and remove old ones (older than 5 seconds)
  //Simulator::Schedule (Seconds (1), &CustomApplication::RemoveOldNeighbors, this);
  //Simulator::Schedule (Seconds (.3) + random_offset, &CustomApplication::SendPacket, this);
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
/*void
CustomApplication::SetMAxtime (Time Maxtime)
{
  m_Max_Time_To_Stop = Maxtime;
}
Time
CustomApplication::GetMAxtime ()
{
  return m_Max_Time_To_Stop;
}*/
void
CustomApplication::BroadcastInformation ()
{
  NS_LOG_FUNCTION (this);

  //std::cout << "A El canal por donde envio es ch: " << std::to_string(ch) << std::endl;
  //std::cout<<"ID Device "<<m_wifiDevice->GetIfIndex ()<< " n: "<<GetNode()->GetNDevices()<<std::endl;
  //if (m_wifiDevice->GetIfIndex () != 0)
  //{
  //std::cout << "AppA::BroadcastInformation1:" << Now ().GetSeconds () << std::endl;
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
      if (m_termina)
        break;
      Time Ultimo_Envio = Now () - it->Tiempo_ultimo_envio;

      if (!it->Estado && m_Canales_Para_Utilizar.size () != 0)
        {

          if (it->NumeroDeEnvios == 0)
            {

              it->NumeroDeEnvios += 1;

              for (std::list<uint32_t>::iterator _it = m_Canales_Para_Utilizar.begin ();
                   _it != m_Canales_Para_Utilizar.end (); _it++)
                {
                  //std::cout << "AppA::BroadcastInformation1" << std::endl;
                  it->Tiempo_primer_envio = Now ();
                  std::string ruta = "";
                  /*Ptr<Packet> packet =
                      Create<Packet> ((uint8_t *) msgx.str ().c_str (), packetSize);*/
                  //std::cout << "Packet 1 enviado:" << ruta <<  std::endl;
                  // Ptr<Packet> packet = Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());
                  Ptr<Packet> packet = Create<Packet> (m_packetSize);
                  // std::cout << "Aqui me quedoA1:" <<  std::endl;
                  SecundariosDataTag etiqueta;
                  etiqueta.SetNodeId (GetNode ()->GetId ());
                  etiqueta.SetNodeIdPrev (GetNode ()->GetId ());
                  etiqueta.SetSEQnumber (it->numeroSEQ);
                  etiqueta.SetChanels (*_it);
                  etiqueta.setCopiaID (it->m_IDcopia);
                  etiqueta.SetSL (1);
                  //etiqueta.SetSL (rand->GetValue (0, 1));
                  etiqueta.SetBufferRoute ((uint8_t *) ruta.c_str ());
                  etiqueta.SetSizeBufferRoute (ruta.length ());
                  //std::cout << "Aqui me quedoA2:" << GetNode()->GetId()<<  std::endl;
                  /*CustomDataTag tag;
                  // El timestamp se configura dentro del constructor del tag
                  tag.SetTypeOfpacket (0);
                  tag.SetNodeId (GetNode ()->GetId ());
                  tag.CopySEQNumber (it->numeroSEQ);
                  tag.SetChanels (*_it);

                  packet->AddPacketTag (tag);
                  */
                  // etiqueta.Print (std::cout);
                  packet->AddPacketTag (etiqueta);
                  //SecundariosDataTag Prueba;
                  //packet->PeekPacketTag (Prueba);
                  //Prueba.Print (std::cout);
                  //std::string ruta2 =
                  //    std::string (Prueba.GetBufferRoute (),
                  //                 Prueba.GetBufferRoute () + Prueba.GetSizeBufferRoute ());
                  // std::cout << "Aqui me quedo en el buffer: " << Now ().GetSeconds () << std::endl;
                  uint32_t NIC = 0;
                  for (std::list<ST_bufferOfCannelsA>::iterator buffer = m_bufferA.begin ();
                       buffer != m_bufferA.end (); buffer++)
                    { //se itera por los canales y se guarda el paquete en el buffer correspondiente
                      if (NIC == *_it)
                        { // se localiza el buffer en el que se guardara el paquete
                          // std::cout << Now ().GetSeconds () << " Nodo A " << GetNode ()->GetId ()
                          //          << ": si guardo paquete " << std::endl;
                          ST_PacketInBufferA newPacket;
                          newPacket.m_packet = packet->Copy ();
                          newPacket.m_TimeTosavedOnBuffer = Now ();
                          newPacket.m_Send = true; //se coloca en true para enviar el paquete
                          buffer->m_PacketAndTime.push_back (newPacket);
                          m_sendEvent = Simulator::Schedule (
                              Seconds ((double) (8 * 1024) / (m_mode.GetDataRate (20))),
                              &CustomApplication::CheckBuffer, this);
                          break;
                        }
                      NIC++;
                    }

                  //std::cout << "SEQQQ en AAAAA"<<etiqueta.GetSEQNumber() <<std::endl;
                  //std::cout<<"channel ->>>>>>>>>>>>>> "<<*_it <<std::endl;
                  //std::cout<<" Me quedo aqui #############################%"<<std::to_string(GetNode()->GetNDevices())<<" El canal es: "<<std::to_string(*_it)<<std::endl;//aqui esta el problema investigar el porque y arreglarlo
                  //m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (*_it));
                  //std::cout<<" Me quedo aqui #############################/"<<" El canal es: "<<std::to_string(*_it)<<std::endl;
                  //m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
                  // std::cout<<"Nodo: "<<GetNode()->GetId()<<" Genero primer paquete correctamente A "<< Now().GetSeconds() << std::endl;
                  //std::cout << "AppA::BroadcastInformation2" << std::endl;
                  break;
                }
            }
          else if (Ultimo_Envio >= m_Tiempo_de_reenvio) //
            {
              //std::string ruta = std::to_string (GetNode ()->GetId ());
              //std::cout << "AppA::BroadcastInformation11" << std::endl;
              it->NumeroDeEnvios += 1;
              it->Tiempo_ultimo_envio = Now ();
              /*std::cout << "AppA1>> m_n_channels = " << m_n_channels
                        << " Nodo: " << GetNode ()->GetId () << " Time: " << Now ().GetSeconds ()
                        << std::endl;
              */
              for (std::list<uint32_t>::iterator _it = m_Canales_Para_Utilizar.begin ();
                   _it != m_Canales_Para_Utilizar.end (); _it++)
                {

                  std::string ruta = std::to_string (GetNode ()->GetId ());
                  //Ptr<Packet> packet = Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());
                  Ptr<Packet> packet = Create<Packet> (m_packetSize);
                  //Ptr<Packet> packet = Create<Packet> (m_packetSize);
                  //std::cout << "Packet 2 enviado:" << ruta <<  std::endl;
                  SecundariosDataTag etiqueta;
                  etiqueta.SetNodeId (GetNode ()->GetId ());
                  etiqueta.SetNodeIdPrev (GetNode ()->GetId ());
                  etiqueta.SetSEQnumber (it->numeroSEQ);
                  etiqueta.SetChanels (*_it);
                  etiqueta.SetTimestamp (Now () - it->Tiempo_primer_envio);
                  etiqueta.setCopiaID (it->m_IDcopia);
                  etiqueta.SetSL (1);
                  //etiqueta.SetSL (rand->GetValue (0, 1));
                  etiqueta.SetBufferRoute ((uint8_t *) ruta.c_str ());
                  etiqueta.SetSizeBufferRoute (ruta.length ());
                  /*
                  CustomDataTag tag;
                  tag.SetTypeOfpacket (1);
                  tag.SetNodeId (GetNode ()->GetId ());
                  tag.CopySEQNumber (it->numeroSEQ);
                  tag.SetChanels (*_it);
                  */
                  packet->AddPacketTag (etiqueta);
                  uint32_t NIC = 0;
                  for (std::list<ST_bufferOfCannelsA>::iterator buffer = m_bufferA.begin ();
                       buffer != m_bufferA.end (); buffer++)
                    { //se itera por los canales y se guarda el paquete en el buffer correspondiente
                      if (NIC == *_it)
                        { // se localiza el buffer en el que se guardara el paquete
                          // std::cout << Now ().GetSeconds () << " Nodo A " << GetNode ()->GetId ()
                          //           << ": guardo paquete2 " << std::endl;
                          ST_PacketInBufferA newPacket;
                          newPacket.m_packet = packet->Copy ();
                          newPacket.m_TimeTosavedOnBuffer = Now ();
                          newPacket.m_Send = true; //se coloca en true para enviar el paquete
                          buffer->m_PacketAndTime.push_back (newPacket);
                          m_sendEvent = Simulator::Schedule (
                              Seconds ((double) (8 * 1024) / (m_mode.GetDataRate (20))),
                              &CustomApplication::CheckBuffer, this);
                          break;
                        }
                      NIC++;
                    }

                  // std::cout << "AppA::BroadcastInformation22" << std::endl;
                  //std::cout<<"channel ->>>>>>>>>>>>>> "<<*_it <<std::endl;
                  // m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (*_it));
                  //m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
                  // std::cout<<"Nodo: "<<GetNode()->GetId()<<" Genero nuevo paquete correctamente A "<< Now().GetSeconds() << std::endl;
                  break;
                }
            }
          break;
        }
      /* else if (m_Paquetes_A_Reenviar.size () != 0)
        {

          std::list<ST_Reenvios>::iterator _it = GetReenvio ();

          for (std::list<uint32_t>::iterator __it = m_Canales_Para_Utilizar.begin ();
               __it != m_Canales_Para_Utilizar.end (); __it++)
            {
              //Aqui el proceso es distinto ya que el paquete a reenviar puede venir de un id  distinto por lo tanto en la tabla de paquetes a reenviar hay que
              //colocar una variable que alamcene el String que viene el paquete y asi poderlo obtener en el reenvio

              std::string ruta = _it->ruta + "," + std::to_string (GetNode ()->GetId ());
              Ptr<Packet> packet = Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());
              //Ptr<Packet> packet = Create<Packet> (_it->Tam_Paquete);
              CustomDataTag tag;
              tag.SetNodeId (_it->ID_Creador);
              tag.CopySEQNumber (_it->numeroSEQ);
              tag.SetTimestamp (_it->Tiempo_ultimo_envio);
              tag.SetTypeOfpacket (_it->tipo_de_paquete);
              tag.SetChanels (*__it);
              
              packet->AddPacketTag (tag);
              //std::cout<<"channel ->>>>>>>>>>>>>> "<<*__it <<std::endl;
              m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (*__it));
              m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
            }
        }*/
    }
  bool termina = true;
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
      if (!it->Estado)
        {
          termina = false;
          break;
        }
    }
  if (termina)
    {
      m_termina = true;
      if (m_sendEvent.IsRunning ())
        {
          Simulator::Cancel (m_sendEvent);
        }
    }
  else
    {
      m_sendEvent =
          Simulator::Schedule (m_broadcast_time, &CustomApplication::BroadcastInformation, this);
    }
  // std::cout << "Aqui me quedo 2" << std::endl;
  //}
  // std::cout << "Nodos A " << GetNode ()->GetId () << " : " << GetNode ()->GetNDevices ()
  //          << " n_chanels: " << std::to_string (m_n_channels) << std::endl;
  //std::cout << m_n_channels << std::endl;
  //Broadcast the packet as WSMP (0x88dc)
  //Schedule next broadcast

  //std::cout<<Now().GetSeconds()<<std::endl;
  m_simulation_time = Now ();
  //std::cout << "AppA::BroadcastInformation2:"  << std::endl;
  /*if (VerificaFinDeSimulacion () || Now () >= Seconds(1000))
    {
      //std::cout<<"Tiempo de simulacion: "<< Now().GetSeconds() <<"Seg."<<std::endl;

      Simulator::Stop ();
    }*/
}

bool
CustomApplication::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,
                                  uint16_t protocol, const Address &sender)
{
  /**Hay dos tipos de paquetes que se pueden recibir
   * 1.- Un paquete que fue enviado por este nodo 
   * 2.- Un paquete que fue enviado por el Sink*/
  if (m_termina)
    {
      return true;
    }

  uint32_t NIC = 0;
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++)
    {

      if (NIC == device->GetIfIndex ())
        {
          //std::cout << "Aqui me quedo 3######################################> "
          //        << device->GetIfIndex () << " | " << NIC << std::endl;
          SecundariosDataTag sec;
          ST_PacketInBufferA newPacket;
          newPacket.m_packet = packet->Copy ();
          newPacket.m_TimeTosavedOnBuffer = Now ();
          newPacket.m_Send =
              false; //este paquete aun debe ser leido por el nodo para ver si es reenviado o desechado.
          if (packet->PeekPacketTag (sec))
            {
              break;
            }
          else
            {
              it->m_PacketAndTime.push_back (newPacket);
              m_sendEvent =
                  Simulator::Schedule (Seconds ((double) (8 * 1024) / (m_mode.GetDataRate (20))),
                                       &CustomApplication::CheckBuffer, this);
            }
          /* if (!VerificaCanal (sec.GetChanels ()) && NIC < m_n_channels)
            { //si el canal no esta disponible es una colision
              m_collissions++;
              break;
            }
          

          Mac48Address source = Mac48Address::ConvertFrom (sender);
          ST_VecinosA NV;
          NV.NodoID = sec.GetNodeIdPrev ();
          NV.SL_canal = sec.GetSL ();
          NV.MacaddressNB = source;
          NV.canal = NIC;
          //std::cout << "Aqui me quedo 4 #######################  = " << NV.NodoID << std::endl;

          UpdateNeighbor (NV);
          //PrintNeighbors ();*/
          break;
        }
      NIC++;
    }

  //std::cout << "Aqui me quedo 4 #######################333" << std::endl;
  //m_BufferA.push_back (packet->Copy());

  /*  std::cout << "Se han copiado: " << m_BufferA.size () << " paquetes en el buffer" << std::endl;

  for (std::list<Ptr<Packet>>::iterator it = m_BufferA.begin (); it != m_BufferA.end (); it++)
    {
      uint8_t *buff = new uint8_t[(*it)->GetSize()];
      (*it)->CopyData (buff, packet->GetSize ());
      std::string sms = std::string (buff, buff + (*it)->GetSize());
      std::cout << "El paquete tiene: " << sms << " y hay "<<(*it)->GetSize()<<"paquetes en el buffer" << std::endl;
    }
  */

  NS_LOG_FUNCTION (device << packet << protocol << sender);

  /*
        Packets received here only have Application data, no WifiMacHeader. 
        We created packets with 1000 bytes payload, so we'll get 1000 bytes of payload.
    */

  NS_LOG_INFO ("ReceivePacket() : Node " << GetNode ()->GetId () << " : Received a packet from "
                                         << sender << " Size:" << packet->GetSize ());

  return true;
}

void
CustomApplication::SendPacket ()
{

  //if(VerificaVisitados()) {ReiniciaVisitados();}
  uint32_t NIC = 0;
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++) //Se itera sobre cada buffer de canal para identificar si hay paquetes a enviar
    {
      Ptr<Packet> packet;
      Time TimeInThisNode;
      SecundariosDataTag SecundariosTag;

      if (it->m_PacketAndTime.size () != 0 &&
          !(*it)
               .m_visitado_Enviar) // Se evalua si el buffer visitado esta vacio o bien si no ha sido visitado
        {
          if (NIC == m_n_channels)
            {
              ReiniciaVisitados_Enviar ();
              break;
            }
          (*it).m_visitado_Enviar = true;
          //std::cout << "Aqui me quedo 1" << std::endl;
          bool find = false;
          for (std::list<ST_PacketInBufferA>::iterator pck = it->m_PacketAndTime.begin ();
               pck != it->m_PacketAndTime.end ();
               pck++) //Este for itera sobre los paquetes del buffer visitado
            { // este for es para obtener solo un paquete dentro del buffer en el canal n
              if (pck->m_Send)
                {
                  find = true;
                  packet = pck->m_packet->Copy ();
                  TimeInThisNode = Now () - pck->m_TimeTosavedOnBuffer;
                  //std::cout << "El retardo en el nodo B" << GetNode ()->GetId ()
                  //             << " Es: " << TimeInThisNode.GetMilliSeconds()<< std::endl;

                  //std::cout << "El paquete tiene un : " << pck->m_Send << std::endl;
                  it->m_PacketAndTime.erase (pck);
                  //std::cout << "AppA::SendPacket2" << std::endl;
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
                  //std::cout << "Aqui me quedo 3" << std::endl;
                  //std::cout << "AppA::SendPacket1" << std::endl;
                  /*uint8_t *buffer = new uint8_t[packet->GetSize ()];
                  packet->CopyData (buffer, packet->GetSize ());
                  std::string ruta = std::string (buffer, buffer + packet->GetSize ());*/
                  /*Ptr<Packet> PacketToReSend =
                      Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());*/

                  SecundariosTag.SetTimestamp (TimeInThisNode + SecundariosTag.GetTimestamp ());
                  /*std::cout << Now ().GetSeconds () << " | "
                            << "Nodo: " << GetNode ()->GetId ()
                            << " El retardo de este paquete enviado es: "
                            << SecundariosTag.GetTimestamp ().GetSeconds ()
                            << " Disponibilidad: " << m_Canales_Para_Utilizar.size () 
                            <<"SEQN: "<< SecundariosTag.GetSEQNumber()<< std::endl;*/
                  //PacketToReSend->AddPacketTag (SecundariosTag);
                  //std::cout<<"Aqui me quedoA1"<<std::endl;
                  packet->ReplacePacketTag (SecundariosTag);
                  //std::cout << "Aqui me quedo A : " << SecundariosTag.GetSerializedSize ();
                  //<< std::endl;
                  //std::cout << "AppA::SendPacket2" << std::endl;
                  if (Entregado (SecundariosTag.GetSEQNumber (), SecundariosTag.GetNodeId (),
                                 SecundariosTag.GetcopiaID ()))
                    {
                      break; //Si el paquete ya está entregado no se envia el paquete
                    }

                  m_wifiDevice = DynamicCast<WifiNetDevice> (
                      GetNode ()->GetDevice (SecundariosTag.GetChanels ()));
                  m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (),
                                      0x88dc); //aqui se hace el broadcast en un canal
                  /*std::cout << Now ().GetSeconds () << " | "
                            << "Nodo: " << GetNode ()->GetId ()
                            << " El retardo de este paquete enviado es: "
                            << SecundariosTag.GetTimestamp ().GetSeconds ()
                            << " Disponibilidad: " << m_Canales_Para_Utilizar.size ()
                            << "SEQN: " << SecundariosTag.GetSEQNumber () << std::endl;*/
                  // std::cout << "Aqui me quedo Fin de SendPacket: " << GetNode ()->GetId ()
                  //          << std::endl;
                  //std::cout << "AppA::SendPacket2" << std::endl;
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
}
void
CustomApplication::ReadPacketOnBuffer ()
{
  /*std::cout << Now ().GetSeconds ()
                            << " 11Nodo A: " << m_Canales_Para_Utilizar.size () << std::endl;*/
  uint32_t NIC = 0;
  //std::cout << "Aqui me quedoA1" << std::endl;

  std::list<ST_bufferOfCannelsA>::iterator SinkAndPrimariosIterator = m_bufferA.end ();
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++) //Se itera sobre cada buffer de canal para identificar si hay paquetes a enviar
    {
      Ptr<Packet> packet;
      Time TimeInThisNode;
      SecundariosDataTag SecundariosTag;
      PrimariosDataTag PrimariosTag;
      bool send;
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      if (NIC == m_n_channels)
        {
          SinkAndPrimariosIterator = it;
          break;
        }
      if (it->m_PacketAndTime.size () != 0 &&
          !(*it)
               .m_visitado_Guardar) // Se evalua si el buffer visitado esta vacio o bien si no ha sido visitado
        {

          (*it).m_visitado_Guardar = true;
          /*std::cout << "Node " << GetNode ()->GetId () << " | BufferA: " << NIC
                    << " Size: " << it->m_PacketAndTime.size () << " Time: " << Now ().GetSeconds ()
                    << std::endl;*/
          for (std::list<ST_PacketInBufferA>::iterator pck = it->m_PacketAndTime.begin ();
               pck != it->m_PacketAndTime.end ();
               pck++) //Este for itera sobre los paquetes del buffer visitado
            { // este for es para obtener solo un paquete dentro del buffer en el canal n
              packet = pck->m_packet;
              TimeInThisNode = Now () - pck->m_TimeTosavedOnBuffer;
              send = pck->m_Send;
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
                { // se verifica si la interfaz por la que llego el paquete no es de los usuarios primarios o del Sink ademas de verificar si el canal por el que se recibe esta libre
                  //Imprimebuffers();
                  break;
                  uint8_t *buffer = new uint8_t[packet->GetSize ()];

                  packet->CopyData (buffer, packet->GetSize ());
                  delete buffer;
                  /*if (send)
                    { // se verifica si el paquete ya está listo para ser enviado o se debe leer
                      std::string ruta = std::string (buffer, buffer + packet->GetSize ());
                      Ptr<Packet> PacketToReSend =
                          Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());

                      SecundariosTag.SetTimestamp (TimeInThisNode + SecundariosTag.GetTimestamp ());
                      // std::cout << "El retardo de este paquete enviado es: "
                      //         << SecundariosTag.GetTimestamp ().GetSeconds ()
                      //        << " Disponibilidad: " << m_Canales_Para_Utilizar.size ()
                      //        << std::endl;
                      PacketToReSend->AddPacketTag (SecundariosTag);
                      if (Entregado (SecundariosTag.GetSEQNumber ()))
                        {
                          break; //Si el paquete ya está entregado no se envia el paquete
                        }
                      if (!BuscaSEQEnTabla (SecundariosTag.GetSEQNumber ()))
                        { // Se busca si este paquete no se ha guardado en el nodo para ser reenviado
                          m_retardo_acumulado += TimeInThisNode.GetMilliSeconds ();
                          //std::cout << "Si me meto aqui AAAAAA" << std::endl;
                          Guarda_Info_Paquete (
                              PacketToReSend->Copy (),
                              TimeInThisNode); //En este punto se almacena en memoria el paquete
                        }

                      m_wifiDevice = DynamicCast<WifiNetDevice> (
                          GetNode ()->GetDevice (SecundariosTag.GetChanels ()));
                      m_wifiDevice->Send (PacketToReSend, Mac48Address::GetBroadcast (),
                                          0x88dc); //aqui se hace el broadcas en un canal
                      //m_wifiDevice->Send (PacketToReSend, GetNextHop (NIC), 0x88dc);
                    }*/
                  if (!send) //esta parte no se ejecuta en este tipo de nodo, ya que solo se envian paquetes que se crean aqui
                    { //si el paquete debe ser leido se identifica si se reenviara o bien solo se guardara en buffer
                      //std::cout << "Si me meto aqui AAAAAA" << std::endl;
                      std::string ruta = std::string (buffer, buffer + packet->GetSize ()) + "," +
                                         std::to_string (GetNode ()->GetId ());
                      //std::cout << "AppA::ReadPacketOnBuffer1 " << std::endl;
                      for (std::list<uint32_t>::iterator FreeChIt =
                               m_Canales_Para_Utilizar.begin ();
                           FreeChIt != m_Canales_Para_Utilizar.end (); FreeChIt++)
                        {
                          Ptr<Packet> PacketToReSend =
                              Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());

                          SecundariosTag.SetChanels ((*FreeChIt));
                          SecundariosTag.SetTimestamp (TimeInThisNode +
                                                       SecundariosTag.GetTimestamp ());
                          SecundariosTag.SetSL (1);
                          SecundariosTag.SetNodeIdPrev (GetNode ()->GetId ());
                          PacketToReSend->AddPacketTag (SecundariosTag);
                          // std::cout << "AppA::ReadPacketOnBuffer2 " << std::endl;

                          if (Entregado (SecundariosTag.GetSEQNumber (),
                                         SecundariosTag.GetcopiaID (),
                                         SecundariosTag.GetcopiaID ()) ||
                              BuscaSEQEnTabla (SecundariosTag.GetSEQNumber (),
                                               SecundariosTag.GetNodeId (),
                                               SecundariosTag.GetcopiaID ()))
                            {
                              break; //Si el paquete ya está entregado no se guarda el paquete
                            }
                          if (!BuscaSEQEnTabla (SecundariosTag.GetSEQNumber (),
                                                SecundariosTag.GetNodeId (),
                                                SecundariosTag.GetcopiaID ()))
                            { // Se busca si este paquete no se ha guardado en el nodo para ser reenviado
                              /*m_retardo_acumulado += TimeInThisNode.GetMilliSeconds ();
                              //std::cout << "Si me meto aqui AAAAAA" << std::endl;
                              Guarda_Info_Paquete (
                                  PacketToReSend->Copy (),
                                  TimeInThisNode); //En este punto se almacena en memoria el paquete*/
                            }
                          else
                            {
                              break;
                            }

                          //PacketToReSend->AddPacketTag (SecundariosTag);

                          uint32_t _NIC = 0;
                          for (std::list<ST_bufferOfCannelsA>::iterator buff = m_bufferA.begin ();
                               buff != m_bufferA.end (); buff++)
                            { //se itera por los canales y se guarda el paquete en el buffer correspondiente
                              if (_NIC == *FreeChIt)
                                { // se localiza el buffer en el que se guardara el paquete
                                  ST_PacketInBufferA newPacket;
                                  newPacket.m_packet = packet->Copy ();
                                  newPacket.m_TimeTosavedOnBuffer = Now ();
                                  newPacket.m_Send =
                                      true; //se coloca en true para enviar el paquete
                                  buff->m_PacketAndTime.push_back (newPacket);
                                  break;
                                }
                              else
                                {
                                  _NIC++;
                                }
                            }
                        }
                      /*Simulator::Schedule (Seconds ((8.0 * 1024.0) / (m_mode.GetDataRate (20))),
                                           &CustomApplication::CheckBuffer, this);*/
                      break; //este break rompe el for que itera sobre los buffer de los canales
                    }
                }
            }
        }
      else
        {
          (*it).m_visitado_Guardar = true;
        }
      NIC++; //es el device correspondiente
    }

  NIC = m_n_channels;
  for (std::list<ST_bufferOfCannelsA>::iterator it = SinkAndPrimariosIterator;
       it != m_bufferA.end ();
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
          for (std::list<ST_PacketInBufferA>::iterator pck = it->m_PacketAndTime.begin ();
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
                  //ImprimeTabla ();
                  //SinkTag.Print (std::cout << "NodoA" << GetNode ()->GetId () << ": ");
                  /* std::string ruta = std::string (SinkTag.GetBufferRoute (),
                                                  SinkTag.GetBufferRoute () +
                                                      SinkTag.GetSizeBufferRoute ());*/
                  /*std::cout << "NodA>> " << GetNode ()->GetId ()
                            << " Si confirmo la entrega con retardo: "
                            << SinkTag.GetTimestamp ().GetSeconds () << std::endl;
                  //std::cout << "Aqui me quedo1" << std::endl;*/

                  ConfirmaEntrega (SinkTag.GetSEQNumber (), SinkTag.GetTimestamp (),
                                   SinkTag.GetNodeId (), SinkTag.GetcopiaID (), SinkTag.GetSG ());
                  //BroadcastInformation ();
                  //std::cout << "Aqui me quedoA2" << std::endl;
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
                  /*std::cout << "Los canales disponibles en A n" << GetNode ()->GetId () << ": "
                            << canalesDP << " proviene de n" << PrimariosTag.GetnodeID ()
                            << "tiempo: " << Now ().GetSeconds () << std::endl;*/

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
  /*std::cout << "AppA3>> m_n_channels = " << m_n_channels << " Nodo: " << GetNode ()->GetId ()
            << " Time: " << Now ().GetSeconds () << std::endl;*/
  if (VerificaVisitados_Guardar ()) // se verifica si todos los buffers fueron visitados
    {
      ReiniciaVisitados_Guardar (); //si ya fueron visitados se reinician de nuevo las visitas al primer canal (es de forma circular)
    }

  //std::cout << "Aqui me quedoA2" << std::endl;

  //Simulator::Schedule (Seconds (10.0), &CustomApplication::ReadPacketOnBuffer, this);
}

Mac48Address
CustomApplication::GetNextHop (uint32_t channel)
{
  //este metodo obtiene el mejor canal para enviar el paquete enviado
  for (std::list<ST_VecinosA>::iterator it = m_vecinos_list.begin (); it != m_vecinos_list.end ();
       it++)
    {
      if (it->canal == channel)
        {
          return it->MacaddressNB;
        }
    }
  //en caso de que no haya un buen canal se envia en broadcast
  return Mac48Address::GetBroadcast ();
}

void
CustomApplication::UpdateNeighbor (ST_VecinosA neighbor)
{
  bool found = false;
  //Go over all neighbors, find one matching the address, and updates its 'last_beacon' time.
  for (std::list<ST_VecinosA>::iterator it = m_vecinos_list.begin (); it != m_vecinos_list.end ();
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
                              << " is adding a neighbor with MAC=" << neighbor.MacaddressNB
                              << END_CODE);
      ST_VecinosA new_n;
      new_n.MacaddressNB = neighbor.MacaddressNB;
      new_n.canal = neighbor.canal;
      new_n.NodoID = neighbor.NodoID;
      new_n.SL_canal = neighbor.SL_canal;
      new_n.last_beacon = Now ();

      /*Se inserta ordenado en la tabla de vecinos con respecto a la SL*/
      if (m_vecinos_list.size () == 0)
        {
          //si no hay vecinos registrados
          m_vecinos_list.push_back (new_n);
        }
      else
        {
          for (std::list<ST_VecinosA>::iterator it = m_vecinos_list.begin ();
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
CustomApplication::PrintNeighbors ()
{
  std::cout << "Neighbor Info for Node: " << GetNode ()->GetId () << std::endl;
  for (std::list<ST_VecinosA>::iterator it = m_vecinos_list.begin (); it != m_vecinos_list.end ();
       it++)
    {
      std::cout << "\tSL: " << it->SL_canal << "\tID: " << it->NodoID
                << "\tMAC: " << it->MacaddressNB << "\tCanal: " << it->canal
                << "\tLast Contact: " << it->last_beacon.GetSeconds () << std::endl;
    }
}

void
CustomApplication::RemoveOldNeighbors ()
{
  //Go over the list of neighbors
  for (std::list<ST_VecinosA>::iterator it = m_vecinos_list.begin (); it != m_vecinos_list.end ();
       it++)
    {
      //Get the time passed since the last time we heard from a node
      Time last_contact = Now () - it->last_beacon;

      if (last_contact >=
          Seconds (
              5)) //if it has been more than 5 seconds, we will remove it. You can change this to whatever value you want.
        {
          NS_LOG_INFO (RED_CODE << Now () << " Node " << GetNode ()->GetId ()
                                << " is removing old neighbor " << it->MacaddressNB << END_CODE);
          //Remove an old entry from the table
          m_vecinos_list.erase (it);
          break;
        }
    }
  //Check the list again after 1 second.
  m_sendEvent = Simulator::Schedule (Seconds (1), &CustomApplication::RemoveOldNeighbors, this);
}

bool
CustomApplication::BuscaPaquete ()
{
  bool find = false;
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++)
    {

      if (it->m_PacketAndTime.size () != 0)
        {
          find = true;

          break;
        }
    }
  // std::cout << "Aqui me quedo BuscaPaquete(): " <<find<< std::endl;

  return find;
}
void
CustomApplication::Imprimebuffers ()
{
  std::cout << "\t ********Paquetes en el buffer del Nodo " << GetNode ()->GetId () << "********"
            << std::endl;
  uint32_t bf = 0;
  for (std::list<ST_bufferOfCannelsA>::iterator buff = m_bufferA.begin (); buff != m_bufferA.end ();
       buff++)
    {
      uint32_t pk = 1;
      for (std::list<ST_PacketInBufferA>::iterator pck = buff->m_PacketAndTime.begin ();
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
CustomApplication::CheckBuffer ()
{
  if (BuscaPaquete ())
    {
      /*std::cout << "Aqui me quedo CheckBuffer1" << std::endl;*/
      ReadPacketOnBuffer ();
      Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
      Time random_offset = MicroSeconds (rand->GetValue (10, 200));
      Simulator::Schedule (Seconds ((8.0 * 1024.0) / (m_mode.GetDataRate (20))) + random_offset,
                           &CustomApplication::SendPacket, this);
      /*Simulator::Schedule (Seconds ((double) (8 * 1000) / (m_mode.GetDataRate (20))),
                           &CustomApplication::CheckBuffer, this);*/
      m_sendEvent = Simulator::Schedule (Seconds ((8.0 * 1024.0) / (m_mode.GetDataRate (20))),
                                         &CustomApplication::CheckBuffer, this);
    }
  else
    {
      //Simulator::Schedule (m_broadcast_time, &CustomApplication::BroadcastInformation, this);
      if (!m_termina)
        {
          //BroadcastInformation ();
          m_sendEvent = Simulator::Schedule (m_broadcast_time,
                                             &CustomApplication::BroadcastInformation, this);
        }
    }

  //El data rate es de 6Mbps
  //std::cout << "Aqui me quedo CheckBuffer2##" << std::endl;
}
bool
CustomApplication::VerificaVisitados_Enviar ()
{
  bool find = true;
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++)
    {
      if ((*it).m_visitado_Enviar == false)
        { //si se encuentra un false en algun buffer es por que no ha sido visitado, por lo tanto aun no se debe reiniciar el orden de visita

          find = false;
          break;
        }
    }
  //std::cout << "Aqui me quedo VerificaVisitados() " << std::to_string (Now ().GetSeconds ())
  //         << std::endl;
  return find;
}
void
CustomApplication::ReiniciaVisitados_Enviar ()
{
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++)
    {
      (*it).m_visitado_Enviar = false;
    }
  // std::cout << "Aqui me quedo Reinicia : " << std::to_string (Now ().GetSeconds ()) << std::endl;
}
bool
CustomApplication::VerificaVisitados_Guardar ()
{
  bool find = true;
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++)
    {
      if ((*it).m_visitado_Guardar == false)
        { //si se encuentra un false en algun buffer es por que no ha sido visitado, por lo tanto aun no se debe reiniciar el orden de visita

          find = false;
          break;
        }
    }
  //std::cout << "Aqui me quedo VerificaVisitados() " << std::to_string (Now ().GetSeconds ())
  //         << std::endl;
  return find;
}
void
CustomApplication::ReiniciaVisitados_Guardar ()
{
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++)
    {
      (*it).m_visitado_Guardar = false;
    }
  // std::cout << "Aqui me quedo Reinicia : " << std::to_string (Now ().GetSeconds ()) << std::endl;
}

void
CustomApplication::IniciaTabla (uint32_t PQts_A_enviar, uint32_t ID)
{

  for (uint32_t i = 0; i < PQts_A_enviar; i++)
    {
      ST_Paquete_A_Enviar Paquete;

      Paquete.ID_Creador = ID;
      Paquete.numeroSEQ = i;
      Paquete.m_IDcopia = 0;
      Paquete.Tam_Paquete = m_packetSize;
      Paquete.Tiempo_ultimo_envio = Now ();
      Paquete.Tiempo_primer_envio = Now ();
      Paquete.Tiempo_de_recibo_envio = Seconds (0);
      Paquete.NumeroDeEnvios = 0;
      Paquete.SG = 0;
      Paquete.Estado = false;
      m_Tabla_paquetes_A_enviar.push_back (Paquete);
    }
  ST_Canales ch;
  ch.m_chanels = ""; //Todos lo canales disponibles
  ch.ID_Persive = ID;
  ch.Tiempo_ultima_actualizacion = Now ();
  //m_Canales_disponibles.push_back (ch);
  // std::cout << "Inicia tabla "<<m_Tabla_paquetes_A_enviar.size()<<std::endl;
}

bool
CustomApplication::Entregado (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia)
{
  bool find = false;
  for (std::list<ST_Reenvios>::iterator it = m_Paquetes_Recibidos.begin ();
       it != m_Paquetes_Recibidos.end (); it++)
    {
      SecundariosDataTag SecTAg;
      it->m_packet->PeekPacketTag (SecTAg);
      if (SecTAg.GetSEQNumber () == SEQ && SecTAg.GetNodeId () == IDcreador)
        {
          find = true;
          break;
        }
    }
  return find;
}
void
CustomApplication::ConfirmaEntrega (
    u_long SEQ, Time delay, uint32_t IDcreador, uint32_t CopiaID,
    double SG) //faltan mas datos para tener una buena entrega, tiempo, satisfaccion, etc
{

  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
      SecundariosDataTag SecTAg;
      ST_Reenvios entregado;
      //std::cout<<"Aqui me quedo1 Nodo: "<<it->ID_Creador<<" Copia:"<<it->m_IDcopia<<std::endl;

      if (it->numeroSEQ == SEQ && it->ID_Creador == IDcreador)
        {
          //std::cout<<"Aqui me quedo2"<<std::endl;
          Ptr<Packet> np = Create<Packet> (m_packetSize);
          SecTAg.SetSEQnumber (SEQ);
          SecTAg.SetNodeId (GetNode ()->GetId ());
          SecTAg.setCopiaID (CopiaID);
          SecTAg.SetSL (SG);
          np->AddPacketTag (SecTAg);
          entregado.retardo = Seconds (0);
          entregado.Tiempo_ultimo_envio = Now ();
          entregado.m_packet = np;
          //it->Tiempo_de_recibo_envio = Now () - it->Tiempo_ultimo_envio;
          it->SG = SG;
          it->Tiempo_de_recibo_envio = delay;
          it->Estado = true;
          m_Paquetes_Recibidos.push_back (entregado);
          //std::cout << "AppA1>> mentregado = " << it->Estado << " Nodo: " << GetNode ()->GetId ()
          //<< " Time: " << Now ().GetSeconds () << std::endl;
          it++;

          if (it == m_Tabla_paquetes_A_enviar.end ())
            {
              break;
            }
          else
            {

              it->Tiempo_ultimo_envio = Now ();
              break;
            }
        }
    }
  // std::cout << "AppA2>> m_n_channels = " << m_n_channels << " Nodo: " << GetNode ()->GetId ()
  //          << " Time: " << Now ().GetSeconds () << std::endl;
}

bool
CustomApplication::BuscaSEQEnTabla (u_long SEQ, uint32_t ID_creador, uint32_t CopiaID)
{
  bool find = false;

  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
      if (it->numeroSEQ == SEQ && it->ID_Creador == ID_creador)
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
                << "\t Tiempo de entrega: " << it->Tiempo_de_recibo_envio.GetSeconds () << " s"
                << "\t Numero de canales: " << m_n_channels << "\t ID Creador: " << it->ID_Creador
                << "\t ID Copia: " << it->m_IDcopia << std::endl;
    }
}
std::string
CustomApplication::ObtenDAtosNodo ()
{
  uint32_t cont = 0;
  std::string datos;
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
      datos = std::to_string (GetNode ()->GetId ()) + "," + std::to_string (cont + 1) + "," +
              std::to_string (it->Estado) + "," +
              std::to_string (it->Tiempo_de_recibo_envio.GetSeconds ()) + "," +
              std::to_string (m_broadcast_time.GetSeconds ()) + ",";
      cont++;
    }
  return datos;
}

void // Esta funcion es llamada desde el main
CustomApplication::iniciaCanales ()
{
  for (u_int32_t i = 0; i < m_n_channels; i++)
    {
      m_Canales_Para_Utilizar.push_back (i);
    }
}
std::string
CustomApplication::operacionORString (std::string str1, std::string str2)
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
CustomApplication::CanalesDisponibles ()
{

  std::string canales = "";
  for (uint32_t i = 0; i < m_n_channels; i++)
    {
      canales = canales + std::to_string (0);
    }

  m_Canales_Para_Utilizar.clear ();
  //std::cout<<"Holaaaaaaaaaaaa "<<m_Canales_disponibles.size()<<std::endl;

  for (std::list<ST_Canales>::iterator it = m_Canales_disponibles.begin ();
       it != m_Canales_disponibles.end (); it++)
    {
      canales = operacionORString (canales, it->m_chanels);
    }

  //std::bitset<64> x (canales);
  //std::string cadena = x.to_string ();
  //std::string disp = std::string (cadena.rbegin (), cadena.rend ());

  /*El bit menos significativo es el canal 0*/
  for (uint32_t i = 0; i < m_n_channels; i++)
    {
      if (canales[i] == '0')
        {
          m_Canales_Para_Utilizar.push_back (i);
        }
    }
  //std::cout << canales << "Se hace bienA: " << m_Canales_Para_Utilizar.size ()
  //         << "Time: " << Now ().GetSeconds () << std::endl;
  // std::cout << Now().GetSeconds() <<" 222Nodo A: " <<m_Canales_Para_Utilizar.size()<<" "<<canales<< std::endl;
}
bool
CustomApplication::VerificaCanal (uint32_t ch)
{
  std::string canales = "";
  for (uint32_t i = 0; i < m_n_channels; i++)
    {
      canales = canales + std::to_string (0);
    }
  bool find = false;
  for (std::list<ST_Canales>::iterator it = m_Canales_disponibles.begin ();
       it != m_Canales_disponibles.end (); it++)
    {
      canales = operacionORString (canales, it->m_chanels);
    }

  // std::bitset<64> x (canales);
  // std::string cadena = x.to_string ();
  //std::string disp = std::string (cadena.rbegin (), cadena.rend ());

  if (canales[ch] == '0')
    {
      find = true;
    }
  return find;
}

bool
CustomApplication::BuscaCanalesID (std::string ch, uint32_t ID, Time tim)
{

  bool find = false;
  for (std::list<ST_Canales>::iterator it = m_Canales_disponibles.begin ();
       it != m_Canales_disponibles.end (); it++)
    {
      if (it->ID_Persive == ID) //
        {
          it->m_chanels = ch;
          it->Tiempo_ultima_actualizacion = Now ();
          find = true;
          break;
        }
    }
  if (!find)
    {
      ST_Canales Nch;
      Nch.ID_Persive = ID;
      Nch.m_chanels = ch;
      Nch.Tiempo_ultima_actualizacion = Now ();
      m_Canales_disponibles.push_back (Nch);
    }
  return find;
}
void // Esta funcion es llamada desde el main
CustomApplication::CreaBuffersCanales ()
{

  for (uint32_t i = 0; i < m_n_channels + 2; i++) //dos mas 1 por los primarios y otro para el sink
    {
      ST_bufferOfCannelsA newBufferCH;
      newBufferCH.m_visitado_Enviar = false;
      newBufferCH.m_visitado_Guardar = false;
      m_bufferA.push_back (newBufferCH);
    }
}
void
CustomApplication::SetChannels (uint32_t channels)
{
  m_n_channels = channels;
}
} // namespace ns3
