#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "applicationSink.h"
#include "SinkTag.h"
#include "SecundariosTag.h"

#include <iostream>
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
  m_broadcast_time = Seconds (10); //every 100ms
  m_packetSize = 10; //10 bytes
  m_Tiempo_de_reenvio = Seconds (1); //Tiempo para reenviar los paquetes ACK
  m_time_limit = Seconds (5); //Tiempo limite para los nodos vecinos
  m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
  m_semilla = 0; // controla los numeros de secuencia
  m_n_channels = 8;
}
ApplicationSink::~ApplicationSink ()
{
}
void
ApplicationSink::StartApplication ()
{
  //Set A Receive callback
  NS_LOG_FUNCTION (this);
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

      for (std::list<ST_Reenvios_Sink>::iterator it = m_Tabla_paquetes_ACK.begin ();
           it != m_Tabla_paquetes_ACK.end (); it++)
        {
          Time Ultimo_Envio = Now () - it->Tiempo_ultimo_envio;
          //std::cout << "Ultimo envio: " << Ultimo_Envio.GetSeconds () << std::endl;
          if (it->NumeroDeEnvios < 1 && (Ultimo_Envio >= m_Tiempo_de_reenvio))
            {
              // std::cout << "Sink::BroadcastInformation1" << std::endl;
              m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (m_n_channels));
              it->NumeroDeEnvios += 1;
              m_wifiDevice->Send (it->m_packet, Mac48Address::GetBroadcast (), 0x88dc);
              it->Tiempo_ultimo_envio = Now ();
              //std::cout << "Envie paquete sink" << std::endl;
              //Simulator::Stop();
              //std::cout << "Sink::BroadcastInformation2" << std::endl;
              break;
            }
        }
      //ImprimeTabla ();
    }

  //Broadcast the packet as WSMP (0x88dc)
  //Schedule next broadcast
  Simulator::Schedule (m_broadcast_time, &ApplicationSink::BroadcastInformation, this);
}
bool
ApplicationSink::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                const Address &sender)
{
  uint32_t NIC = 0;
  for (std::list<ST_bufferOfCannels_sink>::iterator it = m_bufferSink.begin ();
       it != m_bufferSink.end (); it++)
    {
      //std::cout << "Aqui me quedo 3######################################> "
      //         << device->GetIfIndex () << " | " << NIC << std::endl;
      if (NIC == device->GetIfIndex ())
        {
          SecundariosDataTag sec;
          packet->PeekPacketTag (sec);
          /*std::cout << "SEQN: " << sec.GetSEQNumber ()
                    << " El paquete llego corectamente: " << Now ().GetSeconds () << std::endl;*/
          ST_PacketInBuffer_sink newPacket;
          newPacket.m_packet = packet->Copy ();
          newPacket.m_TimeTosavedOnBuffer = Now ();
          it->m_PacketAndTime.push_back (newPacket);
          break;
        }
      NIC++;
    }

  //CheckBuffer ();
  Simulator::Schedule (Seconds ((double) (8 * 1024) / (m_mode.GetDataRate (20))),
                       &ApplicationSink::CheckBuffer, this);
  //std::cout << "Me quedo aqui sink 1 " << GetNode()->GetId()<< std::endl;
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
ApplicationSink::ReadPacketOnBuffer ()
{
  uint32_t NIC = 0;
  for (std::list<ST_bufferOfCannels_sink>::iterator it = m_bufferSink.begin ();
       it != m_bufferSink.end ();
       it++) //Se itera sobre cada buffer de canal para identificar si hay paquetes a enviar
    {
      Ptr<Packet> packet;
      Time TimeInThisNode;
      SecundariosDataTag SecundariosTag;
      SinkDataTag SinkTag;
      Ptr<Packet> PacketToReSend;
      if (it->m_PacketAndTime.size () != 0 &&
          !(*it)
               .m_visitado) // Se evalua si el buffer visitado esta vacio o bien si no ha sido visitado
        {
          (*it).m_visitado = true;
          for (std::list<ST_PacketInBuffer_sink>::iterator pck = it->m_PacketAndTime.begin ();
               pck != it->m_PacketAndTime.end ();
               pck++) //Este for itera sobre los paquetes del buffer visitado
            { // este for es para obtener solo un paquete dentro del buffer en el canal n
              packet = pck->m_packet;

              TimeInThisNode = Now () - pck->m_TimeTosavedOnBuffer;
              it->m_PacketAndTime.erase (pck);
              break;
            }
          if (packet->PeekPacketTag (SecundariosTag))
            { //Se verifica el paquete recibido sea de nodos secundarios

              //uint8_t *buffer = new uint8_t[packet->GetSize ()];

              //packet->CopyData (buffer, packet->GetSize ());

              //std::cout << "Recibi paquete en sink : " << ruta << std::endl;
              /*std::cout << "Recibi paquete en sink : " << Now ().GetSeconds ()
                        << "SEQN: " << SecundariosTag.GetSEQNumber () << " El retardo es: "
                        << SecundariosTag.GetTimestamp ().GetSeconds () +
                               TimeInThisNode.GetSeconds ()
                        << std::endl;*/

              if (Entregado (SecundariosTag.GetSEQNumber (), SecundariosTag.GetNodeId (),
                             SecundariosTag.GetcopiaID ()))
                {
                  // std::cout << "Recibi paquete en sink : " << SinkTag.GetSEQNumber () << " | "
                  //          << Now ().GetSeconds () << std::endl;
                  break; //Si el paquete ya está entregado no se envia el paquete
                }
              if (!BuscaSEQEnTabla (SecundariosTag.GetSEQNumber (), SecundariosTag.GetNodeId (),
                                    SecundariosTag.GetcopiaID ()))
                {
                  //SecundariosTag.Print (std::cout);
                  //std::cout << "Sink::ReadPacketOnBuffer1 " << std::endl;
                  PacketToReSend = Create<Packet> ();
                  std::string ruta = std::string (SecundariosTag.GetBufferRoute (),
                                                  SecundariosTag.GetBufferRoute () +
                                                      SecundariosTag.GetSizeBufferRoute ()) +
                                     "," + std::to_string (GetNode ()->GetId ());
                  //std::string ruta = std::string (buffer, buffer + packet->GetSize ()) + "," +
                  //                   std::to_string (GetNode ()->GetId ());

                  /* for (uint32_t i = 0; i < ruta.length (); i++)
                    {
                      std::cout << ruta[i];
                    }
                  std::cout << std::endl;*/

                  SinkTag.SetNodeId (SecundariosTag.GetNodeId ());
                  SinkTag.setCopiaID (SecundariosTag.GetcopiaID ());
                  SinkTag.SetSG (m_SigmaG);
                  SinkTag.SetSEQNumber (SecundariosTag.GetSEQNumber ());
                  SinkTag.SetTimestamp (SecundariosTag.GetTimestamp () + TimeInThisNode);
                  SinkTag.SetBufferRoute ((uint8_t *) ruta.c_str ());
                  SinkTag.SetSizeBufferRoute (ruta.length ());

                  //std::cout<<"Aqui me quedo11:"<<std::endl;
                  PacketToReSend->AddPacketTag (SinkTag);
                  //packet->AddPacketTag (SinkTag);
                  //std::cout<<"Aqui me quedo2"<<std::endl;

                  //ImprimeTabla();
                  //std::cout << "Recibi paquete en sink222 : " << SinkTag.GetSEQNumber () << " | "
                  //          << Now ().GetSeconds () << std::endl;
                  Guarda_Paquete_para_ACK (
                      PacketToReSend,
                      TimeInThisNode); //En este punto se almacena en memoria el paquete
                  //m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (NIC));
                  //m_wifiDevice->Send (PacketToReSend, Mac48Address::GetBroadcast (), 0x88dc);
                  //std::cout << "Sink::ReadPacketOnBuffer2 " << std::endl;
                }
              else
                {
                  break;
                }

              // std::cout << "Aqui me quedo Envio paquete2" << std::endl;
              break; //este break rompe el for que itera sobre los buffer de los canales
            }
        }
      else
        {
          (*it).m_visitado = true;
        }
      NIC++;
    }
}
void
ApplicationSink::CheckBuffer ()
{
  //std::cout << "Aqui me quedo CheckBuffer1" << std::endl;
  if (BuscaPaquete ())
    {
      //std::cout << "Aqui me quedo CheckBuffer1" << std::endl;
      ReadPacketOnBuffer ();
    }
  if (VerificaVisitados ()) // se verifica si todos los buffers fueron visitados
    {
      //std::cout << "Aqui me quedo CheckBuffer2" << std::endl;
      ReiniciaVisitados (); //si ya fueron visitados se reinician de nuevo las visitas al primer canal (es de forma circular)
    }
  Simulator::Schedule (m_broadcast_time + Seconds (8 * 100 / (m_mode.GetDataRate (10))),
                       &ApplicationSink::CheckBuffer, this);
  //std::cout << "Aqui me quedo CheckBuffer2##" << std::endl;
}
bool
ApplicationSink::VerificaVisitados ()
{
  bool find = true;
  for (std::list<ST_bufferOfCannels_sink>::iterator it = m_bufferSink.begin ();
       it != m_bufferSink.end (); it++)
    {
      if ((*it).m_visitado == false)
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
ApplicationSink::ReiniciaVisitados ()
{
  for (std::list<ST_bufferOfCannels_sink>::iterator it = m_bufferSink.begin ();
       it != m_bufferSink.end (); it++)
    {
      (*it).m_visitado = false;
    }
  // std::cout << "Aqui me quedo Reinicia : " << std::to_string (Now ().GetSeconds ()) << std::endl;
}
bool
ApplicationSink::BuscaPaquete ()
{
  bool find = false;
  for (std::list<ST_bufferOfCannels_sink>::iterator it = m_bufferSink.begin ();
       it != m_bufferSink.end (); it++)
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
/*
bool
ApplicationSink::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                const Address &sender)
{
 
  CustomDataTag tag;
  NS_LOG_FUNCTION (device << packet << protocol << sender);

  
        Packets received here only have Application data, no WifiMacHeader. 
        We created packets with 1000 bytes payload, so we'll get 1000 bytes of payload.
    *

  NS_LOG_INFO ("ReceivePacket() : Node " << GetNode ()->GetId () << " : Received a packet from "
                                         << sender << " Size:" << packet->GetSize ());
  packet->PeekPacketTag (tag);

  //uint32_t size;
  uint8_t *buffer = new uint8_t[packet->GetSize ()];
  packet->CopyData (buffer, packet->GetSize ());

  std::string ruta = std::string (buffer, buffer + packet->GetSize ());
  std::cout << "Received Sink: " << ruta << " Size: " << ruta.length () << std::endl;

  if (!BuscaSEQEnTabla (tag.GetSEQNumber ()) && tag.GetTypeOfPacket () != 2)
    { // Si el numero de secuencia no esta ne la tabla lo guarda para enviar su ACK
      std::cout << "Recibi paquete " << std::endl;
      Guarda_Paquete_para_ACK (tag.GetSEQNumber (), tag.GetNodeId (), packet->GetSize (),
                               tag.GetTimestamp (), tag.GetTypeOfPacket (), ruta);

      //ImprimeTabla ();
    }
  return true;
}*/

void
ApplicationSink::Guarda_Paquete_para_ACK (Ptr<Packet> paquete, Time timeBuff)
{
  ST_Reenvios_Sink ACK;
  ACK.m_packet = paquete->Copy ();
  ACK.retardo = timeBuff;
  ACK.NumeroDeEnvios = 0;
  ACK.Tiempo_ultimo_envio = Now ();
  m_Tabla_paquetes_ACK.push_back (ACK);
}

bool
ApplicationSink::BuscaSEQEnTabla (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia)
{
  bool find = false;
  for (std::list<ST_Reenvios_Sink>::iterator it = m_Tabla_paquetes_ACK.begin ();
       it != m_Tabla_paquetes_ACK.end (); it++)
    {
      SinkDataTag SinkTAg;
      it->m_packet->PeekPacketTag (SinkTAg);
      if (SinkTAg.GetSEQNumber () == SEQ && SinkTAg.GetNodeId () == IDcreador &&
          SinkTAg.GetcopiaID () == IDCopia)
        {
          find = true;
          break;
        }
    }

  return find;
}

bool
ApplicationSink::Entregado (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia)
{
  bool find = false;
  for (std::list<ST_Reenvios_Sink>::iterator it = m_Paquetes_Recibidos.begin ();
       it != m_Paquetes_Recibidos.end (); it++)
    {
      SinkDataTag SinkTAg;
      it->m_packet->PeekPacketTag (SinkTAg);
      if (SinkTAg.GetSEQNumber () == SEQ && SinkTAg.GetNodeId () == IDcreador &&
          SinkTAg.GetcopiaID () == IDCopia)
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
            << Now ().GetSeconds () << std::endl;
  for (std::list<ST_Reenvios_Sink>::iterator it = m_Tabla_paquetes_ACK.begin ();
       it != m_Tabla_paquetes_ACK.end (); it++)
    {
      SinkDataTag Sinktag;
      it->m_packet->PeekPacketTag (Sinktag);
      std::cout << " SEQnumber: " << Sinktag.GetSEQNumber ()
                << "\t Nodo que envia: " << Sinktag.GetNodeId () << std::endl;
    }
}
void // Esta funcion es llamada desde el main
ApplicationSink::CreaBuffersCanales ()
{

  for (uint32_t i = 0; i < m_n_channels + 2; i++) //dos mas 1 por los primarios y otro para el sink
    {
      ST_bufferOfCannels_sink newBufferCH;
      newBufferCH.m_visitado = false;
      m_bufferSink.push_back (newBufferCH);
    }
}
void
ApplicationSink::SetChannels (uint32_t channels)
{
  m_n_channels = channels;
}

} // namespace ns3
