/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdio.h>
#include <stdint.h>
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

static struct timeval print_tdiff = {3600, 0};
static struct timeval print_tstartdiff = {60, 0};
static struct timeval print_tstart;

static int duplicates = 0;
static int chunks = 0;
static int played = 0;
static uint64_t sum_reorder_delay = 0;

static int chunks_received_dup = 0, chunks_received_nodup = 0, chunks_received_old = 0;
static int sum_hopcount = 0;
static uint64_t sum_receive_delay = 0;

static int chunks_sent = 0;

static int neighsize = 0;

static uint64_t bytes_sent, bytes_sent_chunk, bytes_sent_sign, bytes_sent_topo;
static int msgs_sent, msgs_sent_chunk, msgs_sent_sign, msgs_sent_topo;

static uint64_t bytes_recvd, bytes_recvd_chunk, bytes_recvd_sign, bytes_recvd_topo;
static int msgs_recvd, msgs_recvd_chunk, msgs_recvd_sign, msgs_recvd_topo;

static uint64_t sum_offers_in_flight;
static int samples_offers_in_flight;
static double sum_queue_delay;
static int samples_queue_delay;


double tdiff_sec(const struct timeval *a, const struct timeval *b)
{
  struct timeval tdiff;
  timersub(a, b, &tdiff);
  return tdiff.tv_sec + tdiff.tv_usec / 1000000.0;
}

void print_measure(const char *name, double value)
{
  fprintf(stderr,"abouttopublish,,,,%s,%f,,,\n", name, value);
}

