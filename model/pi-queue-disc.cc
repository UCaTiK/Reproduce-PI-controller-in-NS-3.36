/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Priya S Tavarmani <priyast663@gmail.com>
 *          Viyom Mittal <viyommittal@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

/*
 * PORT NOTE: This code was ported from ns-2.36rc1 (queue/pi.cc).
 * Most of the comments are also ported from the same.
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "pi-queue-disc.h"
#include "ns3/drop-tail-queue.h"

#include "ns3/queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PiQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (PiQueueDisc);

TypeId PiQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PiQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<PiQueueDisc> ()
    .AddAttribute ("Mode",
                   "Determines unit for QueueLimit",
                   EnumValue (QueueSizeUnit::PACKETS),
                   MakeEnumAccessor (&PiQueueDisc::SetMode),
                   MakeEnumChecker (QueueSizeUnit::BYTES, "QUEUE_MODE_BYTES",
                                    QueueSizeUnit::PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MeanPktSize",
                   "Average of packet size",
                   UintegerValue (500),
                   MakeUintegerAccessor (&PiQueueDisc::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QueueRef",
                   "Desired queue size",
                   DoubleValue (50),
                   MakeDoubleAccessor (&PiQueueDisc::m_qRef),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("A",
                   "Value of alpha",
                   DoubleValue (0.00001822),
                   MakeDoubleAccessor (&PiQueueDisc::m_a),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("B",
                   "Value of beta",
                   DoubleValue (0.00001816),
                   MakeDoubleAccessor (&PiQueueDisc::m_b),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("W",
                   "Sampling frequency",
                   DoubleValue (170),
                   MakeDoubleAccessor (&PiQueueDisc::m_w),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("QueueLimit",
                   "Queue limit in bytes/packets",
                   DoubleValue (50),
                   MakeDoubleAccessor (&PiQueueDisc::SetQueueLimit),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MaxSize",
                   "The maximum number of packets accepted by this queue disc",
                   QueueSizeValue (QueueSize ("25p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
  ;

  return tid;
}

PiQueueDisc::PiQueueDisc ()
  : QueueDisc ()
{
//  NS_LOG_FUNCTION (this);
  m_uv = CreateObject<UniformRandomVariable> ();
  m_rtrsEvent = Simulator::Schedule (Time (Seconds (1.0 / m_w)), &PiQueueDisc::CalculateP, this);
}

PiQueueDisc::~PiQueueDisc ()
{
//  NS_LOG_FUNCTION (this);
}

void
PiQueueDisc::DoDispose (void)
{
//  NS_LOG_FUNCTION (this);
  m_uv = 0;
  Simulator::Remove (m_rtrsEvent);
  QueueDisc::DoDispose ();
}

void
PiQueueDisc::SetMode (QueueSizeUnit mode)
{
//  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

QueueSizeUnit
PiQueueDisc::GetMode (void)
{
//  NS_LOG_FUNCTION (this);
  return m_mode;
}

void
PiQueueDisc::SetQueueLimit (double lim)
{
//  NS_LOG_FUNCTION (this << lim);
  m_queueLimit = lim;
}

uint32_t
PiQueueDisc::GetQueueSize (void)
{
//  NS_LOG_FUNCTION (this);
  if (GetMode() == QueueSizeUnit::BYTES)
    {
      return GetInternalQueue (0)->GetNBytes ();
    }
  else if (GetMode() == QueueSizeUnit::PACKETS)
    {
      return GetInternalQueue (0)->GetNPackets ();
    }
  else
    {
      NS_ABORT_MSG ("Unknown PI mode.");
    }
}

uint32_t
PiQueueDisc::GetDropCount (void)
{
//  NS_LOG_FUNCTION (this);
  uint32_t drops = m_stats.forcedDrop + m_stats.unforcedDrop;
  return drops;
}

uint32_t
PiQueueDisc::GetThroughput (void)
{
//  NS_LOG_FUNCTION (this);
  uint32_t packetsDequeued = m_stats.packetsDequeued;
  m_stats.packetsDequeued = 0;
  return packetsDequeued * 10;
}

PiQueueDisc::Stats
PiQueueDisc::GetStats ()
{
//  NS_LOG_FUNCTION (this);
  return m_stats;
}

int64_t
PiQueueDisc::AssignStreams (int64_t stream)
{
//  NS_LOG_FUNCTION (this << stream);
  m_uv->SetStream (stream);
  return 1;
}

bool
PiQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
//  NS_LOG_FUNCTION (this << item);


  uint32_t nQueued = GetQueueSize ();

  if ((GetMode () == QueueSizeUnit::PACKETS && nQueued >= m_queueLimit)
      || (GetMode () == QueueSizeUnit::BYTES && nQueued + item->GetSize () > m_queueLimit))
    {
      // Drops due to queue limit: reactive
      DropBeforeEnqueue (item, "Forced drop");
      m_stats.forcedDrop++;
      NS_LOG_LOGIC ("\t QueueLength:: " << GetInternalQueue (0)->GetNPackets ());
      return false;
    }
  else if (DropEarly (item, nQueued))
    {
      // Early probability drop: proactive
      DropBeforeEnqueue (item, "Forced drop");
      m_stats.unforcedDrop++;
      NS_LOG_LOGIC ("\t QueueLength:: " << GetInternalQueue (0)->GetNPackets ());
      return false;
    }

  // No drop
  bool retval = GetInternalQueue (0)->Enqueue (item);
  NS_LOG_LOGIC ("\t QueueLength:: " << GetInternalQueue (0)->GetNPackets ());
  // If Queue::Enqueue fails, QueueDisc::Drop is called by the internal queue
  // because QueueDisc::AddInternalQueue sets the drop callback

//  NS_LOG_LOGIC ("\t bytesInQueue  " << GetInternalQueue (0)->GetNBytes ());
//  NS_LOG_LOGIC ("\t packetsInQueue  " << GetInternalQueue (0)->GetNPackets ());

  return retval;
}

void
PiQueueDisc::InitializeParams (void)
{
  m_dropProb = 0;
  m_stats.forcedDrop = 0;
  m_stats.unforcedDrop = 0;
  m_stats.packetsDequeued = 0;
  m_qOld = 0;
}

bool PiQueueDisc::DropEarly (Ptr<QueueDiscItem> item, uint32_t qSize)
{
//  NS_LOG_FUNCTION (this << item << qSize);

  double p = m_dropProb;
  bool earlyDrop = true;

  if (GetMode () == QueueSizeUnit::BYTES)
    {
      p = p * item->GetSize() / m_meanPktSize;
    }
  p = p > 1 ? 1 : p;

  double u =  m_uv->GetValue ();

  if (u > p)
    {
      earlyDrop = false;
    }
  if (!earlyDrop)
    {
      return false;
    }

  return true;
}

void PiQueueDisc::CalculateP ()
{
//  NS_LOG_FUNCTION (this);
  double p = 0.0;
  uint32_t qlen = GetQueueSize ();
  if (GetMode () == QueueSizeUnit::BYTES)
    {
      p = m_a * ((qlen * 1.0 / m_meanPktSize) - m_qRef) - m_b * ((m_qOld * 1.0 / m_meanPktSize) - m_qRef) + m_dropProb;
    }
  else
    {
      p = m_a * (qlen - m_qRef) - m_b * (m_qOld - m_qRef) + m_dropProb;
    }
  p = (p < 0) ? 0 : p;
  p = (p > 1) ? 1 : p;

  m_dropProb = p;
  m_qOld = qlen;
  m_rtrsEvent = Simulator::Schedule (Time (Seconds (1.0 / m_w)), &PiQueueDisc::CalculateP, this);
}

Ptr<QueueDiscItem>
PiQueueDisc::DoDequeue ()
{
//  NS_LOG_FUNCTION (this);

  if (GetInternalQueue (0)->IsEmpty ())
    {
//      NS_LOG_LOGIC ("Queue empty");
      NS_LOG_LOGIC ("\t QueueLength:: " << GetInternalQueue (0)->GetNPackets ());
      return 0;
    }

  Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ());
  m_stats.packetsDequeued += item->GetSize ();
  NS_LOG_LOGIC ("\t BytesDequeued:: " << item->GetSize ());
  NS_LOG_LOGIC ("\t QueueLength:: " << GetInternalQueue (0)->GetNPackets ());
  return item;
}

Ptr<const QueueDiscItem>
PiQueueDisc::DoPeek () const
{
//  NS_LOG_FUNCTION (this);
  if (GetInternalQueue (0)->IsEmpty ())
    {
//      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<const QueueDiscItem> item = StaticCast<const QueueDiscItem> (GetInternalQueue (0)->Peek ());

//  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
//  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return item;
}

bool
PiQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("PiQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("PiQueueDisc cannot have packet filters");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> >
                         ("MaxSize", QueueSizeValue (QueueSize(m_mode, m_queueLimit))));
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("PiQueueDisc needs 1 internal queue");
      return false;
    }

  if (GetInternalQueue (0)->GetMaxSize().GetUnit() != m_mode)
    {
      NS_LOG_ERROR ("The mode of the provided queue does not match the mode set on the PiQueueDisc");
      return false;
    }

  if ((m_mode == QueueSizeUnit::PACKETS || m_mode == QueueSizeUnit::BYTES) &&
        GetInternalQueue(0)->GetMaxSize ().GetValue() < m_queueLimit)
    {
      NS_LOG_ERROR ("The size of the internal queue is less than the queue disc limit");
      return false;
    }

  return true;
}

} //namespace ns3
