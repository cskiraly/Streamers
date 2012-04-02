/*
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2010-2011 Luca Abeni
 *
 * This file is part of PeerStreamer.
 *
 * PeerStreamer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PeerStreamer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PeerStreamer.  If not, see <http://www.gnu.org/licenses/>.
 *
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

