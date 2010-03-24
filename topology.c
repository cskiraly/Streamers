#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include <net_helper.h>
#include <peerset.h>
#include <peer.h>
#include <topmanager.h>

#include "topology.h"

static struct timeval tout_bmap = {5, 0};

// currently it just makes the peerset grow
void update_peers(struct peerset *pset, const uint8_t *buff, int len)
{
  int n_ids, i;
  const struct nodeID **ids;
  struct peer *peers;
  struct timeval tnow, told;


  topParseData(buff, len);
  ids = topGetNeighbourhood(&n_ids);
  peerset_add_peers(pset,ids,n_ids);

  gettimeofday(&tnow, NULL);
  timersub(&tnow, &tout_bmap, &told);
  peers = peerset_get_peers(pset);
  for (i = 0; i < peerset_size(pset); i++) {
    if (timerisset(&peers[i].bmap_timestamp) && timercmp(&peers[i].bmap_timestamp, &told, <)) {
      topRemoveNeighbour(peers[i].id);
      peerset_remove_peer(pset, peers[i--].id);
    }
  }
}
