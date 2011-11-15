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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <net_helper.h>

#include "nodeid_set.h"

#define MIN(A,B) (((A) < (B)) ? (A) : (B))

// The usual shuffle
void shuffle(void *base, int nmemb, int size) {
  int i;
  unsigned char t[size];
  unsigned char* b = base;

  for (i = nmemb - 1; i > 0; i--) {
    int newpos = (rand()/(RAND_MAX + 1.0)) * (i + 1);
    memcpy(t, b + size * newpos, size);
    memmove(b + size * newpos, b + size * i, size);
    memcpy(b + size * i, t, size);
  }
}

void nidset_shuffle(const struct nodeID **base, int nmemb) {
  shuffle(base, nmemb, sizeof(struct nodeID *));
}

int nidset_filter(const struct nodeID **dst, int *dst_size, const struct nodeID **src, int src_size, bool(*f)(const struct nodeID *)) {
  int i;
  int max_size = *dst_size;
  *dst_size = 0;

  for (i = 0; i < src_size; i++) {
    if (f(src[i])) {
      if (*dst_size < max_size) {
        dst[(*dst_size)++] = src[i];
      } else {
        return -1;
      }
    }
  }

  return 0;
}

// B \ A
int nidset_complement(const struct nodeID **dst, int *dst_size, const struct nodeID **bs, int bs_size, const struct nodeID **as, int as_size) {
  int i, j;
  int max_size = *dst_size;
  *dst_size = 0;

  for (i = 0; i < bs_size; i++) {
    for (j = 0; j < as_size; j++) {
      if (nodeid_equal(bs[i], as[j])) {
        break;
      }
    }
    if (j >= as_size) {
      if (*dst_size < max_size) {
        dst[(*dst_size)++] = bs[i];
      } else {
        return -1;
      }
    }
  }

  return 0;
}

bool nidset_find(int *i, const struct nodeID **ids, int ids_size, const struct nodeID *id) {
  for (*i = 0; *i < ids_size; (*i)++) {
    if (nodeid_equal(ids[*i],id)) {
      return true;
    }
  }
  return false;
}

int nidset_add(const struct nodeID **dst, int *dst_size, const struct nodeID **as, int as_size, const struct nodeID **bs, int bs_size) {
  int r;
  int i;
  int max_size = *dst_size;

  i = MIN(as_size, max_size);
  memcpy(dst, as, i * sizeof(struct nodeID*));
  *dst_size = i;
  if (i < as_size) return -1;

  max_size -= *dst_size;
  r = nidset_complement(dst + *dst_size, &max_size, bs, bs_size, as, as_size);
  *dst_size += max_size;

  return r;
}

int nidset_add_i(const struct nodeID **dst, int *dst_size, int max_size, const struct nodeID **as, int as_size) {
  int r;

  max_size -= *dst_size;
  r = nidset_complement(dst + *dst_size, &max_size, as, as_size, dst, *dst_size);
  *dst_size += max_size;

  return r;
}
