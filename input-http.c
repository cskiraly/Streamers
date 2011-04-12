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

#ifdef HTTPIO_MHD
	#include <microhttpd.h>
	#include <pthread.h>
#elif defined HTTPIO_EVENT
	#include "chunk_external_interface.h"
#endif

#include <chunk.h>
#include <http_default_urls.h>
#include <external_chunk_transcoding.h>

#include "input.h"

extern struct chunk_buffer *cb;
extern int multiply;

#ifdef HTTPIO_MHD
	#ifdef THREADS
	extern pthread_mutex_t cb_mutex;
	extern pthread_mutex_t topology_mutex;
	#else
	pthread_mutex_t cb_mutex = PTHREAD_MUTEX_INITIALIZER;
	#endif
	struct MHD_Daemon *httpd;
#endif

struct input_desc {
  int dummy;
};


//this is the real one, called by the httpd receiver thread
int enqueueBlock(const uint8_t *block, const int block_size) {
	static int ExternalChunk_header_size = 5*CHUNK_TRANSCODING_INT_SIZE + 2*CHUNK_TRANSCODING_INT_SIZE + 2*CHUNK_TRANSCODING_INT_SIZE + 1*CHUNK_TRANSCODING_INT_SIZE*2;
  int decoded_size = 0;
	int res = -1;
//  struct chunk gchunk;

  Chunk* gchunk=NULL;
	gchunk = (Chunk *)malloc(sizeof(Chunk));
	if(!gchunk) {
		fprintf(stderr, "Memory error in gchunk!\n");
		return -1;
	}

  decoded_size = decodeChunk(gchunk, block, block_size);

  if(decoded_size < 0 || decoded_size != GRAPES_ENCODED_CHUNK_HEADER_SIZE + ExternalChunk_header_size + gchunk->size) {
	    fprintf(stderr, "chunk %d probably corrupted!\n", gchunk->id);
		return -1;
	}

	if (gchunk->attributes) {
		free(gchunk->attributes);
		gchunk->attributes = NULL;
		gchunk->attributes_size = 0;
	}
	chunk_attributes_fill(gchunk);

	if(cb) {
		int cnt = 0;
		int i = 0;
#ifdef HTTPIO_MHD
		#ifdef THREADS
		//in case of threaded streamer it also has a topology mutex
		pthread_mutex_lock(&topology_mutex);
		#endif
		pthread_mutex_lock(&cb_mutex);
#endif
		res = add_chunk(gchunk);
		// if(res)
		for(i=0; i < multiply; i++) {	// @TODO: why this cycle?
			send_chunk();
		}
		send_offer();
		if (cnt++ % 10 == 0) {
			update_peers(NULL, NULL, 0);
		}
#ifdef HTTPIO_MHD
		pthread_mutex_unlock(&cb_mutex);
		#ifdef THREADS
  	pthread_mutex_unlock(&topology_mutex);
		#endif
#endif
	}

  return 0;
}

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

#ifdef HTTPIO_MHD
	#ifndef THREADS
	//in case we are using the non-threaded version of streamer
	//we need our own mutex
  pthread_mutex_init(&cb_mutex, NULL);
	#endif
  //this daemon will listen the network for incoming chunks from a streaming source
  //on the following path and port
  httpd = initChunkPuller(UL_DEFAULT_CHUNKBUFFER_PATH, UL_DEFAULT_CHUNKBUFFER_PORT);
	printf("MHD input httpd thread initialized! %d\n", res->dummy);
#elif defined HTTPIO_EVENT
	if(ulEventHttpServerSetup("127.0.0.1", UL_DEFAULT_CHUNKBUFFER_PORT, &enqueueBlock)) {
		return NULL;
	}
	else {
		printf("EVENT input httpd loop initialized! %d\n", res->dummy);
	}
#endif

  return res;
}

void input_close(struct input_desc *s)
{
  free(s);
#ifdef HTTPIO_MHD
  finalizeChunkPuller(httpd);
	#ifndef THREADS
  pthread_mutex_destroy(&cb_mutex);
	#endif
#elif defined HTTPIO_EVENT
	//TODO finalize the event http server
#endif
}

//this one is not used, just routinely called by the firging thread
int input_get(struct input_desc *s, struct chunk *c)
{
  c->data = NULL;
  return 0;
}
