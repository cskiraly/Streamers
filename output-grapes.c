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
#include <chunkiser.h>

#include "output.h"
#include "measures.h"
#include "dbg.h"

static int next_chunk = -1;
static int buff_size;
extern bool chunk_log;

struct outbuf {
  struct chunk c;
};
static struct outbuf *buff;
static struct output_stream *out;

void output_init(int bufsize, const char *config)
{
  out = out_stream_init(config, "media=av");
  if (out == NULL) {
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
      buff[i].c.data = NULL;
    }
  } else {
   fprintf(stderr, "Error: output buffer re-init not allowed!\n");
   exit(1);
  }
}

static void buffer_print(void)
{
#ifdef DEBUG
  int i;

  if (next_chunk < 0) {
    return;
  }

  dprintf("\toutbuf: %d-> ",next_chunk);
  for (i = next_chunk; i < next_chunk + buff_size; i++) {
    if (buff[i % buff_size].c.data) {
      dprintf("%d",i % 10);
    } else {
      dprintf(".");
    }
  }
  dprintf("\n");
#endif
}

static void buffer_free(int i)
{
  dprintf("\t\tFlush Buf %d: %s\n", i, buff[i].c.data);
  chunk_write(out, &buff[i].c);
  free(buff[i].c.data);
  buff[i].c.data = NULL;
  dprintf("Next Chunk: %d -> %d\n", next_chunk, buff[i].c.id + 1);
  reg_chunk_playout(buff[i].c.id, true, buff[i].c.timestamp);
  next_chunk = buff[i].c.id + 1;
}

static void buffer_flush(int id)
{
  int i = id % buff_size;

  while(buff[i].c.data) {
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
      if (buff[i % buff_size].c.data) {
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
    chunk_write(out, c);
    reg_chunk_playout(c->id, true, c->timestamp);
    next_chunk++;
    buffer_flush(next_chunk);
  } else {
    dprintf("Storing %d (in %d)\n", c->id, c->id % buff_size);
    if (buff[c->id % buff_size].c.data) {
      if (buff[c->id % buff_size].c.id == c->id) {
        /* Duplicate of a stored chunk */
        if(chunk_log){fprintf(stderr,"Duplicate! chunkID: %d\n", c->id);}
        dprintf("\tDuplicate!\n");
        reg_chunk_duplicate();
        return;
      }
      fprintf(stderr, "Crap!, chunkid:%d, storedid: %d\n", c->id, buff[c->id % buff_size].c.id);
      exit(-1);
    }
    /* We previously flushed, so we know that c->id is free */
    memcpy(&buff[c->id % buff_size].c, c, sizeof(struct chunk));
    buff[c->id % buff_size].c.data = malloc(c->size);
    memcpy(buff[c->id % buff_size].c.data, c->data, c->size);
  }
}
