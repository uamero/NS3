/*
 * This tag combines position, velocity, and acceleration in one tag.
 */

#ifndef Sink_TAG_H
#define Sink_TAG_H
#include "ns3/tag.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"

namespace ns3 {
/** We're creating a custom tag for simulation. A tag can be added to any packet, but you cannot add a tag of the same type twice.
	*/
class SinkDataTag : public Tag
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

  u_long GetSEQNumber ();
  double GetSG();
  uint32_t GetNodeId();
  //void SetPosition (Vector pos);
  void SetTimestamp (Time t);
  void SetSEQNumber (u_long number);
  void SetSG(double sg);
  void SetNodeId(uint32_t id);
  SinkDataTag ();
  virtual ~SinkDataTag ();
  
private:
  u_long m_SEQNumber; /**> Numero de secuencia del paquete del cual se envia el ACK*/
  double m_SG; /**> Valor de la satisfacción global*/
  uint32_t m_nodeID;
};
} // namespace ns3

#endif