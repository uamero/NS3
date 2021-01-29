/*
 * This tag combines position, velocity, and acceleration in one tag.
 */

#ifndef MY_TAG_H
#define MY_TAG_H
#include "ns3/tag.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"

namespace ns3 {
/** We're creating a custom tag for simulation. A tag can be added to any packet, but you cannot add a tag of the same type twice.
	*/
class CustomDataTag : public Tag
{
public:
  //Functions inherited from ns3::Tag that you have to implement.
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  //These are custom accessor & mutator functions
  //Vector GetPosition(void);
  uint32_t GetNodeId (); /*> Id del nodo que envia*/
  Time GetTimestamp ();
  uint32_t GetNodeSinkId (); /*> Id del nodo que recibira el paquete*/
  u_long GetSEQNumber ();
  uint32_t GetChannel ();
  u_int32_t GetTypeOfPacket ();
  //void SetPosition (Vector pos);
  void SetNodeId (uint32_t node_id); /*> Id del nodo que envia*/
  void SetTimestamp (Time t);
  void CopySEQNumber (u_long number);
  void SetTypeOfpacket (int32_t TType);
  CustomDataTag ();
  virtual ~CustomDataTag ();

private:
  uint32_t m_nodeId; /**> Id del nodo creador*/
  Time m_timestamp; /**> Tiempo en el que el paquete se creó*/
  u_long m_SEQNumber; /**> Numero de secuencia que identifica al paquete*/
  uint32_t m_TypeOfPacket; /**> 0 Paquete nuevo, 1 reenvio de paquete, 2 respuesta del Sink*/
};
} // namespace ns3

#endif