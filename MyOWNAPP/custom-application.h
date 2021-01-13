#ifndef WAVETEST_CUSTOM_APPLICATION_H
#define WAVETEST_CUSTOM_APPLICATION_H
#include "ns3/application.h"
#include "ns3/wave-net-device.h"
#include "ns3/wifi-phy.h"
/*##########################*/
#include <vector>
#include <stdio.h>

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
  u_long SourcePacketID;
  u_long SEQnumberTAG;
  int64_t lastBeacon;
} PreviousTags;
typedef struct
{
  u_long SourcePacketID;
  u_long SEQnumberTAG;
  Time TimeStamp;
  std::string Message;
} MessagesToBuffer;
typedef struct
{
  uint32_t _NPQ_WAS_Created;/*Numero de paquetes que fueron creados en el nodo*/
  uint32_t _NPQ_WAS_Received;/*Numero de paquetes que el nodo recibio durante la simulación*/
  uint32_t _NPQ_WAS_ReSend;/*Numero de paquetes que reenvio el nodo*/
  uint32_t _NPQ_WAS_Duplicated;/*Numero de paquetes duplicados*/
  Time delayOnNode; /*Retardo de los paquetes recibidos*/
} PacketsInformation;

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
  /*DATAtags net device receives a packet. 
             * I connect to the callback in StartApplication. This matches the signiture of NetDevice receive.
             */
  bool ReceivePacket (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                      const Address &sender);

  void PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx, MpduInfo mpdu,
                  SignalNoiseDbm sn);

  void SetBroadcastInterval (Time interval);

  /** \brief Update a neighbor's last contact time, or add a new neighbor
             */
  void UpdateNeighbor (Mac48Address addr);
  /** \brief Print a list of neighbors
             */
  void PrintNeighbors ();

  void PrintPrevSEQnumbers();
  /** \brief Change the data rate used for broadcasts.
             */
  void SetWifiMode (WifiMode mode);

  /** \brief Remove neighbors you haven't heard from after some time.
             */
  void RemoveOldNeighbors ();

  //You can create more functions like getters, setters, and others
  bool FindTags (u_long PrevTag, Time lastBEacon, u_long sourceID);

  void RemoveOldTags ();

  void setSenderOrSink (u_short value);

  void SaveMessageOnBuffer(std::string message,u_long SourcePacketID,u_long SEQnumberTAG,Time TimeStamp);

  std::vector<MessagesToBuffer>::iterator GetMessageOnBuffer();

  void printMessageOnBuffer();

  u_short GetSenderOrSink ();
  
  void SetSinkID(u_short sinkID);

  u_short GetSinkID();
  PacketsInformation m_ControlPakets;/*> Contiene multiples contadores que permiten administrar 
                                        el numero de paquetes creados,eliminados, reenviados, etc.*/
  u_int32_t m_NumberOfPacketsOK; /*< Contador de paquetes recibidos correctamente*/
  std::string m_NextData;/*< Dato para reenviar */
private:
  /** \brief This is an inherited function. Code that executes once the application starts
             */
  void StartApplication ();
  Time m_broadcast_time; /**< How often do you broadcast messages */
  uint32_t m_packetSize; /**< Packet size in bytes */
  
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WifiNetDevice that is attached to this device */
  std::vector<Ptr<WifiNetDevice>> m_Channels;/*>Almacena los punteros de los multiples WifiNetDevice en los nodos*/
  std::vector<NeighborInformation> m_neighbors; /**< A table representing neighbors of this node */
  std::vector<PreviousTags> m_previusTags;
  Time m_time_limit; /**< Time limit to keep neighbors in a list */
  
  WifiMode m_mode; /**< data rate used for broadcasts */
  //You can define more stuff to record statistics, etc.
  
  u_short m_SenderOrSink; /*< Identificador para diferenciar de un nodo Sink y un Sender*/
  u_short m_IDSink;/*> Nodo que actua como Sink*/
 
  std::vector <MessagesToBuffer> m_buffer; /*Mensajes almacenados en el buffer*/
  std::string m_sms1;/*>Mensaje de auxilio 1: Ayuda! edificio caido mensaje 1!*/
  std::string m_sms2;/*>Mensaje de auxilio 2: Ayuda! incendio grave mensaje 2!*/
  std::string m_sms3;/*>Mensaje de auxilio 3: Ayuda! inundación mensaje 3!*/
  std::string m_sms4;/*>Mensaje de auxilio 4: Ayuda! exploción con multiples heridos mensaje 4!*/
};
} // namespace ns3

#endif