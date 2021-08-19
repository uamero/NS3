#ifndef Sink_APPLICATION_H
#define Sink_APPLICATION_H
#include "ns3/application.h"
#include "ns3/wave-net-device.h"
#include "ns3/wifi-phy.h"
#include <vector>
#include <list>
namespace ns3 {
/** \brief A struct to represent information about this node's neighbors. I chose MAC address and the time last message was received form that node
     * The time 'last_beacon' is used to determine whether we should remove the neighbor from the list.
     */


typedef struct
{
  Ptr<Packet> m_packet;
  Time m_TimeTosavedOnBuffer;
} ST_PacketInBuffer_sink;

typedef struct
{
  bool m_visitado;
  std::list<ST_PacketInBuffer_sink> m_PacketAndTime;
} ST_bufferOfCannels_sink;

typedef struct
{
  Ptr<Packet> m_packet;
  Time Tiempo_ultimo_envio;
  Time retardo;
  uint8_t m_Reenvios;
} ST_Reenvios_Sink;

class ApplicationSink : public ns3::Application
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  ApplicationSink ();
  ~ApplicationSink ();

  /** \brief Broadcast some information 
             */
  void BroadcastInformation ();
  void IniciaTabla(uint32_t PQts_A_enviar);
  void ImprimeTabla();
  /** \brief This function is called when a net device receives a packet. 
             * I connect to the callback in StartApplication. This matches the signiture of NetDevice receive.
             */
  bool ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                      const Address &sender);

  void SetBroadcastInterval (Time interval);
  void ReadPacketOnBuffer ();
  bool BuscaPaquete ();
  void CheckBuffer ();
  bool
  VerificaVisitados (); //funcion para iterar sobre todos los canales y ver si ya fueron visitados
  void ReiniciaVisitados (); //funcion para comenzar la iteraci[n desde el primer canal
  bool Entregado (u_long SEQ);
  /** \brief Update a neighbor's last contact time, or add a new neighbor
             */

  /** \brief Change the data rate used for broadcasts.
             */
  void SetWifiMode (WifiMode mode);
 
  void ConfirmaEntrega(u_long SEQ);
  //You can create more functions like getters, setters, and others
  bool BuscaSEQEnTabla(u_long SEQ);
  void Guarda_Paquete_para_ACK(Ptr<Packet> paquete, Time timeBuff);
  uint32_t m_n_channels;
private:
  /** \brief This is an inherited function. Code that executes once the application starts
             */
  void StartApplication ();
  Time m_broadcast_time; /**< How often do you broadcast messages */
  uint32_t m_packetSize; /**< Packet size in bytes */
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WaveNetDevice that is attached to this device */
  std::list<ST_Reenvios_Sink> m_Tabla_paquetes_ACK;/**> Lista de paquetes a enviar*/
  Time m_time_limit; /**< Time limit to keep neighbors in a list*/
  Time m_Tiempo_de_reenvio;
  u_long m_semilla;
  WifiMode m_mode; /**< data rate used for broadcasts */
  uint32_t m_SigmaG;
  std::list<ST_bufferOfCannels_sink>
      m_bufferSink;
  std::list<ST_Reenvios_Sink> m_Paquetes_Recibidos; /**> Lista de paquetes provenientes de otros nodos alarmados*/
 
};
} // namespace ns3

#endif