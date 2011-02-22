/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef MEASURES_H
#define MEASURES_H

#include <stdbool.h>

struct nodeID;

void init_measures();
void end_measures();
void add_measures(struct nodeID *id);
void delete_measures(struct nodeID *id);

void reg_chunk_duplicate();
void reg_chunk_playout(int id, bool b, uint64_t timestamp);
void reg_neigh_size(int s);
void reg_chunk_receive(int id, uint64_t timestamp, int hopcount, bool old, bool dup);
void reg_chunk_send(int id);
void reg_offer_accept(bool b);
void reg_offers_in_flight(int running_offer_threads);
void reg_queue_delay(double last_queue_delay);

#ifdef MONL
double get_rtt(struct nodeID *id);
double get_lossrate(struct nodeID *id);
double get_average_lossrate(struct  nodeID**id, int len);
int get_hopcount(struct nodeID *id);
double get_receive_delay(void);
#endif

#endif
