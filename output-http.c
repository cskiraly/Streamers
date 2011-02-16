	/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <chunk.h>
#include <net_helper.h>

#include "http_default_urls.h"
#include "external_chunk_transcoding.h"

#include "output.h"
#include "dbg.h"

#define PLAYER_IP "127.0.0.1"

struct nodeID *streamer;
static char url[256];
static int base_port = 0;

void output_init1(int bufsize, const char *config)
{
  if(!config) {
     fprintf(stderr, "Error: no http output module configuration issued. Exiting\n");
     exit(1);
  }
  if(sscanf(config, "%d", &base_port) < 1) {
     fprintf(stderr, "Error: can't parse http output module configuration string %s. Exiting\n", config);
     exit(1);
  }
	//we use the -F portnum parameter as the http port number
	sprintf(url, "http://%s:%d%s", PLAYER_IP, base_port, UL_DEFAULT_EXTERNALPLAYER_PATH);
	initChunkPusher();
}

void output_deliver1(const struct chunk *c)
{
  int ret = -1;

	//deliver the chunk to the external http player listening on http port
	//which has been setup via the -F option of the streamer commandline
	//If port was set > 60000 the the http
	//deliver is disabled, to allow mixed testing scenarios
	if(base_port < 60000) {
  	ret = sendViaCurl(*c, GRAPES_ENCODED_CHUNK_HEADER_SIZE + c->size + c->attributes_size, url);
  	dprintf("Chunk %d delivered to %s\n", c->id, url);
	}
}
