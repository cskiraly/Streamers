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

int sigParseData(uint8_t *buff, int buff_len) {
    struct chunkID_set *c_set;
    void *meta;
    int meta_len;
    struct sig_nal *signal;
    int sig;
    dprintf("\tDecoding signaling message...");
    c_set = decodeChunkSignaling(&meta, &meta_len, buff+1, buff_len-1);
    dprintf("SIG_HEADER: len: %d, of which meta: %d\n", buff_len, meta_len);
    if (!c_set) {
      fprintf(stdout, "ERROR decoding signaling message\n");
      return -1;
    }
    dprintf("done\n");    
    signal = (struct sig_nal *) meta;       
    sig = (int) (signal->type);    
    dprintf("\tSignaling Type %d\n", sig);
    //MaxDelivery  and Trans_Id to be defined
    switch (sig) {
        case MSG_SIG_BMOFF:
        {//BufferMap Received
          int dummy;
          struct nodeID *ownerid = nodeid_undump(&(signal->third_peer),&dummy);
          struct peer *owner = peerset_get_peer(pset, ownerid);
          if (!owner) {
            printf("warning: received bmap of unknown peer: %s! Adding it to neighbourhood!\n", node_addr(ownerid));
            peerset_add_peer(pset,ownerid);
            owner = peerset_get_peer(pset,ownerid);
          }
          if (owner) {	//now we have it almost sure
            chunkID_set_union(owner->bmap,c_set);	//don't send it back
            gettimeofday(&owner->bmap_timestamp, NULL);
          }
        }
        default:
        {
            return -1;
        }
    }
    return 1;
}

/**
 * Send a BufferMap to a Peer.
 *
 * Send (our own or some other peer's) BufferMap to a third Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to send.
 * @param[in] bmap the BufferMap to send.
 * @param[in] bmap_len length of the buffer map
 * @param[in] trans_id transaction number associated with this send
 * @return 0 on success, <0 on error
 */
int sendBufferMap(const struct nodeID *to_id, const struct nodeID *owner_id, ChunkIDSet *bmap, int bmap_len, int trans_id) {    
    int buff_len, id_len, msg_len, ret;
    uint8_t *buff;
    //msgtype = MSG_TYPE_SIGNALLING;
    struct sig_nal sigmex;
    sigmex.type = MSG_SIG_BMOFF;
    sigmex.trans_id = trans_id;
    id_len = nodeid_dump(&sigmex.third_peer, owner_id);
    buff_len = 1 + bmap_len * 4 + 12 + sizeof(sigmex)-1 + id_len; // this should be enough
    dprintf("SIG_HEADER: Type %d Tx %d PP %s\n",sigmex.type,sigmex.trans_id,node_addr(owner_id));
    buff = malloc(buff_len);
    if (!buff) {
      return -1;
    }
    buff[0] = MSG_TYPE_SIGNALLING;
    msg_len = 1 + encodeChunkSignaling(bmap, &sigmex, sizeof(sigmex)-1 + id_len, buff+1, buff_len-1);
    dprintf("SIG_HEADER: len: %d, of which meta: %d\n", msg_len, sizeof(sigmex)-1 + id_len);
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

int sendMyBufferMap(const struct nodeID *to_id, ChunkIDSet *bmap, int bmap_len, int trans_id)
{
  return sendBufferMap(to_id, localID, bmap, bmap_len, trans_id);
}

int sigInit(struct nodeID *myID, struct peerset *ps)
{
  localID = myID;
  pset = ps;
  return 1;
}
