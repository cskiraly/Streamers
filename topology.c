/*
 *  Copyright (c) 2010 Csaba Kiraly
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#include <math.h>
#include <net_helper.h>
#include <peerset.h>
#include <peer.h>
#include <grapes_msg_types.h>
#include <topmanager.h>
#include <tman.h>

#include "topology.h"
#include "streaming.h"
#include "dbg.h"
#include "measures.h"

#define NEIGHBORHOOD_TARGET_SIZE 20
#define TMAN_MAX_IDLE 10
#define TMAN_LOG_EVERY 1000

static struct peerset *pset;
static struct timeval tout_bmap = {10, 0};
static int counter = 0;
static int simpleRanker (const void *tin, const void *p1in, const void *p2in);
static tmanRankingFunction rankFunct = simpleRanker;
static double my_metadata;
static int cnt = 0;
static struct nodeID *me = NULL;
static unsigned char mTypes[] = {MSG_TYPE_TOPOLOGY,MSG_TYPE_TMAN};
static struct nodeID ** neighbors;

static void update_metadata(void) {

#ifndef MONL
	my_metadata = 1 + ceil(((double)rand() / (double)RAND_MAX)*1000);
#endif
#ifdef MONL
	my_metadata = get_receive_delay();
#endif
}

static int simpleRanker (const void *tin, const void *p1in, const void *p2in) {

	double t,p1,p2;
	t = *((const double *)tin);
	p1 = *((const double *)p1in);
	p2 = *((const double *)p2in);

	if (isnan(t) || (isnan(p1) && isnan(p2))) return 0;
	else if (isnan(p1)) return 2;
	else if (isnan(p2)) return 1;
	else return (fabs(t-p1) == fabs(t-p2))?0:(fabs(t-p1) < fabs(t-p2))?1:2;

}

int topologyInit(struct nodeID *myID, const char *config)
{
	int i;
	for (i=0;i<2;i++)
		bind_msg_type(mTypes[i]);
	update_metadata();
	me = myID;
	return (topInit(myID, &my_metadata, sizeof(my_metadata), config) && tmanInit(myID,&my_metadata, sizeof(my_metadata),rankFunct,0));
}

void topologyShutdown(void)
{
}

int topoAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
	// TODO: check this!! Just to use this function to bootstrap ncast...
	if (counter < TMAN_MAX_IDLE)
		return topAddNeighbour(neighbour,metadata,metadata_size);
	else return tmanAddNeighbour(neighbour,metadata,metadata_size);
}

static int topoParseData(const uint8_t *buff, int len)
{
	int res = -1,ncs = 0,msize;
	const struct nodeID **n; const void *m;
	if (!buff || buff[0] == MSG_TYPE_TOPOLOGY) {
		res = topParseData(buff,len);
		if (counter <= TMAN_MAX_IDLE)
			counter++;
	}
	if (counter >= TMAN_MAX_IDLE && (!buff || buff[0] == MSG_TYPE_TMAN))
	{
		n = topGetNeighbourhood(&ncs);
		if (ncs) {
		m = topGetMetadata(&msize);
		res = tmanParseData(buff,len,n,ncs,m,msize);
		}
	}
  return res;
}

static const struct nodeID **topoGetNeighbourhood(int *n)
{
	int i; double d;
	if (counter > TMAN_MAX_IDLE) {
		uint8_t *mdata; int msize;
		*n = tmanGetNeighbourhoodSize();
		if (neighbors) free(neighbors);
		neighbors = calloc(*n,sizeof(struct nodeID *));
		tmanGetMetadata(&msize);
		mdata = calloc(*n,msize);
		tmanGivePeers(*n,neighbors,(void *)mdata);

		if (cnt % TMAN_LOG_EVERY == 0) {
			fprintf(stderr,"abouttopublish,%s,%s,,Tman_chunk_delay,%f\n",node_addr(me),node_addr(me),my_metadata);
			for (i=0;i<(*n) && i<NEIGHBORHOOD_TARGET_SIZE;i++) {
				d = *((double *)(mdata+i*msize));
				fprintf(stderr,"abouttopublish,%s,",node_addr(me));
				fprintf(stderr,"%s,,Tman_chunk_delay,%f\n",node_addr(neighbors[i]),d);
			}
			fprintf(stderr,"abouttopublish,%s,%s,,Tman_neighborhood_size,%d\n\n",node_addr(me),node_addr(me),*n);
		}

		free(mdata);
		return (const struct nodeID **)neighbors;
	}
	else
		return topGetNeighbourhood(n);
}

static void topoAddToBL (struct nodeID *id)
{
	if (counter >= TMAN_MAX_IDLE)
		tmanAddToBlackList(id);
//	else
		topAddToBlackList(id);
}

void add_peer(struct nodeID *id)
{
      dprintf("Adding %s to neighbourhood!\n", node_addr(id));
      peerset_add_peer(pset, id);
      /* add measures here */
      add_measures(id);
      send_bmap(peerset_get_peer(pset,id));
}

