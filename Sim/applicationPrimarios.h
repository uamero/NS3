#ifndef PNODES_APPLICATION_H
#define PNODES_APPLICATION_H
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
  u_long numeroSEQ;
  uint32_t ID_Creador;
  uint32_t Tam_Paquete;
  Time Tiempo_ultimo_envio;
  int32_t tipo_de_paquete;
  bool Estado;
} ST_ReenviosP;

class CustomApplicationPnodes : public ns3::Application
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  CustomApplicationPnodes ();
  ~CustomApplicationPnodes ();

  /** \brief Broadcast some information 
             */
  void BroadcastInformation ();

  /** \brief This function is called when a net device receives a packet. 
             * I connect to the callback in StartApplication. This matches the signiture of NetDevice receive.
             */

  void SetBroadcastInterval (Time interval);

  /** \brief Change the data rate used for broadcasts.
             */
  void SetWifiMode (WifiMode mode);

  /** \brief Remove neighbors you haven't heard from after some time.
             */
  //You can create more functions like getters, setters, and others

  uint32_t Corrimientos (uint32_t registro);
  void GetCanales ();
  uint32_t m_n_channels; //esta es la cantidad de devices que tiene cada nodo
  void SetChannels (uint32_t channels);
  uint8_t m_porcentaje_Ch_disp;

private:
  /** \brief This is an inherited function. Code that executes once the application starts
             */
  void StartApplication ();
  Time m_broadcast_time; /**< How often do you broadcast messages */
  Ptr<WifiNetDevice> m_wifiDevice; /**< A WaveNetDevice that is attached to this device */
  WifiMode m_mode; /**< data rate used for broadcasts */
  std::string m_chs; //una cadena de 0 y 1 que representa la disponibilidad de los canales
};
} // namespace ns3

#endif