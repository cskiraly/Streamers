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

void reg_chunk_duplicate()
{
	if (!chunk_dup) {
		enum stat_types st[] = {SUM};
		chunk_dup = monCreateMeasure(GENERIC, 0);
		monSetParameter (chunk_dup, P_PUBLISHING_RATE, 120);
		monPublishStatisticalType(chunk_dup, "ChunkDuplicates", channel, st , sizeof(st)/sizeof(enum stat_types), NULL);
		monActivateMeasure(chunk_dup, NULL, MSG_TYPE_ANY);
	}
	monNewSample(chunk_dup, 1);
}

void reg_chunk_playout(bool b)
{
	if (!chunk_playout) {
		enum stat_types st[] = {AVG, SUM};
		chunk_playout = monCreateMeasure(GENERIC, 0);
		monSetParameter (chunk_playout, P_PUBLISHING_RATE, 120);
		monPublishStatisticalType(chunk_playout, "ChunksPlayed", channel, st , sizeof(st)/sizeof(enum stat_types), NULL);
		monActivateMeasure(chunk_playout, NULL, MSG_TYPE_ANY);
	}
	monNewSample(chunk_playout, b);
}

void reg_neigh_size(int s)
{
	if (!neigh_size) {
		enum stat_types st[] = {LAST};
		neigh_size = monCreateMeasure(GENERIC, 0);
		monSetParameter (neigh_size, P_PUBLISHING_RATE, 120);
		monPublishStatisticalType(neigh_size, "NeighSize", channel, st , sizeof(st)/sizeof(enum stat_types), NULL);
		monActivateMeasure(neigh_size, NULL, MSG_TYPE_ANY);
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
	id->mhs[j] = monCreateMeasure(HOPCOUNT, TXRXUNI | PACKET | IN_BAND);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 600);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], NULL, channel, stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
	j++;

	/* Round Trip Time */
	id->mhs[j] = monCreateMeasure(RTT, TXRXBI | PACKET | IN_BAND);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 60);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "RoundTripDelay", channel, stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_SIGNALLING);
	j++;

	/* Loss */
	id->mhs[j] = monCreateMeasure(LOSS, TXRXUNI | PACKET | IN_BAND);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 60);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], NULL, channel, stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
	j++;

	// RX bytes
//	id->mhs[j] = monCreateMeasure(BYTE, RXONLY | PACKET | IN_BAND);
//	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 120);
	//Uncomment the following line to publish results
//	monPublishStatisticalType(id->mhs[j], "RxBytes", channel, stsum , sizeof(stsum)/sizeof(enum stat_types), NULL);
//	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_ANY);
//	j++;

	// TX bytes
//	id->mhs[j] = monCreateMeasure(BYTE, TXONLY | PACKET | IN_BAND);
//	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 120);
	//Uncomment the following line to publish results
//	monPublishStatisticalType(id->mhs[j], "TxBytes", channel, stsum , sizeof(stsum)/sizeof(enum stat_types), NULL);
//	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_ANY);
//	j++;

	// RX bytes
	id->mhs[j] = monCreateMeasure(BYTE, RXONLY | PACKET | IN_BAND);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 600);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "RxBytesChunk", channel, stsum , sizeof(stsum)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
	j++;

	// TX bytes
	id->mhs[j] = monCreateMeasure(BYTE, TXONLY | PACKET | IN_BAND);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 600);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "TxBytesChunk", channel, stsum , sizeof(stsum)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
	j++;

	id->mhs[j] = monCreateMeasure(BULK_TRANSFER, RXONLY | PACKET | TIMER_BASED);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 600);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "RxChunkSec", channel, stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
        j++;

	id->mhs[j] = monCreateMeasure(BULK_TRANSFER, TXONLY | PACKET | TIMER_BASED);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 600);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "TxChunkSec", channel, stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
        j++;

	id->mhs[j] = monCreateMeasure(BULK_TRANSFER, RXONLY | PACKET | TIMER_BASED);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 600);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "RxSigSec", channel, stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_SIGNALLING);
        j++;

	id->mhs[j] = monCreateMeasure(BULK_TRANSFER, TXONLY | PACKET | TIMER_BASED);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 600);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "TxSigSec", channel, stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_SIGNALLING);
        j++;

	id->mhs[j] = monCreateMeasure(COUNTER, RXONLY | DATA);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 600);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "RxChunks", channel, stsum , sizeof(stsum)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
        j++;

	id->mhs[j] = monCreateMeasure(COUNTER, TXONLY | DATA);
	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 600);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "TxChunks", channel, stsum , sizeof(stsum)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
        j++;

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
