/*
 *  Copyright (c) 2010 Csaba Kiraly
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <net_helper.h>

#include "nodeid_set.h"

#define MIN(A,B) (((A) < (B)) ? (A) : (B))

// The usual shuffle
void shuffle(void *base, size_t nmemb, size_t size) {
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

void nidset_shuffle(const struct nodeID **base, size_t nmemb) {
  shuffle(base, nmemb, sizeof(struct nodeID *));
}

int nidset_filter(const struct nodeID **dst, size_t *dst_size, const struct nodeID **src, size_t src_size, bool(*f)(const struct nodeID *)) {
  size_t i;
  size_t max_size = *dst_size;
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
int nidset_complement(const struct nodeID **dst, size_t *dst_size, const struct nodeID **bs, size_t bs_size, const struct nodeID **as, size_t as_size) {
  size_t i, j;
  size_t max_size = *dst_size;
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

bool nidset_find(size_t *i, const struct nodeID **ids, size_t ids_size, const struct nodeID *id) {
  for (*i = 0; *i < ids_size; (*i)++) {
    if (nodeid_equal(ids[*i],id)) {
      return true;
    }
  }
  return false;
}

int nidset_add(const struct nodeID **dst, size_t *dst_size, const struct nodeID **as, size_t as_size, const struct nodeID **bs, size_t bs_size) {
  int r;
  size_t i;
  size_t max_size = *dst_size;

  i = MIN(as_size, max_size);
  memcpy(dst, as, i * sizeof(struct nodeID*));
  *dst_size = i;
  if (i < as_size) return -1;

  max_size -= *dst_size;
  r = nidset_complement(dst + *dst_size, &max_size, bs, bs_size, as, as_size);
  *dst_size += max_size;

  return r;
}

int nidset_add_i(const struct nodeID **dst, size_t *dst_size, size_t max_size, const struct nodeID **as, size_t as_size) {
  int r;

  max_size -= *dst_size;
  r = nidset_complement(dst + *dst_size, &max_size, as, as_size, dst, *dst_size);
  *dst_size += max_size;

  return r;
}
