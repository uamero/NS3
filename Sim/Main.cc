#include "ns3/wave-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "applicationA.h" //Nodos secundarios generadores
#include "applicationB.h" //Nodos secundarios no generadores
#include "applicationPrimarios.h" //Nodos primarios
#include "applicationSink.h" //Nodo Sink
#include "ns3/netanim-module.h"
#include "My-tag.h"
#include "ns3/random-variable-stream.h"
#include "ns3/gnuplot.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstdlib>
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
/*void CreaGraficoNodoA(Time TS){
  std::string fileNameWithNoExtension = "GraficaN-A-with-error-bars";
  std::string graphicsFileName        = fileNameWithNoExtension + ".png";
  std::string plotFileName            = fileNameWithNoExtension + ".plt";
  std::string dataTitle               = "Envio de 100 paquetes";
  Gnuplot plot (graphicsFileName);
  plot.SetTerminal("png");
  plot.SetLegend("TR","TTS");
  plot.AppendExtra("set xrange[0:"+std::to_string(TS.GetSeconds())+"]");


  Gnuplot2dDataset dataset;
  dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);


  // Add the dataset to the plot.
  plot.AddDataset (dataset);

  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str());

  // Write the plot file.
  plot.GenerateOutput (plotFile);

  // Close the plot file.
  plotFile.close ();
}*/
void
Escenario_Uno ()
{
}
int
main (int argc, char *argv[])
{
  // datos<<"Hola1"<<std::end;

  CommandLine cmd;
  uint32_t n_iteracion = 0;
  uint32_t n_SecundariosA = 1; //Numero de nodos en la red
  uint32_t n_SecundariosB = 15; //Numero de nodos en la red
  uint32_t n_Primarios = 1; //Numero de nodos en la red
  uint32_t n_Sink = 1; //Numero de nodos en la red
  uint32_t Semilla_Sim = 1;
  double simTime = 100; //Tiempo de simulación
  // double interval = (rand->GetValue (0, 100)) / 100; //intervalo de broadcast
  //double interval = (rand()%100)/100.0; //intervalo de broadcast
  double intervalA = 1; //intervalo de broadcast
  double intervalB = 4; //intervalo de broadcast
  //uint16_t N_channels = 1; //valor preestablecido para los canales
  uint32_t n_Packets_A_Enviar =
      100; //numero de paquetes a ser creados por cada uno de los nodos generadores
  std::string CSVFile = "default.csv";
  bool StartSimulation = true;
  cmd.AddValue ("t", "Tiempo de simulacion", simTime);
  cmd.AddValue ("Seed", "Semilla de la simulacion", Semilla_Sim);
  cmd.AddValue ("nit", "Numero de iteraciones", n_iteracion);
  cmd.AddValue ("iA", "Duración del intervalo de broadcast", intervalA);
  cmd.AddValue ("iB", "Duración del intervalo de broadcast", intervalB);
  cmd.AddValue ("nA", "Numero de nodos generadores", n_SecundariosA);
  cmd.AddValue ("nB", "Numero de nodos no generadores", n_SecundariosB);
  cmd.AddValue ("nPTS", "Numero de paquetes a generar", n_Packets_A_Enviar);
  cmd.AddValue ("nP", "Numero de nodos Primarios", n_Primarios);
  cmd.AddValue ("StartSim", "Comienza un escenario de simulación nuevo", StartSimulation);
  cmd.AddValue ("CSVFile",
                "Nombre del archivo CSV donde se almacenaran los resultados de la simulación",
                CSVFile);

  cmd.Parse (argc, argv);

  SeedManager::SetSeed (Semilla_Sim);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  // Gnuplot gnuplot = Gnuplot ("reference-rates.png");
  NodeContainer SecundariosA;
  NodeContainer SecundariosB;
  NodeContainer Primarios;
  NodeContainer Sink;

  SecundariosA.Create (n_SecundariosA);
  SecundariosB.Create (n_SecundariosB);
  Primarios.Create (n_Primarios);
  Sink.Create (n_Sink);

  /*Se configura el modelo de movilidad*/
  MobilityHelper mobilit;
  mobilit.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", "X",
                                StringValue ("ns3::UniformRandomVariable[Min=0|Max=50]"), "Y",
                                StringValue ("ns3::UniformRandomVariable[Min=0|Max=50]"), "Z",
                                StringValue ("ns3::UniformRandomVariable[Min=0|Max=0]"));
  mobilit.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  MobilityHelper mobility;

  /* mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator", "X", StringValue ("50.0"),
                                 "Y", StringValue ("50.0"), "Rho",
                                 StringValue ("ns3::UniformRandomVariable[Min=0|Max=50]"));*/

  /*mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("2s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|100|0|100"));*/

  //mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  //mobility.Install (SecundariosA);
  //mobility.Install (SecundariosB);
  mobilit.Install (SecundariosA);
  mobilit.Install (SecundariosB);
  //mobility.Install (Primarios);
  MobilityHelper mobilitySink;
  mobilitySink.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilitySink.Install (Sink);
  mobilitySink.Install (Primarios);
  //Ptr<ListPositionAllocator> positionAlloc_Sink = CreateObject<ListPositionAllocator> ();
  Ptr<ConstantPositionMobilityModel> Sink_Pos =
      DynamicCast<ConstantPositionMobilityModel> (Sink.Get (0)->GetObject<MobilityModel> ());
  Sink_Pos->SetPosition (Vector (25.1, 27.6, 0));
  Ptr<ConstantPositionMobilityModel> Prim_Pos =
      DynamicCast<ConstantPositionMobilityModel> (Primarios.Get (0)->GetObject<MobilityModel> ());
  Prim_Pos->SetPosition (Vector (15, 24, 0));
  //mobilitySink.Install (Sink);
  /*Termina la configuración de la movilidad*/

  /*Configuración del canal(canales)*/
  YansWifiChannelHelper wifiChannel; //= YansWifiChannelHelper::Default ();
  //wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //wifiChannel.SetPropagationDelay ("ns3::RandomPropagationDelayModel");
  //wifiChannel.AddPropagationLoss("ns3::ThreeLogDistancePropagationLossModel");
  //wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (10));
  //wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");

  //wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel","MaxRange",DoubleValue(30));
  YansWifiChannelHelper wifiChannelSink; // =  YansWifiChannelHelper::Default ();
  wifiChannelSink.SetPropagationDelay ("ns3::RandomPropagationDelayModel");
  wifiChannelSink.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  //wifiChannelSink.AddPropagationLoss ("ns3::RangePropagationLossModel","MaxRange",DoubleValue(100));
  /*Termina conf. del canal*/
  //Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  /*Se configura la capa fisica del dispositivo*/
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiPhyHelper wifiPhySink = YansWifiPhyHelper::Default ();
  /*Termina capa fisica*/

  /*Se crea el canal y se configura la capa física*/
  //wifiPhy.SetChannel (channel);
  wifiPhy.SetChannel (wifiChannel.Create ());

  wifiPhySink.SetChannel (wifiChannelSink.Create ());

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

  /*Revizar por que los devices no se conectan de ofrma correcta con el Sink y por que la potencia del sink no
  llega a todos lo nodos */
  wifi.Install (wifiPhy, wifiMac, SecundariosA);
  wifiSink.Install (wifiPhySink, wifiMac, SecundariosA);
  wifi.Install (wifiPhy, wifiMac, SecundariosB);
  wifiSink.Install (wifiPhySink, wifiMac, SecundariosB);
  wifi.Install (wifiPhy, wifiMac, Primarios);
  wifiSink.Install (wifiPhySink, wifiMac, Primarios);
  wifi.Install (wifiPhy, wifiMac, Sink);
  wifiSink.Install (wifiPhySink, wifiMac, Sink);

  //NetDeviceContainer devices_SecA = wifi.Install (wifiPhy, wifiMac, SecundariosA);
  //NetDeviceContainer devices_SecB = wifi.Install (wifiPhy, wifiMac, SecundariosB);
  //NetDeviceContainer devices_Primarios = wifi.Install (wifiPhy, wifiMac, Primarios);
  //NetDeviceContainer devices_Sink = wifiSink.Install (wifiPhySink, wifiMac, Sink);
  //}

  //wifiPhy.EnableAscii("Manet-Node-",devices);
  //wifiPhy.EnablePcap("Manet-Node-",devices);

  //AnimationInterface anim ("manetPB.xml");
  //NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodos);
  /*Termina configuración de la capa de enlace*/

  /*Comienza la instalación de la aplicación en los nodos*/
  for (uint32_t i = 0; i < SecundariosA.GetN (); i++)
    {
      //std::ostringstream oss;
      // std::cout<< "ID1: "<< SecundariosA.Get(i)->GetId()<<std::endl;
      Ptr<CustomApplication> app_i = CreateObject<CustomApplication> ();
      app_i->SetBroadcastInterval (Seconds (intervalA));
      app_i->SetStartTime (Seconds (0));
      //app_i->SetStopTime (Seconds (simTime));
      app_i->setSemilla (Semilla_Sim+i);
      app_i->IniciaTabla (n_Packets_A_Enviar, i);
      SecundariosA.Get (i)->AddApplication (app_i);
      //anim.UpdateNodeColor (SecundariosA.Get (i)->GetId (), 0, 255, 0); //verde
      //app_i->ImprimeTabla ();
    }
  for (uint32_t i = 0; i < SecundariosB.GetN (); i++)
    {
      //std::cout<< "ID2: "<< SecundariosB.Get(i)->GetId()<<std::endl;
      //std::ostringstream oss;
      Ptr<CustomApplicationBnodes> app_i = CreateObject<CustomApplicationBnodes> ();
      //app_i->SetBroadcastInterval (Seconds (rand->GetInteger (1, 10)));
      app_i->SetBroadcastInterval (Seconds (intervalB));
      app_i->SetStartTime (Seconds (0));
      //app_i->SetStopTime (Seconds (simTime));
      SecundariosB.Get (i)->AddApplication (app_i);
     // std::cout << "El tiempo de broadcast en el nodo " << app_i->GetNode ()->GetId ()
      //          << " es :" << app_i->GetBroadcastInterval ().GetSeconds () << std::endl;
      // anim.UpdateNodeColor (SecundariosB.Get (i)->GetId (), 0, 0, 255); //Azules
    }
  for (uint32_t i = 0; i < Primarios.GetN (); i++)
    {
      //std::cout<< "ID2: "<< SecundariosB.Get(i)->GetId()<<std::endl;
      //std::ostringstream oss;
      Ptr<CustomApplicationPnodes> app_i = CreateObject<CustomApplicationPnodes> ();
      //app_i->SetBroadcastInterval (Seconds (interval)); Que el broadcast se realice cada 100 ms
      app_i->SetStartTime (Seconds (0));
      //app_i->SetStopTime (Seconds (simTime));
      Primarios.Get (i)->AddApplication (app_i);
      //anim.UpdateNodeColor (Primarios.Get (i)->GetId (), 255, 164, 032); //naranja
    }
  for (uint32_t i = 0; i < Sink.GetN (); i++)
    {
      //std::cout<< "ID3: "<< Sink.Get(i)->GetId()<<std::endl;
      //std::ostringstream oss;
      Ptr<ApplicationSink> app_i = CreateObject<ApplicationSink> ();
      //app_i->SetBroadcastInterval (Seconds (interval));
      app_i->SetStartTime (Seconds (0));
      //app_i->SetStopTime (Seconds (simTime));
      Sink.Get (i)->AddApplication (app_i);
      //anim.UpdateNodeColor (Sink.Get (i)->GetId (), 255, 255, 0); //amarillo
    }

  /*Termina instalación de la aplicación*/

  //std::string path = "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop";
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
  //                MakeCallback (&Rx));
  //Config::Connect (path, MakeCallback (&PhyRxDropTrace));

  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback (&MacTxTrace));
  //double new_range = 10;

  // Config::Set("/NodeList/"+ std::to_string(Sink.Get(0)->GetId())+"/DeviceList/*/$ns3::WifiNetDevice/Channel/$ns3::YansWifiChannel/PropagationLossModel/$ns3::RangePropagationLossModel/MaxRange", DoubleValue(new_range) );
  //Simulator::Stop (Seconds (simTime));
  Simulator::Run (); //Termina simulación

  /*###################################################################################*/
  /*###################################################################################*/
  /*Termina lasimulacion */
  /*###################################################################################*/
  /*###################################################################################*/
  //std::cout << "Post Simulation: " << std::endl;
  FILE *datos;
  if (StartSimulation)
    {
      std::string ruta = "/home/manuel/Escritorio/Datos_Sim/" + CSVFile;
      datos = fopen (ruta.c_str (), "w");
      /* std::string info =
          "Información de la simulación | Secundarios A: " + std::to_string (n_SecundariosA) +
          " | Secundarios B: " + std::to_string (n_SecundariosB) + "| Primarios " +
          std::to_string (n_Primarios) + "\n";
      fprintf (datos, info.c_str ());*/
      fprintf (datos,
               "Nodo,No. Paquete,Estatus del paquete,Tiempo de entrega,No. SEQ, No. Veces "
               "enviado,Tamaño en bytes,Iteración,Semilla,TTS,No. Paquetes a enviar,TA,TB \n");
    }
  else
    {
      std::string ruta = "/home/manuel/Escritorio/Datos_Sim/" + CSVFile;
      datos = fopen (ruta.c_str (), "a");
    }

  for (uint32_t i = 0; i < SecundariosA.GetN (); i++)
    {
      Ptr<CustomApplication> appI =
          DynamicCast<CustomApplication> (SecundariosA.Get (i)->GetApplication (0));
      std::string data;
      uint32_t cont = 0;
      for (std::list<ST_Paquete_A_Enviar>::iterator it = appI->m_Tabla_paquetes_A_enviar.begin ();
           it != appI->m_Tabla_paquetes_A_enviar.end (); it++)
        {
          data = std::to_string (appI->GetNode ()->GetId ()) + "," + std::to_string (cont + 1) +
                 "," + std::to_string (it->Estado) + "," +
                 std::to_string (it->Tiempo_de_recibo_envio.GetSeconds ()) + "," +
                 std::to_string (it->numeroSEQ) + "," + std::to_string (it->NumeroDeEnvios) + "," +
                 std::to_string (it->Tam_Paquete) + "," + std::to_string (n_iteracion) + "," +
                 std::to_string (Semilla_Sim) + "," +
                 std::to_string (appI->m_simulation_time.GetSeconds ()) + "," +
                 std::to_string (n_Packets_A_Enviar) + "," + std::to_string (intervalA) + "," +
                 std::to_string (intervalB) + "\n";

          if(it->Estado){
              fprintf (datos, data.c_str ());
          }       
          
          cont++;
        }
      //appI->ImprimeTabla ();
      //std::string data = appI->ObtenDAtosNodo () + "\n";
    }

  fclose (datos);

  /*Ptr<ApplicationSink> appI = DynamicCast<ApplicationSink> (Sink.Get (0)->GetApplication (0));
  appI->ImprimeTabla ();
  for (uint32_t i = 0; i < SecundariosB.GetN (); i++)
    {
      Ptr<CustomApplicationBnodes> appI =
          DynamicCast<CustomApplicationBnodes> (SecundariosB.Get (i)->GetApplication (0));
      //appI->ImprimeTabla ();
    }
*/
  //Termina la simulación
  Simulator::Destroy ();
  //system ("libreoffice /home/manuel/Escritorio/Datos_Sim/datos.csv &");
  return 0;
}
