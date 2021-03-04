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
  Mac48Address neighbor_mac;
  Time last_beacon;
} NeighborInformation_sink;
typedef struct
{
  u_long numeroSEQ;
  uint32_t ID_Creador;
  uint32_t Tam_Paquete;
  Time Tiempo_ultimo_envio;
  int32_t tipo_de_paquete;
  uint32_t NumeroDeEnvios;  
} ST_Paquete_A_Enviar_sink;

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

  /** \brief Update a neighbor's last contact time, or add a new neighbor
             */
  void UpdateNeighbor (Mac48Address addr);
  /** \brief Print a list of neighbors
             */
  void PrintNeighbors ();

  /** \brief Change the data rate used for broadcasts.
             */
  void SetWifiMode (WifiMode mode);

  /** \brief Remove neighbors you haven't heard from after some time.
             */
  void RemoveOldNeighbors ();
 
  u_long CalculaSeqNumber(u_long *sem);
  void ConfirmaEntrega(u_long SEQ);
  //You can create more functions like getters, setters, and others
  bool BuscaSEQEnTabla(u_long SEQ);
  void Guarda_Paquete_para_ACK(u_long SEQ,uint32_t ID_Creador,uint32_t tam_del_paquete,
  Time timeStamp,int32_t type);
  uint32_t m_n_channels;
private:
  /** \brief This is an inherited function. Code that executes once the application starts
             */
  void StartApplication ();
  Time m_broadcast_time; /**< How often do you broadcast messages */
  uint32_t m_packetSize; /**< Packet size in bytes */
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WaveNetDevice that is attached to this device */
  std::vector<NeighborInformation_sink> m_neighbors; /**< A table representing neighbors of this node */
  std::list<ST_Paquete_A_Enviar_sink> m_Tabla_paquetes_ACK;/**> Lista de paquetes a enviar*/
  Time m_time_limit; /**< Time limit to keep neighbors in a list*/
  Time m_Tiempo_de_reenvio;
  u_long m_semilla;
  WifiMode m_mode; /**< data rate used for broadcasts */
};
} // namespace ns3

#endif