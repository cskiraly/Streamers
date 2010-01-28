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


#include <errno.h>
#include <assert.h>
#include "chunk.h"
#include "peer.h"
#include "trade_sig_la.h"
#include "trade_sig_ha.h"
#include "ml.h"
#include "transmissionHandler.h"
#include "chunkidset.h"
#include "chunk_trading.h"
#include "msg_types.h"

BufferMapReceivedNotification bufferMapNotifier = NULL;


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

int sigParseData(char *buff, int buff_len) {
    fprintf(stdout, "Signal Dispatcher - Receives a signaling message\n");
    struct chunkID_set *c_set;
    void *meta;
    int meta_len;
    fprintf(stdout, "\tDecoding signaling message...");
    c_set = decodeChunkSignaling(&meta, &meta_len, ((uint8_t *) buff), buff_len);
    fprintf(stdout, "done\n");    
    struct sig_nal *signal;    
    signal = (SigType *) meta;       
    int cset_len = chunkID_set_size(c_set);    
    int sig = (int) (signal->type);    
    struct peer contact;    
    fprintf(stdout, "\tSignaling Type %d\n", sig);
    //MaxDelivery  and Trans_Id to be defined
    int res = EXIT_SUCCESS;
    switch (sig) {
        case MSG_SIG_BMOFF:
        {//BufferMap Received
            contact.id = 1;
            struct peer third;
            third.id =signal->third_peer;
            if(bufferMapSentNotifier == NULL){
                fprintf(stderr,"No callback registered for BufferMap Offer.");
                return EXIT_FAILURE;
            }
            else
                bufferMapSentNotifier(&contact, &third, c_set, cset_len, signal->trans_id);
            break;
        }
        default:
        {
            res = EXIT_FAILURE;
            break;
        }
    }
    return res;
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
int sendBufferMap(const struct peer *to, const struct peer *owner, ChunkIDSet *bmap, int bmap_len, int trans_id) {    
    unsigned char msgtype;
    int connectionID; //TODO to modify using net-helper to get the proper ConnectionID
    int res;
    send_params sParams;
    int buff_len;
    msgtype = MSG_TYPE_SIGNALLING;
    struct sig_nal sigmex;
    sigmex.type = MSG_SIG_BMOFF;
    sigmex.trans_id = trans_id;
    sigmex.third_peer= owner->id;
    buff_len = bmap_len * 4 + 12 + sizeof (sigmex); // Signaling type + set len
    fprintf(stdout, "SIG_HEADER: Type %d Tx %d PP %d\n",sigmex.type,sigmex.trans_id,sigmex.third_peer);
    uint8_t buff[buff_len];    
    res = encodeChunkSignaling(bmap, &sigmex, sizeof(sigmex), buff, buff_len);
    if (res < 0) {
        fprintf(stderr, "Error in encoding chunk set for sending a buffermap\n");
        return EXIT_FAILURE;
    } else {
        connectionID = 0;
        send_data(connectionID, ((char *) buff), sizeof (buff), msgtype, &sParams);
    }
    return EXIT_SUCCESS;
}

/**
 * Notification function for (individual) SendBufferMap received
 */

/**
 * Register a notification function for SendBufferMap (reception of a buffer map)
 *
 * @param[in] som Handle to the enclosing SOM instance
 * @param[in] p notify receiving a buffer map from Peer p, or from any peer if NULL
 * @param[in] fn pointer to the notification function.
 * @return a handle to the notification or NULL on error @see unregisterChunkRequestNotifier
 */
HANDLE registerBufferMapReceivedNotifier(HANDLE som, BufferMapReceivedNotification fn) {
    if (bufferMapSentNotifier == NULL) {
        fprintf(stdout, "Registering BufferMapReceived notifier\n");
    } else
        fprintf(stdout, "Overwrite BufferMapReceived notifier\n");
    bufferMapSentNotifier = fn;
    return NULL;
}

