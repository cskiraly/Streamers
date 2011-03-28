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
void shuffle(void *base, size_t nmemb, size_t size);

void nidset_shuffle(const struct nodeID **base, size_t nmemb);

int nidset_filter(const struct nodeID **dst, size_t *dst_size, const struct nodeID **src, size_t src_size, bool(*f)(const struct nodeID *));

// B \ A
int nidset_complement(const struct nodeID **dst, size_t *dst_size, const struct nodeID **bs, size_t bs_size, const struct nodeID **as, size_t as_size);

bool nidset_find(size_t *i, const struct nodeID **ids, size_t ids_size, const struct nodeID *id);

int nidset_add(const struct nodeID **dst, size_t *dst_size, const struct nodeID **as, size_t as_size, const struct nodeID **bs, size_t bs_size);

int nidset_add_i(const struct nodeID **dst, size_t *dst_size, size_t max_size, const struct nodeID **as, size_t as_size);

#endif	/* NODEID_SET_H */

