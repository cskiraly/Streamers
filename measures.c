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

static struct timeval print_tdiff = {1, 0};
static struct timeval print_tstartdiff = {20, 0};
static struct timeval print_tstart;

static int duplicates = 0;
static int chunks = 0;
static int played = 0;
static uint64_t sum_reorder_delay = 0;

static int chunks_received = 0;
static int sum_hopcount = 0;
static uint64_t sum_receive_delay = 0;

static int chunks_sent = 0;

static int neighsize = 0;

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

  if (chunks) print_measure("ChunksPlayed", (double)chunks);
  if (chunks) print_measure("DuplicateRatio", (double)duplicates / chunks);
  if (chunks) print_measure("PlayoutRatio", (double)played / chunks);
  if (chunks) print_measure("ReorderDelay", (double)sum_reorder_delay / 1e6 / chunks);
  print_measure("NeighSize", (double)neighsize);
  if (chunks_received) print_measure("OverlayDistance", (double)sum_hopcount / chunks_received);
  if (chunks_received) print_measure("ReceiveDelay", (double)sum_receive_delay / 1e6 / chunks_received);

  gettimeofday(&tnow, NULL);
  if (timerisset(&print_tstart)) print_measure("ChunkReceiveRate", (double) chunks_received / tdiff_sec(&tnow, &print_tstart));
  if (timerisset(&print_tstart)) print_measure("ChunkSendRate", (double) chunks_sent / tdiff_sec(&tnow, &print_tstart));
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

  chunks_received++;
  sum_hopcount += hopcount;
  gettimeofday(&tnow, NULL);
  sum_receive_delay += (tnow.tv_usec + tnow.tv_sec * 1000000ULL) - timestamp;
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
