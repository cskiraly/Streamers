/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif
#ifndef INFINITY
#define INFINITY       (1.0/0.0)
#endif
#include <sys/time.h>

#include "measures.h"
#include "grapes_msg_types.h"

struct timeval print_tdiff = {3600, 0};
struct timeval tstartdiff = {60, 0};
static struct timeval tstart;
static struct timeval print_tstart;
static struct timeval tnext;

struct measures {
  int duplicates;
  int chunks;
  int played;
  uint64_t sum_reorder_delay;

  int chunks_received_dup, chunks_received_nodup, chunks_received_old;
  int sum_hopcount;
  uint64_t sum_receive_delay;

  int chunks_sent;

  uint64_t sum_neighsize;
  int samples_neighsize;

  uint64_t bytes_sent, bytes_sent_chunk, bytes_sent_sign, bytes_sent_topo;
  int msgs_sent, msgs_sent_chunk, msgs_sent_sign, msgs_sent_topo;

  uint64_t bytes_recvd, bytes_recvd_chunk, bytes_recvd_sign, bytes_recvd_topo;
  int msgs_recvd, msgs_recvd_chunk, msgs_recvd_sign, msgs_recvd_topo;

  uint64_t sum_offers_in_flight;
  int samples_offers_in_flight;
  double sum_queue_delay;
  int samples_queue_delay;

  int offers;
  int accepts;
};

static struct measures m;

void clean_measures()
{
  memset(&m, 0, sizeof(m));
}

double tdiff_sec(const struct timeval *a, const struct timeval *b)
{
  struct timeval tdiff;
  timersub(a, b, &tdiff);
  return tdiff.tv_sec + tdiff.tv_usec / 1000000.0;
}

void print_measure(const char *name, double value)
{
  fprintf(stderr,"abouttopublish,,,,%s,%f,,,%f\n", name, value, tdiff_sec(&tnext, &tstart));
}

