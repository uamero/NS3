/*
 * cacc-data-tag.cc
 *
 *  Created on: Oct 29, 2019
 *      Author: adil
 */
#include "SecundariosTag.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SecundariosTag");
NS_OBJECT_ENSURE_REGISTERED (SecundariosDataTag);

SecundariosDataTag::SecundariosDataTag ()
{
  /*Al crear un objeto del tipo SecundariosDataTag se manda a llamar el constructor de clase
  creando un tag predefinido con las siguientes condiciones*/
  //m_timestamp = Simulator::Now ();
  
  m_timestamp = Time(0);
  m_nodeId = -1;
  m_SEQNumber = 1;
  m_chanels = 0;
  m_SL = 1;
}

SecundariosDataTag::~SecundariosDataTag ()
{
}

//Almost all custom tags will have similar implementation of GetTypeId and GetInstanceTypeId
TypeId
SecundariosDataTag::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::SecundariosDataTag").SetParent<Tag> ().AddConstructor<SecundariosDataTag> ();
  return tid; 
}
TypeId
SecundariosDataTag::GetInstanceTypeId (void) const
{
  return SecundariosDataTag::GetTypeId ();
}

/** The size required for the data contained within tag is:
 * 		size needed for a ns3::Time for timestamp + 
 * 		size needed for a uint32_t for node id
 * 	    size needed for a uint32_t for sink node id
 * 		size needed for a u_long for a random sequence number
 */
uint32_t
SecundariosDataTag::GetSerializedSize (void) const
{
  return sizeof (ns3::Time) + sizeof (uint32_t) +sizeof (uint32_t)+ sizeof (double) + sizeof (uint64_t) +
         sizeof (uint64_t);
}
/**
 * The order of how you do Serialize() should match the order of Deserialize()
 */
void
SecundariosDataTag::Serialize (TagBuffer i) const
{
  //we store timestamp first
  i.WriteDouble (m_timestamp.GetDouble ());
  //Then we store the node ID
  i.WriteU32 (m_nodeId);
  i.WriteU32 (m_nodeIdPrev);
  i.WriteU64 (m_SEQNumber);
  i.WriteU64 (m_chanels);
  i.WriteDouble (m_SL);
}
/** This function reads data from a buffer and store it in class's instance variables.
 */

void
SecundariosDataTag::Deserialize (TagBuffer i)
{
  //We extract what we stored first, so we extract the timestamp
  m_timestamp = Time::FromDouble (i.ReadDouble (), Time::NS);
  //Extraemos el nodo del nodo creador del paquete
  m_nodeId = i.ReadU32 ();
  
  m_nodeIdPrev = i.ReadU32 ();
  //Se extrae el numero de secuencia del paquete
  m_SEQNumber = i.ReadU64 ();

  m_chanels = i.ReadU64 ();

  m_SL = i.ReadDouble ();
}
/**
 * This function can be used with ASCII traces if enabled. 
 */

//Your accessor and mutator functions
void 
SecundariosDataTag::Print (std::ostream &os) const
{
  os << "Secundarios Data Tag--- Nodo fuente : " << m_nodeId << "\t SEQ: ("<<m_SEQNumber<< ")" <<"\t Retardo total: (" << m_timestamp.GetSeconds()  << ")" << " Satisfaccíon L: (" << m_SL << ")" << std::endl;
}

uint32_t
SecundariosDataTag::GetNodeId ()
{
  return this->m_nodeId;
}
uint32_t
SecundariosDataTag::GetNodeIdPrev ()
{
  return this->m_nodeIdPrev;
}

Time
SecundariosDataTag::GetTimestamp ()
{
  return this->m_timestamp;
}

void 
SecundariosDataTag::SetTimestamp(Time timeStamp){
  m_timestamp = timeStamp;
}

u_long
SecundariosDataTag::GetSEQNumber ()
{

  return this->m_SEQNumber;
}
uint64_t
SecundariosDataTag::GetChanels ()
{
  return m_chanels;
}
double
SecundariosDataTag::GetSL ()
{
  return m_SL;
}
void
SecundariosDataTag::SetSL (double SL)
{
  m_SL = SL;
}
void
SecundariosDataTag::SetNodeId (uint32_t node_id) /*> Id del nodo que creo el paquete*/
{
  this->m_nodeId = node_id;
}
void
SecundariosDataTag::SetNodeIdPrev (uint32_t node_idPrev) /*> Id del nodo previo que envia*/
{
  this->m_nodeIdPrev = node_idPrev;
}
/*void
SecundariosDataTag::SetTimestamp (Time t)
{
  this->m_timestamp = t;
}*/
void
SecundariosDataTag::SetChanels (uint64_t chanels)
{
  m_chanels = chanels;
}
void
SecundariosDataTag::SetSEQnumber (u_long SEQ)
{
  m_SEQNumber = SEQ;
}
} /* namespace ns3 */
