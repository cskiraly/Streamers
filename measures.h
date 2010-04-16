#ifndef MEASURES_H
#define MEASURES_H

#include <stdbool.h>

void init_measures();
void add_measures(struct nodeID *id);
void delete_measures(struct nodeID *id);
double get_rtt(struct nodeID *id);
double get_lossrate(struct nodeID *id);
void reg_chunk_duplicate();
void reg_chunk_playout(bool b);
void reg_neigh_size(int s);
void reg_chunk_receive(int id);
void reg_chunk_send(int id);

#endif
