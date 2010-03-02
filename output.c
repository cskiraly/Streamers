/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <chunk.h>

#include "out-stream.h"
#include "dbg.h"

#define SIZE 8

static int next_chunk;
static int buff_size = SIZE;

static struct {
  void *data;
  int size;
  int id;
} buff[SIZE];

void buffer_free(int i)
{
  dprintf(stderr, "\t\tFlush Buf %d: %s\n", i, (char *)buff[i].data);
  chunk_write(buff[i].id, buff[i].data, buff[i].size);
  free(buff[i].data);
  buff[i].data = NULL;
  dprintf("Next Chunk: %d -> %d\n", next_chunk, buff[i].id + 1);
  next_chunk = buff[i].id + 1;
}

void buffer_flush(int id)
{
  int i = id % buff_size;

  while(buff[i].data) {
    buffer_free(i);
    i = (i + 1) % buff_size;
    if (i == id % buff_size) {
      break;
    }
  }
}

void output_deliver(const struct chunk *c)
{
  dprintf("Chunk %d delivered\n", c->id);
  if (c->id < next_chunk) {
    return;
  }

  if (c->id >= next_chunk + buff_size) {
    int i;

    /* We might need some space for storing this chunk,
     * or the stored chunks are too old
     */
    for (i = next_chunk; i <= c->id - buff_size; i++) {
      if (buff[i % buff_size].data) {
        buffer_free(i % buff_size);
      } else {
        next_chunk++;
      }
    }
    buffer_flush(next_chunk);
    dprintf("Next is now %d, chunk is %d\n", next_chunk, c->id);
  }

  dprintf("%d == %d?\n", c->id, next_chunk);
  if (c->id == next_chunk) {
    chunk_write(c->id, c->data, c->size);
    next_chunk++;
    buffer_flush(next_chunk);
  } else {
    dprintf("Storing %d (in %d)\n", c->id, c->id % buff_size);
    if (buff[c->id % buff_size].data) {
      if (buff[c->id % buff_size].id == c->id) {
        /* Duplicate of a stored chunk */
        dprintf("\tDuplicate!\n");
        return;
      }
      fprintf(stderr, "Crap!\n");
      exit(-1);
    }
    /* We previously flushed, so we know that c->id is free */
    buff[c->id % buff_size].data = malloc(c->size);
    memcpy(buff[c->id % buff_size].data, c->data, c->size);
    buff[c->id % buff_size].size = c->size;
    buff[c->id % buff_size].id = c->id;
  }
}
