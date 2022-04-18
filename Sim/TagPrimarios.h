/*
 * Este tag pertenece a los nodos primarios de la simulación
 *  y continene unicamente los canales que ocupa el usuario primario 
 */

#ifndef TAGPrimarios_H
#define TAGPrimarios_H
#include "ns3/tag.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"

namespace ns3 {
/** We're creating a custom tag for simulation. A tag can be added to any packet, but you cannot add a tag of the same type twice.
	*/
class PrimariosDataTag : public Tag
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
  uint32_t GetnodeID();
  
  void SetNodeId(uint32_t ID);
  PrimariosDataTag ();
  virtual ~PrimariosDataTag ();

private:
  uint32_t m_nodeID;
};
} // namespace ns3

#endif