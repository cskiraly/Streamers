/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <math.h>
#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif
#ifndef INFINITY
#define INFINITY       (1.0/0.0)
#endif

#include <mon.h>
#include <ml.h>
#include <net_helper.h>
#include <grapes_msg_types.h>

#include "channel.h"
#include "dbg.h"
#include "measures.h"

#define PEER_PUBLISH_INTERVAL 10 //in seconds
#define P2P_PUBLISH_INTERVAL 60 //in seconds

extern const char *peername;

typedef struct nodeID {
	socketID_handle addr;
	int connID;	// connection associated to this node, -1 if myself
	int refcnt;
	//a quick and dirty static vector for measures TODO: make it dinamic
	MonHandler mhs[20];
	int n_mhs;
} nodeID;

static MonHandler chunk_dup, chunk_playout, neigh_size, chunk_receive, chunk_send, offer_accept, chunk_hops, chunk_delay, playout_delay;
//static MonHandler rx_bytes_chunk_per_sec, tx_bytes_chunk_per_sec, rx_bytes_sig_per_sec, tx_bytes_sig_per_sec;
//static MonHandler rx_chunks, tx_chunks;

/*
 * Initialize one measure
*/
void add_measure(MonHandler *mh, MeasurementId id, MeasurementCapabilities mc, MonParameterValue rate, const char *pubname, enum stat_types st[], int length, SocketId dst, MsgType mt)
{
	*mh = monCreateMeasure(id, mc);
	if (rate) monSetParameter (*mh, P_PUBLISHING_RATE, rate);
	if (length) monPublishStatisticalType(*mh, pubname, channel_get_name(), st , length, NULL);
	monActivateMeasure(*mh, dst, mt);
}

