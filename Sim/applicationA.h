#ifndef WAVETEST_CUSTOM_APPLICATION_H
#define WAVETEST_CUSTOM_APPLICATION_H
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
} NeighborInformation;

typedef struct
{
  u_long numeroSEQ;
  uint32_t ID_Creador;
  uint32_t Tam_Paquete;
  Time Tiempo_ultimo_envio;
  Time Tiempo_de_recibo_envio;
  uint32_t NumeroDeEnvios;
  bool Estado; /**> true -> el paquete ha sido entregado*/
}ST_Paquete_A_Enviar;
typedef struct
{
  u_long numeroSEQ;
  uint32_t ID_Creador;
  uint32_t Tam_Paquete;
  Time Tiempo_ultimo_envio;
  int32_t tipo_de_paquete;
}ST_Reenvios;
typedef struct 
{
  uint8_t m_chanels;
  Time Tiempo_ultima_actualizacion;
  uint32_t ID_Persive;
}ST_Canales;

class CustomApplication : public ns3::Application
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  CustomApplication ();
  ~CustomApplication ();

  /** \brief Broadcast some information 
             */
  void BroadcastInformation ();
  void IniciaTabla(uint32_t PQts_A_enviar,uint32_t NodoID);
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
  void SetMAxtime(Time Maxtime);
  Time GetMAxtime();
  u_long CalculaSeqNumber(u_long *sem);
  void ConfirmaEntrega(u_long SEQ);
  //You can create more functions like getters, setters, and others
  bool BuscaSEQEnTabla(u_long SEQ);
  void Guarda_Paquete_reenvio(u_long SEQ,uint32_t ID_Creador,uint32_t tam_del_paquete,
  Time timeStamp,int32_t type);
  std::list<ST_Reenvios>::iterator GetReenvio();
  void setSemilla(u_long sem);
  uint8_t CanalesDisponibles();
  /*Se actualiza o bien se agregan los canales que los usarios primarios ocupan del espectro */
  bool BuscaCanalesID(uint8_t ch,uint32_t ID,Time timD);
  bool VerificaCanal(uint8_t ch);
  std::string ObtenDAtosNodo();
  uint32_t CuentaPQTSEntregados();
  bool VerificaFinDeSimulacion();
  std::list<ST_Paquete_A_Enviar> m_Tabla_paquetes_A_enviar;/**> Lista de paquetes a enviar*/
  Time m_broadcast_time; /**< How often do you broadcast messages */
  Time m_simulation_time;
  Time m_Max_Time_To_Stop;
  void sendACK();
private:
  /** \brief This is an inherited function. Code that executes once the application starts
             */
  void StartApplication ();
 
  uint32_t m_packetSize; /**< Packet size in bytes */
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WaveNetDevice that is attached to this device */

  std::vector<NeighborInformation> m_neighbors; /**< A table representing neighbors of this node */
  u_long m_SEQNumberToACK;
  std::list<ST_Reenvios> m_Paquetes_A_Reenviar;/**> Lista de paquetes a reenviar*/
  std::list<ST_Canales> m_Canales_disponibles;/**> Lista de paquetes a reenviar*/
  Time m_time_limit; /**< Time limit to keep neighbors in a list */
  Time m_Tiempo_de_reenvio;
  u_long m_semilla;
  WifiMode m_mode; /**< data rate used for broadcasts */
};
} // namespace ns3

#endif