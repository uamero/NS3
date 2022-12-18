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

/*typedef struct
{
  u_long numeroSEQ;
  uint32_t ID_Creador;
  uint32_t Tam_Paquete;
  Time Tiempo_ultimo_envio;
  int32_t tipo_de_paquete;
  std::string Ruta;
  bool Estado;
} ST_ReenviosB;*/
typedef struct
{
  Mac48Address MacaddressNB; //ID del vecino[]
  uint32_t NodoID;
  double SL_canal;
  Time last_beacon; //Tiempo del último mensaje
  uint32_t canal; //por que canal lo envio

  // aqui se meten los demas parametros
} ST_VecinosB;

typedef struct
{
  Ptr<Packet> m_packet;
  Time Tiempo_ultimo_envio;
  Time retardo;
  double V_Preferencia;
  uint32_t m_Cantidad_n_visitados;
} ST_ReenviosB;
typedef struct
{
  double N_EE, N_CD, N_NV,
      N_retardo; //CD-> Canales disponibles EE->Efectividad de entrega NV->Nodos Visitados retardo->Tiempo en este nodo
} ST_MatrizR;
typedef struct
{
  std::string m_chanels;
  Time Tiempo_ultima_actualizacion;
  uint32_t ID_Persive;
} ST_CanalesB;
typedef struct
{
  Ptr<Packet> m_packet;
  Time m_TimeTosavedOnBuffer;
  bool
      m_Send; //Si es True el paquete debe ser enviado de lo contrario su información debe ser leída
} ST_PacketInBufferB;

typedef struct
{
  bool m_visitado_Enviar;
  bool m_visitado_Guardar;
  std::list<ST_PacketInBufferB> m_PacketAndTime;
} ST_bufferOfCannelsB;

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
  void ImprimeTabla ();
  void Imprimebuffers ();
  /** \brief This function is called when a net device receives a packet. 
             * I connect to the callback in StartApplication. This matches the signiture of NetDevice receive.
             */
  bool ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                      const Address &sender);

  void SetBroadcastInterval (Time interval);
  Time GetBroadcastInterval ();

  /** \brief Update a neighbor's last contact time, or add a new neighbor
             */
  void UpdateNeighbor (ST_VecinosB);
  /** \brief Print a list of neighbors
             */
  void PrintNeighbors ();

  /** \brief Change the data rate used for broadcasts.
             */
  void SetWifiMode (WifiMode mode);
  void SendPacket ();
  /** \brief Remove neighbors you haven't heard from after some time.
             */
  void RemoveOldNeighbors ();
  void ConfirmaEntrega (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia);
  //You can create more functions like getters, setters, and others
  bool BuscaSEQEnTabla (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia, double SL);
  double BuscaPCKEnTabla (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia);
  void Guarda_Paquete_reenvio (Ptr<Packet> paquete, Time TimeBuff, uint32_t nodosV);
  void CanalesDisponibles ();
  /*Se actualiza o bien se agregan los canales que los usarios primarios ocupan del espectro */
  bool BuscaCanalesID (std::string ch, uint32_t ID, Time timD);
  bool VerificaCanal (uint32_t ch);

  uint32_t m_n_channels;
  std::list<ST_ReenviosB> m_Paquetes_A_Reenviar; /**> Lista de paquetes a reenviar*/
  std::list<ST_ReenviosB>
      m_Paquetes_Recibidos; /**> Lista de paquetes confirmados de entrega por el sink*/
  std::list<uint32_t> m_RangeOfChannels_Info;
  double m_Batery;
  void iniciaCanales ();
  void ReadPacketOnBuffer ();
  bool BuscaPaquete ();
  bool Entregado (u_long SEQ, uint32_t IDcreador, uint32_t IDCopia);
  void CheckBuffer ();
  void CreaBuffersCanales ();
  bool
  VerificaVisitados_Enviar (); //funcion para iterar sobre todos los canales y ver si ya fueron visitados
  bool
  VerificaVisitados_Guardar (); //funcion para iterar sobre todos los canales y ver si ya fueron visitados
  void ReiniciaVisitados_Enviar (); //funcion para comenzar la iteraci[n desde el primer canal
  void ReiniciaVisitados_Guardar (); //funcion para comenzar la iteraci[n desde el primer canal
  std::string operacionORString (std::string str1, std::string str2);
  void SetChannels (uint32_t channels);
  void VerificaNodoEntrega (std::string ruta);
  uint32_t VerificaNodosVisitados (std::string ruta);
  uint32_t m_collissions;

private:
  /** \brief This is an inherited function. Code that executes once the application starts
             */
  void StartApplication ();
  void CalculaX_R_V ();
  Time m_broadcast_time; /**< How often do you broadcast messages */
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WaveNetDevice that is attached to this device */
  std::list<ST_CanalesB> m_Canales_disponibles; /**> Lista de paquetes a reenviar*/
  std::list<ST_VecinosB> m_vecinos_list; /**> Lista de paquetes a reenviar*/
  std::list<uint32_t> m_Canales_Para_Utilizar;
  Time m_time_limit; /**< Time limit to keep neighbors in a list */
  WifiMode m_mode; /**< data rate used for broadcasts */
  std::list<ST_bufferOfCannelsB>
      m_bufferB; //lista que contiene listas que representan a los buffers de cada canal
  std::list<ST_PacketInBufferB> m_memoryB;
  double m_satisfaccionL;
  double m_satisfaccionG;
  double m_n_pckdv; //numero de paquetes que han pasado por este nodo y se han entregado
  uint32_t m_n_max_vecinos; //mayor numero de vecinos registrados durante la simulacion
  double m_n_entregados;
  uint32_t m_NTP;
  uint32_t m_NVmax;
  double m_retardo_acumulado;
  double m_RetardoMax;
  double m_W[4] = {0.4, 0.3, 0.2, 0.1};
};
} // namespace ns3

#endif