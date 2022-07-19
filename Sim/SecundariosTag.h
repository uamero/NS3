/*
 * This tag combines position, velocity, and acceleration in one tag.
 */

#ifndef SEC_TAG_H
#define SEC_TAG_H
#include "ns3/tag.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"

namespace ns3 {
/** We're creating a custom tag for simulation. A tag can be added to any packet, but you cannot add a tag of the same type twice.
	*/
class SecundariosDataTag : public Tag
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
  void SetTimestamp (Time timeStamp);
  uint32_t GetNodeSinkId (); /*> Id del nodo que recibira el paquete*/
  u_long GetSEQNumber ();
  uint64_t GetChanels ();
  double GetSL(); 
  uint32_t GetNodeIdPrev();
  uint8_t* GetBufferRoute();
  uint32_t GetSizeBufferRoute();
  //void SetPosition (Vector pos);
  void SetNodeId (uint32_t node_id); /*> Id del nodo que envia*/
  void SetNodeIdPrev (uint32_t node_idPrev); /*> Id del nodo que envia*/
  //void SetTimestamp (Time t);
  void SetChanels (uint64_t chanels);
  void SetSEQnumber(uint64_t SEQ);
  void SetSL(double SL);
  void SetBufferRoute(uint8_t* buffer);
  void SetSizeBufferRoute(uint32_t SizeBuffer);
  SecundariosDataTag ();
  virtual ~SecundariosDataTag ();
  uint32_t GetcopiaID();
  void setCopiaID(uint32_t CopiaID); 

private:
  uint32_t m_nodeId; /**> Id del nodo creador*/
  uint32_t m_nodeIdPrev; /**> Id del nodo creador*/
  Time m_timestamp; /**> Tiempo en el que el paquete se creó*/
  uint64_t m_SEQNumber; /**> Numero de secuencia que identifica al paquete*/
  uint64_t m_chanels;
  double m_SL;//Valor de la satisfacción local previa de este paquete
  uint8_t *m_BufferRoute;
  uint32_t m_SizeBufferRoute;
  uint32_t m_CopiaID;
};
} // namespace ns3

#endif