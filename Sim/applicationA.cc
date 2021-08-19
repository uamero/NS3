#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "applicationA.h"
#include "SecundariosTag.h"
#include "SinkTag.h"
#include "TagPrimarios.h"

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

CustomApplication::CustomApplication ()
{
  m_broadcast_time = Seconds (1); //every 1s
  m_packetSize = 1000; //1000 bytes
  m_Tiempo_de_reenvio = Seconds (10); //Tiempo para reenviar los paquetes
  m_time_limit = Seconds (5); //Tiempo limite para los nodos vecinos
  m_mode = WifiMode ("OfdmRate6MbpsBW10MHz");
  m_semilla = 0; // controla los numeros de secuencia
  m_n_channels = 8;

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
      Time random_offset = MicroSeconds (rand->GetValue (50, 200));
      Simulator::Schedule (m_broadcast_time + random_offset,
                           &CustomApplication::BroadcastInformation, this);
    }
  else
    {
      NS_FATAL_ERROR ("There's no WifiNetDevice in your node");
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
  //std::cout<<"Nodos A: "<<GetNode()->GetNDevices() << " n_chanels: "<<m_n_channels<<std::endl;
  //std::cout << "A El canal por donde envio es ch: " << std::to_string(ch) << std::endl;
  //std::cout<<"ID Device "<<m_wifiDevice->GetIfIndex ()<< " n: "<<GetNode()->GetNDevices()<<std::endl;
  //if (m_wifiDevice->GetIfIndex () != 0)
  //{
  // std::cout << "Aqui me quedo 1" << std::endl;
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {

      Time Ultimo_Envio = Now () - it->Tiempo_ultimo_envio;

      if (!it->Estado && m_Canales_Para_Utilizar.size () != 0)
        {

          if (it->NumeroDeEnvios == 0)
            {

              it->NumeroDeEnvios += 1;

              for (std::list<uint32_t>::iterator _it = m_Canales_Para_Utilizar.begin ();
                   _it != m_Canales_Para_Utilizar.end (); _it++)
                {
                  std::string ruta = std::to_string (GetNode ()->GetId ());
                  /*Ptr<Packet> packet =
                      Create<Packet> ((uint8_t *) msgx.str ().c_str (), packetSize);*/
                  //std::cout << "Packet 1 enviado:" << ruta <<  std::endl;
                  Ptr<Packet> packet = Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());
                  //Ptr<Packet> packet = Create<Packet> (m_packetSize);
                  SecundariosDataTag etiqueta;

                  etiqueta.SetNodeId (GetNode ()->GetId ());
                  etiqueta.SetSEQnumber (it->numeroSEQ);
                  etiqueta.SetChanels (*_it);
                  /*CustomDataTag tag;
                  // El timestamp se configura dentro del constructor del tag
                  tag.SetTypeOfpacket (0);
                  tag.SetNodeId (GetNode ()->GetId ());
                  tag.CopySEQNumber (it->numeroSEQ);
                  tag.SetChanels (*_it);

                  packet->AddPacketTag (tag);
                  */
                  packet->AddPacketTag (etiqueta);

                  //std::cout << "SEQQQ en AAAAA"<<tag.GetSEQNumber() <<std::endl;
                  //std::cout<<"channel ->>>>>>>>>>>>>> "<<*_it <<std::endl;
                  //std::cout<<" Me quedo aqui #############################%"<<std::to_string(GetNode()->GetNDevices())<<" El canal es: "<<std::to_string(*_it)<<std::endl;//aqui esta el problema investigar el porque y arreglarlo
                  m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (*_it));
                  //std::cout<<" Me quedo aqui #############################/"<<" El canal es: "<<std::to_string(*_it)<<std::endl;
                  m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
                }
            }
          else if (Ultimo_Envio >= m_Tiempo_de_reenvio) //
            {
              std::string ruta = std::to_string (GetNode ()->GetId ());
              it->NumeroDeEnvios += 1;
              it->Tiempo_ultimo_envio = Now ();
              for (std::list<uint32_t>::iterator _it = m_Canales_Para_Utilizar.begin ();
                   _it != m_Canales_Para_Utilizar.end (); _it++)
                {
                  std::string ruta = std::to_string (GetNode ()->GetId ());
                  Ptr<Packet> packet = Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());
                  //Ptr<Packet> packet = Create<Packet> (m_packetSize);
                  SecundariosDataTag etiqueta;
                  etiqueta.SetNodeId (GetNode ()->GetId ());
                  etiqueta.SetSEQnumber (it->numeroSEQ);
                  etiqueta.SetChanels (*_it);
                  /*
                  CustomDataTag tag;
                  tag.SetTypeOfpacket (1);
                  tag.SetNodeId (GetNode ()->GetId ());
                  tag.CopySEQNumber (it->numeroSEQ);
                  tag.SetChanels (*_it);
                  */
                  packet->AddPacketTag (etiqueta);
                  //std::cout<<"channel ->>>>>>>>>>>>>> "<<*_it <<std::endl;
                  m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (*_it));
                  m_wifiDevice->Send (packet, Mac48Address::GetBroadcast (), 0x88dc);
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
  // std::cout << "Aqui me quedo 2" << std::endl;
  //}

  //Broadcast the packet as WSMP (0x88dc)
  //Schedule next broadcast
  Simulator::Schedule (m_broadcast_time, &CustomApplication::BroadcastInformation, this);
  //std::cout<<Now().GetSeconds()<<std::endl;
  m_simulation_time = Now ();

  /*if (VerificaFinDeSimulacion () || Now () >= Seconds(1000))
    {
      //std::cout<<"Tiempo de simulacion: "<< Now().GetSeconds() <<"Seg."<<std::endl;

      Simulator::Stop ();
    }*/
}
void
CustomApplication::ReenviaPaquete ()
{
  /*La lista que contiende los paquetes en memoria es m_paquetes_Recibidos*/
  for (std::list<ST_Reenvios>::iterator it = m_Paquetes_A_Reenviar.begin ();
       it != m_Paquetes_A_Reenviar.end (); it++)
    { //Se itera sobre los paquetes que se almacenan en memoria
      Time Ultimo_Envio = Now () - it->Tiempo_ultimo_envio;
      it->retardo = it->retardo +
                    Ultimo_Envio; // Esta variable se modifica inicialmente en la lectura de buffer

      if (m_Canales_Para_Utilizar.size () != 0 && Ultimo_Envio >= m_Tiempo_de_reenvio)
        {
          /**Aqui se debe actualizar el satisfacción local y se debe almacenar en la etiqueta que ya tiene el paquete
           * -> preguntar si se debe poner en cero o bien 
          */
          it->Tiempo_ultimo_envio = Now ();
          for (std::list<uint32_t>::iterator _it = m_Canales_Para_Utilizar.begin ();
               _it != m_Canales_Para_Utilizar.end (); _it++)
            {
              SecundariosDataTag etiqueta;
              it->m_packet->PeekPacketTag (
                  etiqueta); //Este paquete proviene de la lectura del buffer por lo que ya contiene una etiqueta
              etiqueta.SetSL (it->retardo.GetSeconds ()); //aqui se debe involucrar el retardo
              etiqueta.SetChanels (*_it);
              it->m_packet->AddPacketTag (etiqueta);

              //std::cout<<"channel ->>>>>>>>>>>>>> "<<*_it <<std::endl;
              m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (*_it));
              m_wifiDevice->Send (it->m_packet, Mac48Address::GetBroadcast (), 0x88dc);
              break;
            }
        }
    }
  Simulator::Schedule (Now () + m_Tiempo_de_reenvio, &CustomApplication::ReenviaPaquete, this);
}

