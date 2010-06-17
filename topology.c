#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include <net_helper.h>
#include <peerset.h>
#include <peer.h>
#include <topmanager.h>

#include "topology.h"
#include "streaming.h"
#include "dbg.h"
#include "measures.h"

#define NEIGHBORHOOD_TARGET_SIZE 0

static struct peerset *pset;
static struct timeval tout_bmap = {10, 0};


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
  const struct nodeID **ids;
  struct peer *peers;
  struct timeval tnow, told;

  dprintf("Update peers: topo_msg:%d, ",len);
  if (from) {
    dprintf("from:%s, ",node_addr(from));
    if (peerset_check(pset, from) < 0) {
      topAddNeighbour(from, NULL, 0);	//@TODO: this is agressive
      if (!NEIGHBORHOOD_TARGET_SIZE || peerset_size(pset) < NEIGHBORHOOD_TARGET_SIZE) {
        add_peer(from);
      }
    }
  }

  dprintf("before:%d, ",peerset_size(pset));
  topParseData(buff, len);
  ids = topGetNeighbourhood(&n_ids);
  for(i = 0; i < n_ids; i++) {
    if(peerset_check(pset, ids[i]) < 0) {
      if (!NEIGHBORHOOD_TARGET_SIZE || peerset_size(pset) < NEIGHBORHOOD_TARGET_SIZE) {
        add_peer(ids[i]);
      }
    }
  }
  dprintf("after:%d, ",peerset_size(pset));

  gettimeofday(&tnow, NULL);
  timersub(&tnow, &tout_bmap, &told);
  peers = peerset_get_peers(pset);
  for (i = 0; i < peerset_size(pset); i++) {
    if ( (!timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].creation_timestamp, &told, <) ) ||
         ( timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].bmap_timestamp, &told, <)     )   ) {
      //if (peerset_size(pset) > 1) {	// avoid dropping our last link to the world
        topRemoveNeighbour(peers[i].id);
        remove_peer(peers[i--].id);
      //}
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
      topAddNeighbour(id, NULL, 0);	//@TODO: this is agressive
      add_peer(id);
      p = peerset_get_peer(pset,id);
    }
  }

  return p;
}

int peers_init()
{
  fprintf(stderr,"peers_init\n");
  pset = peerset_init(0);
  return pset ? 1 : 0;
}

struct peerset *get_peers()
{
  return pset;
}
