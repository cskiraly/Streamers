#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <net_helper.h>
#include <chunk.h> 
#include <chunkbuffer.h> 
#include <trade_msg_la.h>
#include <trade_msg_ha.h>

#include "streaming.h"
#include "output.h"
#include "input.h"
#include "dbg.h"

static struct chunk_buffer *cb;
static struct input_desc *input;

void stream_init(int size, struct nodeID *myID)
{
  char conf[32];

  sprintf(conf, "size=%d", size);
  cb = cb_init(conf);
  chunkInit(myID);
}

int source_init(const char *fname, struct nodeID *myID)
{
  input = input_open(fname);
  if (input == NULL) {
    return -1;
  }

  stream_init(1, myID);
  return 0;
}

void received_chunk(const uint8_t *buff, int len)
{
  int res;
  static struct chunk c;

  res = decodeChunk(&c, buff + 1, len - 1);
  if (res > 0) {
    output_deliver(&c);
    res = cb_add_chunk(cb, &c);
    if (res < 0) {
      free(c.data);
      free(c.attributes);
    }
  }
}

void generated_chunk(void)
{
  int res;
  struct chunk c;

  if (input_get(input, &c) <= 0) {
    fprintf(stderr, "Error in input!\n");
    exit(-1);
  }
  res = cb_add_chunk(cb, &c);
  if (res < 0) {
    free(c.data);
    free(c.attributes);
  }
}

void send_chunk(const struct nodeID **neighbours, int n)
{
  struct chunk *buff;
  int target, c, size, res;

  dprintf("Send Chunk: %d neighbours\n", n);
  if (n == 0) return;
  buff = cb_get_chunks(cb, &size);
  dprintf("\t %d chunks in buffer...\n", size);
  if (size == 0) return;

  /************ STUPID DUMB SCHEDULING ****************/
  target = n * (rand() / (RAND_MAX + 1.0)); /*0..n-1*/
  c = size * (rand() / (RAND_MAX + 1.0)); /*0..size-1*/
  /************ /STUPID DUMB SCHEDULING ****************/
  dprintf("\t sending chunk[%d] (%d) to ", buff[c].id, c);
  dprintf("%s\n", node_addr(neighbours[target]));

  res = sendChunk(neighbours[target], buff + c);
  dprintf("Result: %d\n", res);
}
