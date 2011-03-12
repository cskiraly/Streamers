/*
 *  Copyright (c) 2009 Alessandro Russo
 *  Copyright (c) 2009 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 *
 * Chunk Signaling API - Higher Abstraction
 *
 * The Chunk Signaling HA provides a set of primitives for chunks signaling negotiation with other peers, in order to collect information for the effective chunk exchange with other peers. <br>
 * This is a part of the Data Exchange Protocol which provides high level abstraction for chunks' negotiations, like requesting and proposing chunks.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include "peer.h"
#include "peerset.h"
#include "chunkidset.h"
#include "trade_sig_la.h"
#include "chunk_signaling.h"
#include "net_helper.h"
#include <trade_sig_ha.h>

#include "streaming.h"
#include "topology.h"
#include "dbg.h"

static bool neigh_on_sign_recv = true;

void bmap_received(const struct nodeID *fromid, const struct nodeID *ownerid, struct chunkID_set *c_set, int cb_size, uint16_t trans_id) {
  struct peer *owner;
  if (nodeid_equal(fromid, ownerid)) {
    owner = nodeid_to_peer(ownerid, neigh_on_sign_recv);
  } else {
    dprintf("%s might be behind ",node_addr(ownerid));
    dprintf("NAT:%s\n",node_addr(fromid));
    owner = nodeid_to_peer(fromid, neigh_on_sign_recv);
  }
  
  if (owner) {	//now we have it almost sure
    chunkID_set_clear(owner->bmap,cb_size+5);	//TODO: some better solution might be needed to keep info about chunks we sent in flight.
    chunkID_set_union(owner->bmap,c_set);
    owner->cb_size = cb_size;
    gettimeofday(&owner->bmap_timestamp, NULL);
  }
}

void offer_received(const struct nodeID *fromid, struct chunkID_set *cset, int max_deliver, uint16_t trans_id) {
  struct chunkID_set *cset_acc;

  struct peer *from = nodeid_to_peer(fromid, neigh_on_sign_recv);
  dprintf("The peer %s offers %d chunks, max deliver %d.\n", node_addr(fromid), chunkID_set_size(cset), max_deliver);

  if (from) {
    //register these chunks in the buffermap. Warning: this should be changed when offers become selective.
    chunkID_set_clear(from->bmap,0);	//TODO: some better solution might be needed to keep info about chunks we sent in flight.
    chunkID_set_union(from->bmap,cset);
    gettimeofday(&from->bmap_timestamp, NULL);
  }

    //decide what to accept
    cset_acc = get_chunks_to_accept(fromid, cset, max_deliver, trans_id);

    //send accept message
    dprintf("\t accept %d chunks from peer %s, trans_id %d\n", chunkID_set_size(cset_acc), node_addr(fromid), trans_id);
    acceptChunks(fromid, cset_acc, trans_id);

    chunkID_set_free(cset_acc);
}

void accept_received(const struct nodeID *fromid, struct chunkID_set *cset, int max_deliver, uint16_t trans_id) {
  //TODO: verify if such a verification is needed
  //struct peer *from = nodeid_to_peer(fromid,0);   //verify that we have really offered, 0 at least garantees that we've known the peer before

  dprintf("The peer %s accepted our offer for %d chunks, max deliver %d.\n", node_addr(fromid), chunkID_set_size(cset), max_deliver);

  send_accepted_chunks(fromid, cset, max_deliver, trans_id);
}


 /**
 * Dispatcher for signaling messages.
 *
 * This method decodes the signaling messages, retrieving the set of chunk and the signaling
 * message, invoking the corresponding method.
 *
 * @param[in] buff buffer which contains the signaling message
 * @param[in] buff_len length of the buffer
 * @return 0 on success, <0 on error
 */

int sigParseData(const struct nodeID *fromid, uint8_t *buff, int buff_len) {
    struct chunkID_set *c_set;
    struct nodeID *ownerid;
    enum signaling_type sig_type;
    int max_deliver = 0;
    uint16_t trans_id = 0;
    int ret = 1;
    dprintf("Decoding signaling message...\n");

    ret = parseSignaling(buff + 1, buff_len-1, &ownerid, &c_set, &max_deliver, &trans_id, &sig_type);
    if (ret < 0) {
      fprintf(stdout, "ERROR parsing signaling message\n");
      return -1;
    }
    switch (sig_type) {
        case sig_send_buffermap:
          bmap_received(fromid, ownerid, c_set, max_deliver, trans_id); //FIXME: cb_size has gone from signaling
          break;
        case sig_offer:
          offer_received(fromid, c_set, max_deliver, trans_id);
          break;
        case sig_accept:
          accept_received(fromid, c_set, chunkID_set_size(c_set), trans_id);
          break;
        default:
          ret = -1;
    }
    chunkID_set_free(c_set);
    nodeid_free(ownerid);
    return ret;
}
