#include <stdint.h>
#include <math.h>
#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif
#ifndef INFINITY
#define INFINITY       (1.0/0.0)
#endif

#include "measures.h"

/*
 * Register duplicate arrival
*/
void reg_chunk_duplicate()
{
}

/*
 * Register playout/loss of a chunk before playout
*/
void reg_chunk_playout(int id, bool b, uint64_t timestamp)
{
}

/*
 * Register actual neghbourhood size
*/
void reg_neigh_size(int s)
{
}

/*
 * Register chunk receive event
*/
void reg_chunk_receive(int id, uint64_t timestamp, int hopcount)
{
}

/*
 * Register chunk send event
*/
void reg_chunk_send(int id)
{
}

/*
 * Register chunk accept evemt
*/
void reg_offer_accept(bool b)
{
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
