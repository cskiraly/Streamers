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
#include <string.h>

#include <math.h>
#include <net_helper.h>
#include <peerset.h>
#include <peer.h>
#include <grapes_msg_types.h>
#include <topmanager.h>
#include <tman.h>

#include "compatibility/timer.h"

#include "topology.h"
#include "nodeid_set.h"
#include "streaming.h"
#include "dbg.h"
#include "measures.h"
#include "streamer.h"

#define MIN(A,B) (((A) < (B)) ? (A) : (B))
#define MAX(A,B) (((A) > (B)) ? (A) : (B))

double desired_bw = 1000000;
double desired_rtt = 0.2;
double alpha_target = 0.4;
double topo_mem = 0.7;

bool topo_out = true; //peer selects out-neighbours
bool topo_in = true; //peer selects in-neighbours (combined means bidirectional)

bool topo_keep_best = false;
bool topo_add_best = false;

int NEIGHBORHOOD_TARGET_SIZE = 30;
#define TMAN_MAX_IDLE 10
#define TMAN_LOG_EVERY 1000

#define STREAMER_TOPOLOGY_MSG_ADD 0
#define STREAMER_TOPOLOGY_MSG_REMOVE 1

static struct peerset *pset;
static struct timeval tout_bmap = {20, 0};
static int counter = 0;
static int simpleRanker (const void *tin, const void *p1in, const void *p2in);
static tmanRankingFunction rankFunct = simpleRanker;

struct metadata {
  uint16_t cb_size;
  uint16_t cps;
  float capacity;
  float recv_delay;
} __attribute__((packed));

static struct metadata my_metadata;
static int cnt = 0;
static struct nodeID *me = NULL;
static unsigned char mTypes[] = {MSG_TYPE_TOPOLOGY,MSG_TYPE_TMAN,MSG_TYPE_STREAMER_TOPOLOGY};
static struct nodeID ** neighbors;

static void update_metadata(void) {
	my_metadata.cb_size = am_i_source() ? 0 : get_cb_size();
	my_metadata.recv_delay = get_receive_delay();
	my_metadata.cps = get_chunks_per_sec();
	my_metadata.capacity = get_capacity();
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
	for (i=0;i<sizeof(mTypes)/sizeof(mTypes[0]);i++)
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
	struct metadata m = {0};	//TODO: check what metadata option should mean

	if (counter < TMAN_MAX_IDLE)
		return topAddNeighbour(neighbour,&m,sizeof(m));
	else return tmanAddNeighbour(neighbour,&m,sizeof(m));
}

