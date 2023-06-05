#ifndef PI_QUEUE_DISC_H
#define PI_QUEUE_DISC_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/timer.h"
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class TraceContainer;
class UniformRandomVariable;

/**
 * \ingroup traffic-control
 *
 * \brief Implements PI Active Queue Management discipline
 */
class PiQueueDisc : public QueueDisc
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief PiQueueDisc Constructor
   */
  PiQueueDisc ();

  /**
   * \brief PiQueueDisc Destructor
   */
  virtual ~PiQueueDisc ();

  /**
   * \brief Stats
   */
  typedef struct
  {
    uint32_t unforcedDrop;      //!< Early probability drops: proactive
    uint32_t forcedDrop;        //!< Drops due to queue limit: reactive
    uint32_t packetsDequeued;
  } Stats;

  /**
   * \brief Set the operating mode of this queue.
   *
   * \param mode The operating mode of this queue.
   */
  void SetMode (QueueSizeUnit mode);

  /**
   * \brief Get the encapsulation mode of this queue.
   *
   * \returns The encapsulation mode of this queue.
   */
  QueueSizeUnit GetMode (void);

  /**
   * \brief Get the current value of the queue in bytes or packets.
   *
   * \returns The queue size in bytes or packets.
   */
  uint32_t GetQueueSize (void);

  /**
   * \brief Set the limit of the queue in bytes or packets.
   *
   * \param lim The limit in bytes or packets.
   */
  void SetQueueLimit (double lim);

  /**
   * \brief Get drop count
   */
  uint32_t GetDropCount (void);
  
  /**
   * \brief Get throughput
   */
  uint32_t GetThroughput (void);
  /**
   * \brief Get PI statistics after running.
   *
   * \returns The drop statistics.
   */
  Stats GetStats ();

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

protected:
  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void) const;
  virtual bool CheckConfig (void);

  /**
   * \brief Initialize the queue parameters.
   */
  virtual void InitializeParams (void);

  /**
   * \brief Check if a packet needs to be dropped due to probability drop
   * \param item queue item
   * \param qSize queue size
   * \returns 0 for no drop, 1 for drop
   */
  bool DropEarly (Ptr<QueueDiscItem> item, uint32_t qSize);

  /**
   * Periodically update the drop probability based on the delay samples:
   * not only the current delay sample but also the trend where the delay
   * is going, up or down
   */
  void CalculateP ();

  Stats m_stats;                                //!< PI statistics

  // ** Variables supplied by user
  QueueSizeUnit m_mode;                      //!< Mode (bytes or packets)
  double m_queueLimit;                          //!< Queue limit in bytes / packets
  uint32_t m_meanPktSize;                       //!< Average packet size in bytes
  double m_qRef;                                //!< Desired queue size
  double m_a;                                   //!< Parameter to pi controller
  double m_b;                                   //!< Parameter to pi controller
  double m_w;                                   //!< Sampling frequency (Number of times per second)

  // ** Variables maintained by PI
  double m_dropProb;                            //!< Variable used in calculation of drop probability
  Time m_qDelay;                                //!< Current value of queue delay
  uint32_t m_qOld;                              //!< Old value of queue length
  double m_count;                               //!< Number of packets since last drop
  uint32_t m_countBytes;                        //!< Number of bytes since last drop
  EventId m_rtrsEvent;                          //!< Event used to decide the decision of interval of drop probability calculation
  Ptr<UniformRandomVariable> m_uv;              //!< Rng stream
};

};   // namespace ns3

#endif

