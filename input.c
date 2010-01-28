#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

struct input_desc *input_open(const char *fname)
{
  struct input_desc *res;
  struct timeval tv;

  res = malloc(sizeof(struct input_desc));
  if (res == NULL) {
    return NULL;
  }
  res->id = 0;
  gettimeofday(&tv, NULL);
  res->start_time = tv.tv_usec + tv.tv_sec * 1000000ULL;
  res->s = input_stream_open(fname, &res->interframe);
  if (res->s == NULL) {
    free(res);
    res = NULL;
  }

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
  c->id = s->id++;
  c->attributes_size = 0;
  c->attributes = NULL;
  if (s->first_ts == 0) {
    s->first_ts = c->timestamp;
  }
  delta = c->timestamp - s->first_ts + s->interframe;
  gettimeofday(&now, NULL);
  delta = delta + s->start_time - now.tv_sec * 1000000ULL - now.tv_usec;
  dprintf("Delta: %lld\n", delta);
  dprintf("Generate Chunk[%d] (TS: %llu): %s\n", c->id, c->timestamp, c->data);

  return delta > 0 ? delta : 0;
}

