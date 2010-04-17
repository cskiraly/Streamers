#include <math.h>
#ifndef NAN	//NAN is missing in some old math.h versions
#define NAN            (0.0/0.0)
#endif
#ifndef INFINITY
#define INFINITY       (1.0/0.0)
#endif

#include <mon.h>
#include <ml.h>

#include "dbg.h"
#include <net_helper.h>

static const char* channel = "OfferStreamer2";

typedef struct nodeID {
	socketID_handle addr;
	int connID;	// connection associated to this node, -1 if myself
	int refcnt;
	//a quick and dirty static vector for measures TODO: make it dinamic
	MonHandler mhs[20];
	int n_mhs;
} nodeID;

static MonHandler chunk_dup, chunk_playout, neigh_size, chunk_receive, chunk_send;
static MonHandler rx_bytes_chunk_per_sec, tx_bytes_chunk_per_sec, rx_bytes_sig_per_sec, tx_bytes_sig_per_sec;
static MonHandler rx_chunks, tx_chunks;

/*
 * Initialize one measure
*/
void add_measure(MonHandler *mh, MeasurementId id, MeasurementCapabilities mc, MonParameterValue rate, const char *pubname, enum stat_types st[], int length, SocketId dst, MsgType mt)
{
	*mh = monCreateMeasure(id, mc);
	monSetParameter (*mh, P_PUBLISHING_RATE, rate);
	monPublishStatisticalType(*mh, pubname, channel, st , length, NULL);
	monActivateMeasure(*mh, dst, mt);
}

/*
 * Register duplicate arrival
*/
void reg_chunk_duplicate()
{
	if (!chunk_dup) {
		enum stat_types st[] = {SUM, RATE};
		add_measure(&chunk_dup, GENERIC, 0, 120, "ChunkDuplicates", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks]
		monNewSample(chunk_dup, 0);	//force publish even if there are no events
	}
	monNewSample(chunk_dup, 1);
}

