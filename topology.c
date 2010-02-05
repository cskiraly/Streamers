#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include <net_helper.h>
#include <peerset.h>
#include <peer.h>
#include <topmanager.h>

#include "topology.h"
#include "dbg.h"

static struct timeval tout_bmap = {3, 0};

// currently it just makes the peerset grow
void update_peers(struct peerset *pset, struct nodeID *from, const uint8_t *buff, int len)
{
  int n_ids, i;
  const struct nodeID **ids;
  struct peer *peers;
  struct timeval tnow, told;

  dprintf("Update peers: topo_msg:%d, ",len);
  dprintf("before:%d, ",peerset_size(pset));
  topParseData(buff, len);
  ids = topGetNeighbourhood(&n_ids);
  peerset_add_peers(pset,ids,n_ids);
  dprintf("after:%d, ",peerset_size(pset));

  gettimeofday(&tnow, NULL);
  timersub(&tnow, &tout_bmap, &told);
  peers = peerset_get_peers(pset);
  for (i = 0; i < peerset_size(pset); i++) {
    if ( (!timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].creation_timestamp, &told, <) ) ||
         ( timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].bmap_timestamp, &told, <)     )   ) {
      topRemoveNeighbour(peers[i].id);
      peerset_remove_peer(pset, peers[i--].id);
    }
  }
  dprintf("after timer check:%d\n",peerset_size(pset));
}
