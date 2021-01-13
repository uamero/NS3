/*
 * cacc-data-tag.cc
 *
 *  Created on: Oct 29, 2019
 *      Author: adil
 */
#include "custom-data-tag.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CustomDataTag");
NS_OBJECT_ENSURE_REGISTERED (CustomDataTag);

CustomDataTag::CustomDataTag ()
{
  m_timestamp = Simulator::Now ();
  m_nodeId = -1;
  m_nodeSinkId = -1;
  m_SEQNumber = 1;
  m_channel=1;
}
CustomDataTag::CustomDataTag (uint32_t node_id, uint32_t nodeSink_id)
{
  m_timestamp = Simulator::Now ();
  m_nodeId = node_id;
  m_nodeSinkId = nodeSink_id;
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
  return sizeof (ns3::Time) + sizeof (uint32_t) + sizeof (uint32_t)+sizeof(uint32_t) + sizeof (u_long);
}
/**
 * The order of how you do Serialize() should match the order of Deserialize()
 */
void
CustomDataTag::Serialize (TagBuffer i) const
{
  //we store timestamp first
  i.WriteDouble (m_timestamp.GetDouble ());
  i.WriteU32 (m_nodeSinkId);
  i.WriteU32(m_channel);
  //Then we store the node ID
  i.WriteU32 (m_nodeId);
  i.WriteU64 (m_SEQNumber);
}
/** This function reads data from a buffer and store it in class's instance variables.
 */
void
CustomDataTag::Deserialize (TagBuffer i)
{
  //We extract what we stored first, so we extract the timestamp
  m_timestamp = Time::FromDouble (i.ReadDouble (), Time::NS);
  
  //Extraemos el id del sink
  m_nodeSinkId = i.ReadU32 ();
  //Se extrae el canal en el que se envio el dato
  m_channel=i.ReadU32();
  //we extract the node id
  m_nodeId = i.ReadU32 ();
  //Se extrae el numero de secuencia del paquete
  m_SEQNumber = i.ReadU64 ();
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
  return m_nodeId;
}

uint32_t
CustomDataTag::GetNodeSinkId ()
{
  return m_nodeSinkId;
}

Time
CustomDataTag::GetTimestamp ()
{
  return m_timestamp;
}

u_long
CustomDataTag::GetSEQNumber ()
{
  return m_SEQNumber;
}
u_int32_t
CustomDataTag::GetChannel ()
{
  return m_channel;
}
void
CustomDataTag::SetNodeId (uint32_t node_id)
{
  m_nodeId = node_id;
}
void
CustomDataTag::SetTimestamp (Time t)
{
  m_timestamp = t;
}

void
CustomDataTag::SetNodeSinkId (uint32_t sink_id)
{
  m_nodeSinkId = sink_id;
}

void
CustomDataTag::SetSEQNumber (int64_t sem)
{
  u_long m = 2147483648;
  u_long a = 314159269;
  u_long c = 453806247;
  sem = ((u_long) sem * a + c) % m;
  m_SEQNumber = (u_long) sem;
}

void
CustomDataTag::CopySEQNumber (u_long number)
{
  m_SEQNumber = number;
}
void
CustomDataTag::SetChannel (uint32_t channel)
{
  this->m_channel = channel;
}
} /* namespace ns3 */
