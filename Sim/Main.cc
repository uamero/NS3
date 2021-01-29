#include "ns3/wave-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "applicationA.h" //Nodos secundarios generadores
#include "applicationB.h" //Nodos secundarios no generadores
#include "applicationSink.h" //Nodo Sink
#include "ns3/netanim-module.h"
#include "My-tag.h"

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
MacTxTrace (std::string context, Ptr<const Packet> p)
{
  CustomDataTag tag;
  p->PeekPacketTag (tag);
  std::cout << context << " " << tag.GetSEQNumber () << std::endl;
}
void
PhyRxDropTrace (std::string context, Ptr<const Packet> packet,
                WifiPhyRxfailureReason reason) //this was added in ns-3.30
{
  //What to do when packet loss happened.
  //Suggestion: Maybe check some packet tags you attached to the packet
  CustomDataTag tag;
  packet->PeekPacketTag (tag);
  std::cout << context << " " << reason << " " << tag.GetSEQNumber () << std::endl;
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
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();

  CommandLine cmd;
  uint32_t n_SecundariosA = 1; //Numero de nodos en la red
  uint32_t n_SecundariosB = 2; //Numero de nodos en la red
  //uint32_t n_Primarios = 1; //Numero de nodos en la red
  uint32_t n_Sink = 1; //Numero de nodos en la red
  double simTime = 10; //Tiempo de simulación
  //double interval = (rand->GetValue(0,100))/100; //intervalo de broadcast
  double interval = 0.5; //intervalo de broadcast
  //uint16_t N_channels = 1; //valor preestablecido para los canales

  cmd.AddValue ("t", "Tiempo de simulacion", simTime);
  cmd.AddValue ("i", "Duración del intervalo de broadcast", interval);
  cmd.AddValue ("nA", "Numero de nodos generadores", n_SecundariosA);
  cmd.AddValue ("nB", "Numero de nodos no generadores", n_SecundariosB);
  //cmd.AddValue ("nP", "Numero de nodos Primarios", n_Primarios);
  cmd.Parse (argc, argv);

  // Gnuplot gnuplot = Gnuplot ("reference-rates.png");
  NodeContainer SecundariosA;
  NodeContainer SecundariosB;
  //NodeContainer Primarios;
  NodeContainer Sink;

  SecundariosA.Create (n_SecundariosA);
  SecundariosB.Create (n_SecundariosB);
  //Primarios.Create(n_Primarios);
  Sink.Create (n_Sink);

  /*Se configura el modelo de movilidad*/
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator", "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"), "Rho",
                                 StringValue ("ns3::UniformRandomVariable[Min=0|Max=60]"));
  /*mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("2s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|100|0|100"));*/

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (SecundariosA);
  mobility.Install (SecundariosB);
  //mobility.Install (Primarios);
  mobility.Install (Sink);
  /*Termina la configuración de la movilidad*/

  /*Configuración del canal(canales)*/
  YansWifiChannelHelper wifiChannel; //= YansWifiChannelHelper::Default ();
  //wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.SetPropagationDelay ("ns3::RandomPropagationDelayModel");
  //wifiChannel.AddPropagationLoss("ns3::ThreeLogDistancePropagationLossModel");
  //wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  //wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel","MaxRange",DoubleValue(30));
  /*Termina conf. del canal*/
  Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  /*Se configura la capa fisica del dispositivo*/
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiPhyHelper wifiPhySink;
  /*Termina capa fisica*/

  /*Se crea el canal y se configura la capa física*/
  wifiPhy.SetChannel (channel);
  wifiPhySink.SetChannel (channel);

  //wifiPhy.SetPcapDataLinkType (
  //WifiPhyHelper::DLT_IEEE802_11_RADIO); /*Añade informacion adicional sobre el enlace*/
  //AsciiTraceHelper ascii;
  //wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
  //Aqui se pueden generar los archivos .pcap par wireshark
  wifiPhy.Set ("TxPowerStart", DoubleValue (10)); //5
  wifiPhy.Set ("TxPowerEnd", DoubleValue (40)); //33
  wifiPhy.Set ("TxPowerLevels", UintegerValue (16)); //8
  wifiPhySink.Set ("TxPowerStart", DoubleValue (10)); //5
  wifiPhySink.Set ("TxPowerEnd", DoubleValue (40)); //33
  wifiPhySink.Set ("TxPowerLevels", UintegerValue (16));
  /*Termina configuración*/

  /*Comienza la configuración de la capa de enlace*/

  WifiMacHelper wifiMac; //Permite crear nodo ad-hoc

  wifiMac.SetType ("ns3::AdhocWifiMac");

  WifiHelper wifi, wifiSink;
  wifi.SetStandard (WIFI_STANDARD_80211g);
  wifiSink.SetStandard (WIFI_STANDARD_80211b);
  /*wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "ControlMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "NonUnicastMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"));*/
  /* wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "ControlMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "NonUnicastMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"));*/

  //for (uint16_t i = 0; i < N_channels; i++)
  //{
  NetDeviceContainer devices_SecA = wifi.Install (wifiPhy, wifiMac, SecundariosA);
  NetDeviceContainer devices_SecB = wifi.Install (wifiPhy, wifiMac, SecundariosB);
  //NetDeviceContainer devices_Primarios = wifi.Install (wifiPhy, wifiMac, Primarios);
  NetDeviceContainer devices_Sink = wifiSink.Install (wifiPhy, wifiMac, Sink);
  //}

  //wifiPhy.EnableAscii("Manet-Node-",devices);
  //wifiPhy.EnablePcap("Manet-Node-",devices);

  AnimationInterface anim ("manetPB.xml");
  //NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodos);
  /*Termina configuración de la capa de enlace*/

  /*Comienza la instalación de la aplicación en los nodos*/
  for (uint32_t i = 0; i < SecundariosA.GetN (); i++)
    {
      //std::ostringstream oss;
      std::cout<< "ID1: "<< SecundariosA.Get(i)->GetId()<<std::endl;
      Ptr<CustomApplication> app_i = CreateObject<CustomApplication> ();
      app_i->SetBroadcastInterval (Seconds (interval));
      app_i->SetStartTime (Seconds (0));
      app_i->SetStopTime (Seconds (simTime));
      app_i->IniciaTabla (1, i);
      SecundariosA.Get (i)->AddApplication (app_i);
      anim.UpdateNodeColor (SecundariosA.Get(i)->GetId(), 0, 255, 0); //verde
    }
  for (uint32_t i = 0; i < SecundariosB.GetN (); i++)
    {
      std::cout<< "ID2: "<< SecundariosB.Get(i)->GetId()<<std::endl;
      //std::ostringstream oss;
      Ptr<CustomApplicationBnodes> app_i = CreateObject<CustomApplicationBnodes> ();
      app_i->SetBroadcastInterval (Seconds (interval));
      app_i->SetStartTime (Seconds (0));
      app_i->SetStopTime (Seconds (simTime));
      SecundariosB.Get (i)->AddApplication (app_i);
      anim.UpdateNodeColor (SecundariosB.Get(i)->GetId() , 0,0, 255); //Azules
    }
  for (uint32_t i = 0; i < Sink.GetN (); i++)
    {
      std::cout<< "ID3: "<< Sink.Get(i)->GetId()<<std::endl;
      //std::ostringstream oss;
      Ptr<ApplicationSink> app_i = CreateObject<ApplicationSink> ();
      app_i->SetBroadcastInterval (Seconds (interval));
      app_i->SetStartTime (Seconds (0));
      app_i->SetStopTime (Seconds (simTime));
      Sink.Get (i)->AddApplication (app_i);
      anim.UpdateNodeColor (Sink.Get(i)->GetId(), 255, 255, 0); //amarillo
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
      DynamicCast<1126276363
CustomApplication> (nodos.Get (0)->GetApplication (0));
  c_appp->setSenderOrSink (1); //Valor para ser el nodo que envia
*/
  // Simulator::Schedule (Seconds (30), &SomeEvent);

  /*Termina configuración de nodos que envian y del Sink*/

  //std::string path = "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop";
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
  //                MakeCallback (&Rx));
  //Config::Connect (path, MakeCallback (&PhyRxDropTrace));

  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback (&MacTxTrace));

  Simulator::Stop (Seconds (simTime));
  Simulator::Run (); //Termina simulación
  std::cout << "Post Simulation: " << std::endl;

  //FILE *datos = fopen ("/home/manuel/Escritorio/Datos_Sim/datos.csv", "w");
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
  /*
  fprintf (datos, "Nodo;Paquetes creados;Paquetes retransmitidos;Paquetes Duplicados;Paquetes "
                  "Recibidos;Paquetes Reenviados;Retardo promedio (ms);Paquetes recibidos por el "
                  "sink correctamente\n");
  for (uint32_t i = 0; i < nodos.GetN (); i++)
    {
      Ptr<CustomApplication> appI =
          DynamicCast<CustomApplication> (nodos.Get (i)->GetApplication (0));
      std::string valores = std::to_string (i) + ";" +
                            std::to_string (appI->m_ControlPakets._NPQ_WAS_Created) + ";" +
                            std::to_string (appI->m_ControlPakets._NPQ_WAS_ReBroadcast) + ";" +
                            std::to_string (appI->m_ControlPakets._NPQ_WAS_Duplicated) + ";" +
                            std::to_string (appI->m_ControlPakets._NPQ_WAS_Received) + ";" +
                            std::to_string (appI->m_ControlPakets._NPQ_WAS_ReSend) + ";" +
                            std::to_string (appI->m_ControlPakets.delayOnNode) + ";" +
                            std::to_string (appI->m_NumberOfPacketsOK) + "\n";
      //+std::to_string(StringValue(appI->m_ControlPakets.delayOnNode))
      fprintf (datos, valores.c_str ());
      appI->PrintPrevSEQnumbers ();

      NS_LOG_UNCOND ("\n\n****************************\n");
      appI->printMessageOnBuffer ();

      // std::cout<<"Nodo "+std::to_string (i) +"**\n"+appI->m_NextData<<std::endl;
      //appI->PrintNeighbors ();
    }

  fclose (datos);*/
  //Termina la simulación
  Simulator::Destroy ();
  //system ("libreoffice /home/manuel/Escritorio/Datos_Sim/datos.csv &");
  return 0;
}
