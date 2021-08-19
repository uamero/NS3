
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "applicationA.h" //Nodos secundarios generadores
#include "applicationB.h" //Nodos secundarios no generadores
#include "applicationPrimarios.h" //Nodos primarios
#include "applicationSink.h" //Nodo Sink
#include "SecundariosTag.h"
#include "SinkTag.h"
#include "TagPrimarios.h"


#include "ns3/random-variable-stream.h"
#include "ns3/gnuplot.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/basic-energy-source.h"
#include "ns3/energy-source-container.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/buildings-helper.h"
#include "ns3/buildings-module.h"
#include "ns3/building-position-allocator.h"
#include "ns3/mobility-building-info.h"
#include "ns3/building.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/animation-interface.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstdlib>
#include <map>
#define YELLOW_CODE "\033[33m"
#define TEAL_CODE "\033[36m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

using namespace ns3;
//NS_LOG_COMPONENT_DEFINE ("OwnSimulation");
/*Variables globales*/
Time m_Simulation_Time;
NodeContainer SA, SB;
DeviceEnergyModelContainer deviceModels;
uint32_t n_Size_Batery = 36000; // este valor esta en Joules
uint32_t n_nodos_muertos = 0;
std::list<uint32_t> m_baterias;
uint32_t n_channels;
/**********************************************************************************************/
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
showPosition (NodeContainer nodes, double deltaTime)
{
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {

      uint32_t nodeId = nodes.Get (i)->GetId ();
      Ptr<MobilityModel> mobModel = nodes.Get (i)->GetObject<MobilityModel> ();
      Vector3D pos = mobModel->GetPosition ();
      Vector3D speed = mobModel->GetVelocity ();
      std::cout << "At " << Simulator::Now ().GetSeconds () << " node " << nodeId << ": Position("
                << pos.x << ", " << pos.y << ", " << pos.z << ");   Speed(" << speed.x << ", "
                << speed.y << ", " << speed.z << ")" << std::endl;
    }
}
void
MacTxTrace (std::string context, Ptr<const Packet> p)
{

  SecundariosDataTag TagSec;
  SinkDataTag sinkTag;
  PrimariosDataTag PrimTag;
  if (p->PeekPacketTag (TagSec))
    {
      std::cout << context << " " << TagSec.GetSEQNumber () << std::endl;
    }
  else if (p->PeekPacketTag (sinkTag))
    {
      std::cout << context << " " << sinkTag.GetSEQNumber () << std::endl;
    }
  else if (p->PeekPacketTag (PrimTag))
    {
      std::cout << context << " " << PrimTag.GetTypeId() << std::endl;
    }
}
void
PhyRxDropTrace (std::string context, Ptr<const Packet> packet,
                WifiPhyRxfailureReason reason) //this was added in ns-3.30
{
  //What to do when packet loss happened.
  //Suggestion: Maybe check some packet tags you attached to the packet
  SecundariosDataTag TagSec;
  SinkDataTag sinkTag;
  PrimariosDataTag PrimTag;
  if (packet->PeekPacketTag (TagSec))
    {
     std::cout << context << " " << reason << " " << TagSec.GetSEQNumber () << std::endl;
    }
  else if (packet->PeekPacketTag (sinkTag))
    {
      std::cout << context << " " << reason << " " << sinkTag.GetSEQNumber () << std::endl;
    }
  else if (packet->PeekPacketTag (PrimTag))
    {
      std::cout << context << " " << reason << " " << PrimTag.GetTypeId() << std::endl;
    }
  
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

      for (NodeContainer::Iterator nodoIT = SB.Begin (); nodoIT != SB.End (); nodoIT++)
        {
          Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
          std::cout << "EL pisodel nodo " << std::to_string ((*nodoIT)->GetId ())
                    << " es: " << std::to_string (MB->GetFloorNumber ())
                    << std::to_string (MB->GetFloorNumber ())
                    << std::to_string (MB->GetRoomNumberY ()) << std::endl;
        }
      for (NodeContainer::Iterator nodoIT = SA.Begin (); nodoIT != SA.End (); nodoIT++)
        {
          Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();

          std::cout << "EL pisodel nodo " << std::to_string ((*nodoIT)->GetId ())
                    << " es: " << std::to_string (MB->GetFloorNumber ())
                    << std::to_string (MB->GetRoomNumberX ()) << std::endl;
        }
    }
  std::cout << " Termina RX ()1///////////////////" << std::endl;
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
verifica_termino_Simulacion ()
{
  bool termina = true;

  for (uint32_t i = 0; i < SA.GetN (); i++)
    {
      Ptr<CustomApplication> appI = DynamicCast<CustomApplication> (SA.Get (i)->GetApplication (0));

      for (std::list<ST_Paquete_A_Enviar>::iterator it = appI->m_Tabla_paquetes_A_enviar.begin ();
           it != appI->m_Tabla_paquetes_A_enviar.end (); it++)
        {
          if (!it->Estado)
            {
              termina = false;
              break;
            }
        }
    }
  if (termina)
    {
      m_Simulation_Time = Now ();
      Simulator::Stop ();
    }
  else
    {
      Simulator::Schedule (Seconds (1), &verifica_termino_Simulacion);
    }
}
void
Muerte_nodo_B (EnergySourceContainer source)
{
  std::list<uint32_t>::iterator it = m_baterias.begin ();
  for (uint32_t i = 0; i < source.GetN (); i++)
    {
      Ptr<CustomApplicationBnodes> appI =
          DynamicCast<CustomApplicationBnodes> (SB.Get (i)->GetApplication (0));
      double Pbatery = 100 * ((source.Get (i)->GetRemainingEnergy () - (*it)) / n_Size_Batery);
      appI->m_Batery = Pbatery;
      it++;
      if (Pbatery < 1)
        {
          n_nodos_muertos++;
        }
    }
  Simulator::Schedule (Seconds (.5), &Muerte_nodo_B, source);
}
void
RemainingEnergy (double oldValue, double remainingEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Current remaining energy = " << remainingEnergy << "J");
}

