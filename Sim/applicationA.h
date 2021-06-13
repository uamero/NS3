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
  std::string ruta;
  Time Tiempo_ultimo_envio;
  int32_t tipo_de_paquete;
}ST_Reenvios;
typedef struct 
{
  uint8_t m_chanels;
  Time Tiempo_ultima_actualizacion;
  uint32_t ID_Persive;
}ST_Canales;


/*typedef struct
{
  double Retardo_promedio;
  
}ST_SatisfacciónL;*/
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
  void Guarda_Paquete_reenvio(u_long SEQ,uint32_t ID_Creador,uint32_t tam_del_paquete,std::string Ruta,
  Time timeStamp,int32_t type);
  std::list<ST_Reenvios>::iterator GetReenvio();
  void setSemilla(u_long sem);
  void CanalesDisponibles();
  void CreaBuffersCanales();
  /*Se actualiza o bien se agregan los canales que los usarios primarios ocupan del espectro */
  bool BuscaCanalesID(uint64_t ch,uint32_t ID,Time timD);
  bool VerificaCanal(uint8_t ch);
  std::string ObtenDAtosNodo();
  uint32_t CuentaPQTSEntregados();
  bool VerificaFinDeSimulacion();
  std::list<ST_Paquete_A_Enviar> m_Tabla_paquetes_A_enviar;/**> Lista de paquetes a enviar*/
  Time m_broadcast_time; /**< How often do you broadcast messages */
  Time m_simulation_time;
  uint32_t m_n_channels;
  std::list<uint32_t> m_RangeOfChannels_Info;

   void iniciaCanales();
private:
  /** \brief This is an inherited function. Code that executes once the application starts
             */
  void StartApplication ();
 
  uint32_t m_packetSize; /**< Packet size in bytes */
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WaveNetDevice that is attached to this device */
  std::vector<NeighborInformation> m_neighbors; /**< A table representing neighbors of this node */
  std::list<ST_Reenvios> m_Paquetes_A_Reenviar;/**> Lista de paquetes a reenviar*/
  std::list<ST_Canales> m_Canales_disponibles;/**> Lista de paquetes a reenviar*/
  std::list<uint32_t> m_Canales_Para_Utilizar;
  //std::list<Ptr<Packet>> m_BufferA;
  std::list<std::list<Ptr<Packet>>> m_BuffersA;//lista que contiene listas que representan a los buffers de cada canal

  Time m_time_limit; /**< Time limit to keep neighbors in a list */
  Time m_Tiempo_de_reenvio;
  u_long m_semilla;
  WifiMode m_mode; /**< data rate used for broadcasts */
  uint32_t m_satisfaccionL;
  uint32_t m_satisfaccionG;
};
} // namespace ns3

#endif