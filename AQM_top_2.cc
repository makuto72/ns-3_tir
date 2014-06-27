#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleGlobalRoutingExample");

int 
main (int argc, char *argv[])
{
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
#if 0 
  LogComponentEnable ("SimpleGlobalRoutingExample", LOG_LEVEL_INFO);
#endif

  //DefaultValue::Bind ("DropTailQueue::m_maxPackets", 30);

  // Allow the user to override any of the defaults and the above
  // DefaultValue::Bind ()s at run-time, via command-line arguments
  CommandLine cmd;
  bool enableFlowMonitor = false;
  cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.Parse (argc, argv);
  

  // --------------------- CREAZIONE NODI ------------------------------
  NS_LOG_INFO ("Create nodes.");
  NodeContainer c;
  c.Create (3);
  NodeContainer n0n1 = NodeContainer (c.Get (0), c.Get (1));
  NodeContainer n2n1 = NodeContainer (c.Get (1), c.Get (2));

  InternetStackHelper internet;
  internet.Install (c);
  

  // -------------------- CREAZIONE CANALI -----------------------------
  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
  // Canale nodo1<-->router
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer d0d1 = p2p.Install (n0n1);
  // Canale router<-->nodo2
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer d2d1 = p2p.Install (n2n1);
  

  // ----------------- ASSEGNAZIONE INDIRIZZI --------------------------
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i1 = ipv4.Assign (d2d1);
  

  // ------------------- TABELLE DI ROUTING ----------------------------
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  

  // ----------------- CREAZIONE APPLICAZIONI --------------------------
  NS_LOG_INFO ("Create Applications.");
  
  // Definizione porte
  uint16_t port_ftp = 20;         // FTP data
  //uint32_t maxBytes = 10000000; //inutile?
  
  // Mittente FTP (server su nodo 0)
  OnOffHelper onoff_ftp1("ns3::TcpSocketFactory", Address(InetSocketAddress (i2i1.GetAddress (0), port_ftp)));
  onoff_ftp1.SetConstantRate (DataRate ("5Mbps"));
  ApplicationContainer ftp_server1 = onoff_ftp1.Install (c.Get (0));
  ftp_server1.Start(Seconds(0.0));
  ftp_server1.Stop(Seconds(11.0)); 
  
  // Destinatario FTP (client su nodo 2)
  PacketSinkHelper sink_ftp1 ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_ftp)));
  ApplicationContainer ftp_client1 = sink_ftp1.Install (c.Get (2));
  ftp_client1.Start (Seconds (0.0));
  ftp_client1.Stop (Seconds (15.0));

  // Mittente FTP (server su nodo 2)
  OnOffHelper onoff_ftp2("ns3::TcpSocketFactory", Address(InetSocketAddress (i0i1.GetAddress (0), port_ftp)));
  onoff_ftp2.SetConstantRate (DataRate ("2Mbps"));
  ApplicationContainer ftp_server2 = onoff_ftp2.Install (c.Get (2));
  ftp_server2.Start(Seconds(0.0));
  ftp_server2.Stop(Seconds(11.0)); 
  
  // Destinatario FTP (client su nodo 0)
  PacketSinkHelper sink_ftp2 ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny (), port_ftp)));
  ApplicationContainer ftp_client2 = sink_ftp2.Install (c.Get (0));
  ftp_client2.Start (Seconds (0.0));
  ftp_client2.Stop (Seconds (15.0));

  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("iRouting_AQM_top2.tr"));
  p2p.EnablePcapAll ("iRouting_AQM_top2");

  // Flow Monitor
  FlowMonitorHelper flowmonHelper;
  if (enableFlowMonitor){
      flowmonHelper.InstallAll();
  }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  NS_LOG_INFO ("Done.");

  if (enableFlowMonitor){
      flowmonHelper.SerializeToXmlFile ("simple-global-routing.flowmon", false, false);
  }

  Simulator::Destroy ();
  return 0;
}