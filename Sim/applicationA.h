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
  Mac48Address MacaddressNB; //ID del vecino
  uint32_t NodoID;
  double SL_canal;
  Time last_beacon; //Tiempo del último mensaje
  uint32_t canal; //por que canal lo envio
} ST_VecinosA;

typedef struct
{
  u_long numeroSEQ;
  uint32_t ID_Creador;
  uint32_t Tam_Paquete;
  uint32_t m_IDcopia;
  Time Tiempo_ultimo_envio;
  Time Tiempo_primer_envio;
  Time Tiempo_de_recibo_envio;
  uint32_t NumeroDeEnvios;
  double SG;
  bool Estado; /**> true -> el paquete ha sido entregado*/
} ST_Paquete_A_Enviar;
typedef struct
{
  Ptr<Packet> m_packet;
  Time Tiempo_ultimo_envio;
  Time retardo;
} ST_Reenvios; //Esta estructura sirve para almacenar los paquetes
    //a reenviar dentro de la memoria del nodo y acumula el retardo que se acumula en el paquete
typedef struct
{
  std::string m_chanels; //esta variable sirve para limitar los canales que se reciben
  Time Tiempo_ultima_actualizacion;
  uint32_t ID_Persive; //El Id que se persive
} ST_Canales;
typedef struct
{

  Ptr<Packet> m_packet;
  Time m_TimeTosavedOnBuffer;
  bool
      m_Send; //Si es True el paquete debe ser enviado de lo contrario su información debe ser leída
} ST_PacketInBufferA;

typedef struct
{
  bool m_visitado_Enviar;
  bool m_visitado_Guardar;
  std::list<ST_PacketInBufferA> m_PacketAndTime;
} ST_bufferOfCannelsA;

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
  void StoApplication();
  /** \brief Broadcast some information 
             */
  void BroadcastInformation ();
  void IniciaTabla (uint32_t PQts_A_enviar, uint32_t NodoID);
  void ImprimeTabla ();
  /** \brief This function is called when a net device receives a packet. 
             * I connect to the callback in StartApplication. This matches the signiture of NetDevice receive.
             */
  bool ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                      const Address &sender);

  void SetBroadcastInterval (Time interval);

  /** \brief Update a neighbor's last contact time, or add a new neighbor
             */
  void UpdateNeighbor (ST_VecinosA neighbor);
  /** \brief Print a list of neighbors
             */
  void PrintNeighbors ();

  /** \brief Change the data rate used for broadcasts.
             */
  void SetWifiMode (WifiMode mode);
  void SendPacket ();
  bool
  VerificaVisitados_Enviar (); //funcion para iterar sobre todos los canales y ver si ya fueron visitados
  bool
  VerificaVisitados_Guardar (); //funcion para iterar sobre todos los canales y ver si ya fueron visitados
  void ReiniciaVisitados_Enviar (); //funcion para comenzar la iteraci[n desde el primer canal
  void ReiniciaVisitados_Guardar (); //funcion para comenzar la iteraci[n desde el primer canal
  /** \brief Remove neighbors you haven't heard from after some time.
             */
  void Imprimebuffers ();
  void RemoveOldNeighbors ();
  void SetMAxtime (Time Maxtime);
  void ReadPacketOnBuffer ();
  bool BuscaPaquete ();
  void CheckBuffer ();
  Time GetMAxtime ();
  void ConfirmaEntrega (u_long SEQ, Time delay, uint32_t IDcreador, uint32_t CopiaID,double SG);
  /*bool
  VerificaVisitados (); //funcion para iterar sobre todos los canales y ver si ya fueron visitados
  void ReiniciaVisitados (); //funcion para comenzar la iteraci[n desde el primer canal*/
  //You can create more functions like getters, setters, and others
  bool BuscaSEQEnTabla (u_long SEQ, uint32_t ID_creador,uint32_t CopiaID);

  void CanalesDisponibles ();
  void CreaBuffersCanales ();
  /*Se actualiza o bien se agregan los canales que los usarios primarios ocupan del espectro */
  bool BuscaCanalesID (std::string ch, uint32_t ID, Time timD);
  bool VerificaCanal (uint32_t ch);
  std::string ObtenDAtosNodo ();
  Mac48Address GetNextHop (uint32_t channel);
  std::list<ST_Paquete_A_Enviar> m_Tabla_paquetes_A_enviar; /**> Lista de paquetes a enviar*/
  Time m_broadcast_time; /**< How often do you broadcast messages */
  Time m_simulation_time;
  uint32_t m_n_channels;
  std::list<uint32_t> m_RangeOfChannels_Info;
  bool Entregado (u_long SEQ,uint32_t IDcreador,uint32_t IDCopia);
  void iniciaCanales ();
  std::list<ST_Reenvios>
      m_Paquetes_Recibidos; /**> Lista de paquetes provenientes de otros nodos alarmados*/
  std::string operacionORString (std::string str1, std::string str2);
  uint32_t m_collissions;
  void SetChannels(uint32_t channels);
  bool m_termina; 
private:
  /** \brief This is an inherited function. Code that executes once the application starts
             */
  void StartApplication ();
  uint32_t m_packetSize; /**< Packet size in bytes */
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WaveNetDevice that is attached to this device */
  std::list<ST_Canales> m_Canales_disponibles; /**> Lista de paquetes a reenviar*/
  std::list<uint32_t> m_Canales_Para_Utilizar;
  std::list<ST_VecinosA> m_vecinos_list; //lista de vecinos
  std::list<ST_bufferOfCannelsA>
      m_bufferA; //lista que contiene listas que representan a los buffers de cada canal

  Time m_time_limit; /**< Time limit to keep neighbors in a list */
  Time m_Tiempo_de_reenvio;
  WifiMode m_mode; /**< data rate used for broadcasts */
  uint32_t m_satisfaccionL;
  uint32_t m_satisfaccionG;
  EventId m_sendEvent;
  double m_retardo_acumulado;
};
} // namespace ns3

#endif