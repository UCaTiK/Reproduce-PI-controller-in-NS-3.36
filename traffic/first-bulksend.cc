/*
 * This script simulates light TCP traffic for PI evaluation
 * Authors: Viyom Mittal and Mohit P. Tahiliani
 * Wireless Information Networking Group
 * NITK Surathkal, Mangalore, India
*/

/* Network topology
 *
 *           10Mb/s, 5ms              10Mb/s, 50ms              10Mb/s, 5ms
 *   (n1-n5)-------------(gateway0)------------------(gateway1)-------------(sink)
 *   5 nodes                        QueueLimit = 200
 *
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <fstream>
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-header.h"
#include "ns3/traffic-control-module.h"
#include  <string>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("PiTests");

// Файл для записи результатов
stringstream filePlotQueue;
// Переменная для подсчета количества вызовов CheckQueueSize
uint32_t checkTimes = 0;
// Переменная для хранения суммарного значения всей длины очереди
double avgQueueDiscSize = 0;

// Метод для вывода размера очереди и среднего значентия очереди в отдельный файл
void CheckQueueSize (Ptr<QueueDisc> queue)
{
	// Запись размера очереди в переменную
	uint32_t qSize = StaticCast<PiQueueDisc> (queue)->GetQueueSize ();

	// Изменяем глобальные переменные для нахождения среднего размера очереди
	avgQueueDiscSize += qSize;
	checkTimes++;

	// Вызываем данный метод через 0.1 секунду 
	Simulator::Schedule (Seconds (0.1), &CheckQueueSize, queue);

	// Запись в файл размера очереди и среднего размера очереди
	ofstream fPlotQueue (filePlotQueue.str ().c_str (), ios::out | ios::app);
	fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << " " << avgQueueDiscSize / checkTimes << endl;
	fPlotQueue.close ();
}

int main (int argc, char *argv[])
{
	// Вывод статистики
	bool printPiStats = true;
	// Время начала симуляции
	float startTime = 0.0;		// в секундах
	// Длительность симуляции
	float simDuration = 101;	// в секундах
	// Время окончания симуляции
	float stopTime = startTime + simDuration;		// в секундах
	// Каталог для записи выводимых файлов
	string pathOut = ".";
	// Запись данных очереди в файл
	bool writeForPlot = true;

	// Параметры уязвимого места
	string bottleneckBandwidth = "10Mbps";
	string bottleneckDelay = "50ms";

	// Параметры всей остальной сети
	string accessBandwidth = "10Mbps";
	string accessDelay = "5ms";

	// Параметры алгоритма PI
	// Средний размер одного пакета
	uint32_t meanPktSize = 1000;		// В байтах
	//
	string piMode = "QUEUE_MODE_PACKETS";
	// Желаемый размер очереди для PI
	uint32_t piQueueRef = 50;
	// Предел очереди
	uint32_t piQueueLimit = 200;
	// А параметр (unused)
	// uint32_t A = 0.00001822*2;
	// B параметр (unused)
	// uint32_t B = 0.00001816*2;

	// Возможность менять параметры из консоли
	CommandLine cmd;
	cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
	cmd.AddValue ("writeForPlot", "<0/1> to write results for plot (gnuplot)", writeForPlot);
	cmd.Parse (argc,argv);

	LogComponentEnable ("PiQueueDisc", LOG_LEVEL_INFO);

	// 5 узлов источников
	NodeContainer source;
	source.Create (5);

	// 2 связующих шлюза
	NodeContainer gateway;
	gateway.Create (2);

	// 1 приёмник
	NodeContainer sink;
	sink.Create (1);

	Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", StringValue ("13p"));
	Config::SetDefault ("ns3::PfifoFastQueueDisc::MaxSize", QueueSizeValue (QueueSize ("50p")));

	// Значение времени ожидания для отложенных подтверждений TCP (в секундах)
	Config::SetDefault ("ns3::TcpSocket::DelAckTimeout", TimeValue(Seconds (0)));
	// Выключение алгоритма ограничения передачи
	Config::SetDefault ("ns3::TcpSocketBase::LimitedTransmit", BooleanValue (false));
	// Максимальный размер сегмента TCP в байтах (может быть скорректирован в зависимости от оббнаружения MTU)
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (meanPktSize));
	// Включение возможности TCP window scale (параметр для увеличения размера окна приема)
	Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (true));

	// Настройка параметров PI алгоритма
	// Средний размер пакета
	Config::SetDefault ("ns3::PiQueueDisc::MeanPktSize", UintegerValue (meanPktSize));
	// Режим работы (В данном случае с пакетами)
	Config::SetDefault ("ns3::PiQueueDisc::Mode", StringValue (piMode));
	// Желаемый размер очереди
	Config::SetDefault ("ns3::PiQueueDisc::QueueRef", DoubleValue (piQueueRef));
	// Предел очереди
	Config::SetDefault ("ns3::PiQueueDisc::QueueLimit", DoubleValue (piQueueLimit));
	// Возможность изменить параметры в расчете p
	// Config::SetDefault ("ns3::PiQueueDisc::A", DoubleValue (A));
	// Config::SetDefault ("ns3::PiQueueDisc::B", DoubleValue (B));

	NS_LOG_INFO ("Install internet stack on all nodes.");
	// Даёт возможность узлам использовать протоколы ip/tcp/udp
	InternetStackHelper internet;
	internet.InstallAll ();

	// Настройка pfifo(алгоритм обслуживания очередей работа с пакетами)
	// С помощью TrafficControlHelper и устанавливаются все QueueDisc протоколы
	TrafficControlHelper tchPfifo;
	// Установка основного алгоритма
	uint16_t handle = tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");
	// Настраиваем DropTail как алгоритм обработки очереди
	tchPfifo.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));

	// Настройка PI алгоритма на одельный TrafficControlHelper
	TrafficControlHelper tchPi;
	tchPi.SetRootQueueDisc ("ns3::PiQueueDisc");

	// Настройка параметров основных связей
	PointToPointHelper accessLink;
	accessLink.SetQueue ("ns3::DropTailQueue");
	accessLink.SetDeviceAttribute ("DataRate", StringValue (accessBandwidth));
	accessLink.SetChannelAttribute ("Delay", StringValue (accessDelay));

	// Все 5 узлов соединяются с первым шлюзом с нормальными параметрами соединения
	NetDeviceContainer devices[5];
	for (int i = 0; i < 5; i++) {
		devices[i] = accessLink.Install (source.Get (i), gateway.Get (0));
		tchPfifo.Install (devices[i]);
	}

	// Правый шлюз соединяем с приёмником с нормальными параметрами соединения
	NetDeviceContainer devices_sink;
	devices_sink = accessLink.Install (gateway.Get (1), sink.Get (0));
	tchPfifo.Install (devices_sink);

	// Настраиваем узкое место сети, которое устанавливаем между двумя шлюзами
	PointToPointHelper bottleneckLink;
	bottleneckLink.SetQueue ("ns3::DropTailQueue");
	bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bottleneckBandwidth));
	bottleneckLink.SetChannelAttribute ("Delay", StringValue (bottleneckDelay));

	NetDeviceContainer devices_gateway;
	devices_gateway = bottleneckLink.Install (gateway.Get (0), gateway.Get (1));
	// Только здесь используется PI алгоритма
	QueueDiscContainer queueDiscs = tchPi.Install (devices_gateway);

	NS_LOG_INFO ("Assign IP Addresses");
	// Указываем адрес всей сети (с маской)
	Ipv4AddressHelper address;
	address.SetBase ("10.0.0.0", "255.255.255.0");

	// Переменные для хранения данных о соединении
	Ipv4InterfaceContainer interfaces[5];
	Ipv4InterfaceContainer interfaces_sink;
	Ipv4InterfaceContainer interfaces_gateway;

	// Выдаём всем источникам возможные адреса из заданной сети
	for (int i = 0; i < 5; i++) {
		address.NewNetwork ();
		interfaces[i] = address.Assign (devices[i]);
	}

	// Выдаём адрес для соединения шлюз-приёмник
	address.NewNetwork ();
	interfaces_sink = address.Assign (devices_sink);

	// Выдаём адрес для соединения шлюз-шлюз
	address.NewNetwork ();
	interfaces_gateway = address.Assign (devices_gateway);

	NS_LOG_INFO ("Initialize Global Routing.");
	// Создаёт базу данных маршрутизации и инициализирует таблицы маршрутизации узлов в моделировании
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	uint16_t port = 50000;
	Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

	// Адрес приёмника
	AddressValue remoteAddress (InetSocketAddress (interfaces_sink.GetAddress (1), port));
	for (uint16_t i = 0; i < source.GetN (); i++) {
		// Настраиваем вспомогающие приложения для генерации трафика
		BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
		// Адрес приёмника
		ftp.SetAttribute ("Remote", remoteAddress);
		// размер данных которые отпарвляются каждый раз
		ftp.SetAttribute ("SendSize", UintegerValue (meanPktSize));

		// Установка ftp на узлы и запуск установка параметров запуска и окончания работы
		ApplicationContainer sourceApp = ftp.Install (source.Get (i));
		sourceApp.Start (Seconds (startTime));
		sourceApp.Stop (Seconds (stopTime - 1));
	}
	// Установка работы приёмника
	sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
	ApplicationContainer sinkApp = sinkHelper.Install (sink);
	sinkApp.Start (Seconds (startTime));
	sinkApp.Stop (Seconds (stopTime));

	// Запись в файл данных очереди
	if (writeForPlot) {
		filePlotQueue << pathOut << "/" << "pi-queue1.plotme";
		remove (filePlotQueue.str ().c_str ());
		Ptr<QueueDisc> queue = queueDiscs.Get (0);
		Simulator::ScheduleNow (&CheckQueueSize, queue);
	}

	// Запуск симуляции
	Simulator::Stop (Seconds (stopTime));
	Simulator::Run ();

	// Вывод информации о выкинутых пакетах
	if (printPiStats) {
		PiQueueDisc::Stats st = StaticCast<PiQueueDisc> (queueDiscs.Get (0))->GetStats ();
		cout << "*** pi stats from bottleneck queue ***" << endl;
		cout << "\t " << st.unforcedDrop << " drops due to probability " << endl;
		cout << "\t " << st.forcedDrop << " drops due queue full" << endl;
	}

	Simulator::Destroy ();
	return 0;
}
