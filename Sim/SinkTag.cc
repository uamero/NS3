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
  m_timestamp = Time (0);
  m_BufferRoute = new uint8_t[5];
  m_SizeBufferRoute = 5;
  m_SG = 0;
  m_CopiaID = 0;
  // std::cout<<"Si salgo"<<std::endl;
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
  std::string ruta = std::string (m_BufferRoute, m_BufferRoute + m_SizeBufferRoute);
  os << " Sink Data Tag--- from Node :" << m_nodeID << "\t SEQN: (" << m_SEQNumber << ")"
     << " Copia: " << m_CopiaID << " Satisfación G:  (" << m_SG << ")"
     << " Retardo total: (" << m_timestamp.GetSeconds () << ")"
     << " Ruta: " << ruta << std::endl;
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
  return sizeof (uint64_t) + sizeof (double) + sizeof (uint32_t) + /*sizeof (ns3::Time)*/sizeof (double) +
         sizeof (uint32_t) /*+ sizeof (uint8_t *)*/ + m_SizeBufferRoute + sizeof (uint32_t);
}
/**
 * The order of how you do Serialize() should match the order of Deserialize()
 */
void
SinkDataTag::Serialize (TagBuffer i) const
{

  i.WriteU64 (m_SEQNumber);
  i.WriteDouble (m_SG);
  i.WriteU32 (m_nodeID);
  i.WriteDouble (m_timestamp.GetDouble ());
  i.WriteU32 (m_CopiaID);
  i.WriteU32 (m_SizeBufferRoute);
  i.Write (m_BufferRoute, m_SizeBufferRoute);
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
  m_nodeID = i.ReadU32 ();

  m_timestamp = Time::FromDouble (i.ReadDouble (), Time::NS);

  m_CopiaID = i.ReadU32 ();

  m_SizeBufferRoute = i.ReadU32 ();

  m_BufferRoute = new uint8_t[m_SizeBufferRoute];

  i.Read (m_BufferRoute, m_SizeBufferRoute);
}
/**
 * This function can be used with ASCII traces if enabled. 
 */

u_long
SinkDataTag::GetSEQNumber ()
{

  return this->m_SEQNumber;
}
Time
SinkDataTag::GetTimestamp ()
{
  return m_timestamp;
}
uint32_t
SinkDataTag::GetcopiaID ()
{
  return m_CopiaID;
}
void
SinkDataTag::setCopiaID (uint32_t CopiaID)
{
  m_CopiaID = CopiaID;
}
void
SinkDataTag::SetSEQNumber (u_long number)
{
  this->m_SEQNumber = number;
}

double
SinkDataTag::GetSG ()
{
  return m_SG;
}
void
SinkDataTag::SetSG (double sg)
{
  m_SG = sg;
}
uint32_t
SinkDataTag::GetNodeId ()
{
  return m_nodeID;
}
void
SinkDataTag::SetNodeId (uint32_t id)
{
  m_nodeID = id;
}
void
SinkDataTag::SetTimestamp (Time delay)
{
  m_timestamp = delay;
}
uint8_t *
SinkDataTag::GetBufferRoute ()
{
  return m_BufferRoute;
}
uint32_t
SinkDataTag::GetSizeBufferRoute ()
{
  return m_SizeBufferRoute;
}
void
SinkDataTag::SetSizeBufferRoute (uint32_t SizeBuffer)
{
  m_SizeBufferRoute = SizeBuffer;
}
void
SinkDataTag::SetBufferRoute (uint8_t *buffer)
{
  m_BufferRoute = buffer;
}
} /* namespace ns3 */
