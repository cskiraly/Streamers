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

ChunkRequestNotification chunkRequestNotifier = NULL;
ChunkDeliverNotification chunkDeliverNotifier = NULL;
ChunkOfferNotification chunkOfferNotifier = NULL;
ChunkAcceptNotification chunkAcceptNotifier = NULL;
BufferMapReceivedNotification bufferMapSentNotifier = NULL;
BufferMapRequestedNotification bufferMapRequestedNotifier = NULL;


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
        case MSG_SIG_REQ://Request a set of chunks
        {
/*
            fprintf(stdout, "REQUEST\n");
*/
            contact.id = 2; //TODO to change witht he corresponding peer
            chunkRequestNotifier(NULL, &contact, c_set, cset_len, signal->max_deliver, signal->trans_id);
            break;
        }
        case MSG_SIG_DEL://Deliver a set of chunks requested
        {
/*
            fprintf(stdout, "DELIVER\n");
*/
            contact.id = 1;
            chunkDeliverNotifier(&contact, c_set, cset_len, signal->max_deliver, signal->trans_id);
            break;
        }
        case MSG_SIG_OFF://Request a set of chunks
        {
/*
            fprintf(stdout, "OFFER\n");
*/
            contact.id = 1; //TODO to change witht he corresponding peer
            chunkOfferNotifier(&contact,c_set, cset_len,signal->max_deliver, signal->trans_id);
            break;
        }
        case MSG_SIG_ACC://Deliver a set of chunks requested
        {
/*
            fprintf(stdout, "ACCEPT\n");
*/
            contact.id = 2;
            chunkAcceptNotifier(&contact,c_set,cset_len, signal->trans_id);
            break;
        }
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
        case MSG_SIG_BMREQ:
        {//BufferMap requested -> to be sent            
            contact.id = 2;
            struct peer third;            
            third.id = signal->third_peer;            
            if(bufferMapRequestedNotifier == NULL){
                fprintf(stderr,"No callback registered for BufferMap Request.");
                return EXIT_FAILURE;
            }
            else{                
                bufferMapRequestedNotifier(&contact, &third, signal->trans_id);                
            }
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
 * ML wrapper for sigParseData
 */
int signalDispatcher(char *buff, int buff_len, unsigned char msgtype, recv_params *arg) {
    assert(msgtype == MSG_TYPE_SIGNALLING);
    //fprintf(stdout, "Connection ID %d\n", rps->connectionID);
    sigParseData(buff,buff_len);
};

/**
 * Request a (sub)set of chunks from a Peer.
 *
 * Initiate a request for a set of Chunks towards a Peer, and specify the  maximum number of Chunks attempted to deliver
 * (attempted to deliver: i.e. the destination peer would like to receive at most this number of Chunks from the set)
 *
 * @param[in] to target peer to request the ChunkIDs from
 * @param[in] cset array of ChunkIDs
 * @param[in] cset_len length of the ChunkID set
 * @param[in] max_deliver deliver at most this number of Chunks
 * @param[in] trans_id transaction number associated with this request
 * @return >0 on success, 0 no chunks, <0 on error,
 */
int requestChunks(const struct peer *to, const ChunkIDSet *cset, int cset_len, int max_deliver, int trans_id) {
    unsigned char msgtype;
    int connectionID; //TODO to modify using net-helper to get the proper ConnectionID
    int res;
    send_params sParams;
    int buff_len;
    msgtype = MSG_TYPE_SIGNALLING;
    struct sig_nal sigmex;
    sigmex.type = MSG_SIG_REQ;
    sigmex.max_deliver= max_deliver;
    sigmex.trans_id = trans_id;
    fprintf(stdout, "SIG_HEADER: Type %d Tx %d PP %d\n",sigmex.type,sigmex.trans_id,sigmex.third_peer);
    buff_len = cset_len * 4 + 12 + sizeof (sigmex); // Signaling type + set len
    uint8_t buff[buff_len];
    res = encodeChunkSignaling(cset, &sigmex, sizeof (sigmex), buff, buff_len);
    if (res < 0) {
        fprintf(stderr, "Error in encoding chunk set for chunks request\n");
        return EXIT_FAILURE;
    } else {
        connectionID = 0;
        send_data(connectionID, ((char *) buff), sizeof (buff), msgtype, &sParams);
    }
    return EXIT_SUCCESS;
}

/**
 * Register a notification function for explicit (single) chunk request received.
 *
 * @param[in] som Handle to the enclosing SOM instance
 * @param[in] p notify requests from Peer p, or from any peer if NULL
 * @param[in] fn pointer to the notification function.
 * @return a handle to the notification or NULL on error @see unregisterChunkRequestNotifier
 */
HANDLE registerChunkRequestNotifier(HANDLE som, ChunkRequestNotification fn) {
    if (chunkRequestNotifier == NULL) {
        fprintf(stdout, "Registering ChunkRequest notifier\n");
    } else fprintf(stdout, "Overwrite ChunkRequest notifier\n");
    chunkRequestNotifier = fn;
    return NULL;
}

/**
 * Deliver a set of Chunks to a Peer as a reply of its previous request of Chunks
 *
 * Announce to the Peer which sent a request of a set of Chunks, that we have these chunks (sub)set available
 * among all those requested and willing to deliver them.
 *
 * @param[in] to target peer to which deliver the ChunkIDs
 * @param[in] cset array of ChunkIDs
 * @param[in] cset_len length of the ChunkID set
 * @param[in] max_deliver we are able to deliver at most this many from the set
 * @param[in] trans_id transaction number associated with this request
 * @return 0 on success, <0 on error
 */

int deliverChunks(const struct peer *to, ChunkIDSet *cset, int cset_len, int max_deliver, int trans_id) {
    unsigned char msgtype;
    int connectionID; //TODO to modify using net-helper to get the proper ConnectionID
    int res;
    send_params sParams;
    int buff_len;
    msgtype = MSG_TYPE_SIGNALLING;
    struct sig_nal sigmex;
    sigmex.type = MSG_SIG_DEL;
    sigmex.max_deliver= max_deliver;
    sigmex.trans_id = trans_id;
    fprintf(stdout, "SIG_HEADER: Type %d Tx %d PP %d\n",sigmex.type,sigmex.trans_id,sigmex.third_peer);
    buff_len = cset_len * 4 + 12 + sizeof (sigmex); // Signaling type + set len
    uint8_t buff[buff_len];
    res = encodeChunkSignaling(cset, &sigmex, sizeof (sigmex), buff, buff_len);
    if (res < 0) {
        fprintf(stderr, "Error in encoding chunk set for chunks deliver\n");
        return EXIT_FAILURE;
    } else {
        connectionID = 0;
        send_data(connectionID, ((char *) buff), sizeof (buff), msgtype, &sParams);
    }
    return EXIT_SUCCESS;
}

/**
 * Register a notification for explicit (single) Chunk deliver
 *
 * Register a notification function for explicit (single) Chunk deliver received.
 *
 * @param[in] som Handle to the enclosing SOM instance
 * @param[in] p notify deliver from Peer p, or from any peer if NULL
 * @param[in] fn pointer to the notification function.
 * @return a handle to the notification or NULL on error @see unregisterChunkDeliverNotifier
 */

HANDLE registerChunkDeliverNotifier(HANDLE som, ChunkDeliverNotification fn) {
    if (chunkDeliverNotifier == NULL) {
        fprintf(stdout, "Registering ChunkDeliver notifier\n");
    } else
        fprintf(stdout, "Overwrite ChunkDeliver notifier\n");
    chunkDeliverNotifier = fn;
    return NULL;
}

/**
 * Offer a (sub)set of chunks to a Peer.
 *
 * Initiate a offer for a set of Chunks towards a Peer, and specify the  maximum number of Chunks attempted to deliver
 * (attempted to deliver: i.e. the sender peer should try to send at most this number of Chunks from the set)
 *
 * @param[in] to target peer to offer the ChunkIDs
 * @param[in] cset array of ChunkIDs
 * @param[in] cset_len length of the ChunkID set
 * @param[in] max_deliver deliver at most this number of Chunks
 * @param[in] trans_id transaction number associated with this request
 * @return 0 on success, <0 on error
 */
int offerChunks(const struct peer *to, ChunkIDSet *cset, int cset_len, int max_deliver, int trans_id) {
    unsigned char msgtype;
    int connectionID; //TODO to modify using net-helper to get the proper ConnectionID
    int res;
    send_params sParams;
    int buff_len;
    msgtype = MSG_TYPE_SIGNALLING;
    struct sig_nal sigmex;
    sigmex.type = MSG_SIG_OFF;
    sigmex.max_deliver = max_deliver;
    sigmex.trans_id = trans_id;
    fprintf(stdout, "SIG_HEADER: Type %d Tx %d PP %d\n",sigmex.type,sigmex.trans_id,sigmex.third_peer);
    buff_len =cset_len * 4 + 12 + sizeof (sigmex); // Signaling type + set len
    uint8_t buff[buff_len];
    res = encodeChunkSignaling(cset, &sigmex, sizeof (sigmex), buff, buff_len);
    if (res < 0) {
        fprintf(stderr, "Error in encoding chunk set for chunks offer\n");
        return EXIT_FAILURE;
    } else {
        connectionID = 0;
        send_data(connectionID, ((char *) buff), sizeof (buff), msgtype, &sParams);
    }
    return EXIT_SUCCESS;
}

/**
 * Register a notification for explicit (single) ChunkOffer.
 *
 * Register a notification function for explicit (single) ChunkOffer received.
 *
 * @param[in] som Handle to the enclosing SOM instance
 * @param[in] p notify offer from Peer p, or from any peer if NULL
 * @param[in] fn pointer to the notification function.
 * @return a handle to the notification or NULL on error @see unregisterChunkRequestNotifier
 */
HANDLE registerChunkOfferNotifier(HANDLE som, ChunkOfferNotification fn) {
    if (chunkOfferNotifier == NULL) {
        fprintf(stdout, "Registering ChunkOffer notifier\n");
    } else
        fprintf(stdout, "Overwrite ChunkOffer notifier\n");
    chunkOfferNotifier = fn;
    return NULL;

}

/**
 * Accept a (sub)set of chunks from a Peer.
 *
 * Announce to accept a (sub)set of Chunks from a Peer which sent a offer before, and specify the  maximum number of Chunks attempted to receive
 * (attempted to receive: i.e. the receiver peer would like to receive at most this number of Chunks from the set offerd before)
 *
 * @param[in] to target peer to accept the ChunkIDs
 * @param[in] cset array of ChunkIDs
 * @param[in] cset_len length of the ChunkID set
 * @param[in] max_deliver accept at most this number of Chunks
 * @param[in] trans_id transaction number associated with this request
 * @return 0 on success, <0 on error
 */
int acceptChunks(const struct peer *to, ChunkIDSet *cset, int cset_len, int max_deliver, int trans_id) {
    unsigned char msgtype;
    int connectionID; //TODO to modify using net-helper to get the proper ConnectionID
    int res;
    send_params sParams;
    int buff_len;
    msgtype = MSG_TYPE_SIGNALLING;
    struct sig_nal sigmex;
    sigmex.type = MSG_SIG_ACC;
    sigmex.max_deliver = max_deliver;
    sigmex.trans_id = trans_id;
    fprintf(stdout, "SIG_HEADER: Type %d Tx %d PP %d\n",sigmex.type,sigmex.trans_id,sigmex.third_peer);
    buff_len = cset_len * 4 + 12 + sizeof (sigmex); // Signaling type + set len
    uint8_t buff[buff_len];
    res = encodeChunkSignaling(cset, &sigmex, sizeof (sigmex), buff, buff_len);
    if (res < 0) {
        fprintf(stderr, "Error in encoding chunk set for chunk accept\n");
        return EXIT_FAILURE;
    } else {
        connectionID = 0;
        send_data(connectionID, ((char *) buff), sizeof (buff), msgtype, &sParams);
    }
    return EXIT_SUCCESS;
}

/**
 * Notification function for (individual) ChunkAccept received
 */

/**
 * Register a notification for explicit (single) ChunkAccept.
 *
 * Register a notification function for explicit (single) ChunkAccept received.
 *
 * @param[in] som Handle to the enclosing SOM instance
 * @param[in] p notify accept from Peer p, or from any peer if NULL
 * @param[in] fn pointer to the notification function.
 * @return a handle to the notification or NULL on error @see unregisterChunkRequestNotifier
 */
HANDLE registerChunkAcceptNotifier(HANDLE som, ChunkAcceptNotification fn) {    
    if (chunkAcceptNotifier == NULL) {
        fprintf(stdout, "Registering ChunkAccept notifier\n");
    } else
        fprintf(stdout, "Overwrite ChunkAccept notifier\n");
    chunkAcceptNotifier = fn;
    return NULL;

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

/**
 * Request a BufferMap to a Peer.
 *
 * Request (target peer or some other peer's) BufferMap to target Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to request.
 * @param[in] trans_id transaction number associated with this request
 * @return 0 on success, <0 on error
 */
int requestBufferMap(const struct peer *to, const struct peer *owner, int trans_id) {
    unsigned char msgtype;
    int connectionID; //TODO to modify using net-helper to get the proper ConnectionID
    int res;
    send_params sParams;
    int buff_len;
    msgtype = MSG_TYPE_SIGNALLING;
    struct sig_nal sigmex;
    sigmex.type = MSG_SIG_BMREQ;
    sigmex.trans_id = trans_id;
    sigmex.third_peer = owner->id;
    fprintf(stdout, "SIG_HEADER: Type %d Tx %d PP %d\n",sigmex.type,sigmex.trans_id,sigmex.third_peer);
    ChunkIDSet *bmap;//TODO Receive would require a buffer map from a given chunk, with a given length
    bmap = chunkID_set_init(0);
    buff_len = 12 + sizeof(sigmex);
    uint8_t buff[buff_len];
    res = encodeChunkSignaling(bmap, &sigmex, sizeof(sigmex), buff, buff_len);    
    if (res < 0) {
        fprintf(stderr, "Error in encoding chunk set for requesting buffer map\n");
        return EXIT_FAILURE;
    } else {
        connectionID = 0;
        send_data(connectionID, ((char *) buff), sizeof (buff), msgtype, &sParams);
    }
    return EXIT_SUCCESS;
}

/**
 * Register a notification for explicit (single) RequestBufferMap
 *
 * Register a notification function for explicit (single) RequestBufferMap received.
 *
 * @param[in] som Handle to the enclosing SOM instance
 * @param[in] p notify requesting a buffer map to Peer p, or from any peer if NULL
 * @param[in] fn pointer to the notification function.
 * @return a handle to the notification or NULL on error @see unregisterChunkRequestNotifier
 */
HANDLE registerBufferMapRequestedNotifier(HANDLE som, BufferMapRequestedNotification fn) {
    if (bufferMapRequestedNotifier == NULL) {
        fprintf(stdout, "Registering BufferMapRequested notifier\n");
    } else
        fprintf(stdout, "Overwrite BufferMapRequested notifier\n");
    bufferMapRequestedNotifier = fn;
    return NULL;
}
