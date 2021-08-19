/*
 * cacc-data-tag.cc
 *
 *  Created on: Oct 29, 2019
 *      Author: adil
 */
#include "TagPrimarios.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PrimariosTag");
NS_OBJECT_ENSURE_REGISTERED (PrimariosDataTag);

PrimariosDataTag::PrimariosDataTag ()
{
  /*Al crear un objeto del tipo PrimariosDataTag se manda a llamar el constructor de clase
  creando un tag predefinido con las siguientes condiciones*/

  m_chanels = 0;
}

PrimariosDataTag::~PrimariosDataTag ()
{
}

//Almost all custom tags will have similar implementation of GetTypeId and GetInstanceTypeId
TypeId
PrimariosDataTag::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::PrimariosDataTag").SetParent<Tag> ().AddConstructor<PrimariosDataTag> ();
  return tid;
}
TypeId
PrimariosDataTag::GetInstanceTypeId (void) const
{
  return PrimariosDataTag::GetTypeId ();
}

/** The size required for the data contained within tag is:
 * 		size needed for a ns3::Time for timestamp + 
 * 		size needed for a uint32_t for node id
 * 	    size needed for a uint32_t for sink node id
 * 		size needed for a u_long for a random sequence number
 */
uint32_t
PrimariosDataTag::GetSerializedSize (void) const
{
  /** Para este tag unicamente se requiere una variable de 64bits para almacenar los canales*/
  return sizeof (uint64_t) + sizeof (uint32_t);
}
/**
 * The order of how you do Serialize() should match the order of Deserialize()
 */
void
PrimariosDataTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_nodeID);
  i.WriteU64 (m_chanels);
}
/** This function reads data from a buffer and store it in class's instance variables.
 */

void
PrimariosDataTag::Deserialize (TagBuffer i)
{
  m_nodeID = i.ReadU32 ();
  m_chanels = i.ReadU64 ();
}

//Your accessor and mutator functions

uint64_t
PrimariosDataTag::GetChanels ()
{
  return m_chanels;
}
uint32_t
PrimariosDataTag::GetnodeID ()
{
  return m_nodeID;
}
void
PrimariosDataTag::SetChanels (uint64_t chanels)
{
  m_chanels = chanels;
}
void
PrimariosDataTag::SetNodeId (uint32_t nodeId)
{
  m_nodeID = nodeId;
}

} /* namespace ns3 */