void print_measures()
{
  struct timeval tnow;

  if (chunks) print_measure("PlayoutRatio", (double)played / chunks);
  if (chunks) print_measure("ReorderDelay(ok&lost)", (double)sum_reorder_delay / 1e6 / chunks);
  print_measure("NeighSize", (double)neighsize);
  if (chunks_received_nodup) print_measure("OverlayDistance(intime&nodup)", (double)sum_hopcount / chunks_received_nodup);
  if (chunks_received_nodup) print_measure("ReceiveDelay(intime&nodup)", (double)sum_receive_delay / 1e6 / chunks_received_nodup);

  gettimeofday(&tnow, NULL);
  if (timerisset(&print_tstart)) print_measure("ChunkRate", (double) chunks / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(all)", (double) (chunks_received_old + chunks_received_nodup + chunks_received_dup)  / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(old)", (double) chunks_received_old / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(intime&nodup)", (double) chunks_received_nodup / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate(intime&dup)", (double) chunks_received_dup / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkSendRate", (double) chunks_sent / tdiff_sec(&tnow, &print_tstart));

  if (timerisset(&print_tstart)) {
    print_measure("SendRateMsgs(all)", (double) msgs_sent / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateMsgs(chunk)", (double) msgs_sent_chunk / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateMsgs(sign)", (double) msgs_sent_sign / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateMsgs(topo)", (double) msgs_sent_topo / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateMsgs(other)", (double) (msgs_sent - msgs_sent_chunk - msgs_sent_sign - msgs_sent_topo) / tdiff_sec(&tnow, &print_tstart));

    print_measure("SendRateBytes(all)", (double) bytes_sent / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateBytes(chunk)", (double) bytes_sent_chunk / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateBytes(sign)", (double) bytes_sent_sign / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateBytes(topo)", (double) bytes_sent_topo / tdiff_sec(&tnow, &print_tstart));
    print_measure("SendRateBytes(other)", (double) (bytes_sent - bytes_sent_chunk - bytes_sent_sign - bytes_sent_topo) / tdiff_sec(&tnow, &print_tstart));

    print_measure("RecvRateMsgs(all)", (double) msgs_recvd / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateMsgs(chunk)", (double) msgs_recvd_chunk / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateMsgs(sign)", (double) msgs_recvd_sign / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateMsgs(topo)", (double) msgs_recvd_topo / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateMsgs(other)", (double) (msgs_recvd - msgs_recvd_chunk - msgs_recvd_sign - msgs_recvd_topo) / tdiff_sec(&tnow, &print_tstart));

    print_measure("RecvRateBytes(all)", (double) bytes_recvd / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateBytes(chunk)", (double) bytes_recvd_chunk / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateBytes(sign)", (double) bytes_recvd_sign / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateBytes(topo)", (double) bytes_recvd_topo / tdiff_sec(&tnow, &print_tstart));
    print_measure("RecvRateBytes(other)", (double) (bytes_recvd - bytes_recvd_chunk - bytes_recvd_sign - bytes_recvd_topo) / tdiff_sec(&tnow, &print_tstart));
  }

  if (chunks_received_old + chunks_received_nodup + chunks_received_dup) print_measure("ReceiveRatio(intime&nodup-vs-all)", (double)chunks_received_nodup / (chunks_received_old + chunks_received_nodup + chunks_received_dup));

  if (samples_offers_in_flight) print_measure("OffersInFlight", (double)sum_offers_in_flight / samples_offers_in_flight);
  if (samples_queue_delay) print_measure("QueueDelay", sum_queue_delay / samples_queue_delay);

}

bool print_every()
{
  static struct timeval tnext;
  static bool startup = true;
  struct timeval tnow;

  gettimeofday(&tnow, NULL);
  if (startup) {
    if (!timerisset(&print_tstart)) {
      timeradd(&tnow, &print_tstartdiff, &print_tstart);
    }
    if (timercmp(&tnow, &print_tstart, <)) {
      return false;
    } else {
      startup = false;
    }
  }

  if (!timerisset(&tnext)) {
    timeradd(&print_tstart, &print_tdiff, &tnext);
  }
  if (!timercmp(&tnow, &tnext, <)) {
    print_measures();
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

  duplicates++;
}

/*
 * Register playout/loss of a chunk before playout
*/
void reg_chunk_playout(int id, bool b, uint64_t timestamp)
{
  struct timeval tnow;

  if (!print_every()) return;

  played += b ? 1 : 0;
  chunks++;
  gettimeofday(&tnow, NULL);
  sum_reorder_delay += (tnow.tv_usec + tnow.tv_sec * 1000000ULL) - timestamp;
}

/*
 * Register actual neghbourhood size
*/
void reg_neigh_size(int s)
{
  if (!print_every()) return;

  neighsize = s;
}

/*
 * Register chunk receive event
*/
void reg_chunk_receive(int id, uint64_t timestamp, int hopcount, bool old, bool dup)
{
  struct timeval tnow;

  if (!print_every()) return;

  if (old) {
    chunks_received_old++;
  } else {
    if (dup) { //duplicate detection works only for in-time arrival
      chunks_received_dup++;
    } else {
      chunks_received_nodup++;
      sum_hopcount += hopcount;
      gettimeofday(&tnow, NULL);
      sum_receive_delay += (tnow.tv_usec + tnow.tv_sec * 1000000ULL) - timestamp;
    }
  }
}

/*
 * Register chunk send event
*/
void reg_chunk_send(int id)
{
  if (!print_every()) return;

  chunks_sent++;
}

/*
 * Register chunk accept evemt
*/
void reg_offer_accept(bool b)
{
  static int offers = 0;
  static int accepts = 0;

  offers++;
  if (b) accepts++;
}

/*
 * messages sent (bytes vounted at message content level)
*/
void reg_message_send(int size, uint8_t type)
{
  if (!print_every()) return;

  bytes_sent += size;
  msgs_sent++;

  switch (type) {
   case MSG_TYPE_CHUNK:
     bytes_sent_chunk+= size;
     msgs_sent_chunk++;
     break;
   case MSG_TYPE_SIGNALLING:
     bytes_sent_sign+= size;
     msgs_sent_sign++;
     break;
   case MSG_TYPE_TOPOLOGY:
   case MSG_TYPE_TMAN:
     bytes_sent_topo+= size;
     msgs_sent_topo++;
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

  bytes_recvd += size;
  msgs_recvd++;

  switch (type) {
   case MSG_TYPE_CHUNK:
     bytes_recvd_chunk+= size;
     msgs_recvd_chunk++;
     break;
   case MSG_TYPE_SIGNALLING:
     bytes_recvd_sign+= size;
     msgs_recvd_sign++;
     break;
   case MSG_TYPE_TOPOLOGY:
   case MSG_TYPE_TMAN:
     bytes_recvd_topo+= size;
     msgs_recvd_topo++;
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

  sum_offers_in_flight += running_offers_threads;
  samples_offers_in_flight++;
}

/*
 * Register the sample for RTT
*/
void reg_queue_delay(double last_queue_delay)
{
  if (!print_every()) return;

  sum_queue_delay += last_queue_delay;
  samples_queue_delay++;
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
