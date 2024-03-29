
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
#include "ns3/data-collector.h"
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
#include "ns3/traffic-control-helper.h"

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
      std::cout << context << " " << PrimTag.GetTypeId () << std::endl;
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
      std::cout << context << " " << reason << " " << PrimTag.GetTypeId () << std::endl;
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

      /*for (NodeContainer::Iterator nodoIT = SB.Begin (); nodoIT != SB.End (); nodoIT++)
        {
          Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
          Ptr<GaussMarkovMobilityModel> SBn_Pos =
              DynamicCast<GaussMarkovMobilityModel> ((*nodoIT)->GetObject<MobilityModel> ());
          std::cout << "EL pisodel nodo " << std::to_string ((*nodoIT)->GetId ())
                    << " es: " << std::to_string (MB->GetRoomNumberX ())
                    << std::to_string (MB->GetRoomNumberY ())
                    << std::to_string (MB->GetFloorNumber ()) << " | "
                    << "(" << std::to_string (SBn_Pos->GetPosition ().x) << ","
                    << std::to_string (SBn_Pos->GetPosition ().y) << ","
                    << std::to_string (SBn_Pos->GetPosition ().z) << ")" << std::endl;
        }
      for (NodeContainer::Iterator nodoIT = SA.Begin (); nodoIT != SA.End (); nodoIT++)
        {
          Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
          Ptr<ConstantPositionMobilityModel> SAn_Pos =
              DynamicCast<ConstantPositionMobilityModel> ((*nodoIT)->GetObject<MobilityModel> ());

          std::cout << "EL piso del nodo " << std::to_string ((*nodoIT)->GetId ())
                    << " es: " << std::to_string (MB->GetRoomNumberX ())
                    << std::to_string (MB->GetRoomNumberY ())
                    << std::to_string (MB->GetFloorNumber ()) << " | "
                    << "(" << std::to_string (SAn_Pos->GetPosition ().x) << ","
                    << std::to_string (SAn_Pos->GetPosition ().y) << ","
                    << std::to_string (SAn_Pos->GetPosition ().z) << ")" << std::endl;
        }*/
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
          //std::cout << "#####Tiempo: " <<Now().GetSeconds()<<"Nodo: "<<appI->GetNode ()->GetId ()<<" Estado> " <<it->Estado<< std::endl;
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
      Simulator::Schedule (Seconds (5), &verifica_termino_Simulacion);
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
  uint32_t n_SecundariosA = 25; //Numero de nodos alarmados en la red
  uint32_t n_SecundariosB = 225; //Numero de nodos ruteadores en la red
  uint32_t n_Primarios = 9; //Numero de estaciones base secundarias en la red
  uint32_t n_Sink = 1; //Numero de nodos Sink en la red
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  n_channels = 10; // Numero de canales, por default 8
  uint32_t Semilla_Sim = 3070854080;
  uint32_t escenario =
      3; //escenario 1.- edificio con muros 2.-Escenario en exteriores con obs. 3.- Escenario exteriores sin obstaculos
  uint32_t porcentajeOcupacion = 10;
  uint8_t RWP = 1;
  bool homogeneo = true;
  //const double R = 0.046; //J, Energy consumption on reception
  //const double T = 0.067; //J, Energy consumption on sending
  double simTime = 1000; //Tiempo de simulación
  // double interval = (rand->GetValue (0, 100)) / 100; //intervalo de broadcast
  //double interval = (rand()%100)/100.0; //intervalo de broadcast
  double intervalA = 20; //intervalo de broadcast
  //uint16_t N_channels = 1; //valor preestablecido para los canales
  uint32_t n_Packets_A_Enviar =
      1; //numero de paquetes a ser creados por cada uno de los nodos generadores
  uint32_t TP = 20;
  std::string CSVFile = "default.csv";
  bool StartSimulation = true;
  bool mebi = false;
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
  cmd.AddValue ("pO", "Porcentaje de ocupacion", porcentajeOcupacion);
  cmd.AddValue ("mebi", "Mecanismo de encaminamiento", mebi);
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

  Ptr<Building> b;
  MobilityHelper mobility_nB;

  double x_min = 10.0;
  double x_max = 160.0;
  double y_min = 10.0;
  double y_max = 60.0;
  double z_min = 0.0;
  double z_max = 50.0;
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
  MobilityHelper mobilit;
  Ptr<OutdoorPositionAllocator> position = CreateObject<OutdoorPositionAllocator> ();
  Ptr<UniformRandomVariable> xPos = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> yPos = CreateObject<UniformRandomVariable> ();
  /*Creación de nodos -> Contenedor.Create(Número de nodos)*/
  NodeContainer SecundariosA, Primarios, Sink;
  SecundariosA.Create (n_SecundariosA);
  SecundariosB.Create (n_SecundariosB);
  Primarios.Create (n_Primarios);
  Sink.Create (n_Sink);

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

      /*mobility_nB.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Mode", StringValue ("Time"),
                                    "Time", StringValue ("1s"), "Speed",
                                    StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                                    "Bounds", StringValue ("10|160|10|60"));*/
      mobility_nB.SetMobilityModel (
          "ns3::GaussMarkovMobilityModel", "Bounds",
          BoxValue (Box (x_min, x_max, y_min, y_max, z_min, z_max)), "TimeStep",
          TimeValue (Seconds (0.1)), "Alpha", DoubleValue (0.85), "MeanVelocity",
          StringValue ("ns3::UniformRandomVariable[Min=0|Max=100]"), "MeanDirection",
          StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"), "MeanPitch",
          StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"), "NormalVelocity",
          StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
          "NormalDirection",
          StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.8]"), "NormalPitch",
          StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));

      mobility_nB.SetPositionAllocator ("ns3::RandomBuildingPositionAllocator");
      /* mobility_nB.SetPositionAllocator (
          "ns3::RandomBoxPositionAllocator", "X",
          StringValue ("ns3::UniformRandomVariable[Min=10|Max=160]"), "Y",
          StringValue ("ns3::UniformRandomVariable[Min=10|Max=60]"), "Z",
          StringValue ("ns3::UniformRandomVariable[Min=0|Max=50]"));*/
      mobility_nB.Install (SecundariosB);

      BuildingsHelper::Install (SecundariosB);
      for (NodeContainer::Iterator nodoIT = SecundariosB.Begin (); nodoIT != SecundariosB.End ();
           nodoIT++)
        {
          Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
          Ptr<MobilityModel> mm = (*nodoIT)->GetObject<MobilityModel> ();
          MB->MakeConsistent (mm);
        }

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

      mobility_nB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility_nB.Install (Primarios);
      for (uint32_t i = 0; i < Primarios.GetN (); i++)
        {
          Ptr<ConstantPositionMobilityModel> PUs_Pos = DynamicCast<ConstantPositionMobilityModel> (
              Primarios.Get (i)->GetObject<MobilityModel> ());
          PUs_Pos->SetPosition (Vector (i * 30 + 20, 35, 0.1));
        }
      BuildingsHelper::Install (Primarios);
      for (NodeContainer::Iterator nodoIT = Primarios.Begin (); nodoIT != Primarios.End ();
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
      xPos->SetAttribute ("Min", DoubleValue (-streetWidth));
      xPos->SetAttribute ("Max", DoubleValue (maxAxisX));
      yPos->SetAttribute ("Min", DoubleValue (-streetWidth));
      yPos->SetAttribute ("Max", DoubleValue (maxAxisY));
      position->SetAttribute ("X", PointerValue (xPos));
      position->SetAttribute ("Y", PointerValue (yPos));
      mobility_nB.SetPositionAllocator (position);
      // install the mobility model
      mobility_nB.Install (SecundariosB);
      BuildingsHelper::Install (SecundariosB);

      for (NodeContainer::Iterator nodoIT = SecundariosB.Begin (); nodoIT != SecundariosB.End ();
           nodoIT++)
        {
          Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
          Ptr<MobilityModel> mm = (*nodoIT)->GetObject<MobilityModel> ();
          MB->MakeConsistent (mm);
        }

      mobility_nB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility_nB.SetPositionAllocator ("ns3::RandomBuildingPositionAllocator");
      mobility_nB.Install (SecundariosA);
      mobility_nB.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility_nB.Install (Primarios);
      for (uint32_t i = 0; i < Primarios.GetN (); i++)
        {
          Ptr<ConstantPositionMobilityModel> PUs_Pos = DynamicCast<ConstantPositionMobilityModel> (
              Primarios.Get (i)->GetObject<MobilityModel> ());
          PUs_Pos->SetPosition (Vector (i * 30 + 20, 35, 0.1));
        }
      BuildingsHelper::Install (Primarios);
      for (NodeContainer::Iterator nodoIT = Primarios.Begin (); nodoIT != Primarios.End ();
           nodoIT++)
        {
          Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
          Ptr<MobilityModel> mm = (*nodoIT)->GetObject<MobilityModel> ();
          MB->MakeConsistent (mm);
        }
      break;
    //default:
    /*Se crea el escenario con elmodelo de movilidad RPGM paralos nodos tipo B
      Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
      ns2.Install ();
      break;*/
    case 3:
      /*Movilidad Secundarios*/
      mobilit.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", "X",
                                    StringValue ("ns3::UniformRandomVariable[Min=0|Max=500]"), "Y",
                                    StringValue ("ns3::UniformRandomVariable[Min=0|Max=500]"), "Z",
                                    StringValue ("ns3::UniformRandomVariable[Min=0|Max=0]"));
      // mobilit.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      switch (RWP)
        {
        case 1:
          mobilit.SetMobilityModel (
              "ns3::GaussMarkovMobilityModel", "Bounds", BoxValue (Box (0, 500, 0, 500, 0, 0)),
              "TimeStep", TimeValue (Seconds (1)), "Alpha", DoubleValue (0.85), "MeanVelocity",
              StringValue ("ns3::UniformRandomVariable[Min=0|Max=4]"), "MeanDirection",
              StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"), "MeanPitch",
              StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"), "NormalVelocity",
              StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
              "NormalDirection",
              StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.8]"),
              "NormalPitch",
              StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));

          /* mobility_nB.SetPositionAllocator (
              "ns3::RandomBoxPositionAllocator", "X",
              StringValue ("ns3::UniformRandomVariable[Min=10|Max=160]"), "Y",
              StringValue ("ns3::UniformRandomVariable[Min=10|Max=60]"), "Z",
              StringValue ("ns3::UniformRandomVariable[Min=0|Max=50]"));*/
          break;
        case 2:
          mobilit.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Mode", StringValue ("Time"),
                                    "Time", StringValue ("0.1s"), "Speed",
                                    StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                                    "Bounds", StringValue ("0|300|0|300"));
          break;
        case 3:
          mobilit.SetMobilityModel ("ns3::RandomDirection2dMobilityModel", "Bounds",
                                    RectangleValue (Rectangle (0, 300, 0, 300)), "Speed",
                                    StringValue ("ns3::ConstantRandomVariable[Constant=5]"),
                                    "Pause",
                                    StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
          break;

        default:
          //probar con nodos estaticos
          std::cout << "No ha seleccionado el modelo de movilidad a utilizar" << std::endl;
          break;
        }
      mobilit.Install (SecundariosB);
      mobilit.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      //mobilit.Install (SecundariosB);
      mobilit.Install (SecundariosA);
      if (n_SecundariosA == 1)
        {

          Ptr<ConstantPositionMobilityModel> SA_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosA.Get (0)->GetObject<MobilityModel> ());
          SA_Pos->SetPosition (Vector (500, 500, 0));
          Ptr<ConstantPositionMobilityModel> SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (0)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (428, 428, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (1)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (356, 356, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (2)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (284, 284, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (3)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (212, 212, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (4)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (140, 140, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (5)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (68, 68, 0));
          //el alcance de los nodos es de 103 metros aprox
        }
      else if (n_SecundariosA == 2)
        {
          Ptr<ConstantPositionMobilityModel> SA_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosA.Get (0)->GetObject<MobilityModel> ());
          SA_Pos->SetPosition (Vector (500, 500, 0));
          SA_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosA.Get (1)->GetObject<MobilityModel> ());
          SA_Pos->SetPosition (Vector (0, 500, 0));
          Ptr<ConstantPositionMobilityModel> SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (0)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (428, 428, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (1)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (356, 356, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (2)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (284, 284, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (3)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (212, 212, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (4)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (140, 140, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (5)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (68, 68, 0));

        }
      else if (n_SecundariosA == 3)
        {
          Ptr<ConstantPositionMobilityModel> SA_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosA.Get (0)->GetObject<MobilityModel> ());
          SA_Pos->SetPosition (Vector (500, 500, 0));
          SA_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosA.Get (1)->GetObject<MobilityModel> ());
          SA_Pos->SetPosition (Vector (0, 500, 0));
          SA_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosA.Get (2)->GetObject<MobilityModel> ());
          SA_Pos->SetPosition (Vector (500, 0, 0));
          Ptr<ConstantPositionMobilityModel> SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (0)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (428, 428, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (1)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (356, 356, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (2)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (284, 284, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (3)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (212, 212, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (4)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (140, 140, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (5)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (68, 68, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (6)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (68, 428, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (7)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (140, 356, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (8)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (428, 68, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (9)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (284, 212, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (10)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (212, 284, 0));
          SB_Pos = DynamicCast<ConstantPositionMobilityModel> (
              SecundariosB.Get (11)->GetObject<MobilityModel> ());
          SB_Pos->SetPosition (Vector (356, 140, 0));

        }

      /* mobilit.SetPositionAllocator ("ns3::RandomDiscPositionAllocator", "X", StringValue ("250"),
                                    "Y", StringValue ("250"), "Rho",
                                    StringValue ("ns3::UniformRandomVariable[Min=0|Max=250]"));*/
      mobilit.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (75), "MinY",
                                    DoubleValue (75), "DeltaX", DoubleValue (150), "DeltaY",
                                    DoubleValue (150), "GridWidth", UintegerValue (3), "LayoutType",
                                    StringValue ("RowFirst"));
      mobilit.Install (Primarios);
      /* for (uint32_t i = 0; i < Primarios.GetN (); i++)
        {
          Ptr<ConstantPositionMobilityModel> PUs_Pos = DynamicCast<ConstantPositionMobilityModel> (
              Primarios.Get (i)->GetObject<MobilityModel> ());
          PUs_Pos->SetPosition (Vector (i * 30 + 20, 35, 0.1));
        }
      */
      break;
    }

  /*Termina configuración del primer escenario */

  /*Se configura el modelo de movilidad*/

  //BuildingsHelper::MakeMobilityModelConsistent ();

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
  Sink_Pos->SetPosition (Vector (0, 0, 0));
  if (escenario != 3)
    {
      BuildingsHelper::Install (Sink);
    }

  /*for (NodeContainer::Iterator nodoIT = Sink.Begin (); nodoIT != Sink.End (); nodoIT++)
    {
      Ptr<MobilityBuildingInfo> MB = (*nodoIT)->GetObject<MobilityBuildingInfo> ();
      Ptr<MobilityModel> mm = (*nodoIT)->GetObject<MobilityModel> ();
      MB->MakeConsistent (mm);
    }*/
  //BuildingsHelper::MakeMobilityModelConsistent ();
  /*Termina la configuración de la movilidad*/

  /*Configuración del canal(canales)*/

  YansWifiChannelHelper wifiChannelSink;

  wifiChannelSink.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  wifiChannelSink.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange",
                                      DoubleValue (1000));
  YansWifiChannelHelper wifiChannelPrimarios;

  if (escenario == 1)
    { // caso del edificio

      wifiChannelPrimarios.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      wifiChannelPrimarios.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange",
                                               DoubleValue (150));
    }
  else
    {
      wifiChannelPrimarios.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      wifiChannelPrimarios.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange",
                                               DoubleValue (150));
    }
  std::list<uint32_t> List_RangeOfChannels;
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  WifiHelper wifi, wifiSink, wifiPrimarios;
  wifi.SetStandard (WIFI_STANDARD_80211g);
  wifiSink.SetStandard (WIFI_STANDARD_80211g);
  wifiPrimarios.SetStandard (WIFI_STANDARD_80211g);

  /*#####Implementación de los canales en los nodos tipo A y tipo B*/
  for (uint32_t i = 0; i < n_channels; i++)
    {
      YansWifiChannelHelper wifiChannel;
      wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");

      YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
      wifiPhy.SetChannel (wifiChannel.Create ());
      wifiPhy.Set ("ChannelWidth", UintegerValue (20));
      wifiPhy.Set ("TxPowerStart", DoubleValue (5));
      wifiPhy.Set ("TxPowerEnd", DoubleValue (10));
      wifiPhy.Set ("TxPowerLevels", UintegerValue (2));
      wifi.Install (wifiPhy, wifiMac,
                    SecundariosA); //Device para comunicar a los nodos tipo A y B
      wifi.Install (wifiPhy, wifiMac,
                    SecundariosB); //Device para comunicar a los nodos tipo A y B
      wifi.Install (
          wifiPhy, wifiMac,
          Primarios); //Device para comunicar a los nodos tipo A y B con los nodos primarios
      wifi.Install (wifiPhy, wifiMac,
                    Sink); //Device para comunicar a los nodos tipo A y B con el nodo Sink
      /*Ese ciclo for instala de 0 a n-1 interfaces en los Nodecontainer de Sencundarios A,B,Primarios y el Sink */
    }
  //-------------------->>>>>>>>>>>Probar e identificar que interfaces son las del Sink y las de los usuarios primarios
  /*Termina los canales */
  /*Termina conf. del canal*/
  //Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  /*Se configura la capa fisica del dispositivo*/
  YansWifiPhyHelper wifiPhySink = YansWifiPhyHelper::Default ();
  wifiPhySink.SetChannel (wifiChannelSink.Create ());
  wifiPhySink.Set ("TxPowerStart", DoubleValue (10));
  wifiPhySink.Set ("TxPowerEnd", DoubleValue (40));
  wifiPhySink.Set ("TxPowerLevels", UintegerValue (16));
  YansWifiPhyHelper wifiPhyPrimarios = YansWifiPhyHelper::Default ();
  wifiPhyPrimarios.SetChannel (wifiChannelPrimarios.Create ());
  wifiPhyPrimarios.Set ("TxPowerStart", DoubleValue (10));
  wifiPhyPrimarios.Set ("TxPowerEnd", DoubleValue (40));
  wifiPhyPrimarios.Set ("TxPowerLevels", UintegerValue (16));
  /*Termina capa fisica*/

  //wifiPhy.SetPcapDataLinkType (
  //WifiPhyHelper::DLT_IEEE802_11_RADIO); /*Añade informacion adicional sobre el enlace*/
  //AsciiTraceHelper ascii;
  //wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
  //Aqui se pueden generar los archivos .pcap par wireshark
  /*wifiPhy.Set ("TxPowerStart", DoubleValue (10)); //5
  wifiPhy.Set ("TxPowerEnd", DoubleValue (40)); //33
  wifiPhy.Set ("TxPowerLevels", UintegerValue (16)); //8*/

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
  anim.SetMaxPktsPerTraceFile (5000000);
  uint32_t img_phone = anim.AddResource ("/home/manolo/bake/source/ns-3.32/cel.png");
  uint32_t img_tower = anim.AddResource ("/home/manolo/bake/source/ns-3.32/torre2.png");
  uint32_t img_alarmado = anim.AddResource ("/home/manolo/bake/source/ns-3.32/response.png");
  uint32_t img_fm = anim.AddResource ("/home/manolo/bake/source/ns-3.32/fire-man.png");

  /*Comienza la instalación de la aplicación en los nodos*/
  for (uint32_t i = 0; i < SecundariosA.GetN (); i++)
    {
      //uint32_t value = rand->GetInteger (1, 20);
      //std::cout<<"Inicia en "<<std::to_string (value)<<std::endl;
      Ptr<CustomApplication> app_i = CreateObject<CustomApplication> ();
      //app_i->SetStartTime (Seconds ((double) value));
      app_i->SetStartTime (Seconds (0));
      app_i->SetBroadcastInterval (Seconds (intervalA));
      app_i->IniciaTabla (n_Packets_A_Enviar, SecundariosA.Get (i)->GetId ());

      app_i->SetChannels (n_channels);
      app_i->iniciaCanales ();
      //app_i->CanalesDisponibles ();
      app_i->CreaBuffersCanales ();
      SecundariosA.Get (i)->AddApplication (app_i);
      //app_i->ImprimeTabla();
      anim.UpdateNodeColor (SecundariosA.Get (i)->GetId (), 0, 255, 0); //verde
      anim.UpdateNodeImage (SecundariosA.Get (i)->GetId (), img_alarmado);
    }
  for (uint32_t i = 0; i < SecundariosB.GetN (); i++)
    {
      Ptr<CustomApplicationBnodes> app_i = CreateObject<CustomApplicationBnodes> ();
      app_i->SetBroadcastInterval (Seconds (rand->GetInteger (1, 20)));
      app_i->SetStartTime (Seconds (0));
      app_i->SetChannels (n_channels);
      app_i->iniciaCanales ();
      app_i->CreaBuffersCanales ();
      app_i->m_mebi = mebi;
      SecundariosB.Get (i)->AddApplication (app_i);
      anim.UpdateNodeColor (SecundariosB.Get (i)->GetId (), 0, 0, 255); //Azules
      anim.UpdateNodeImage (SecundariosB.Get (i)->GetId (), img_phone);
    }

  for (uint32_t i = 0; i < Primarios.GetN (); i++)
    {
      Ptr<CustomApplicationPnodes> app_i = CreateObject<CustomApplicationPnodes> ();
      app_i->SetStartTime (Seconds (0));
      app_i->SetChannels (n_channels);
      app_i->SetBroadcastInterval (Seconds (TP));
      //app_i->m_porcentaje_Ch_disp = (uint8_t) rand->GetInteger (1, 3) * 10;
      app_i->m_porcentaje_Ch_disp = 20;
      //std::cout<<"El nodo primario "<<i<<" Tiene "<<std::to_string (app_i->m_porcentaje_Ch_disp)<<" de ocupacion"<<std::endl;
      Primarios.Get (i)->AddApplication (app_i);
      anim.UpdateNodeColor (Primarios.Get (i)->GetId (), 255, 164, 032); //naranja
      anim.UpdateNodeImage (Primarios.Get (i)->GetId (), img_tower);
    }
  for (uint32_t i = 0; i < Sink.GetN (); i++)
    { //En este caso solo es un solo Sink
      Ptr<ApplicationSink> app_i = CreateObject<ApplicationSink> ();
      app_i->SetStartTime (Seconds (0));
      app_i->SetChannels (n_channels);
      app_i->CreaBuffersCanales ();
      Sink.Get (i)->AddApplication (app_i);
      anim.UpdateNodeColor (Sink.Get (i)->GetId (), 255, 255, 0); //amarillo
      anim.UpdateNodeImage (Sink.Get (i)->GetId (), img_fm);
    }
  Packet::EnablePrinting ();
  /*Termina instalación de la aplicación*/

  std::string path = "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop";
  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
  //                MakeCallback (&Rx));
  // Config::Connect (path, MakeCallback (&PhyRxDropTrace));

  //Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback (&MacTxTrace));
  //double new_range = 10;

  // Config::Set("/NodeList/"+ std::to_string(Sink.Get(0)->GetId())+"/DeviceList/*/$ns3::WifiNetDevice/Channel/$ns3::YansWifiChannel/PropagationLossModel/$ns3::RangePropagationLossModel/MaxRange", DoubleValue(new_range) );
  //Simulator::Stop (Seconds (simTime));
  SA = SecundariosA; /*Sirve para establecer el final de la simulación*/
  SB = SecundariosB; /*Sirve para establecer el final de la simulación*/

  /*flow_nodes->SetAttribute ("DelayBinWidth", DoubleValue (0.01));
  flow_nodes->SetAttribute ("JitterBinWidth", DoubleValue (0.01));
  flow_nodes->SetAttribute ("PacketSizeBinWidth", DoubleValue (1));*/
  //AsciiTraceHelper ascii;
  //MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("mobility-trace-example.mob"));

  Simulator::Schedule (Seconds (1), &verifica_termino_Simulacion);
  Simulator::Schedule (Seconds (1), &Muerte_nodo_B, sources);
  //Ptr<FlowMonitor> flowMonitor;
  //FlowMonitorHelper flowHelper;
  //flowMnitor = flowHelper.InstallAll ();

  Simulator::Run (); //Termina simulación
  //flowMonitor->SerializeToXmlFile ("results.xml", true, true);

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
  //double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * m_Simulation_Time.GetSeconds ()));

  FILE *datos;
  if (StartSimulation)
    {
      std::cout << "Post Simulation: " << std::endl;
      std::string ruta = "/home/manolo/bake/source/ns-3.32/results/" + CSVFile;
      datos = fopen (ruta.c_str (), "w");
      /* std::string info =
          "Información de la simulación | Secundarios A: " + std::to_string (n_SecundariosA) +
          " | Secundarios B: " + std::to_string (n_SecundariosB) + "| Primarios " +
          std::to_string (n_Primarios) + "\n";
      fprintf (datos, info.c_str ());*/
      fprintf (datos, "Nodo,No. Paquete,Estatus del paquete,Tiempo de entrega,No. SEQ, No. Veces "
                      "enviado,Tamaño en bytes,Semilla,TTS,No. Paquetes a enviar,TA,No. nodos "
                      "muertos,No. canales,No. colisiones,No. Iteracion,Total de Colisiones,SG \n");
    }
  else
    {
      //std::string ruta = "/home/manuel/Escritorio/Datos_Sim/" + CSVFile;
      std::string ruta = "/home/manolo/bake/source/ns-3.32/results/" + CSVFile;
      datos = fopen (ruta.c_str (), "a");
    }
  uint32_t colisiones = 0;
  for (uint32_t i = 0; i < SecundariosB.GetN (); i++)
    {
      Ptr<CustomApplicationBnodes> appI =
          DynamicCast<CustomApplicationBnodes> (SecundariosB.Get (i)->GetApplication (0));
      std::string data;
      colisiones += appI->m_collissions;
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
                 std::to_string (n_nodos_muertos) + "," + std::to_string (n_channels) + "," +
                 std::to_string (appI->m_collissions) + "," + std::to_string (n_iteracion) + "," +
                 std::to_string (colisiones) + "," + std::to_string (it->SG) + "\n";
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
  std::string comando = "chmod 777 /home/manolo/bake/source/ns-3.32/results/" + CSVFile;
  system (comando.c_str ());

  //system ("libreoffice /home/manuel/Escritorio/Datos_Sim/datos.csv &");
  return 0;
}