static int topoParseData(const uint8_t *buff, int len)
{
	int res = -1,ncs = 0,msize;
	const struct nodeID **n; const void *m;
	if (!buff || buff[0] == MSG_TYPE_TOPOLOGY) {
		res = topParseData(buff,len);
//		if (counter <= TMAN_MAX_IDLE)
//			counter++;
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
			fprintf(stderr,"abouttopublish,%s,%s,,Tman_chunk_delay,%f\n",node_addr(me),node_addr(me),my_metadata.recv_delay);
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

//TODO: send metadata as well
static int send_topo_msg(struct nodeID *dst, uint8_t type)
{
  char msg[2];
  msg[0] = MSG_TYPE_STREAMER_TOPOLOGY;
  msg[1] = type;
  return send_to_peer(me, dst, msg, 2);
}

static void add_peer(const struct nodeID *id, const struct metadata *m, bool local, bool remote)
{
  if (local) {
      dprintf("Adding %s to neighbourhood! cb_size:%d\n", node_addr(id), m?m->cb_size:-1);
      peerset_add_peer(pset, id);
      if (m) peerset_get_peer(pset, id)->cb_size = m->cb_size;
      if (m) peerset_get_peer(pset, id)->capacity = m->capacity;
      /* add measures here */
      add_measures(id);
      send_bmap(id);
  }
  if (remote) {
      dtprintf("Topo: explicit topo message sent!!! to %s (peers:%d)\n", node_addr(id));
      send_topo_msg(id, STREAMER_TOPOLOGY_MSG_ADD);
  }
}

static void remove_peer(const struct nodeID *id, bool local, bool remote)
{
  if (local) {
      dprintf("Removing %s from neighbourhood!\n", node_addr(id));
      /* add measures here */
      delete_measures(id);
      peerset_remove_peer(pset, id);
  }
  if (remote) {
      dtprintf("Topo: explicit topo message sent!!! to %s (peers:%d)\n", node_addr(id));
      send_topo_msg(id, STREAMER_TOPOLOGY_MSG_REMOVE);
  }
}

//get the rtt. Currenly only MONL version is supported
static double get_rtt_of(const struct nodeID* n){
#ifdef MONL
  return get_rtt(n);
#else
  return NAN;
#endif
}

//get the declared capacity of a node
static double get_capacity_of(const struct nodeID* n){
  struct peer *p = peerset_get_peer(pset, n);
  if (p) {
    return p->capacity;
  }

  return NAN;
}

//returns: 1:yes 0:no -1:unknown
int desiredness(const struct nodeID* n) {
  double rtt = get_rtt_of(n);
  double bw =  get_capacity_of(n);

  if ((isnan(rtt) && finite(desired_rtt)) || (isnan(bw) && desired_bw > 0)) {
    return -1;
  } else if ((isnan(rtt) || rtt <= desired_rtt) && (isnan(bw) || bw >= desired_bw)) {
    return 1;
  }

  return 0;
}

bool is_desired(const struct nodeID* n) {
  return (desiredness(n) == 1);
}

int cmp_rtt(const struct nodeID* a, const struct nodeID* b) {
  double ra, rb;
  ra = get_rtt_of(a);
  rb = get_rtt_of(a);
  if ((isnan(ra) && isnan(rb)) || ra == rb) return 0;
  else if (isnan(rb) || ra < rb) return -1;
  else return 1;
}

int vcmp_rtt(const void* a, const void* b) {
  return cmp_rtt((const struct nodeID*)a, (const struct nodeID*)b);
}

// currently it just makes the peerset grow
void update_peers(struct nodeID *from, const uint8_t *buff, int len)
{
  int n_ids, metasize, i, newids_size, max_ids;
  static const struct nodeID **newids;
  static const struct metadata *metas;
  struct peer *peers;
  struct timeval tnow, told;
  static const struct nodeID **savedids;
  static int savedids_size;

  if timerisset(&tout_bmap) {
    gettimeofday(&tnow, NULL);
    timersub(&tnow, &tout_bmap, &told);
    peers = peerset_get_peers(pset);
    for (i = 0; i < peerset_size(pset); i++) {
      if ( (!timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].creation_timestamp, &told, <) ) ||
           ( timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].bmap_timestamp, &told, <)     )   ) {
        ftprintf(stderr,"Topo: dropping inactive %s (peers:%d)\n", node_addr(peers[i].id), peerset_size(pset));
        //if (peerset_size(pset) > 1) {	// avoid dropping our last link to the world
        topoAddToBL(peers[i].id);
        remove_peer(peers[i--].id, true, true);
        //}
      }
    }
  }

  if (cnt++ % 100 == 0) {
	update_metadata();
    if (counter > TMAN_MAX_IDLE) {
	tmanChangeMetadata(&my_metadata,sizeof(my_metadata));
    }
  }

  //handle explicit add/remove messages
  if (topo_in && buff && buff[0] == MSG_TYPE_STREAMER_TOPOLOGY) {
    dtprintf(stderr,"Topo: explicit topo message received!!!from %s (peers:%d)\n", node_addr(from), peerset_size(pset));
    if (len != 2) {
      fprintf(stderr, "Bad streamer topo message received, len:%d!\n", len);
      return;
    }
    switch (buff[1]) {
      case STREAMER_TOPOLOGY_MSG_ADD:
        ftprintf(stderr,"Topo: adding on request %s (peers:%d)\n", node_addr(from), peerset_size(pset));
	if (!peerset_get_peer(pset, from))
        	add_peer(from, NULL, true, false);
        break;
      case STREAMER_TOPOLOGY_MSG_REMOVE:
        ftprintf(stderr,"Topo: removing on request %s (peers:%d)\n", node_addr(from), peerset_size(pset));
        remove_peer(from, true, false);
        break;
      default:
        fprintf(stderr, "Bad streamer topo message received!\n");
    }
    reg_neigh_size(peerset_size(pset));
    return;
  }

  topoParseData(buff, len);

  if (!buff) {
    reg_neigh_size(peerset_size(pset));
    return;
  }

  peers = peerset_get_peers(pset);
  n_ids = peerset_size(pset);
  newids = topoGetNeighbourhood(&newids_size);	//TODO handle both tman and topo
  metas = topGetMetadata(&metasize);	//TODO: check metasize
  max_ids = n_ids + savedids_size + newids_size;
  ftprintf(stderr,"Topo modify start peers:%d candidates:%d\n", n_ids, newids_size);
  {
    int desired_part;
    const struct nodeID *oldids[max_ids], *nodeids[max_ids], *candidates[max_ids], *desireds[max_ids], *selecteds[max_ids], *others[max_ids], *toadds[max_ids], *toremoves[max_ids];
    int oldids_size, nodeids_size, candidates_size, desireds_size, selecteds_size, others_size, toadds_size, toremoves_size, keep_size, random_size;
    nodeids_size = candidates_size = desireds_size = selecteds_size = others_size = toadds_size = toremoves_size = max_ids;

    if (topo_out) {
      for (i = 0, oldids_size = 0; i < peerset_size(pset); i++) {
        oldids[oldids_size++] = peers[i].id;
        fprintf(stderr," %s - RTT: %f\n", node_addr(peers[i].id) , get_rtt_of(peers[i].id));
      }
    } else {
      for (i = 0, oldids_size = 0; i < savedids_size; i++) {
        oldids[oldids_size++] = savedids[i];
        fprintf(stderr," %s - RTT: %f\n", node_addr(savedids[i]) , get_rtt_of(savedids[i]));
      }
      savedids_size = 0;
      free(savedids);
    }

    // select the topo_mem portion of peers to be kept (uniform random)
    if (topo_keep_best) {
      qsort(oldids, oldids_size, sizeof(oldids[0]), vcmp_rtt);
    } else {
      nidset_shuffle(oldids, oldids_size);
    }
    keep_size = selecteds_size = (int) (topo_mem * oldids_size);
    memcpy(selecteds, oldids, selecteds_size * sizeof(selecteds[0]));

    // compose list of known nodeids
    nidset_add(nodeids, &nodeids_size, oldids, oldids_size, newids, newids_size);

    // compose list of candidate nodeids
    nidset_complement(candidates, &candidates_size, nodeids, nodeids_size, selecteds, selecteds_size);

    // select the alpha_target portion of desired peers
    desired_part = (1 - alpha_target) * (NEIGHBORHOOD_TARGET_SIZE ? (MAX (NEIGHBORHOOD_TARGET_SIZE - selecteds_size, 0)) : 0);
    nidset_filter(desireds, &desireds_size, candidates, candidates_size, is_desired);
    if (topo_add_best) {
      qsort(desireds, desireds_size, sizeof(desireds[0]), vcmp_rtt);
    } else {
      nidset_shuffle(desireds, desireds_size);
    }
    nidset_add_i(selecteds, &selecteds_size, max_ids, desireds, MIN(desireds_size,desired_part));

    // random from the rest
    nidset_complement(others, &others_size, candidates, candidates_size, selecteds, selecteds_size);
    nidset_shuffle(others, others_size);
    random_size = NEIGHBORHOOD_TARGET_SIZE ? MIN(others_size, MAX(NEIGHBORHOOD_TARGET_SIZE - selecteds_size, 0)) : others_size;
    nidset_add_i(selecteds, &selecteds_size, max_ids, others, random_size);

    fprintf(stderr,"Topo modify sel:%d (from:%d) = keep: %d (of old:%d) + desired: %d (from %d of %d; target:%d) + random: %d (from %d)\n",
            selecteds_size, nodeids_size,
            keep_size, oldids_size,
            MIN(desireds_size,desired_part), desireds_size, candidates_size, desired_part,
            random_size, others_size);
    // add new ones
    nidset_complement(toadds, &toadds_size, selecteds, selecteds_size, oldids, oldids_size);
    for (i = 0; i < toadds_size; i++) {
      int j;
      //searching for the metadata
      if (nidset_find(&j, newids, newids_size, toadds[i])) {
        fprintf(stderr," adding %s\n", node_addr(toadds[i]));
        add_peer(newids[j], &metas[j], topo_out, topo_in);
      } else {
        fprintf(stderr," Error: missing metadata for %s\n", node_addr(toadds[i]));
      }
    }

    // finally, remove those not needed
    fprintf(stderr,"Topo remove start (peers:%d)\n", n_ids);
    nidset_complement(toremoves, &toremoves_size, oldids, oldids_size, selecteds, selecteds_size);
    for (i = 0; i < toremoves_size; i++) {
      fprintf(stderr," removing %s\n", node_addr(toremoves[i]));
      remove_peer(toremoves[i], topo_out, topo_in);
    }
    fprintf(stderr,"Topo remove end\n");

    if (!topo_out) {
      savedids = malloc(selecteds_size * sizeof(savedids[0]));	//TODO: handle errors
      for (i = 0, savedids_size = 0; i < selecteds_size; i++) {
        savedids[savedids_size++] = nodeid_dup(selecteds[i]);
      }
      for (i = 0; i < oldids_size; i++) {
        nodeid_free(oldids[i]);
      }
    }
  }

  reg_neigh_size(peerset_size(pset));
}

struct peer *nodeid_to_peer(const struct nodeID* id, int reg)
{
  struct peer *p = peerset_get_peer(pset, id);
  if (!p) {
    //fprintf(stderr,"warning: received message from unknown peer: %s!%s\n",node_addr(id), reg ? " Adding it to pset." : "");
    if (reg) {
      add_peer(id,NULL, topo_out, false);
      fprintf(stderr,"Topo: ext adding %s (peers:%d)\n", node_addr(id), peerset_size(pset));
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