void print_measures()
{
  struct timeval tnow;

  if (m.chunks) print_measure("PlayoutRatio", (double)m.played / m.chunks);
  if (m.chunks) print_measure("ReorderDelay(ok&lost)", (double)m.sum_reorder_delay / 1e6 / m.chunks);
  if (m.samples_neighsize) print_measure("NeighSize", (double)m.sum_neighsize / m.samples_neighsize);
  if (m.chunks_received_nodup) print_measure("OverlayDistance(intime&nodup)", (double)m.sum_hopcount / m.chunks_received_nodup);
  if (m.chunks_received_nodup) print_measure("ReceiveDelay(intime&nodup)", (double)m.sum_receive_delay / 1e6 / m.chunks_received_nodup);

  gettimeofday(&tnow, NULL);
  if (timerisset(&print_tstart)) print_measure("ChunkRate", (double) m.chunks / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(all)", (double) (m.chunks_received_old + m.chunks_received_nodup + m.chunks_received_dup)  / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(old)", (double) m.chunks_received_old / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(intime&nodup)", (double) m.chunks_received_nodup / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(intime&dup)", (double) m.chunks_received_dup / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkSendRate", (double) m.chunks_sent / tdiff_sec(&tnow, &print_tstart));

  if (timerisset(&print_tstart)) {
    print_measure("SendRateMsgs(all)", (double) m.msgs_sent / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateMsgs(chunk)", (double) m.msgs_sent_chunk / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateMsgs(sign)", (double) m.msgs_sent_sign / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateMsgs(topo)", (double) m.msgs_sent_topo / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateMsgs(other)", (double) (m.msgs_sent - m.msgs_sent_chunk - m.msgs_sent_sign - m.msgs_sent_topo) / tdiff_sec(&tnow, &print_tstart));

    print_measure("SendRateBytes(all)", (double) m.bytes_sent / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateBytes(chunk)", (double) m.bytes_sent_chunk / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateBytes(sign)", (double) m.bytes_sent_sign / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateBytes(topo)", (double) m.bytes_sent_topo / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateBytes(other)", (double) (m.bytes_sent - m.bytes_sent_chunk - m.bytes_sent_sign - m.bytes_sent_topo) / tdiff_sec(&tnow, &print_tstart));

    print_measure("RecvRateMsgs(all)", (double) m.msgs_recvd / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateMsgs(chunk)", (double) m.msgs_recvd_chunk / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateMsgs(sign)", (double) m.msgs_recvd_sign / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateMsgs(topo)", (double) m.msgs_recvd_topo / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateMsgs(other)", (double) (m.msgs_recvd - m.msgs_recvd_chunk - m.msgs_recvd_sign - m.msgs_recvd_topo) / tdiff_sec(&tnow, &print_tstart));

    print_measure("RecvRateBytes(all)", (double) m.bytes_recvd / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateBytes(chunk)", (double) m.bytes_recvd_chunk / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateBytes(sign)", (double) m.bytes_recvd_sign / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateBytes(topo)", (double) m.bytes_recvd_topo / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateBytes(other)", (double) (m.bytes_recvd - m.bytes_recvd_chunk - m.bytes_recvd_sign - m.bytes_recvd_topo) / tdiff_sec(&tnow, &print_tstart));
  }

  if (m.chunks_received_old + m.chunks_received_nodup + m.chunks_received_dup) print_measure("ReceiveRatio(intime&nodup-vs-all)", (double)m.chunks_received_nodup / (m.chunks_received_old + m.chunks_received_nodup + m.chunks_received_dup));

  if (m.samples_offers_in_flight) print_measure("OffersInFlight", (double)m.sum_offers_in_flight / m.samples_offers_in_flight);
  if (m.samples_queue_delay) print_measure("QueueDelay", m.sum_queue_delay / m.samples_queue_delay);

  if (timerisset(&print_tstart)) {
    print_measure("OfferRate", (double) m.offers / tdiff_sec(&tnow, &print_tstart));
    print_measure("AcceptRate", (double) m.accepts / tdiff_sec(&tnow, &print_tstart));
  }
  if (m.offers) print_measure("OfferAcceptRatio", (double)m.accepts / m.offers);
}

bool print_every()
{
  static bool startup = true;
  struct timeval tnow;

  gettimeofday(&tnow, NULL);
  if (startup) {
    if (!timerisset(&tstart)) {
      timeradd(&tnow, &tstartdiff, &tstart);
      print_tstart = tstart;
    }
    if (timercmp(&tnow, &tstart, <)) {
      return false;
    } else {
      startup = false;
    }
  }

  if (!timerisset(&tnext)) {
    timeradd(&tstart, &print_tdiff, &tnext);
  }
  if (!timercmp(&tnow, &tnext, <)) {
    print_measures();
    clean_measures();
    print_tstart = tnext;
    timeradd(&tnext, &print_tdiff, &tnext);
  }
  return true;
}

/*
 * Register duplicate arrival
*/
void reg_chunk_duplicate()
{
  if (!print_every()) return;

  m.duplicates++;
}

/*
 * Register playout/loss of a chunk before playout
*/
void reg_chunk_playout(int id, bool b, uint64_t timestamp)
{
  struct timeval tnow;

  if (!print_every()) return;

  m.played += b ? 1 : 0;
  m.chunks++;
  gettimeofday(&tnow, NULL);
  m.sum_reorder_delay += (tnow.tv_usec + tnow.tv_sec * 1000000ULL) - timestamp;
}

/*
 * Register actual neghbourhood size
*/
void reg_neigh_size(int s)
{
  if (!print_every()) return;

  m.sum_neighsize += s;
  m.samples_neighsize++;
}

/*
 * Register chunk receive event
*/
void reg_chunk_receive(int id, uint64_t timestamp, int hopcount, bool old, bool dup)
{
  struct timeval tnow;

  if (!print_every()) return;

  if (old) {
    m.chunks_received_old++;
  } else {
    if (dup) { //duplicate detection works only for in-time arrival
      m.chunks_received_dup++;
    } else {
      m.chunks_received_nodup++;
      m.sum_hopcount += hopcount;
      gettimeofday(&tnow, NULL);
      m.sum_receive_delay += (tnow.tv_usec + tnow.tv_sec * 1000000ULL) - timestamp;
    }
  }
}

/*
 * Register chunk send event
*/
void reg_chunk_send(int id)
{
  if (!print_every()) return;

  m.chunks_sent++;
}

/*
 * Register chunk accept evemt
*/
void reg_offer_accept(bool b)
{
  m.offers++;
  if (b) m.accepts++;
}

/*
 * messages sent (bytes vounted at message content level)
*/
void reg_message_send(int size, uint8_t type)
{
  if (!print_every()) return;

  m.bytes_sent += size;
  m.msgs_sent++;

  switch (type) {
   case MSG_TYPE_CHUNK:
     m.bytes_sent_chunk+= size;
     m.msgs_sent_chunk++;
     break;
   case MSG_TYPE_SIGNALLING:
     m.bytes_sent_sign+= size;
     m.msgs_sent_sign++;
     break;
   case MSG_TYPE_TOPOLOGY:
   case MSG_TYPE_TMAN:
     m.bytes_sent_topo+= size;
     m.msgs_sent_topo++;
     break;
   default:
     break;
  }
}

/*
 * messages sent (bytes vounted at message content level)
*/
void reg_message_recv(int size, uint8_t type)
{
  if (!print_every()) return;

  m.bytes_recvd += size;
  m.msgs_recvd++;

  switch (type) {
   case MSG_TYPE_CHUNK:
     m.bytes_recvd_chunk+= size;
     m.msgs_recvd_chunk++;
     break;
   case MSG_TYPE_SIGNALLING:
     m.bytes_recvd_sign+= size;
     m.msgs_recvd_sign++;
     break;
   case MSG_TYPE_TOPOLOGY:
   case MSG_TYPE_TMAN:
     m.bytes_recvd_topo+= size;
     m.msgs_recvd_topo++;
     break;
   default:
     break;
  }
}

/*
 * Register the number of offers in flight
*/
void reg_offers_in_flight(int running_offers_threads)
{
  if (!print_every()) return;

  m.sum_offers_in_flight += running_offers_threads;
  m.samples_offers_in_flight++;
}

/*
 * Register the sample for RTT
*/
void reg_queue_delay(double last_queue_delay)
{
  if (!print_every()) return;

  m.sum_queue_delay += last_queue_delay;
  m.samples_queue_delay++;
}

/*
 * Initialize peer level measurements
*/
void init_measures()
{
}

/*
 * End peer level measurements
*/
void end_measures()
{
  print_measures();
}

/*
 * Initialize p2p measurements towards a peer
*/
void add_measures(struct nodeID *id)
{
}

/*
 * Delete p2p measurements towards a peer
*/
void delete_measures(struct nodeID *id)
{
}
