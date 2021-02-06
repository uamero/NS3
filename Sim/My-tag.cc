/*
 * cacc-data-tag.cc
 *
 *  Created on: Oct 29, 2019
 *      Author: adil
 */
#include "My-tag.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MyCustomDataTag");
NS_OBJECT_ENSURE_REGISTERED (CustomDataTag);

CustomDataTag::CustomDataTag ()
{
  /*Al crear un objeto del tipo CustomDataTag se manda a llamar el constructor de clase
  creando un tag predefinido con las siguientes condiciones*/
  m_timestamp = Simulator::Now ();
  m_nodeId = -1;
  m_SEQNumber = 1;
  m_TypeOfPacket = 0;
  m_chanels=0;
}

CustomDataTag::~CustomDataTag ()
{
}

//Almost all custom tags will have similar implementation of GetTypeId and GetInstanceTypeId
TypeId
CustomDataTag::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::CustomDataTag").SetParent<Tag> ().AddConstructor<CustomDataTag> ();
  return tid;
}
TypeId
CustomDataTag::GetInstanceTypeId (void) const
{
  return CustomDataTag::GetTypeId ();
}

/** The size required for the data contained within tag is:
 * 		size needed for a ns3::Time for timestamp + 
 * 		size needed for a uint32_t for node id
 * 	    size needed for a uint32_t for sink node id
 * 		size needed for a u_long for a random sequence number
 */
uint32_t
CustomDataTag::GetSerializedSize (void) const
{
  return sizeof (ns3::Time) + sizeof (uint32_t) +
         sizeof (u_long) + sizeof (uint32_t) + sizeof (uint8_t);
}
/**
 * The order of how you do Serialize() should match the order of Deserialize()
 */
void
CustomDataTag::Serialize (TagBuffer i) const
{
  //we store timestamp first
  i.WriteDouble (m_timestamp.GetDouble ());
  //Then we store the node ID
  i.WriteU32 (m_nodeId);
  i.WriteU64 (m_SEQNumber);
  i.WriteU32 (m_TypeOfPacket);
  i.WriteU8(m_chanels);
}
/** This function reads data from a buffer and store it in class's instance variables.
 */
void
CustomDataTag::Deserialize (TagBuffer i)
{
  //We extract what we stored first, so we extract the timestamp
  m_timestamp = Time::FromDouble (i.ReadDouble (), Time::NS);
  //Extraemos el nodo del nodo creador del paquete
  m_nodeId = i.ReadU32 ();
  //Se extrae el numero de secuencia del paquete
  m_SEQNumber = i.ReadU64 ();
  //Se extrae el tipo del paquete
  m_TypeOfPacket = i.ReadU32 ();

  m_chanels=i.ReadU8();
}
/**
 * This function can be used with ASCII traces if enabled. 
 */
void
CustomDataTag::Print (std::ostream &os) const
{
  os << "Custom Data --- Node :" << m_nodeId << "\t(" << m_timestamp << ")";
}

//Your accessor and mutator functions


uint32_t
CustomDataTag::GetNodeId ()
{
  return this->m_nodeId;
}



Time
CustomDataTag::GetTimestamp ()
{
  return this->m_timestamp;
}
uint32_t
CustomDataTag::GetTypeOfPacket ()
{
  return this->m_TypeOfPacket;
}
u_long
CustomDataTag::GetSEQNumber ()
{
  return this->m_SEQNumber;
}
uint8_t
CustomDataTag::GetChanels(){
  return m_chanels;
}
void
CustomDataTag::SetTypeOfpacket (int32_t TType)
{
  this->m_TypeOfPacket = TType;
}
void
CustomDataTag::SetNodeId (uint32_t node_id) /*> Id del nodo que envia*/
{
  this->m_nodeId = node_id;
}
void
CustomDataTag::SetTimestamp (Time t)
{
  this->m_timestamp = t;
}
void
CustomDataTag::SetChanels(uint8_t chanels){
  m_chanels=chanels;
}

void
CustomDataTag::CopySEQNumber (u_long number)
{
  this->m_SEQNumber = number;
}

} /* namespace ns3 */
