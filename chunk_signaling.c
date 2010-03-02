/*
 *  Copyright (c) 2009 Alessandro Russo.
 *
 *  This is free software;
 *  see GPL.txt
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
#include "msg_types.h"
#include "net_helper.h"

#include "dbg.h"

static struct nodeID *localID;
static struct peerset *pset;


struct peer *nodeid_to_peer(const struct nodeID* id, int reg)
{
  struct peer *p = peerset_get_peer(pset, id);
  if (!p) {
    fprintf(stderr,"warning: received message from unknown peer: %s!\n",node_addr(id));
    if (reg) {
      fprintf(stderr,"Adding %s to neighbourhood!\n", node_addr(id));
      peerset_add_peer(pset,id);
      p = peerset_get_peer(pset,id);
    }
  }

  return p;
}

int sendSignalling(int type, const struct nodeID *to_id, const struct nodeID *owner_id, struct chunkID_set *cset, int max_deliver, int cb_size, int trans_id)
{
    int buff_len, meta_len, msg_len, ret;
    uint8_t *buff;
    struct sig_nal *sigmex;
    uint8_t *meta;

    meta = malloc(1024);

    sigmex = (struct sig_nal*) meta;
    sigmex->type = type;
    sigmex->max_deliver = max_deliver;
    sigmex->cb_size = cb_size;
    sigmex->trans_id = trans_id;
    meta_len = sizeof(*sigmex)-1;
      sigmex->third_peer = 0;
    if (owner_id) {
      meta_len += nodeid_dump(&sigmex->third_peer, owner_id);
    }

    buff_len = 1 + chunkID_set_size(cset) * 4 + 12 + meta_len; // this should be enough
    buff = malloc(buff_len);
    if (!buff) {
      fprintf(stderr, "Error allocating buffer\n");
      return -1;
    }

    buff[0] = MSG_TYPE_SIGNALLING;
    msg_len = 1 + encodeChunkSignaling(cset, meta, meta_len, buff+1, buff_len-1);
    free(meta);
    if (msg_len < 0) {
      fprintf(stderr, "Error in encoding chunk set for sending a buffermap\n");
      ret = -1;
    } else {
      send_to_peer(localID, to_id, buff, msg_len);
    }
    ret = 1;
    free(buff);
    return ret;
}

/**
 * Send a BufferMap to a Peer.
 *
 * Send (our own or some other peer's) BufferMap to a third Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to send.
 * @param[in] bmap the BufferMap to send.
 * @param[in] trans_id transaction number associated with this send
 * @return 0 on success, <0 on error
 */
int sendBufferMap(const struct nodeID *to_id, const struct nodeID *owner_id, struct chunkID_set *bmap, int cb_size, int trans_id) {
  return sendSignalling(MSG_SIG_BMOFF, to_id, owner_id, bmap, 0, cb_size, trans_id);
}

int sendMyBufferMap(const struct nodeID *to_id, struct chunkID_set *bmap, int cb_size, int trans_id)
{
  return sendBufferMap(to_id, localID, bmap, cb_size, trans_id);
}

int offerChunks(const struct nodeID *to_id, struct chunkID_set *cset, int max_deliver, int trans_id) {
  return sendSignalling(MSG_SIG_OFF, to_id, NULL, cset, max_deliver, -1, trans_id);
}

int acceptChunks(const struct nodeID *to_id, struct chunkID_set *cset, int max_deliver, int trans_id) {
  return sendSignalling(MSG_SIG_ACC, to_id, NULL, cset, max_deliver, -1, trans_id);
}


/// ==================== ///
///        RECEIVE       ///
/// ==================== ///

void bmap_received(const struct nodeID *fromid, const struct nodeID *ownerid, struct chunkID_set *c_set, int cb_size, int trans_id) {
  struct peer *owner = nodeid_to_peer(ownerid,1);
  if (owner) {	//now we have it almost sure
    chunkID_set_clear(owner->bmap,cb_size+5);	//TODO: some better solution might be needed to keep info about chunks we sent in flight.
    chunkID_set_union(owner->bmap,c_set);
    owner->cb_size = cb_size;
    gettimeofday(&owner->bmap_timestamp, NULL);
  }
}

 /**
 * Dispatcher for signaling messages.
 *
 * This method decodes the signaling messages, retrieving the set of chunk and the signaling
 * message, invoking the corresponding method.
 *
 * @param[in] buff buffer which contains the signaling message
 * @param[in] buff_len length of the buffer
 * @param[in] msgtype type of message in the buffer
 * @param[in] max_deliver deliver at most this number of Chunks
 * @param[in] arg parameters associated to the signaling message
 * @return 0 on success, <0 on error
 */

int sigParseData(const struct nodeID *fromid, uint8_t *buff, int buff_len) {
    struct chunkID_set *c_set;
    void *meta;
    int meta_len;
    struct sig_nal *signal;
    int sig;
    int ret = 1;
    dprintf("Decoding signaling message...");
    c_set = decodeChunkSignaling(&meta, &meta_len, buff+1, buff_len-1);
    dprintf(" SIG_HEADER: len: %d, of which meta: %d\n", buff_len, meta_len);
    if (!c_set) {
      fprintf(stdout, "ERROR decoding signaling message\n");
      return -1;
    }
    signal = (struct sig_nal *) meta;
    sig = (int) (signal->type);
    dprintf("\tSignaling Type %d\n", sig);
    //MaxDelivery  and Trans_Id to be defined
    switch (sig) {
        case MSG_SIG_BMOFF:
        {
          int dummy;
          struct nodeID *ownerid = nodeid_undump(&(signal->third_peer),&dummy);
          bmap_received(fromid, ownerid, c_set, signal->cb_size, signal->trans_id);
          nodeid_free(ownerid);
          break;
        }
        default:
          ret = -1;
    }
    
    chunkID_set_clear(c_set,0);
    free(c_set);
    free(meta);
    return ret;
}

/// ==================== ///
///          INIT        ///
/// ==================== ///

int sigInit(struct nodeID *myID, struct peerset *ps)
{
  localID = myID;
  pset = ps;
  return 1;
}