/*
 * Register duplicate arrival
*/
void reg_chunk_duplicate()
{
	if (!chunk_dup) {
		enum stat_types st[] = {SUM, RATE};
		// number of chunks which have been received more then once
		add_measure(&chunk_dup, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ChunkDuplicates", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks]
		monNewSample(chunk_dup, 0);	//force publish even if there are no events
	}
	monNewSample(chunk_dup, 1);
}

/*
 * Register playout/loss of a chunk before playout
*/
void reg_chunk_playout(int id, bool b, uint64_t timestamp)
{
	static MonHandler chunk_loss_burst_size;
	static int last_arrived_chunk = -1;

	struct timeval tnow;
	if (!chunk_playout && b) {	//don't count losses before the first arrived chunk
		enum stat_types st[] = {WIN_AVG, AVG, SUM, RATE};
		//number of chunks played
		add_measure(&chunk_playout, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ChunksPlayed", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks]
	}
	monNewSample(chunk_playout, b);

	if (!playout_delay) {
		enum stat_types st[] = {WIN_AVG, WIN_VAR};
		//delay after reorder buffer, however http module does not use reorder buffer
		add_measure(&playout_delay, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ReorderDelay", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[seconds]
	}
	if (b) {	//count delay only if chunk has arrived
		gettimeofday(&tnow, NULL);
		monNewSample(playout_delay, ((int64_t)(tnow.tv_usec + tnow.tv_sec * 1000000ULL) - (int64_t)timestamp) / 1000000.0);
	}

	//if (!chunk_loss_burst_size) {
	//	enum stat_types st[] = {WIN_AVG, WIN_VAR};
	//	// number of consecutive lost chunks
	//	add_measure(&chunk_loss_burst_size, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ChunkLossBurstSize", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks]
	//}
	if (b) {
		if (last_arrived_chunk >= 0) {
			int burst_size = id - 1 - last_arrived_chunk;
			if (burst_size) monNewSample(chunk_loss_burst_size, burst_size);
		}
		last_arrived_chunk = id;
	}
}

/*
 * Register actual neghbourhood size
*/
void reg_neigh_size(int s)
{
	if (!neigh_size) {
		enum stat_types st[] = {LAST};
		// number of peers in the neighboorhood
		add_measure(&neigh_size, GENERIC, 0, PEER_PUBLISH_INTERVAL, "NeighSize", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
	}
	monNewSample(neigh_size, s);
}

/*
 * Register chunk receive event
*/
void reg_chunk_receive(int id, uint64_t timestamp, int hopcount, bool old, bool dup)
{
	struct timeval tnow;

	if (!chunk_receive) {
		enum stat_types st[] = {RATE};
		// total number of received chunks per second
		add_measure(&chunk_receive, GENERIC, 0, PEER_PUBLISH_INTERVAL, "TotalRxChunk", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks/s]
		monNewSample(chunk_receive, 0);	//force publish even if there are no events
	}
	monNewSample(chunk_receive, 1);

	if (!chunk_hops) {
		enum stat_types st[] = {WIN_AVG};
		// number of hops from source on the p2p network
		add_measure(&chunk_hops, GENERIC, 0, PEER_PUBLISH_INTERVAL, "OverlayHops", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
	}
	monNewSample(chunk_hops, hopcount);

	if (!chunk_delay) {
		enum stat_types st[] = {WIN_AVG, WIN_VAR};
		// time elapsed since the source emitted the chunk
		add_measure(&chunk_delay, GENERIC, 0, PEER_PUBLISH_INTERVAL, "ReceiveDelay", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[seconds]
	}
	gettimeofday(&tnow, NULL);
	monNewSample(chunk_delay, ((int64_t)(tnow.tv_usec + tnow.tv_sec * 1000000ULL) - (int64_t)timestamp) / 1000000.0);
}

/*
 * Register chunk send event
*/
void reg_chunk_send(int id)
{
	if (!chunk_send) {
		enum stat_types st[] = {RATE};
		add_measure(&chunk_send, GENERIC, 0, PEER_PUBLISH_INTERVAL, "TotalTxChunk", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks/s]
		monNewSample(chunk_send, 0);	//force publish even if there are no events
	}
	monNewSample(chunk_send, 1);
}

/*
 * Register chunk accept evemt
*/
void reg_offer_accept(bool b)
{
	if (!offer_accept) {
		enum stat_types st[] = {WIN_AVG};
		// ratio between number of offers and number of accepts
		add_measure(&offer_accept, GENERIC, 0, PEER_PUBLISH_INTERVAL, "OfferAccept", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[no unit -> ratio]
	}
	monNewSample(offer_accept, b);
}

/*
 * Initialize peer level measurements
*/
void init_measures()
{
	if (peername) monSetPeerName(peername);
}

/*
 * End peer level measurements
*/
void end_measures()
{
}

/*
 * Initialize p2p measurements towards a peer
*/
void add_measures(struct nodeID *id)
{
	// Add measures
	int j = 0;
	enum stat_types stwinavgwinvar[] = {WIN_AVG, WIN_VAR};
	enum stat_types stwinavg[] = {WIN_AVG};
	enum stat_types stwinavgrate[] = {WIN_AVG, RATE};
//	enum stat_types stsum[] = {SUM};
	enum stat_types stsumwinsumrate[] = {SUM, PERIOD_SUM, WIN_SUM, RATE};

	dprintf("adding measures to %s\n",node_addr(id));

	/* HopCount */
	// number of hops at IP level
       add_measure(&id->mhs[j++], HOPCOUNT, PACKET | IN_BAND, P2P_PUBLISH_INTERVAL, "HopCount", stwinavg, sizeof(stwinavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[IP hops]

	/* Round Trip Time */
       add_measure(&id->mhs[j++], RTT, PACKET | IN_BAND, P2P_PUBLISH_INTERVAL, "RoundTripDelay", stwinavgwinvar, sizeof(stwinavgwinvar)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[seconds]

	/* Loss */
       add_measure(&id->mhs[j++], SEQWIN, PACKET | IN_BAND, 0, NULL, NULL, 0, id->addr, MSG_TYPE_CHUNK);
       add_measure(&id->mhs[j++], LOSS, PACKET | IN_BAND, P2P_PUBLISH_INTERVAL, "LossRate", stwinavg, sizeof(stwinavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//LossRate_avg [probability 0..1] LossRate_rate [lost_pkts/sec]

       /* RX,TX volume in bytes (only chunks) */
       add_measure(&id->mhs[j++], RX_BYTE, PACKET | IN_BAND, P2P_PUBLISH_INTERVAL, "RxBytesChunk", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes]
       add_measure(&id->mhs[j++], TX_BYTE, PACKET | IN_BAND, P2P_PUBLISH_INTERVAL, "TxBytesChunk", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes]

       /* RX,TX volume in bytes (only signaling) */
       add_measure(&id->mhs[j++], RX_BYTE, PACKET | IN_BAND, P2P_PUBLISH_INTERVAL, "RxBytesSig", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[bytes]
       add_measure(&id->mhs[j++], TX_BYTE, PACKET | IN_BAND, P2P_PUBLISH_INTERVAL, "TxBytesSig", stsumwinsumrate, sizeof(stsumwinsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[bytes]

	// Chunks
       add_measure(&id->mhs[j++], RX_PACKET, DATA | IN_BAND, P2P_PUBLISH_INTERVAL, "RxChunks", stwinavgrate, sizeof(stwinavgrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//RxChunks_sum [chunks] RxChunks_rate [chunks/sec]
       add_measure(&id->mhs[j++], TX_PACKET, DATA | IN_BAND, P2P_PUBLISH_INTERVAL, "TxChunks", stwinavgrate, sizeof(stwinavgrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//TxChunks_sum [chunks] TxChunks_rate [chunks/sec]
//	// Capacity
	add_measure(&id->mhs[j++], CLOCKDRIFT, PACKET | IN_BAND, 0, NULL, NULL, 0, id->addr, MSG_TYPE_CHUNK);
	monSetParameter (id->mhs[j-1], P_CLOCKDRIFT_ALGORITHM, 1);
	add_measure(&id->mhs[j++], CORRECTED_DELAY, PACKET | IN_BAND, 0, NULL, NULL, 0, id->addr, MSG_TYPE_CHUNK);
	add_measure(&id->mhs[j++], CAPACITY_CAPPROBE, PACKET | IN_BAND, P2P_PUBLISH_INTERVAL, "Capacity", stwinavg, sizeof(stwinavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes/s]
	monSetParameter (id->mhs[j-1], P_CAPPROBE_DELAY_TH, -1); //disable: seems to have  implementation problems
//	monSetParameter (id->mhs[j-1], P_CAPPROBE_PKT_TH, 5);
//	monSetParameter (mh, P_CAPPROBE_PKT_TH, 100);
//	monSetParameter (mh, P_CAPPROBE_IPD_TH, 60);
//	monPublishStatisticalType(mh, NULL, st , sizeof(st)/sizeof(enum stat_types), repoclient);

	// for static must not be more then 10 or whatever size is in net_helper-ml.c
	id->n_mhs = j;
}

/*
 * Delete p2p measurements towards a peer
*/
void delete_measures(struct nodeID *id)
{
	dprintf("deleting measures from %s\n",node_addr(id));
	while(id->n_mhs) {
		monDestroyMeasure(id->mhs[--(id->n_mhs)]);
	}
}

/*
 * Helper to retrieve a measure
*/
double get_measure(struct nodeID *id, int j, enum stat_types st)
{
	return (id->n_mhs > j) ? monRetrieveResult(id->mhs[j], st) : NAN;
}

/*
 * Hopcount to a given peer
*/
int get_hopcount(struct nodeID *id){
	double r = get_measure(id, 0, LAST);
	return isnan(r) ? -1 : (int) r;
}

/*
 * RTT to a given peer in seconds
*/
double get_rtt(struct nodeID *id){
	return get_measure(id, 1, WIN_AVG);
}

/*
 * average RTT to a set of peers in seconds
*/
double get_average_rtt(struct nodeID **ids, int len){
	int i;
	int n = 0;
	double sum = 0;

	for (i = 0; i < len; i++) {
		double l = get_rtt(ids[i]);
		if (!isnan(l)) {
			sum += l;
			n++;
		}
	}
	return (n > 0) ? sum / n : NAN;
}

/*
 * loss ratio from a given peer as 0..1
*/
double get_lossrate(struct nodeID *id){
	return get_measure(id, 3, WIN_AVG);
}

/*
 * average loss ratio from a set of peers as 0..1
*/
double get_average_lossrate(struct nodeID **ids, int len){
	int i;
	int n = 0;
	double sum = 0;

	for (i = 0; i < len; i++) {
		double l = get_lossrate(ids[i]);
		if (!isnan(l)) {
			sum += l;
			n++;
		}
	}
	return (n > 0) ? sum / n : NAN;
}

double get_receive_delay(void) {
	return chunk_delay ? monRetrieveResult(chunk_delay, WIN_AVG) : NAN;
}
