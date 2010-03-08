/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <net_helper.h>
#include <chunk.h> 
#include <chunkbuffer.h> 
#include "chunkbuffer_helper.h"
#include <trade_msg_la.h>
#include <trade_msg_ha.h>
#include <peerset.h>
#include <peer.h>
#include <chunkidset.h>

#include "streaming.h"
#include "output.h"
#include "input.h"
#include "dbg.h"
#include "chunk_signaling.h"

#include "scheduler_la.h"

static struct chunk_buffer *cb;
static struct input_desc *input;
static int cb_size;

int _needs(struct chunkID_set *cset, int cb_size, int cid);

void stream_init(int size, struct nodeID *myID)
{
  char conf[32];

  cb_size = size;

  sprintf(conf, "size=%d", cb_size);
  cb = cb_init(conf);
  chunkDeliveryInit(myID);
}

int source_init(const char *fname, struct nodeID *myID, bool loop)
{
  input = input_open(fname, loop ? INPUT_LOOP : 0);
  if (input == NULL) {
    return -1;
  }

  stream_init(1, myID);
  return 0;
}

struct chunkID_set *cb_to_bmap(struct chunk_buffer *chbuf)
{
  struct chunk *chunks;
  int num_chunks, i;
  struct chunkID_set *my_bmap = chunkID_set_init(0);
  chunks = cb_get_chunks(chbuf, &num_chunks);

  for(i=0; i<num_chunks; i++) {
    chunkID_set_add_chunk(my_bmap, chunks[i].id);
  }
  return my_bmap;
}

void send_bmap(struct peer *to)
{
  struct chunkID_set *my_bmap = cb_to_bmap(cb);
   sendMyBufferMap(to->id, my_bmap, cb_size, 0);

  chunkID_set_clear(my_bmap,0);
  free(my_bmap);
}

void received_chunk(struct peerset *pset, struct nodeID *from, const uint8_t *buff, int len)
{
  int res;
  static struct chunk c;
  struct peer *p;

  res = decodeChunk(&c, buff + 1, len - 1);
  if (res > 0) {
    dprintf("Received chunk %d from peer: %s\n", c.id, node_addr(from));
    output_deliver(&c);
    res = cb_add_chunk(cb, &c);
    if (res < 0) {
      dprintf("\tchunk too old, buffer full with newer chunks\n");
      free(c.data);
      free(c.attributes);
    }
    p = peerset_get_peer(pset,from);
    if (!p) {
      fprintf(stderr,"warning: received chunk %d from unknown peer: %s! Adding it to neighbourhood!\n", c.id, node_addr(from));
      peerset_add_peer(pset,from);
      p = peerset_get_peer(pset,from);
    }
    if (p) {	//now we have it almost sure
      chunkID_set_add_chunk(p->bmap,c.id);	//don't send it back
      send_bmap(p);	//send explicit ack
    }
  } else {
    fprintf(stderr,"\tError: can't decode chunk!\n");
  }
}

int generated_chunk(suseconds_t *delta)
{
  int res;
  struct chunk c;

  *delta = input_get(input, &c);
  if (*delta < 0) {
    fprintf(stderr, "Error in input!\n");
    exit(-1);
  }
  if (c.data == NULL) {
    return 0;
  }
  res = cb_add_chunk(cb, &c);
  if (res < 0) {
    free(c.data);
    free(c.attributes);
  }

  return 1;
}

/**
 *example function to filter chunks based on whether a given peer needs them.
 *
 * Looks at buffermap information received about the given peer.
 */
int needs(struct peer *p, struct chunk *c){
  dprintf("\t%s needs c%d ? :",node_addr(p->id),c->id);
  if (! p->bmap) {
    dprintf("no bmap\n");
    return 1;	// if we have no bmap information, we assume it needs the chunk (aggressive behaviour!)
  }
  return _needs(p->bmap, p->cb_size, c->id);
}

int _needs(struct chunkID_set *cset, int cb_size, int cid){
  if (chunkID_set_check(cset,cid) < 0) { //it might need the chunk
    int missing, min;
    //@TODO: add some bmap_timestamp based logic

    if (chunkID_set_size(cset) == 0) {
      dprintf("bmap empty\n");
      return 1;	// if the bmap seems empty, it needs the chunk
    }
    missing = cb_size - chunkID_set_size(cset);
    missing = missing < 0 ? 0 : missing;
    min = chunkID_set_get_chunk(cset,0);
      dprintf("%s ... cid(%d) >= min(%d) - missing(%d) ?\n",(cid >= min - missing)?"YES":"NO",cid, min, missing);
    return (cid >= min - missing);
  }

  dprintf("has it\n");
  return 0;
}

double randomPeer(struct peer **p){
  return 1;
}
double getChunkTimestamp(struct chunk **c){
  return (double) (*c)->timestamp;
}


void send_chunk(const struct peerset *pset)
{
  struct chunk *buff;
  int size, res, i, n;
  struct peer *neighbours;

  n = peerset_size(pset);
  neighbours = peerset_get_peers(pset);
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
  {
    size_t selectedpairs_len = 1;
    struct chunk *chunkps[size];
    struct peer *peerps[n];
    struct PeerChunk selectedpairs[1];
  
    for (i = 0;i < size; i++) chunkps[i] = buff+i;
    for (i = 0; i<n; i++) peerps[i] = neighbours+i;
    schedSelectPeerFirst(SCHED_WEIGHTED, peerps, n, chunkps, size, selectedpairs, &selectedpairs_len, needs, randomPeer, getChunkTimestamp);
  /************ /USE SCHEDULER ****************/

    for (i=0; i<selectedpairs_len ; i++){
      struct peer *p = selectedpairs[i].peer;
      struct chunk *c = selectedpairs[i].chunk;
      dprintf("\t sending chunk[%d] to ", c->id);
      dprintf("%s\n", node_addr(p->id));

      send_bmap(p);
      res = sendChunk(p->id, c);
      dprintf("\tResult: %d\n", res);
      if (res>=0) {
        chunkID_set_add_chunk(p->bmap,c->id); //don't send twice ... assuming that it will actually arrive
      }
    }
  }
}
