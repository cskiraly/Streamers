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

static MonHandler chunk_dup, chunk_playout, neigh_size;

void add_measure(MonHandler *mh, MeasurementId id, MeasurementCapabilities mc, MonParameterValue rate, const char *pubname, enum stat_types st[], int length, SocketId dst, MsgType mt)
{
	*mh = monCreateMeasure(id, mc);
	monSetParameter (*mh, P_PUBLISHING_RATE, rate);
	monPublishStatisticalType(*mh, pubname, channel, st , length, NULL);
	monActivateMeasure(*mh, dst, mt);
}

void reg_chunk_duplicate()
{
	if (!chunk_dup) {
		enum stat_types st[] = {SUM};
		add_measure(&chunk_dup, GENERIC, 0, 120, "ChunkDuplicates", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);
	}
	monNewSample(chunk_dup, 1);
}

void reg_chunk_playout(bool b)
{
	if (!chunk_playout && b) {	//don't count losses before the first arrived chunk
		enum stat_types st[] = {AVG, SUM};
		add_measure(&chunk_playout, GENERIC, 0, 120, "ChunksPlayed", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);
	}
	monNewSample(chunk_playout, b);
}

void reg_neigh_size(int s)
{
	if (!neigh_size) {
		enum stat_types st[] = {LAST};
		add_measure(&neigh_size, GENERIC, 0, 120, "NeighSize", st, sizeof(st)/sizeof(enum stat_types), NULL, MSG_TYPE_ANY);
	}
	monNewSample(neigh_size, s);
}

void add_measures(struct nodeID *id)
{
	// Add measures
	int j = 0;
	enum stat_types stavg[] = {AVG, WIN_AVG};
	enum stat_types stsum[] = {SUM};

	dprintf("adding measures to %s\n",node_addr(id));

	/* HopCount */
       add_measure(&id->mhs[j++], HOPCOUNT, TXRXUNI | PACKET | IN_BAND, 600, "HopCount", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);

	/* Round Trip Time */
       add_measure(&id->mhs[j++], RTT, TXRXBI | PACKET | IN_BAND, 60, "RoundTripDelay", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);

	/* Loss */
       add_measure(&id->mhs[j++], LOSS, TXRXUNI | PACKET | IN_BAND, 60, "LossRate", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);

	// Cumulative Traffic
       //add_measure(&id->mhs[j++], BYTE, RXONLY | PACKET | IN_BAND, 120, "RxBytes", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_ANY);
       //add_measure(&id->mhs[j++], BYTE, TXONLY | PACKET | IN_BAND, 120, "TxBytes", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_ANY);
       add_measure(&id->mhs[j++], BYTE, RXONLY | PACKET | IN_BAND, 120, "RxBytesChunk", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);
       add_measure(&id->mhs[j++], BYTE, TXONLY | PACKET | IN_BAND, 120, "TxBytesChunk", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);

	// Traffic
       add_measure(&id->mhs[j++], BULK_TRANSFER, RXONLY | PACKET | TIMER_BASED, 120, "RxBytesChunkPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);
       add_measure(&id->mhs[j++], BULK_TRANSFER, TXONLY | PACKET | TIMER_BASED, 120, "TxBytesChunkPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);
       add_measure(&id->mhs[j++], BULK_TRANSFER, RXONLY | PACKET | TIMER_BASED, 120, "RxBytesSigPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);
       add_measure(&id->mhs[j++], BULK_TRANSFER, TXONLY | PACKET | TIMER_BASED, 120, "TxBytesSigPSec", stavg, sizeof(stavg)/sizeof(enum stat_types), id->addr, MSG_TYPE_SIGNALLING);

	// Chunks
       add_measure(&id->mhs[j++], COUNTER, RXONLY | DATA | IN_BAND, 120, "RxChunks", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);
       add_measure(&id->mhs[j++], COUNTER, TXONLY | DATA | IN_BAND, 120, "TxChunks", stsum, sizeof(stsum)/sizeof(enum stat_types), id->addr, MSG_TYPE_CHUNK);

	// for static must not be more then 10 or whatever size is in net_helper-ml.c
	id->n_mhs = j;
}

void delete_measures(struct nodeID *id)
{
	int j;
	dprintf("deleting measures from %s\n",node_addr(id));
	for(j = 0; j < id->n_mhs; j++) {
		monDestroyMeasure(id->mhs[j]);
	}
}

double get_measure(struct nodeID *id, int j, enum stat_types st)
{
	return monRetrieveResult(id->mhs[j], st);
}

//in seconds
double get_rtt(struct nodeID *id){
	return get_measure(id, 1, WIN_AVG);
}

double get_lossrate(struct nodeID *id){
	return get_measure(id, 2, WIN_AVG);
}
