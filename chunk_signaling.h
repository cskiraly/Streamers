#ifndef CHUNK_SIGNALING_H
#define CHUNK_SIGNALING_H

#include "chunkidset.h"

//Type of signaling message
//Request a ChunkIDSet
#define MSG_SIG_REQ 0
//Diliver a ChunkIDSet (reply to a request)
#define MSG_SIG_DEL 1
//Offer a ChunkIDSet
#define MSG_SIG_OFF 2
//Accept a ChunkIDSet (reply to an offer)
#define MSG_SIG_ACC 3
//Receive the BufferMap
#define MSG_SIG_BMOFF 4
//Request the BufferMap
#define MSG_SIG_BMREQ 5


struct sig_nal {
    uint8_t type;//type of signal.
    uint8_t max_deliver;//Max number of chunks to deliver.
    uint16_t cb_size;
    uint16_t trans_id;//future use...
    uint8_t third_peer;//for buffer map exchange from other peers, just the first byte!
} __attribute__((packed));


int sigParseData(const struct nodeID *from_id, uint8_t *buff, int buff_len);

int sendBufferMap(const struct nodeID *to_id, const struct nodeID *owner_id, ChunkIDSet *bmap, int cb_size, int trans_id);

int sendMyBufferMap(const struct nodeID *to_id, ChunkIDSet *bmap, int cb_size, int trans_id);

int offerChunks(const struct nodeID *to_id, struct chunkID_set *cset, int max_deliver, int trans_id);

int acceptChunks(const struct nodeID *to_id, struct chunkID_set *cset, int max_deliver, int trans_id);

/**
  * Init the chunk signaling stuff...
  *
  *
  * @param myID address of this peer
  * @param pset the peerset to use
  * @return >= 0 on success, <0 on error
  */
int sigInit(struct nodeID *myID, struct peerset *pset);

#endif