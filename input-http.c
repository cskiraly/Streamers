/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <microhttpd.h>

#include <chunk.h>
#include <http_default_urls.h>
#include <external_chunk_transcoding.h>

#include "input.h"

extern struct chunk_buffer *cb;

#ifdef THREADS
extern pthread_mutex_t cb_mutex;
extern pthread_mutex_t topology_mutex;
#else
pthread_mutex_t cb_mutex;
#endif

struct input_desc {
  int dummy;
};

struct MHD_Daemon *httpd;

struct input_desc *input_open(const char *fname, uint16_t flags, int *fds, int fds_size)
{
  struct input_desc *res;

  if (fds_size >= 1) {
    *fds = -1; //This input module needs no fds to monitor
  }

  res = malloc(sizeof(struct input_desc));
  if (res == NULL) {
    return NULL;
  }

  res->dummy = 0;
dprintf("BEFORE INIT! %d\n", res->dummy);
#ifndef THREADS
  pthread_mutex_init(&cb_mutex, NULL);
#endif
  //this daemon will listen the network for incoming chunks from a streaming source
  //on the following path and port
  httpd = initChunkPuller(UL_DEFAULT_CHUNKBUFFER_PATH, UL_DEFAULT_CHUNKBUFFER_PORT);
dprintf("AFTER INIT! %d\n", res->dummy);

  return res;
}

void input_close(struct input_desc *s)
{
  free(s);
  finalizeChunkPuller(httpd);
}

//this one is not used, just routinely called by the firging thread
int input_get(struct input_desc *s, struct chunk *c)
{
  c->data = NULL;
  return 0;
}

//this is the real one, called by the http receiver thread
int enqueueBlock(const uint8_t *block, const int block_size) {
	static int ExternalChunk_header_size = 5*CHUNK_TRANSCODING_INT_SIZE + 2*CHUNK_TRANSCODING_INT_SIZE + 2*CHUNK_TRANSCODING_INT_SIZE + 1*CHUNK_TRANSCODING_INT_SIZE*2;
  int decoded_size = 0;
	int res = -1;
//  struct chunk gchunk;

  Chunk* gchunk=NULL;
	gchunk = (Chunk *)malloc(sizeof(Chunk));
	if(!gchunk) {
		printf("Memory error in gchunk!\n");
		return -1;
	}

  decoded_size = decodeChunk(gchunk, block, block_size);

  if(decoded_size < 0 || decoded_size != GRAPES_ENCODED_CHUNK_HEADER_SIZE + ExternalChunk_header_size + gchunk->size) {
	    fprintf(stderr, "chunk %d probably corrupted!\n", gchunk->id);
		return -1;
	}

  if(cb) {
#ifdef THREADS
  	pthread_mutex_lock(&topology_mutex);
#endif
  	pthread_mutex_lock(&cb_mutex);
  	res = add_chunk(gchunk);
//  	free(gchunk);
//  	pthread_mutex_unlock(&cb_mutex);
//  }
//  if (res < 0) { //chunk sequence is older than previous chunk (SHOULD SEND ANYWAY!!!)
//    free(gchunk->data);
//    free(gchunk->attributes);
//    fprintf(stderr, "Chunk %d of %d bytes FAIL res %d\n", gchunk->id, gchunk->size, res);
//  }
//  else {
//    pthread_mutex_lock(&cb_mutex);
    if(res) send_chunk(); //push it
    pthread_mutex_unlock(&cb_mutex);
#ifdef THREADS
  	pthread_mutex_unlock(&topology_mutex);
#endif
  }

  return 0;
}

