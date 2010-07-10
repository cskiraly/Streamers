/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <chunk.h>

#include "input.h"
#include "input-stream.h"
#include "dbg.h"

struct input_desc {
  struct input_stream *s;
  int id;
  int interframe;
  uint64_t start_time;
  uint64_t first_ts;
};

struct input_desc *input_open(const char *fname, uint16_t flags)
{
  struct input_desc *res;
  struct timeval tv;

  res = (struct input_desc *)malloc(sizeof(struct input_desc));
  if (res == NULL) {
    return NULL;
  }
  gettimeofday(&tv, NULL);
  res->start_time = tv.tv_usec + tv.tv_sec * 1000000ULL;
  res->first_ts = 0;
  res->s = input_stream_open(fname, &res->interframe, flags);
  if (res->s == NULL) {
    free(res);
    return NULL;
  }
  res->id = (res->start_time / res->interframe) % INT_MAX; //TODO: verify 32/64 bit

  return res;
}

void input_close(struct input_desc *s)
{
  input_stream_close(s->s);
  free(s);
}

int input_get(struct input_desc *s, struct chunk *c)
{
  struct timeval now;
  int64_t delta;

  c->data = chunkise(s->s, s->id, &c->size, &c->timestamp);
  if (c->size == -1) {
    return -1;
  }
  if (c->data) {
    c->id = s->id++;
  }
  c->attributes_size = 0;
  c->attributes = NULL;
  if (s->first_ts == 0) {
    s->first_ts = c->timestamp;
  }
  delta = c->timestamp - s->first_ts + s->interframe;
  gettimeofday(&now, NULL);
  delta = delta + s->start_time - now.tv_sec * 1000000ULL - now.tv_usec;
  dprintf("Delta: %lld\n", delta);
  dprintf("Generate Chunk[%d] (TS: %llu)\n", c->id, c->timestamp);

  c->timestamp = now.tv_sec * 1000000ULL + now.tv_usec;

  return delta > 0 ? delta : 0;
}