void remove_peer(struct nodeID *id)
{
      dprintf("Removing %s from neighbourhood!\n", node_addr(id));
      /* add measures here */
      delete_measures(id);
      peerset_remove_peer(pset, id);
}

// currently it just makes the peerset grow
void update_peers(struct nodeID *from, const uint8_t *buff, int len)
{
  int n_ids, i;
  static const struct nodeID **ids;
  struct peer *peers;
  struct timeval tnow, told;

//  dprintf("Update peers: topo_msg:%d, ",len);
//  if (from) {
//    dprintf("from:%s, ",node_addr(from));
//    if (peerset_check(pset, from) < 0) {
//      topAddNeighbour(from, NULL, 0);	//@TODO: this is agressive
//      if (!NEIGHBORHOOD_TARGET_SIZE || peerset_size(pset) < NEIGHBORHOOD_TARGET_SIZE) {
//        add_peer(from);
//      }
//    }
//  }

  dprintf("before:%d, ",peerset_size(pset));

  if (cnt++ % 100 == 0) {
	update_metadata();
	tmanChangeMetadata(&my_metadata,sizeof(my_metadata));
  }

  topoParseData(buff, len);
  ids = topoGetNeighbourhood(&n_ids);
  for(i = 0; i < n_ids; i++) {
    if(peerset_check(pset, ids[i]) < 0) {
      if (!NEIGHBORHOOD_TARGET_SIZE || peerset_size(pset) < NEIGHBORHOOD_TARGET_SIZE) {
        add_peer(ids[i]);
      }
    }
  }
  dprintf("after:%d, ",peerset_size(pset));

  if timerisset(&tout_bmap) {
    gettimeofday(&tnow, NULL);
    timersub(&tnow, &tout_bmap, &told);
    peers = peerset_get_peers(pset);
    for (i = 0; i < peerset_size(pset); i++) {
      if ( (!timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].creation_timestamp, &told, <) ) ||
           ( timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].bmap_timestamp, &told, <)     )   ) {
        //if (peerset_size(pset) > 1) {	// avoid dropping our last link to the world
        topoAddToBL(peers[i].id);
        remove_peer(peers[i--].id);
        //}
      }
    }
  }

  reg_neigh_size(peerset_size(pset));

  dprintf("after timer check:%d\n",peerset_size(pset));
}

struct peer *nodeid_to_peer(const struct nodeID* id, int reg)
{
  struct peer *p = peerset_get_peer(pset, id);
  if (!p) {
    fprintf(stderr,"warning: received message from unknown peer: %s!\n",node_addr(id));
    if (reg) {
      topoAddNeighbour(id, NULL, 0);
      add_peer(id);
      p = peerset_get_peer(pset,id);
    }
  }

  return p;
}

int peers_init(void)
{
  fprintf(stderr,"peers_init\n");
  pset = peerset_init(0);
  return pset ? 1 : 0;
}

struct peerset *get_peers(void)
{
  return pset;
}