/// Trace function for total energy consumption at node.
void
TotalEnergy (double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Total energy consumed by radio = " << totalEnergy << "J");
}
void
PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);

  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index << " rect from " << box.xMin << "," << box.yMin << " to "
              << box.xMax << "," << box.yMax << std::endl;
    }
}
int
main (int argc, char *argv[])
{
  // datos<<"Hola1"<<std::end;

  CommandLine cmd;
  uint32_t n_iteracion = 0;
  uint32_t n_SecundariosA = 1; //Numero de nodos en la red
  uint32_t n_SecundariosB = 20; //Numero de nodos en la red
  uint32_t n_Primarios = 1; //Numero de nodos en la red
  uint32_t n_Sink = 1; //Numero de nodos en la red
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  n_channels = 5; // Numero de canales, por default 8
  uint32_t Semilla_Sim = 3;
  uint32_t escenario = 1; //escenario por deffault

  bool RWP = true;
  bool homogeneo = true;
  //const double R = 0.046; //J, Energy consumption on reception
  //const double T = 0.067; //J, Energy consumption on sending
  double simTime = 1000; //Tiempo de simulación
  // double interval = (rand->GetValue (0, 100)) / 100; //intervalo de broadcast
  //double interval = (rand()%100)/100.0; //intervalo de broadcast
  double intervalA = 1; //intervalo de broadcast
  //uint16_t N_channels = 1; //valor preestablecido para los canales
  uint32_t n_Packets_A_Enviar =
      1; //numero de paquetes a ser creados por cada uno de los nodos generadores
  uint32_t TP = 5;
  std::string CSVFile = "default.csv";
  bool StartSimulation = true;

  cmd.AddValue ("t", "Tiempo de simulacion", simTime);
  cmd.AddValue ("Seed", "Semilla de la simulacion", Semilla_Sim);
  cmd.AddValue ("nit", "Numero de iteraciones", n_iteracion);
  cmd.AddValue ("iA", "Duración del intervalo de broadcast", intervalA);
  cmd.AddValue ("nA", "Numero de nodos generadores", n_SecundariosA);
  cmd.AddValue ("nB", "Numero de nodos no generadores", n_SecundariosB);
  cmd.AddValue ("nPTS", "Numero de paquetes a generar", n_Packets_A_Enviar);
  cmd.AddValue ("nP", "Numero de nodos Primarios", n_Primarios);
  cmd.AddValue ("rwp", "Modelo de movilidad random Way point", RWP);
  cmd.AddValue ("hg", "Activa la parte heterogenea de la red, por default es homogeneo", homogeneo);
  cmd.AddValue ("tp", "Tiempo de actualizacion de los canales de los primarios", TP);
  cmd.AddValue ("nch", "Numero de canales a instalar", n_channels);
  cmd.AddValue ("StartSim", "Comienza un escenario de simulación nuevo", StartSimulation);
  cmd.AddValue ("CSVFile",
                "Nombre del archivo CSV donde se almacenaran los resultados de la simulación",
                CSVFile);

  cmd.Parse (argc, argv);
  /*
  std::string cmdSC =
      "cd /home/manuel/Escritorio/bonnmotion-3.0.1/bin/ && ./bm -f desastre RPGM -d 5000 -i 0 -n " +
      std::to_string (n_SecundariosB) + " -x 510 -y 510 -a 1 -p 20";

  //system (cmdSC.c_str ());
  system (cmdSC.c_str ());
  cmdSC = "cd /home/manuel/Escritorio/bonnmotion-3.0.1/bin/ &&./bm NSFile -f desastre -d";
  system (cmdSC.c_str ());
  cmdSC = "mv /home/manuel/Escritorio/bonnmotion-3.0.1/bin/desastre.ns_movements "
          "/home/manuel/Documentos/bake/source/ns-3.32/scratch/Sim";
  system (cmdSC.c_str ());
  cmdSC = "cd /home/manuel/Escritorio/bonnmotion-3.0.1/bin/ && rm desastre.movements.gz "
          "desastre.ns_params desastre.params";
  system (cmdSC.c_str ());
  //std::string traceFile = "/home/manuel/Documentos/bake/source/ns-3.32/scratch/emer2.ns_movements";
  std::string traceFile =
      "/home/manuel/Documentos/bake/source/ns-3.32/scratch/Sim/desastre.ns_movements";
*/
  SeedManager::SetSeed (Semilla_Sim);

  NodeContainer SecundariosB;
  SecundariosB.Create (n_SecundariosB);

  Ptr<Building> b;
  MobilityHelper mobility_nB;

  double x_min = 10.0;
  double x_max = 160.0;
  double y_min = 10.0;
  double y_max = 40.0;
  double z_min = 0.0;
  double z_max = 50.0;

  switch (escenario)
    {
    case 1:

      b = CreateObject<Building> ();
      b->SetBoundaries (Box (x_min, x_max, y_min, y_max, z_min, z_max));
      b->SetBuildingType (Building::Residential);
      //b->SetBuildingType (Building::Office);
      //b->SetBuildingType (Building::Commercial);
      b->SetExtWallsType (Building::ConcreteWithWindows);
      b->SetNFloors (4);
      b->SetNRoomsX (10);
      b->SetNRoomsY (3);

      mobility_nB.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Mode", StringValue ("Time"),
                                    "Time", StringValue ("1s"), "Speed",
                                    StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                                    "Bounds", StringValue ("10|160|10|40"));

      mobility_nB.SetPositionAllocator ("ns3::RandomBuildingPositionAllocator");
      mobility_nB.Install (SecundariosB);

      BuildingsHelper::Install (SecundariosB);
      for (NodeContainer::Iterator nodoIT = SecundariosB.Begin (); nodoIT != SecundariosB.End ();
           nodoIT++)
        {
          Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
          Ptr<MobilityModel> mm = (*nodoIT)->GetObject<MobilityModel> ();
          MB->MakeConsistent (mm);
        }
      //BuildingsHelper::MakeMobilityModelConsistent ();
      /* for (NodeContainer::Iterator it = SecundariosB.Begin (); it != SecundariosB.End (); it++)
        {
          //Ptr<MobilityModel> mm = (*it)->GetObject<MobilityModel> ();
          //Ptr<MobilityBuildingInfo> bmm = mm->GetObject<MobilityBuildingInfo> ();
          Ptr<HybridBuildingsPropagationLossModel> LossModel  = (*it)->GetObject<HybridBuildingsPropagationLossModel>();
          std::cout<<"El modelo de perdidas por propagación es: "<<(*it)->GetObject<HybridBuildingsPropagationLossModel>()<<std::endl;
           //Vector p = mm->GetPosition ();
           //Para revisar los parametros visitar: //https://www.nsnam.org/doxygen/group__propagation.html#ga29c9a1b1a58b6a56054ff5ea4c5a574d
          LossModel->SetCitySize(SmallCity);
          LossModel->SetEnvironment(UrbanEnvironment);
        }*/
      break;
    case 2:
      // create a grid of buildings
      double buildingSizeX = 100; // m
      double buildingSizeY = 50; // m
      double streetWidth = 25; // m
      double buildingHeight = 10; // m
      uint32_t numBuildingsX = 10;
      uint32_t numBuildingsY = 10;
      double maxAxisX = (buildingSizeX + streetWidth) * numBuildingsX;
      double maxAxisY = (buildingSizeY + streetWidth) * numBuildingsY;
      std::vector<Ptr<Building>> buildingVector;

      for (uint32_t buildingIdX = 0; buildingIdX < numBuildingsX; ++buildingIdX)
        {
          for (uint32_t buildingIdY = 0; buildingIdY < numBuildingsY; ++buildingIdY)
            {
              Ptr<Building> building;
              building = CreateObject<Building> ();

              building->SetBoundaries (
                  Box (buildingIdX * (buildingSizeX + streetWidth),
                       buildingIdX * (buildingSizeX + streetWidth) + buildingSizeX,
                       buildingIdY * (buildingSizeY + streetWidth),
                       buildingIdY * (buildingSizeY + streetWidth) + buildingSizeY, 0.0,
                       buildingHeight));
              building->SetNRoomsX (1);
              building->SetNRoomsY (1);
              building->SetNFloors (1);
              buildingVector.push_back (building);
            }
        }
      PrintGnuplottableBuildingListToFile ("buildings.txt");
      mobility_nB.SetMobilityModel (
          "ns3::RandomWalk2dOutdoorMobilityModel", "Bounds",
          RectangleValue (Rectangle (-streetWidth, maxAxisX, -streetWidth, maxAxisY)));
      // create an OutdoorPositionAllocator and set its boundaries to match those of the mobility model
      Ptr<OutdoorPositionAllocator> position = CreateObject<OutdoorPositionAllocator> ();
      Ptr<UniformRandomVariable> xPos = CreateObject<UniformRandomVariable> ();
      xPos->SetAttribute ("Min", DoubleValue (-streetWidth));
      xPos->SetAttribute ("Max", DoubleValue (maxAxisX));
      Ptr<UniformRandomVariable> yPos = CreateObject<UniformRandomVariable> ();
      yPos->SetAttribute ("Min", DoubleValue (-streetWidth));
      yPos->SetAttribute ("Max", DoubleValue (maxAxisY));
      position->SetAttribute ("X", PointerValue (xPos));
      position->SetAttribute ("Y", PointerValue (yPos));
      mobility_nB.SetPositionAllocator (position);
      // install the mobility model
      mobility_nB.Install (SecundariosB);
      BuildingsHelper::Install (SecundariosB);
      break;
      //default:
      /*Se crea el escenario con elmodelo de movilidad RPGM paralos nodos tipo B
      Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
      ns2.Install ();
      break;*/
    }

  /*Termina configuración del primer escenario */

  NodeContainer SecundariosA;
  NodeContainer Primarios;
  NodeContainer Sink;

  SecundariosA.Create (n_SecundariosA);

  Primarios.Create (n_Primarios);
  Sink.Create (n_Sink);

  /*Se configura el modelo de movilidad*/
  mobility_nB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_nB.SetPositionAllocator ("ns3::RandomBuildingPositionAllocator");

  mobility_nB.Install (SecundariosA);

  BuildingsHelper::Install (SecundariosA);
  for (NodeContainer::Iterator nodoIT = SecundariosA.Begin (); nodoIT != SecundariosA.End ();
       nodoIT++)
    {
      Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
      Ptr<MobilityModel> mm = (*nodoIT)->GetObject<MobilityModel> ();
      MB->MakeConsistent (mm);
    }
  //BuildingsHelper::MakeMobilityModelConsistent ();

  mobility_nB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility_nB.Install (Primarios);
  for (uint32_t i = 0; i < Primarios.GetN (); i++)
    {
      Ptr<ConstantPositionMobilityModel> PUs_Pos = DynamicCast<ConstantPositionMobilityModel> (
          Primarios.Get (i)->GetObject<MobilityModel> ());
      PUs_Pos->SetPosition (Vector (0.1, 50, 0.1));
    }
  BuildingsHelper::Install (Primarios);
  for (NodeContainer::Iterator nodoIT = Primarios.Begin (); nodoIT != Primarios.End (); nodoIT++)
    {
      Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
      Ptr<MobilityModel> mm = (*nodoIT)->GetObject<MobilityModel> ();
      MB->MakeConsistent (mm);
    }
  //BuildingsHelper::MakeMobilityModelConsistent ();
  /* 
  MobilityHelper mobilit;
  mobilit.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", "X",
                                StringValue ("ns3::UniformRandomVariable[Min=0|Max=500]"), "Y",
                                StringValue ("ns3::UniformRandomVariable[Min=0|Max=500]"), "Z",
                                StringValue ("ns3::UniformRandomVariable[Min=0|Max=0]"));
  mobilit.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  //mobilit.Install (SecundariosA);
  mobilit.Install (Primarios);
  
  MobilityHelper mobility;
  */
  /* mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator", "X", StringValue ("50.0"),
                                 "Y", StringValue ("50.0"), "Rho",
                                 StringValue ("ns3::UniformRandomVariable[Min=0|Max=50]"));
  if (RWP)
    {
      mobilit.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Mode", StringValue ("Time"),
                                "Time", StringValue ("1s"), "Speed",
                                StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"), "Bounds",
                                StringValue ("0|50|0|50"));
    }
  else
    {
      mobilit.SetMobilityModel ("ns3::RandomDirection2dMobilityModel", "Bounds",
                                RectangleValue (Rectangle (0, 500, 0, 500)), "Speed",
                                StringValue ("ns3::ConstantRandomVariable[Constant=5]"), "Pause",
                                StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
    }
   */
  //############mobilit.Install (SecundariosB);
  //mobility.Install (Primarios);
  MobilityHelper mobilitySink;
  mobilitySink.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilitySink.Install (Sink);
  //mobilitySink.Install (Primarios);
  //mobilitySink.Install (SecundariosA);
  //Ptr<ListPositionAllocator> positionAlloc_Sink = CreateObject<ListPositionAllocator> ();
  Ptr<ConstantPositionMobilityModel> Sink_Pos =
      DynamicCast<ConstantPositionMobilityModel> (Sink.Get (0)->GetObject<MobilityModel> ());
  //Sink_Pos->SetPosition (Vector (250, 250, 0));
  Sink_Pos->SetPosition (Vector (0.5, 0.5, 0.5));
  BuildingsHelper::Install (Sink);
  //BuildingsHelper::MakeMobilityModelConsistent ();
  /*Termina la configuración de la movilidad*/

  /*Configuración del canal(canales)*/

  YansWifiChannelHelper wifiChannelSink;

  wifiChannelSink.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  wifiChannelSink.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange",
                                      DoubleValue (5000));

  YansWifiChannelHelper wifiChannelPrimarios;
  wifiChannelPrimarios.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  wifiChannelPrimarios.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange",
                                           DoubleValue (5000));
  WifiMacHelper wifiMac; //Permite crear nodo ad-hoc
  wifiMac.SetType ("ns3::AdhocWifiMac");
  WifiHelper wifi, wifiSink, wifiPrimarios;
  wifi.SetStandard (WIFI_STANDARD_80211g);
  wifiSink.SetStandard (WIFI_STANDARD_80211g);
  wifiPrimarios.SetStandard (WIFI_STANDARD_80211g);
  std::list<uint32_t> List_RangeOfChannels;

  /*#####Implementación de los canales en los nodos tipo A y tipo B*/
  for (uint32_t i = 0; i < n_channels; i++)
    {

      YansWifiChannelHelper wifiChannel;
      wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      uint32_t distance;
      if (homogeneo)
        distance = 38;
      else
        distance = rand->GetInteger (38, 140);
      List_RangeOfChannels.push_back (distance);

      /* wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange",
                                      DoubleValue (distance));*/

      //wifiChannel.AddPropagationLoss ("ns3::HybridBuildingsPropagationLossModel");
      /*El modelo ohBuildings convina dos modelos para mas informacion observar https://www.nsnam.org/docs/release/3.32/doxygen/classns3_1_1_oh_buildings_propagation_loss_model.html#details
      https://www.nsnam.org/docs/release/3.32/doxygen/oh-buildings-propagation-loss-model_8h.html
      */
      wifiChannel.AddPropagationLoss ("ns3::OhBuildingsPropagationLossModel");
      //wifiChannel.AddPropagationLoss ("ns3::BuildingsPropagationLossModel");

      YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
      wifiPhy.SetChannel (wifiChannel.Create ());
      wifiPhy.Set ("TxPowerStart", DoubleValue (5)); //5
      wifiPhy.Set ("TxPowerEnd", DoubleValue (33)); //33
      wifiPhy.Set ("TxPowerLevels", UintegerValue (8)); //8
      wifi.Install (wifiPhy, wifiMac,
                    SecundariosA); //Device para comunicar a los nodos tipo A y B
      wifi.Install (wifiPhy, wifiMac,
                    SecundariosB); //Device para comunicar a los nodos tipo A y B
      wifi.Install (wifiPhy, wifiMac, Primarios); //Device para comunicar a los nodos tipo A y B
      wifi.Install (wifiPhy, wifiMac, Sink);
      /*Ese ciclo for instala de 0 a n-1 interfaces en los Nodecontainer de Sencundarios A,B,Primarios y el Sink */
    }

  //-------------------->>>>>>>>>>>Probar e identificar que interfaces son las del Sink y las de los usuarios primarios
  /*Termina los canales */
  /*Termina conf. del canal*/
  //Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  /*Se configura la capa fisica del dispositivo*/
  //#########YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiPhyHelper wifiPhySink = YansWifiPhyHelper::Default ();
  YansWifiPhyHelper wifiPhyPrimarios = YansWifiPhyHelper::Default ();
  /*Termina capa fisica*/

  /*Se crea el canal y se configura la capa física*/
  // wifiPhy.SetChannel (channel);
  //Ptr<ns3::YansWifiChannel>chann = wifiChannel.Create ();
  //#################wifiPhy.SetChannel (wifiChannel.Create ());
  //wifiPhy.SetChannel (chann);

  //wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (20));

  //wifiPhy.SetChannel (wifiChannel);

  wifiPhySink.SetChannel (wifiChannelSink.Create ());

  wifiPhyPrimarios.SetChannel (wifiChannelPrimarios.Create ());
  //wifiPhy.SetPcapDataLinkType (
  //WifiPhyHelper::DLT_IEEE802_11_RADIO); /*Añade informacion adicional sobre el enlace*/
  //AsciiTraceHelper ascii;
  //wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
  //Aqui se pueden generar los archivos .pcap par wireshark
  /*wifiPhy.Set ("TxPowerStart", DoubleValue (10)); //5
  wifiPhy.Set ("TxPowerEnd", DoubleValue (40)); //33
  wifiPhy.Set ("TxPowerLevels", UintegerValue (16)); //8*/
  wifiPhySink.Set ("TxPowerStart", DoubleValue (10)); //5
  wifiPhySink.Set ("TxPowerEnd", DoubleValue (40)); //33
  wifiPhySink.Set ("TxPowerLevels", UintegerValue (16));

  wifiPhyPrimarios.Set ("TxPowerStart", DoubleValue (10)); //5
  wifiPhyPrimarios.Set ("TxPowerEnd", DoubleValue (40)); //33
  wifiPhyPrimarios.Set ("TxPowerLevels", UintegerValue (16));
  /*Termina configuración*/

  /*Comienza la configuración de la capa de enlace*/

  /*WifiMacHelper wifiMac; //Permite crear nodo ad-hoc

  wifiMac.SetType ("ns3::AdhocWifiMac");*/

  /*wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "ControlMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "NonUnicastMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"));*/
  /* wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "ControlMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"), "NonUnicastMode",
                                StringValue ("OfdmRate6MbpsBW10MHz"));*/

  //wifi.Install (wifiPhy, wifiMac, SecundariosA); //Device para comunicar a los nodos tipo A y B
  /*Se instala la interaz de red con ID n_channels (ej. si n_channels es 8 el ID de esta interfas es 8)*/
  wifiSink.Install (wifiPhySink, wifiMac,
                    SecundariosA); //Device n para comunicar a los nodos tipo A,B y Sink
  /*Interfaz con ID n_channels + 1*/

  wifiPrimarios.Install (wifiPhyPrimarios, wifiMac,
                         SecundariosA); //Device para comunicar a los nodos tipo A,B y PU's
  //wifi.Install (wifiPhy, wifiMac, SecundariosB); //Device para comunicar a los nodos tipo A y B

  wifiSink.Install (wifiPhySink, wifiMac,
                    SecundariosB); //Device para comunicar a los nodos tipo A,B y Sink

  NetDeviceContainer devicesB =
      wifiPrimarios.Install (wifiPhyPrimarios, wifiMac,
                             SecundariosB); //Device para comunicar a los nodos tipo A y B
  //wifi.Install (wifiPhy, wifiMac, Primarios); //Device para comunicar a los nodos tipo A y B

  /*wifiSink.Install (wifiPhySink, wifiMac,
                    Primarios); //Device para comunicar a los nodos tipo A,B y Sink*/
  wifiPrimarios.Install (wifiPhyPrimarios, wifiMac,
                         Primarios); //Device para comunicar a los nodos tipo A,B y PU's
  //wifi.Install (wifiPhy, wifiMac, Sink);
  wifiSink.Install (wifiPhySink, wifiMac, Sink);

  //NetDeviceContainer devices_SecA = wifi.Install (wifiPhy, wifiMac, SecundariosA);
  //NetDeviceContainer devices_SecB = wifi.Install (wifiPhy, wifiMac, SecundariosB);
  //NetDeviceContainer devices_Primarios = wifi.Install (wifiPhy, wifiMac, Primarios);
  //NetDeviceContainer devices_Sink = wifiSink.Install (wifiPhySink, wifiMac, Sink);
  //}

  //wifiPhy.EnableAscii("Manet-Node-",devices);
  //wifiPhy.EnablePcap("Manet-Node-",devices);
  /*Instalación de la bateria***********************************************************************************/

  /* energy source */
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  /*Para una bateria de 2000mAh a 5V la equivalencia en Joules es 36000*/
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (n_Size_Batery));
  basicSourceHelper.Set ("PeriodicEnergyUpdateInterval", TimeValue (Seconds (0.2)));

  // install source
  EnergySourceContainer sources = basicSourceHelper.Install (SecundariosB);

  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;

  // configure radio energy model
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0174));
  deviceModels = radioEnergyHelper.Install (devicesB, sources);
  //radioEnergyHelper.Set ("TxCurrentA", DoubleValue (T));
  /* radioEnergyHelper.Set ("RxCurrentA", DoubleValue (R));
  radioEnergyHelper.Set ("IdleCurrentA", DoubleValue (0.01));
  radioEnergyHelper.Set ("CcaBusyCurrentA", DoubleValue (0.01));
  radioEnergyHelper.Set ("SwitchingCurrentA", DoubleValue (0.01));
  radioEnergyHelper.Set ("SleepCurrentA", DoubleValue (0.001));*/
  /*Termina instalación de la bateria***************************************************************/
  /** connect trace sources **/
  /***************************************************************************/
  // all sources are connected to node 1
  // energy source

  for (uint32_t i; i < sources.GetN (); i++)
    {
      m_baterias.push_back (
          rand->GetInteger (uint32_t (n_Size_Batery * .10), uint32_t (n_Size_Batery * .90)));
      /*Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (sources.Get (i));
      basicSourcePtr->TraceConnectWithoutContext ("RemainingEnergy",
                                                  MakeCallback (&RemainingEnergy));
      // device energy model
      Ptr<DeviceEnergyModel> basicRadioModelPtr =
          basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
      NS_ASSERT (basicRadioModelPtr != NULL);
      basicRadioModelPtr->TraceConnectWithoutContext ("TotalEnergyConsumption",
                                                      MakeCallback (&TotalEnergy));*/
    }

  /***************************************************************************/
  //FlowMonitorHelper flujo;

  //flujo.Install (SecundariosB);
  //Ptr<FlowMonitor> flow_nodes = flujo.InstallAll ();
  AnimationInterface anim ("manetPB.xml");
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
      //app_i->SetMAxtime (Seconds (MaxTimeToStop));
      //app_i->SetStopTime (Seconds (simTime));
      app_i->setSemilla (Semilla_Sim + i);
      app_i->IniciaTabla (n_Packets_A_Enviar, i);
      app_i->m_n_channels = n_channels;
      app_i->iniciaCanales ();
      app_i->CreaBuffersCanales ();
      app_i->m_RangeOfChannels_Info = List_RangeOfChannels;
      SecundariosA.Get (i)->AddApplication (app_i);
      anim.UpdateNodeColor (SecundariosA.Get (i)->GetId (), 0, 255, 0); //verde
      //app_i->ImprimeTabla ();
    }
  for (uint32_t i = 0; i < SecundariosB.GetN (); i++)
    {
      //std::cout<< "ID2: "<< SecundariosB.Get(i)->GetId()<<std::endl;
      //std::ostringstream oss;
      Ptr<CustomApplicationBnodes> app_i = CreateObject<CustomApplicationBnodes> ();
      app_i->SetBroadcastInterval (Seconds (rand->GetInteger (1, 10)));
      //app_i->SetBroadcastInterval (Seconds (intervalB));
      app_i->SetStartTime (Seconds (0));
      app_i->m_n_channels = n_channels;
      app_i->iniciaCanales ();
      app_i->CreaBuffersCanales ();
      //app_i->SetStopTime (Seconds (simTime));
      SecundariosB.Get (i)->AddApplication (app_i);
      // std::cout << "El tiempo de broadcast en el nodo " << app_i->GetNode ()->GetId ()
      //          << " es :" << app_i->GetBroadcastInterval ().GetSeconds () << std::endl;
      anim.UpdateNodeColor (SecundariosB.Get (i)->GetId (), 0, 0, 255); //Azules
    }

  for (uint32_t i = 0; i < Primarios.GetN (); i++)
    {
      //std::cout<< "ID2: "<< SecundariosB.Get(i)->GetId()<<std::endl;
      //std::ostringstream oss;
      Ptr<CustomApplicationPnodes> app_i = CreateObject<CustomApplicationPnodes> ();
      //app_i->SetBroadcastInterval (Seconds (interval)); Que el broadcast se realice cada 100 ms
      app_i->SetStartTime (Seconds (0));
      app_i->m_n_channels = n_channels;
      app_i->SetBroadcastInterval (Seconds (TP));
      //app_i->SetStopTime (Seconds (simTime));
      Primarios.Get (i)->AddApplication (app_i);
      anim.UpdateNodeColor (Primarios.Get (i)->GetId (), 255, 164, 032); //naranja
    }
  for (uint32_t i = 0; i < Sink.GetN (); i++)
    {
      //std::cout<< "ID3: "<< Sink.Get(i)->GetId()<<std::endl;
      //std::ostringstream oss;
      Ptr<ApplicationSink> app_i = CreateObject<ApplicationSink> ();
      //app_i->SetBroadcastInterval (Seconds (interval));
      app_i->SetStartTime (Seconds (0));
      app_i->m_n_channels = n_channels;
      //app_i->SetStopTime (Seconds (simTime));
      Sink.Get (i)->AddApplication (app_i);
      anim.UpdateNodeColor (Sink.Get (i)->GetId (), 255, 255, 0); //amarillo
    }

  /*Termina instalación de la aplicación*/

  std::string path = "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop";
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
  //                 MakeCallback (&Rx));
  //Config::Connect (path, MakeCallback (&PhyRxDropTrace));

  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback (&MacTxTrace));
  //double new_range = 10;

  // Config::Set("/NodeList/"+ std::to_string(Sink.Get(0)->GetId())+"/DeviceList/*/$ns3::WifiNetDevice/Channel/$ns3::YansWifiChannel/PropagationLossModel/$ns3::RangePropagationLossModel/MaxRange", DoubleValue(new_range) );
  //Simulator::Stop (Seconds (simTime));
  SA = SecundariosA; /*Sirve para establecer el final de la simulación*/
  SB = SecundariosB; /*Sirve para establecer el final de la simulación*/

  /*flow_nodes->SetAttribute ("DelayBinWidth", DoubleValue (0.01));
  flow_nodes->SetAttribute ("JitterBinWidth", DoubleValue (0.01));
  flow_nodes->SetAttribute ("PacketSizeBinWidth", DoubleValue (1));*/
  AsciiTraceHelper ascii;
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("mobility-trace-example.mob"));

  Simulator::Schedule (Seconds (1), &verifica_termino_Simulacion);
  Simulator::Schedule (Seconds (1), &Muerte_nodo_B, sources);

  Simulator::Run (); //Termina simulación

  //flow_nodes->SerializeToXmlFile ("estadistics.xml", true, true);

  /*###################################################################################*/
  /*###################################################################################*/
  /*Termina lasimulacion */
  /*###################################################################################*/
  /*###################################################################################*/
  /* for (DeviceEnergyModelContainer::Iterator iter = deviceModels.Begin ();
       iter != deviceModels.End (); iter++)
    {
      double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
      NS_LOG_UNCOND ("End of simulation ("
                     << Simulator::Now ().GetSeconds ()
                     << "s) Total energy consumed by radio = " << energyConsumed << "J");
      // NS_ASSERT (energyConsumed <= 1.0);
    }*/
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
      fprintf (datos, "Nodo,No. Paquete,Estatus del paquete,Tiempo de entrega,No. SEQ, No. Veces "
                      "enviado,Tamaño en bytes,Semilla,TTS,No. Paquetes a enviar,TA,No. nodos "
                      "muertos,No. canales \n");
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
                 std::to_string (it->Tam_Paquete) + "," + std::to_string (Semilla_Sim) + "," +
                 std::to_string (m_Simulation_Time.GetSeconds ()) + "," +
                 std::to_string (n_Packets_A_Enviar) + "," + std::to_string (intervalA) + "," +
                 std::to_string (n_nodos_muertos) + "," + std::to_string (n_channels) + "\n";
          if (it->Estado)
            {
              fprintf (datos, "%s", data.c_str ());
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
  std::string comando = "chmod 777 /home/manuel/Escritorio/Datos_Sim/" + CSVFile;
  system (comando.c_str ());
  //system ("libreoffice /home/manuel/Escritorio/Datos_Sim/datos.csv &");
  return 0;
}
