/*
 * cacc-data-tag.cc
 *
 *  Created on: Oct 29, 2019
 *      Author: adil
 */
#include "SinkTag.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SinkTag");
NS_OBJECT_ENSURE_REGISTERED (SinkDataTag);

SinkDataTag::SinkDataTag ()
{
  /*Al crear un objeto del tipo SinkDataTag se manda a llamar el constructor de clase
  creando un tag predefinido con las siguientes condiciones*/
  m_SEQNumber = 1;
  m_SG = 0;
}

SinkDataTag::~SinkDataTag ()
{
}

//Almost all custom tags will have similar implementation of GetTypeId and GetInstanceTypeId
TypeId
SinkDataTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SinkDataTag").SetParent<Tag> ().AddConstructor<SinkDataTag> ();
  return tid;
}
void 
SinkDataTag::Print (std::ostream &os) const
{
  
  os << "Sink Data Tag--- from Node :" << m_nodeID <<  "\t SEQN: (" << m_SEQNumber  << ")" << " Satisfación G:  (" << m_SG << ")" << std::endl;;
}

TypeId
SinkDataTag::GetInstanceTypeId (void) const
{
  return SinkDataTag::GetTypeId ();
}

/** The size required for the data contained within tag is:
 * 		size needed for a ns3::Time for timestamp + 
 * 		size needed for a uint32_t for node id
 * 	    size needed for a uint32_t for sink node id
 * 		size needed for a u_long for a random sequence number
 */
uint32_t
SinkDataTag::GetSerializedSize (void) const
{
  return sizeof (u_long) + sizeof (double) + sizeof(uint32_t);
}
/**
 * The order of how you do Serialize() should match the order of Deserialize()
 */
void
SinkDataTag::Serialize (TagBuffer i) const
{

  i.WriteU64 (m_SEQNumber);
  i.WriteDouble (m_SG);
  i.WriteU32(m_nodeID);
}
/** This function reads data from a buffer and store it in class's instance variables.
 */

void
SinkDataTag::Deserialize (TagBuffer i)
{

  //Se extrae el numero de secuencia del paquete
  m_SEQNumber = i.ReadU64 ();
  //Se extrae la satisfacción global
  m_SG = i.ReadDouble ();
  //Se lee el Id del nodo que envia el paquete
  m_nodeID = i.ReadU32();
}
/**
 * This function can be used with ASCII traces if enabled. 
 */

u_long
SinkDataTag::GetSEQNumber ()
{

  return this->m_SEQNumber;
}

void
SinkDataTag::SetSEQNumber (u_long number)
{
  this->m_SEQNumber = number;
}

double
SinkDataTag::GetSG()
{
  return m_SG;
}
void
SinkDataTag::SetSG (double sg)
{
  m_SG = sg;
}
uint32_t
SinkDataTag::GetNodeId(){
  return m_nodeID;
}
void
SinkDataTag::SetNodeId(uint32_t id){
m_nodeID = id;
}
} /* namespace ns3 */