bool
CustomApplication::ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet,
                                  uint16_t protocol, const Address &sender)
{
  /**Hay dos tipos de paquetes que se pueden recibir
   * 1.- Un paquete que fue enviado por este nodo 
   * 2.- Un paquete que fue enviado por el Sink*/

  uint32_t NIC = 0;

  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++)
    {
      if (NIC == device->GetIfIndex ())
        {
          //std::cout << "Aqui me quedo 3######################################> "
          //        << device->GetIfIndex () << " | " << NIC << std::endl;
          ST_PacketInBufferA newPacket;
          newPacket.m_packet = packet->Copy ();
          newPacket.m_TimeTosavedOnBuffer = Now ();
          it->m_PacketAndTime.push_back (newPacket);
          break;
        }
      NIC++;
    }
  CheckBuffer ();
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
CustomApplication::ReadPacketOnBuffer ()
{
  uint32_t NIC = 0;
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++) //Se itera sobre cada buffer de canal para identificar si hay paquetes a enviar
    {
      Ptr<Packet> packet;
      Time TimeInThisNode;
      SecundariosDataTag SecundariosTag;
      SinkDataTag SinkTag;
      PrimariosDataTag PrimariosTag;

      if (packet == NULL)
        {
          std::cout << "El paquete esta vacio" << std::endl;
        }
      if (it->m_PacketAndTime.size () != 0 &&
          !(*it)
               .m_visitado) // Se evalua si el buffer visitado esta vacio o bien si no ha sido visitado
        {
          (*it).m_visitado = true;
          for (std::list<ST_PacketInBufferA>::iterator pck = it->m_PacketAndTime.begin ();
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
              if (NIC != m_n_channels && NIC != m_n_channels + 1 &&
                  VerificaCanal (SecundariosTag.GetChanels ()))
                {

                  uint8_t *buffer = new uint8_t[packet->GetSize ()];

                  packet->CopyData (buffer, packet->GetSize ());

                  std::string ruta = std::string (buffer, buffer + packet->GetSize ()) + "," +
                                     std::to_string (GetNode ()->GetId ());

                  Ptr<Packet> PacketToReSend =
                      Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());

                  for (std::list<uint32_t>::iterator FreeChIt = m_Canales_Para_Utilizar.begin ();
                       FreeChIt != m_Canales_Para_Utilizar.end (); FreeChIt++)
                    {
                      SecundariosTag.SetChanels ((*FreeChIt));

                      PacketToReSend->AddPacketTag (SecundariosTag);
                      if (Entregado (SecundariosTag.GetSEQNumber ()))
                        {
                          break;//Si el paquete ya está entregado no se envia el paquete
                        }
                      if (!BuscaSEQEnTabla (SecundariosTag.GetSEQNumber ()))
                        {
                          Guarda_Info_Paquete (
                              PacketToReSend,
                              TimeInThisNode); //En este punto se almacena en memoria el paquete
                        }

                      m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (NIC));
                      m_wifiDevice->Send (PacketToReSend, Mac48Address::GetBroadcast (), 0x88dc);
                      // std::cout << "Aqui me quedo Envio paquete2" << std::endl;
                      break;
                    }
                  break; //este break rompe el for que itera sobre los buffer de los canales
                }
            }
          else if (packet->PeekPacketTag (SinkTag))
            {
              if (NIC == m_n_channels)
                {
                  std::cout << "Si confirmo la entrega" << std::endl;
                  ConfirmaEntrega (SinkTag.GetSEQNumber ());
                }
            }
          else if (packet->PeekPacketTag (PrimariosTag))
            {
              if (NIC == m_n_channels + 1)
                {
                  BuscaCanalesID (PrimariosTag.GetChanels (), PrimariosTag.GetnodeID (), Now ());
                  CanalesDisponibles ();
                }
            }
        }
      else
        {
          (*it).m_visitado = true;
        }
      NIC++; //es el device correspondiente
    }
}
/*
void
CustomApplication::ReadPacketOnBuffer ()
{
  //por cada iteración se debe sacar un paquete
  // esta función se manda a llamar
  uint32_t NIC = 0;

  //uint64_t data_rate =
  //m_mode.GetDataRate (10); //me da un numero entero con la cantidad de bps que se pueden enviar
  //std::cout << "El data rate es: " << data_rate << " con un ancho de banda de 10 MB" << std::endl;
  //std::cout << "Aqui me quedo 5" << std::endl;
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++) //Se itera sobre cada buffer de canal para identificar si hay paquetes a enviar
    {

      //CustomDataTag tag;

      Ptr<Packet> packet;
      Time TimeInThisNode;

      if (it->m_PacketAndTime.size () != 0 &&
          !(*it)
               .m_visitado) // Se evalua si el buffer visitado esta vacio o bien si no ha sido visitado
        {
          (*it).m_visitado = true;
          for (std::list<ST_PacketInBufferA>::iterator pck = it->m_PacketAndTime.begin ();
               pck != it->m_PacketAndTime.end ();
               pck++) //Este for itera sobre los paquetes del buffer visitado
            { // este for es para obtener solo un paquete dentro del buffer en el canal n
              packet = pck->m_packet;
              TimeInThisNode = Now () - pck->m_TimeTosavedOnBuffer;
              it->m_PacketAndTime.erase (pck);
              break;
            }
         

          packet->PeekPacketTag (tag);
          uint8_t *buffer = new uint8_t[packet->GetSize ()];
          packet->CopyData (buffer, packet->GetSize ());
          std::string ruta = std::string (buffer, buffer + packet->GetSize ()) + "," +
                             std::to_string (GetNode ()->GetId ());
          uint64_t ch = tag.GetChanels ();
          //std::cout << "NIC->>>>>> " << NIC << <<std::endl;
          // std::cout << "Aqui me quedo Envio paquete1: " << tag.GetSEQNumber () << " | " << NIC
          //          << " | " << m_n_channels << "| " << tag.GetTypeOfPacket () << " | "
          //          << VerificaCanal (ch) << std::endl;
          if (!BuscaSEQEnTabla (tag.GetSEQNumber ()) && NIC != m_n_channels &&
              NIC != m_n_channels + 1 && VerificaCanal (ch))
            { // Si el numero de secuencia no esta en la tabla lo saca del buffer para reenviar
              Guarda_Info_Paquete (
                  tag.GetSEQNumber (), tag.GetNodeId (), tag.GetCopyNumber (), ruta,
                  tag.GetTimestamp (),
                  tag.GetTypeOfPacket ()); // esta funcion deja de servir por lo tanto hay que borrarla
              //NS_LOG_UNCOND ("Aqui estoy dentro del envio");
              Ptr<Packet> PacketToReSend =
                  Create<Packet> ((uint8_t *) ruta.c_str (), ruta.length ());

              for (std::list<uint32_t>::iterator FreeChIt = m_Canales_Para_Utilizar.begin ();
                   FreeChIt != m_Canales_Para_Utilizar.end (); FreeChIt++)
                {
                  tag.SetChanels ((*FreeChIt));

                  PacketToReSend->AddPacketTag (tag);

                  m_wifiDevice = DynamicCast<WifiNetDevice> (GetNode ()->GetDevice (NIC));
                  m_wifiDevice->Send (PacketToReSend, Mac48Address::GetBroadcast (), 0x88dc);
                  // std::cout << "Aqui me quedo Envio paquete2" << std::endl;
                  break;
                }
              break; //este break rompe el for que itera sobre los buffer de los canales
              //aQUI IMPLEMENTAR EL ENVIO DEL PAQUETE
            }
          else if (tag.GetTypeOfPacket () == 2 && NIC == m_n_channels)
            { //Paquete proveniente del Sink
              std::cout << "Si confirmo la entrega" << std::endl;
              ConfirmaEntrega (tag.GetSEQNumber ());
            }
          else if (tag.GetTypeOfPacket () == 3 && NIC == m_n_channels + 1)
            { //Paquete proveniente de los usuarios primarios
              //std::bitset<8> ocu(ch);
              //std::cout << "A-> La ocupación recibida en "<<GetNode()->GetId()<<" es: "<<ocu<<std::endl;
              BuscaCanalesID (tag.GetChanels (), tag.GetNodeId (), Now ());
              CanalesDisponibles ();
            }
          break;
        }
      else
        {
          (*it).m_visitado = true;
        }
      NIC++; //es el device correspondiente
    }
  //std::cout << "Aqui me quedo 6" << std::endl;
}
*/
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
CustomApplication::CheckBuffer ()
{
  //std::cout << "Aqui me quedo CheckBuffer1" << std::endl;
  if (BuscaPaquete ())
    {
      // std::cout << "Aqui me quedo CheckBuffer1" << std::endl;
      ReadPacketOnBuffer ();
    }
  if (VerificaVisitados ()) // se verifica si todos los buffers fueron visitados
    {
      //std::cout << "Aqui me quedo CheckBuffer2" << std::endl;
      ReiniciaVisitados (); //si ya fueron visitados se reinician de nuevo las visitas al primer canal (es de forma circular)
    }
  Simulator::Schedule (Now () + Seconds (8 * 100 / (m_mode.GetDataRate (10))),
                       &CustomApplication::CheckBuffer, this);
  //std::cout << "Aqui me quedo CheckBuffer2##" << std::endl;
}
bool
CustomApplication::VerificaVisitados ()
{
  bool find = true;
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++)
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
CustomApplication::ReiniciaVisitados ()
{
  for (std::list<ST_bufferOfCannelsA>::iterator it = m_bufferA.begin (); it != m_bufferA.end ();
       it++)
    {
      (*it).m_visitado = false;
    }
  // std::cout << "Aqui me quedo Reinicia : " << std::to_string (Now ().GetSeconds ()) << std::endl;
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
      Paquete.Tiempo_de_recibo_envio = Seconds (0);
      Paquete.NumeroDeEnvios = 0;
      Paquete.Estado = false;
      m_Tabla_paquetes_A_enviar.push_back (Paquete);
    }
  ST_Canales ch;
  ch.m_chanels = 0; //Todos lo canales disponibles
  ch.ID_Persive = ID;
  ch.Tiempo_ultima_actualizacion = Now ();
  m_Canales_disponibles.push_back (ch);
  // std::cout << "Inicia tabla "<<m_Tabla_paquetes_A_enviar.size()<<std::endl;
}
void
CustomApplication::Guarda_Info_Paquete (Ptr<Packet> paquete, Time timeBuff)
{

  ST_Reenvios reenvio;
  reenvio.m_packet = paquete;
  reenvio.Tiempo_ultimo_envio = Now ();
  reenvio.retardo = timeBuff;
  m_Paquetes_A_Reenviar.push_back (reenvio);
}
bool
CustomApplication::Entregado (u_long SEQ)
{
  bool find = false;
  for (std::list<ST_Reenvios>::iterator it = m_Paquetes_Recibidos.begin ();
       it != m_Paquetes_Recibidos.end (); it++)
    {
      SecundariosDataTag SecTAg;
      it->m_packet->PeekPacketTag (SecTAg);
      if (SecTAg.GetSEQNumber () == SEQ)
        {
          find = true;
          break;
        }
    }
  return find;
}
void
CustomApplication::ConfirmaEntrega (u_long SEQ)//faltan mas datos para tener un buena entrega
{
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
       SecundariosDataTag SecTAg;
       ST_Reenvios entregado;

      if (it->numeroSEQ == SEQ)
        {
          Ptr<Packet> np = Create<Packet>();
          SecTAg.SetSEQnumber(SEQ);
          SecTAg.SetNodeId(GetNode()->GetId());

          np->AddPacketTag(SecTAg);
          entregado.retardo=Seconds(0);
          entregado.Tiempo_ultimo_envio=Now();
          entregado.m_packet=np;

          it->Tiempo_de_recibo_envio = Now () - it->Tiempo_ultimo_envio;
          it->Estado = true;
          m_Paquetes_Recibidos.push_back (entregado);
          it++;
          it->Tiempo_ultimo_envio = Now ();
          break;
        }
    }
  for (std::list<ST_Reenvios>::iterator it = m_Paquetes_A_Reenviar.begin ();
       it != m_Paquetes_A_Reenviar.end (); it++)
    {
      SecundariosDataTag SecTAg;
      it->m_packet->PeekPacketTag (SecTAg);
      if (SecTAg.GetSEQNumber () == SEQ)
        {
           m_Paquetes_Recibidos.push_back ((*it));
           m_Paquetes_A_Reenviar.erase (it);
          break;
        }
    }
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
  for (std::list<ST_Reenvios>::iterator it = m_Paquetes_A_Reenviar.begin ();
       it != m_Paquetes_A_Reenviar.end (); it++)
    {
      SecundariosDataTag SecTAg;
      it->m_packet->PeekPacketTag (SecTAg);
      if (SecTAg.GetSEQNumber () == SEQ)
        {
          find = true;
          break;
        }
    }
  return find;
}
void
CustomApplication::setSemilla (u_long sem)
{
  this->m_semilla = sem;
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
                << "\t Tiempo de entrega: "
                << (it->Tiempo_de_recibo_envio.GetMilliSeconds ()) / 1000.0 << " s" << std::endl;
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
              std::to_string ((it->Tiempo_de_recibo_envio.GetMilliSeconds ()) / 1000.0) + "," +
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
void
CustomApplication::CanalesDisponibles ()
{
  uint64_t canales = 0;
  m_Canales_Para_Utilizar.clear ();
  for (std::list<ST_Canales>::iterator it = m_Canales_disponibles.begin ();
       it != m_Canales_disponibles.end (); it++)
    {
      canales = it->m_chanels | canales;
    }

  std::bitset<64> x (canales);
  std::string cadena = x.to_string ();
  std::string disp = std::string (cadena.rbegin (), cadena.rend ());
  /*El bit menos significativo es el canal 0*/
  for (uint8_t i = 0; i < m_n_channels; i++)
    {
      if (disp[i] == '0')
        {

          m_Canales_Para_Utilizar.push_back (i);
        }
    }
}
bool
CustomApplication::VerificaCanal (uint8_t ch)
{
  uint64_t canales = 0;
  bool find = false;
  for (std::list<ST_Canales>::iterator it = m_Canales_disponibles.begin ();
       it != m_Canales_disponibles.end (); it++)
    {
      canales = it->m_chanels | canales;
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
CustomApplication::BuscaCanalesID (uint64_t ch, uint32_t ID, Time tim)
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
    }
  return find;
}
bool // Esta funcion es llamada desde el main
CustomApplication::VerificaFinDeSimulacion ()
{
  bool find = true;
  for (std::list<ST_Paquete_A_Enviar>::iterator it = m_Tabla_paquetes_A_enviar.begin ();
       it != m_Tabla_paquetes_A_enviar.end (); it++)
    {
      if (!it->Estado)
        {
          find = false;
          break;
        }
    }
  return find;
}
void // Esta funcion es llamada desde el main
CustomApplication::CreaBuffersCanales ()
{

  for (uint32_t i = 0; i < m_n_channels + 2; i++) //dos mas 1 por los primarios y otro para el sink
    {
      ST_bufferOfCannelsA newBufferCH;
      newBufferCH.m_visitado = false;
      m_bufferA.push_back (newBufferCH);
    }
}
} // namespace ns3
