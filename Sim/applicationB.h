#ifndef BNODES_APPLICATION_H
#define BNODES_APPLICATION_H
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
} NeighborInformationB;

typedef struct
{
  u_long numeroSEQ;
  uint32_t ID_Creador;
  uint32_t Tam_Paquete;
  Time Tiempo_ultimo_envio;
  int32_t tipo_de_paquete;
  std::string Ruta;
  bool Estado;
} ST_ReenviosB;
typedef struct 
{
  uint8_t m_chanels;
  Time Tiempo_ultima_actualizacion;
  uint32_t ID_Persive;
}ST_CanalesB;

class CustomApplicationBnodes : public ns3::Application
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  CustomApplicationBnodes ();
  ~CustomApplicationBnodes ();

  /** \brief Broadcast some information 
             */
  void BroadcastInformation ();
  void ImprimeTabla();
  /** \brief This function is called when a net device receives a packet. 
             * I connect to the callback in StartApplication. This matches the signiture of NetDevice receive.
             */
  bool ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                      const Address &sender);

  void SetBroadcastInterval (Time interval);
  Time GetBroadcastInterval ();

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
  bool VerificaSEQRecibido(u_long SEQ);
  void ConfirmaEntrega(u_long SEQ);
  //You can create more functions like getters, setters, and others
  bool BuscaSEQEnTabla(u_long SEQ);
  void Guarda_Paquete_reenvio(u_long SEQ,uint32_t ID_Creador,uint32_t tam_del_paquete,
  Time timeStamp,int32_t type,std::string Ruta);
  std::list<ST_ReenviosB>::iterator GetReenvio();
  void CanalesDisponibles();
  /*Se actualiza o bien se agregan los canales que los usarios primarios ocupan del espectro */
  bool BuscaCanalesID(uint8_t ch,uint32_t ID,Time timD);
  bool VerificaCanal(uint8_t ch);
  uint32_t m_n_channels;
  std::list<ST_ReenviosB> m_Paquetes_A_Reenviar;/**> Lista de paquetes a reenviar*/
  std::list<ST_ReenviosB> m_Paquetes_Recibidos;/**> Lista de paquetes confirmados de entrega por el sink*/
  std::list<uint32_t> m_RangeOfChannels_Info;
  double m_Batery;
  void iniciaCanales();
private:
  /** \brief This is an inherited function. Code that executes once the application starts
             */
  void StartApplication ();
  Time m_broadcast_time; /**< How often do you broadcast messages */
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WaveNetDevice that is attached to this device */
  std::vector<NeighborInformationB> m_neighbors; /**< A table representing neighbors of this node */
  std::list<ST_CanalesB> m_Canales_disponibles;/**> Lista de paquetes a reenviar*/
  std::list<uint32_t> m_Canales_Para_Utilizar;
  Time m_time_limit; /**< Time limit to keep neighbors in a list */
  WifiMode m_mode; /**< data rate used for broadcasts */
};
} // namespace ns3

#endif