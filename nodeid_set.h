/*
 *  Copyright (c) 2010 Csaba Kiraly
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#ifndef NODEID_SET_H
#define NODEID_SET_H

#include <stdbool.h>

struct nodeID;

// The usual shuffle
void shuffle(void *base, int nmemb, int size);

void nidset_shuffle(const struct nodeID **base, int nmemb);

int nidset_filter(const struct nodeID **dst, int *dst_size, const struct nodeID **src, int src_size, bool(*f)(const struct nodeID *));

// B \ A
int nidset_complement(const struct nodeID **dst, int *dst_size, const struct nodeID **bs, int bs_size, const struct nodeID **as, int as_size);

bool nidset_find(int *i, const struct nodeID **ids, int ids_size, const struct nodeID *id);

int nidset_add(const struct nodeID **dst, int *dst_size, const struct nodeID **as, int as_size, const struct nodeID **bs, int bs_size);

int nidset_add_i(const struct nodeID **dst, int *dst_size, int max_size, const struct nodeID **as, int as_size);

#endif	/* NODEID_SET_H */

