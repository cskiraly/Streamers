#ifndef CHUNK_TRADING_H
#define CHUNK_TRADING_H

#include <sys/time.h>
#include "chunkbuffer.h"


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
    char type;//type of signal.
    int max_deliver;//Max number of chunks to deliver.
    int trans_id;//future use...
    int third_peer;//for buffer map exchange from other peers
    int future;//future use...
} ;


struct timeval simple_push(struct chunk_buffer *);




#endif