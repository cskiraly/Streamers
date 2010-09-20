/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdint.h>
#include <math.h>
#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif
#ifndef INFINITY
#define INFINITY       (1.0/0.0)
#endif

#include "measures.h"

static int duplicates = 0;
static int chunks = 0;
static int played = 0;

static int neighsize = 0;

/*
 * Register duplicate arrival
*/
void reg_chunk_duplicate()
{
  duplicates++;
}

/*
 * Register playout/loss of a chunk before playout
*/
void reg_chunk_playout(int id, bool b, uint64_t timestamp)
{
  played += b ? 1 : 0;
  chunks++;
}

/*
 * Register actual neghbourhood size
*/
void reg_neigh_size(int s)
{
  neighsize = s;
}

/*
 * Register chunk receive event
*/
void reg_chunk_receive(int id, uint64_t timestamp, int hopcount)
{
  static int chunks_received = 0;
  static int sum_hopcount = 0;

  chunks_received++;
  sum_hopcount += hopcount;
}

/*
 * Register chunk send event
*/
void reg_chunk_send(int id)
{
  static int chunks_sent = 0;

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
