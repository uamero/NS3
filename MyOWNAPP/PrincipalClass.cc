#include "ns3/wave-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "custom-application.h"
#include "ns3/netanim-module.h"
#include "ns3/gnuplot.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#define YELLOW_CODE "\033[33m"
#define TEAL_CODE "\033[36m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

using namespace ns3;
//NS_LOG_COMPONENT_DEFINE ("OwnSimulation");
void
SomeEvent ()
{

  for (uint32_t i = 0; i < NodeList::GetNNodes (); i++)
    {
      Ptr<Node> n = NodeList::GetNode (i);
      Ptr<CustomApplication> c_app = DynamicCast<CustomApplication> (n->GetApplication (0));
      c_app->SetWifiMode (WifiMode ("OfdmRate3MbpsBW10MHz"));
    }
  std::cout << "******************" << std::endl;
}
void
PhyRxDropTrace (std::string context, Ptr<const Packet> packet,
                WifiPhyRxfailureReason reason) //this was added in ns-3.30
{
  //What to do when packet loss happened.
  //Suggestion: Maybe check some packet tags you attached to the packet

  std::cout << context << std::endl;
}
void
Rx (std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector,
    MpduInfo aMpdu, SignalNoiseDbm signalNoise)
{
  //context will include info about the source of this event. Use string manipulation if you want to extract info.
  std::cout << BOLD_CODE << context << END_CODE << std::endl;
  //Print the info.
  std::cout << "\tSize=" << packet->GetSize () << " Freq=" << channelFreqMhz
            << " Mode=" << txVector.GetMode () << " Signal=" << signalNoise.signal
            << " Noise=" << signalNoise.noise << std::endl;

  //We can also examine the WifiMacHeader
  WifiMacHeader hdr;
  if (packet->PeekHeader (hdr))
    {
      std::cout << "\tDestination MAC : " << hdr.GetAddr1 () << "\tSource MAC : " << hdr.GetAddr2 ()
                << std::endl;
    }
}
int
main (int argc, char *argv[])
{
  
  // datos<<"Hola1"<<std::end;
  CommandLine cmd;
  uint32_t n = 5; //Numero de nodos en la red
  double simTime = 20; //Tiempo de simulación
  double interval = 0.5; //intervalo de broadcast
  uint16_t N_channels = 5; //valor preestablecido para los canales

  cmd.AddValue ("t", "Simulation Time", simTime);
  cmd.AddValue ("i", "Broadcast interval in seconds", interval);
  cmd.AddValue ("n", "Number of nodes", n);
  cmd.Parse (argc, argv);


  
  // Gnuplot gnuplot = Gnuplot ("reference-rates.png");
  NodeContainer nodos;
  nodos.Create (n); //se crean n nodos

  /*Se configura el modelo de movilidad*/
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator", "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"), "Rho",
                                 StringValue ("ns3::UniformRandomVariable[Min=0|Max=40]"));
  /*mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("2s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|100|0|100"));*/

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (nodos);
  /*Termina la configuración de la movilidad*/

  /*Configuración del canal(canales)*/
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  
  /*Termina conf. del canal*/

  /*Se configura la capa fisica del dispositivo*/
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  /*Termina capa fisica*/

  /*Se crea el canal y se configura la capa física*/
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.SetPcapDataLinkType (
      WifiPhyHelper::DLT_IEEE802_11_RADIO); /*Añade informacion adicional sobre el enlace*/
  //Aqui se pueden generar los archivos .pcap par wireshark
  wifiPhy.Set ("TxPowerStart", DoubleValue (5));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (33));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (8));
  /*Termina configuración*/

  /*Comienza la configuración de la capa de enlace*/

  WifiMacHelper wifiMac; //Permite crear nodo ad-hoc
  wifiMac.SetType ("ns3::AdhocWifiMac");
  
  WifiHelper wifi;
  //wifi.SetStandard (WIFI_STANDARD_80211g);
  /*wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "ControlMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "NonUnicastMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"));*/
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "ControlMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "NonUnicastMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"));
  for (uint16_t i = 0; i < N_channels; i++)
    {
      wifi.Install (wifiPhy, wifiMac, nodos);
    }
  u_short SinkID = 4;
  //NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodos);
  /*Termina configuración de la capa de enlace*/

  /*Comienza la instalación de la aplicación en los nodos*/
  for (uint32_t i = 0; i < nodos.GetN (); i++)
    {
      //std::ostringstream oss;
      Ptr<CustomApplication> app_i = CreateObject<CustomApplication> ();
      app_i->SetBroadcastInterval (Seconds (interval));
      app_i->SetStartTime (Seconds (0));
      app_i->SetStopTime (Seconds (simTime));
      if (i == 0)
        {
          app_i->setSenderOrSink (1);
        }
      else if (i == SinkID)
        {
          app_i->setSenderOrSink (2);
        }

      nodos.Get (i)->AddApplication (app_i);
    }

  /*for (uint32_t i = 0; i < nodos.GetN (); i++)
    {
      ObjectFactory fact;
      fact.SetTypeId ("ns3::CustomApplication");
      fact.Set ("Interval", TimeValue (Seconds (interval)));
      Ptr<CustomApplication> appI = fact.Create<CustomApplication> ();
      appI->SetStartTime (Seconds (0));
      appI->SetStopTime (Seconds (0));
      appI->SetSinkID(SinkID);
      if(i==SinkID){
        appI->setSenderOrSink(2);
      }
      else{
        appI->setSenderOrSink(1);
      }
      nodos.Get (i)->AddApplication (appI);
    }*/
  /*Termina instalación de la aplicación*/

  /*Se seleccionan los nodos que envian y el sink*/
  /*Ptr<CustomApplication> c_app = DynamicCast<CustomApplication> (nodos.Get (4)->GetApplication (0));
  c_app->setSenderOrSink (2); //Valor para ser sink
  Ptr<CustomApplication> c_appp =
      DynamicCast<CustomApplication> (nodos.Get (0)->GetApplication (0));
  c_appp->setSenderOrSink (1); //Valor para ser el nodo que envia
*/
  // Simulator::Schedule (Seconds (30), &SomeEvent);
  AnimationInterface anim ("manet.xml");
  anim.UpdateNodeColor (SinkID, 0, 0, 255);
  anim.UpdateNodeColor (0, 0, 255, 0);
  /*Termina configuración de nodos que envian y del Sink*/

  // std::string path="/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop";
  //Config::Connect (path, MakeCallback(&PhyRxDropTrace));
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx", MakeCallback(&Rx));

  Simulator::Stop (Seconds (simTime));
  Simulator::Run (); //Termina simulación
  std::cout << "Post Simulation: " << std::endl;

  FILE *datos = fopen ("/home/manuel/Escritorio/Datos_Sim/datos.csv", "w");
 /* fprintf(datos,"|Nodo| \t |Paquetes creados| \t |Paquetes Duplicados| \t |Paquetes Recibidos| \t |Paquetes Reenviados| \t |retardo promedio|\n");
  for (uint32_t i = 0; i < nodos.GetN (); i++)
    {
      Ptr<CustomApplication> appI =
          DynamicCast<CustomApplication> (nodos.Get (i)->GetApplication (0));
        std::string valores= "  "+std::to_string(i)+"\t\t  "+std::to_string (appI->m_ControlPakets._NPQ_WAS_Created)
            +"\t\t\t  "+std::to_string(appI->m_ControlPakets._NPQ_WAS_Duplicated)+"\t\t\t  "+std::to_string(appI->m_ControlPakets._NPQ_WAS_Received)
            +"\t\t\t  "+std::to_string(appI->m_ControlPakets._NPQ_WAS_ReSend)+"\t\t\t  "+std::to_string(appI->m_ControlPakets.delayOnNode.GetMilliSeconds())+" ms"
            +"\n";
        //+std::to_string(StringValue(appI->m_ControlPakets.delayOnNode))
        fprintf(datos,  valores.c_str());
      //appI->PrintNeighbors ();
    }*/

  fprintf(datos,"Nodo;Paquetes creados;Paquetes Duplicados;Paquetes Recibidos;Paquetes Reenviados;Retardo promedio (ms)\n");
  for (uint32_t i = 0; i < nodos.GetN (); i++)
    {
      Ptr<CustomApplication> appI =
          DynamicCast<CustomApplication> (nodos.Get (i)->GetApplication (0));
        std::string valores= std::to_string(i)+";"+std::to_string (appI->m_ControlPakets._NPQ_WAS_Created)
            +";"+std::to_string(appI->m_ControlPakets._NPQ_WAS_Duplicated)+";"+std::to_string(appI->m_ControlPakets._NPQ_WAS_Received)
            +";"+std::to_string(appI->m_ControlPakets._NPQ_WAS_ReSend)+";"+std::to_string(appI->m_ControlPakets.delayOnNode.GetMilliSeconds())+"\n";
        //+std::to_string(StringValue(appI->m_ControlPakets.delayOnNode))
        fprintf(datos,  valores.c_str());
        
       // std::cout<<"Nodo "+std::to_string (i) +"**\n"+appI->m_NextData<<std::endl;
      //appI->PrintNeighbors ();
    }
  fclose (datos);
  //Termina la simulación
  Simulator::Destroy ();
  system("libreoffice /home/manuel/Escritorio/Datos_Sim/datos.csv &");
}
