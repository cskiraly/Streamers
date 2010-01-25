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

#include "scheduler_la.h"

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

/**
 *example function to filter chunks based on whether a given peer needs them. The real implementation
 * should look at buffermap information received about the given peer (or it should guess)
 */
int needs(const struct peer *p, const struct chunk *c){
  return 1;	//TODO: avoid at least sending to the source :)
}
double randomPeer(const struct peer **p){
  return 1;
}
double getChunkTimestamp(const struct chunk **c){
  return (double) (*c)->timestamp;
}


void send_chunk(const struct nodeID **neighbours, int n)
{
  struct chunk *buff;
  int target, c, size, res, i;

  dprintf("Send Chunk: %d neighbours\n", n);
  if (n == 0) return;
  buff = cb_get_chunks(cb, &size);
  dprintf("\t %d chunks in buffer...\n", size);
  if (size == 0) return;

  /************ STUPID DUMB SCHEDULING ****************/
  //target = n * (rand() / (RAND_MAX + 1.0)); /*0..n-1*/
  //c = size * (rand() / (RAND_MAX + 1.0)); /*0..size-1*/
  /************ /STUPID DUMB SCHEDULING ****************/

  /************ USE SCHEDULER ****************/
  size_t selectedpairs_len = 1;
  struct chunk *pbuff[size];
  for (i=0;i<size;i++) pbuff[i]=buff+i;
  struct PeerChunk selectedpairs[1];
  schedSelectPeerFirst(SCHED_BEST, neighbours, n, pbuff, size, selectedpairs, &selectedpairs_len, needs, randomPeer, getChunkTimestamp);
  /************ /USE SCHEDULER ****************/

  for (i=0; i<selectedpairs_len ; i++){
    dprintf("\t sending chunk[%d] to ", selectedpairs[i].chunk->id);
    dprintf("%s\n", node_addr(selectedpairs[i].peer));

    res = sendChunk(selectedpairs[i].peer, selectedpairs[i].chunk);
    dprintf("Result: %d\n", res);
  }
}
