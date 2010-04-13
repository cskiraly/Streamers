#include <mon.h>
#include <ml.h>

#include "dbg.h"
#include <net_helper.h>

typedef struct nodeID {
	socketID_handle addr;
	int connID;	// connection associated to this node, -1 if myself
	int refcnt;
	//a quick and dirty static vector for measures TODO: make it dinamic
	MonHandler mhs[10];
	int n_mhs;
} nodeID;


void add_measures(struct nodeID *id)
{
	// Add measures
	int j = 0;
	enum stat_types stavg[] = {AVG};
	enum stat_types stsum[] = {SUM};

	dprintf("adding measures to %s\n",node_addr(id));

	// RX bytes
	id->mhs[j] = monCreateMeasure(BYTE, RXONLY | PACKET | IN_BAND);
//	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 100);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "RxBytesChunk", "OfferStreamer", stsum , sizeof(stsum)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
	j++;

	// TX bytes
	id->mhs[j] = monCreateMeasure(BYTE, TXONLY | PACKET | IN_BAND);
//	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 100);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "TxBytesChunk", "OfferStreamer", stsum , sizeof(stsum)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
	j++;

	/* HopCount */
	id->mhs[j] = monCreateMeasure(HOPCOUNT, TXRXUNI | PACKET | IN_BAND);
//	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 100);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], NULL, "OfferStreamer", stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
	j++;

	/* Round Trip Time */
	id->mhs[j] = monCreateMeasure(RTT, TXRXBI | PACKET | IN_BAND);
	//monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 100);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], "RoundTripDelay", "OfferStreamer", stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_SIGNALLING);
	j++;

	/* Loss */
	id->mhs[j] = monCreateMeasure(LOSS, TXRXUNI | PACKET | IN_BAND);
	//monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 100);
	//monSetParameter (id->mhs[j], P_WINDOW_SIZE, 100);
	//Uncomment the following line to publish results
	monPublishStatisticalType(id->mhs[j], NULL, "OfferStreamer", stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_CHUNK);
	j++;

	// RX bytes
	//id->mhs[j] = monCreateMeasure(BYTE, RXONLY | PACKET | IN_BAND);
//	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 100);
	//Uncomment the following line to publish results
	//monPublishStatisticalType(id->mhs[j], "RxBytes", "OfferStreamer", stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	//monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_ANY);
	//j++;

	// TX bytes
	//id->mhs[j] = monCreateMeasure(BYTE, TXONLY | PACKET | IN_BAND);
//	monSetParameter (id->mhs[j], P_PUBLISHING_RATE, 100);
	//Uncomment the following line to publish results
	//monPublishStatisticalType(id->mhs[j], "TxBytes", "OfferStreamer", stavg , sizeof(stavg)/sizeof(enum stat_types), NULL);
	//monActivateMeasure(id->mhs[j], id->addr, MSG_TYPE_ANY);
	//j++;

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
	return get_measure(id, 3, WIN_AVG);
}

double get_lossrate(struct nodeID *id){
	return get_measure(id, 4, WIN_AVG);
}