/*
 * Register playout/loss of a chunk before playout
*/
void reg_chunk_playout(bool b)
{
	if (!chunk_playout && b) {	//don't count losses before the first arrived chunk
		enum stat_types st[] = {AVG, SUM, RATE};
		add_measure(&chunk_playout, GENERIC, 0, 120, "ChunksPlayed", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[chunks]
	}
	monNewSample(chunk_playout, b);
}

/*
 * Register actual neghbourhood size
*/
void reg_neigh_size(int s)
{
	if (!neigh_size) {
		enum stat_types st[] = {LAST};
		add_measure(&neigh_size, GENERIC, 0, 120, "NeighSize", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
	}
	monNewSample(neigh_size, s);
}

/*
 * Register chunk receive event
*/
void reg_chunk_receive(int id)
{
	if (!chunk_receive) {
		enum stat_types st[] = {RATE};
		add_measure(&chunk_receive, GENERIC, 0, 120, "RxChunkAll", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
		monNewSample(chunk_receive, 0);	//force publish even if there are no events
	}
	monNewSample(chunk_receive, 1);
}

/*
 * Register chunk send event
*/
void reg_chunk_send(int id)
{
	if (!chunk_send) {
		enum stat_types st[] = {RATE};
		add_measure(&chunk_send, GENERIC, 0, 120, "TxChunkAll", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);	//[peers]
		monNewSample(chunk_send, 0);	//force publish even if there are no events
	}
	monNewSample(chunk_send, 1);
}

/*
 * Initialize peer level measurements
*/
void init_measures()
{
	enum stat_types stavg[] = {AVG, WIN_AVG};
	enum stat_types stsum[] = {SUM};

	// Traffic
       add_measure(&rx_bytes_chunk_per_sec, BULK_TRANSFER, RXONLY | PACKET | TIMER_BASED, 120, "RxBytesChunkPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), NULL, MSG_TYPE_CHUNK);	//[bytes/s]
       add_measure(&tx_bytes_chunk_per_sec, BULK_TRANSFER, TXONLY | PACKET | TIMER_BASED, 120, "TxBytesChunkPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), NULL, MSG_TYPE_CHUNK);	//[bytes/s]
       add_measure(&rx_bytes_sig_per_sec, BULK_TRANSFER, RXONLY | PACKET | TIMER_BASED, 120, "RxBytesSigPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), NULL, MSG_TYPE_SIGNALLING);	//[bytes/s]
       add_measure(&tx_bytes_sig_per_sec, BULK_TRANSFER, TXONLY | PACKET | TIMER_BASED, 120, "TxBytesSigPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), NULL, MSG_TYPE_SIGNALLING);	//[bytes/s]

	// Chunks
       // replaced by reg_chun_receive add_measure(&rx_chunks, COUNTER, RXONLY | DATA | IN_BAND, 120, "RxChunksAll", stsum, sizeof(stsum)/sizeof(enum stat_types), NULL, MSG_TYPE_CHUNK);	//[chunks]
       // replaced by reg_chun_send add_measure(&tx_chunks, COUNTER, TXONLY | DATA | IN_BAND, 120, "TxChunksAll", stsum, sizeof(stsum)/sizeof(enum stat_types), NULL, MSG_TYPE_CHUNK);	//[chunks]
}

/*
 * Initialize p2p measurements towards a peer
*/
void add_measures(struct nodeID *id)
{
	// Add measures
	int j = 0;
	enum stat_types stavg[] = {AVG, WIN_AVG};
	enum stat_types stsum[] = {SUM};
	enum stat_types stavgrate[] = {AVG, WIN_AVG, RATE};
	enum stat_types stsumrate[] = {SUM, RATE};

	dprintf("adding measures to %s\n",node_addr(id));

	/* HopCount */
       add_measure(&id->mhs[j++], HOPCOUNT, TXRXUNI | PACKET | IN_BAND, 600, "HopCount", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[IP hops]

	/* Round Trip Time */
       add_measure(&id->mhs[j++], RTT, TXRXBI | PACKET | IN_BAND, 60, "RoundTripDelay", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[seconds]

	/* Loss */
       add_measure(&id->mhs[j++], LOSS, TXRXUNI | PACKET | IN_BAND, 60, "LossRate", stavgrate, sizeof(stavgrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//LossRate_avg [probability 0..1] LossRate_rate [lost_pkts/sec]

	// Cumulative Traffic
       //add_measure(&id->mhs[j++], BYTE, RXONLY | PACKET | IN_BAND, 120, "RxBytes", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_ANY);
       //add_measure(&id->mhs[j++], BYTE, TXONLY | PACKET | IN_BAND, 120, "TxBytes", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_ANY);
       add_measure(&id->mhs[j++], BYTE, RXONLY | PACKET | IN_BAND, 120, "RxBytesChunk", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes]
       add_measure(&id->mhs[j++], BYTE, TXONLY | PACKET | IN_BAND, 120, "TxBytesChunk", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes]

	// Traffic
       add_measure(&id->mhs[j++], BULK_TRANSFER, RXONLY | PACKET | TIMER_BASED, 120, "RxBytesChunkPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes/s]
       add_measure(&id->mhs[j++], BULK_TRANSFER, TXONLY | PACKET | TIMER_BASED, 120, "TxBytesChunkPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//[bytes/s]
       add_measure(&id->mhs[j++], BULK_TRANSFER, RXONLY | PACKET | TIMER_BASED, 120, "RxBytesSigPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[bytes/s]
       add_measure(&id->mhs[j++], BULK_TRANSFER, TXONLY | PACKET | TIMER_BASED, 120, "TxBytesSigPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);	//[bytes/s]

	// Chunks
       add_measure(&id->mhs[j++], COUNTER, RXONLY | DATA | IN_BAND, 120, "RxChunks", stsumrate, sizeof(stsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//RxChunks_sum [chunks] RxChunks_rate [chunks/sec]
       add_measure(&id->mhs[j++], COUNTER, TXONLY | DATA | IN_BAND, 120, "TxChunks", stsumrate, sizeof(stsumrate)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);	//TxChunks_sum [chunks] TxChunks_rate [chunks/sec]

	// for static must not be more then 10 or whatever size is in net_helper-ml.c
	id->n_mhs = j;
}

/*
 * Delete p2p measurements towards a peer
*/
void delete_measures(struct nodeID *id)
{
	int j;
	dprintf("deleting measures from %s\n",node_addr(id));
	for(j = 0; j < id->n_mhs; j++) {
		monDestroyMeasure(id->mhs[j]);
	}
}

/*
 * Helper to retrieve a measure
*/
double get_measure(struct nodeID *id, int j, enum stat_types st)
{
	return monRetrieveResult(id->mhs[j], st);
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
 * loss ratio from a given peer as 0..1
*/
double get_lossrate(struct nodeID *id){
	return get_measure(id, 2, WIN_AVG);
}

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
