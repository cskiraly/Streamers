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
};

struct input_desc *input_open(const char *fname)
{
  struct input_desc *res;

  res = malloc(sizeof(struct input_desc));
  if (res == NULL) {
    return NULL;
  }
  res->id = 0;
  res->s = input_stream_open(fname);
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
  c->data = chunkise(s->s, s->id, &c->size, &c->timestamp);
  c->id = s->id++;
  c->attributes_size = 0;
  c->attributes = NULL;

  dprintf("Generate Chunk[%d] (TS: %llu): %s\n", c->id, c->timestamp, c->data);

  return 1;
}

