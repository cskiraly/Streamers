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

void stream_init(int size)
{
  char conf[32];

  sprintf(conf, "size=%d", size);
  cb = cb_init(conf);
}

void received_chunk(const uint8_t *buff, int len)
{
  int res;
  struct chunk c;

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

  input_get(&c);
  if (res > 0) {
    res = cb_add_chunk(cb, &c);
    if (res < 0) {
      free(c.data);
      free(c.attributes);
    }
  }
}

void send_chunk(const struct nodeID **neighbours, int n)
{
  struct chunk *buff;
  int target, c, size;

  dprintf("Send Chunk: %d neighbours\n", n);
  if (n == 0) return;
  buff = cb_get_chunks(cb, &size);
  dprintf("\t %d chunks in buffer...\n", size);
  if (size == 0) return;

  /************ STUPID DUMB SCHEDULING ****************/
  target = n * (rand() / (RAND_MAX + 1.0)); /*0..n-1*/
  c = size * (rand() / (RAND_MAX + 1.0)); /*0..size-1*/
  /************ /STUPID DUMB SCHEDULING ****************/
  dprintf("\t sending chunk[%d]\n", buff[c].id);

  sendChunk(neighbours[target], buff + c);
}
