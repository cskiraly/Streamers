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

#include "output.h"
#include "measures.h"
#include "out-stream.h"
#include "dbg.h"

static int next_chunk = -1;
static int buff_size;

struct outbuf {
  void *data;
  int size;
  int id;
  uint64_t timestamp;
};
static struct outbuf *buff;

void output_init(int bufsize, const char *config)
{
  if (out_stream_init(config) < 0) {
     fprintf(stderr, "Error: can't initialize output module\n");
     exit(1);
  }

  if (!buff) {
    int i;

    buff_size = bufsize;
    buff = malloc(sizeof(struct outbuf) * buff_size);
    if (!buff) {
     fprintf(stderr, "Error: can't allocate output buffer\n");
     exit(1);
    }
    for (i = 0; i < buff_size; i++) {
      buff[i].data = NULL;
    }
  } else {
   fprintf(stderr, "Error: output buffer re-init not allowed!\n");
   exit(1);
  }
}

void buffer_print()
{
#ifdef DEBUG
  int i;

  if (next_chunk < 0) {
    return;
  }

  dprintf("\toutbuf: %d-> ",next_chunk);
  for (i = next_chunk; i < next_chunk + buff_size; i++) {
    if (buff[i % buff_size].data) {
      dprintf("%d",i % 10);
    } else {
      dprintf(".");
    }
  }
  dprintf("\n");
#endif
}

void buffer_free(int i)
{
  dprintf("\t\tFlush Buf %d: %s\n", i, buff[i].data);
  chunk_write(buff[i].id, buff[i].data, buff[i].size);
  free(buff[i].data);
  buff[i].data = NULL;
  dprintf("Next Chunk: %d -> %d\n", next_chunk, buff[i].id + 1);
  reg_chunk_playout(buff[i].id, true, buff[i].timestamp);
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
  if (!buff) {
    fprintf(stderr, "Warning: code should use output_init!!! Setting output buffer to 8\n");
    output_init(8, NULL);
  }

  dprintf("Chunk %d delivered\n", c->id);
  buffer_print();
  if (c->id < next_chunk) {
    return;
  }

  /* Initialize buffer with first chunk */
  if (next_chunk == -1) {
    next_chunk = c->id; // FIXME: could be anything between c->id and (c->id - buff_size + 1 > 0) ? c->id - buff_size + 1 : 0
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
        reg_chunk_playout(c->id, false, c->timestamp); // FIXME: some chunks could be counted as lost at the beginning, depending on the initialization of next_chunk
        next_chunk++;
      }
    }
    buffer_flush(next_chunk);
    dprintf("Next is now %d, chunk is %d\n", next_chunk, c->id);
  }

  dprintf("%d == %d?\n", c->id, next_chunk);
  if (c->id == next_chunk) {
    dprintf("\tOut Chunk[%d] - %d: %s\n", c->id, c->id % buff_size, c->data);
    chunk_write(c->id, c->data, c->size);
    reg_chunk_playout(c->id, true, c->timestamp);
    next_chunk++;
    buffer_flush(next_chunk);
  } else {
    dprintf("Storing %d (in %d)\n", c->id, c->id % buff_size);
    if (buff[c->id % buff_size].data) {
      if (buff[c->id % buff_size].id == c->id) {
        /* Duplicate of a stored chunk */
	fprintf(stderr,"Duplicate! chunkID: %d\n", c->id); // ENST
        dprintf("\tDuplicate!\n");
        reg_chunk_duplicate();
        return;
      }
      fprintf(stderr, "Crap!, chunkid:%d, storedid: %d\n", c->id, buff[c->id % buff_size].id);
      exit(-1);
    }
    /* We previously flushed, so we know that c->id is free */
    buff[c->id % buff_size].data = malloc(c->size);
    memcpy(buff[c->id % buff_size].data, c->data, c->size);
    buff[c->id % buff_size].size = c->size;
    buff[c->id % buff_size].id = c->id;
    buff[c->id % buff_size].timestamp = c->timestamp;
  }
}
