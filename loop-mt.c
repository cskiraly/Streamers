/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <net_helper.h>
#include <grapes_msg_types.h>
#include <peerset.h>
#include <peer.h>

#include "dbg.h"
#include "chunk_signaling.h"
#include "streaming.h"
#include "topology.h"
#include "loop.h"

#define BUFFSIZE 512 * 1024
#define FDSSIZE 16
static int chunks_per_period = 1;
static int period = 500000;
static int done;
pthread_mutex_t cb_mutex;
pthread_mutex_t topology_mutex;
static struct nodeID *s;

static void *chunk_forging(void *dummy)
{
  suseconds_t d;
  struct chunk *c;

  while(!done) {
    c = generated_chunk(&d);
    if (c) {
      pthread_mutex_lock(&cb_mutex);
      add_chunk(c);
      pthread_mutex_unlock(&cb_mutex);
    }
    usleep(d);
  }

  return NULL;
}

static void *source_receive(void *dummy)
{
  while (!done) {
    int len;
    struct nodeID *remote;
  static uint8_t buff[BUFFSIZE];

    len = recv_from_peer(s, &remote, buff, BUFFSIZE);
    if (len < 0) {
      fprintf(stderr,"Error receiving message. Maybe larger than %d bytes\n", BUFFSIZE);
      nodeid_free(remote);
      continue;
    }
    switch (buff[0] /* Message Type */) {
      case MSG_TYPE_TOPOLOGY:
        pthread_mutex_lock(&topology_mutex);
        update_peers(remote, buff, len);
        pthread_mutex_unlock(&topology_mutex);
        break;
      case MSG_TYPE_CHUNK:
        fprintf(stderr, "Some dumb peer pushed a chunk to me! peer:%s\n",node_addr(remote));
        break;
      case MSG_TYPE_SIGNALLING:
        pthread_mutex_lock(&topology_mutex);
        sigParseData(remote, buff, len);
        pthread_mutex_unlock(&topology_mutex);
        break;
      default:
        fprintf(stderr, "Unknown Message Type %x\n", buff[0]);
    }
    nodeid_free(remote);
  }

  return NULL;
}

static void *receive(void *dummy)
{
  while (!done) {
    int len;
    struct nodeID *remote;
  static uint8_t buff[BUFFSIZE];

    len = recv_from_peer(s, &remote, buff, BUFFSIZE);
    if (len < 0) {
      fprintf(stderr,"Error receiving message. Maybe larger than %d bytes\n", BUFFSIZE);
      nodeid_free(remote);
      continue;
    }
    dprintf("Received message (%c) from %s\n", buff[0], node_addr(remote));
    switch (buff[0] /* Message Type */) {
      case MSG_TYPE_TOPOLOGY:
        pthread_mutex_lock(&topology_mutex);
        update_peers(remote, buff, len);
        pthread_mutex_unlock(&topology_mutex);
        break;
      case MSG_TYPE_CHUNK:
        dprintf("Chunk message received:\n");
        pthread_mutex_lock(&cb_mutex);
        received_chunk(remote, buff, len);
        pthread_mutex_unlock(&cb_mutex);
        break;
      case MSG_TYPE_SIGNALLING:
        pthread_mutex_lock(&topology_mutex);
        sigParseData(remote, buff, len);
        pthread_mutex_unlock(&topology_mutex);
        break;
      default:
        fprintf(stderr, "Unknown Message Type %x\n", buff[0]);
    }
    nodeid_free(remote);
  }

  return NULL;
}

static void *topology_sending(void *dummy)
{
  int gossiping_period = period * 10;

  pthread_mutex_lock(&topology_mutex);
  update_peers(NULL, NULL, 0);
  pthread_mutex_unlock(&topology_mutex);
  while(!done) {
    pthread_mutex_lock(&topology_mutex);
    update_peers(NULL, NULL, 0);
    pthread_mutex_unlock(&topology_mutex);
    usleep(gossiping_period);
  }

  return NULL;
}

static void *chunk_sending(void *dummy)
{
  int chunk_period = period / chunks_per_period;

  while(!done) {
    pthread_mutex_lock(&topology_mutex);
    pthread_mutex_lock(&cb_mutex);
    send_chunk();
    pthread_mutex_unlock(&cb_mutex);
    pthread_mutex_unlock(&topology_mutex);
    usleep(chunk_period);
  }

  return NULL;
}

void loop(struct nodeID *s1, int csize, int buff_size)
{
  pthread_t receive_thread, gossiping_thread, distributing_thread;
  
  period = csize;
  s = s1;
 
  peers_init();
  stream_init(buff_size, s);
  pthread_mutex_init(&cb_mutex, NULL);
  pthread_mutex_init(&topology_mutex, NULL);
  pthread_create(&receive_thread, NULL, receive, NULL); 
  pthread_create(&gossiping_thread, NULL, topology_sending, NULL); 
  pthread_create(&distributing_thread, NULL, chunk_sending, NULL); 

  pthread_join(receive_thread, NULL);
  pthread_join(gossiping_thread, NULL);
  pthread_join(distributing_thread, NULL);
}

void source_loop(const char *fname, struct nodeID *s1, int csize, int chunks, bool loop)
{
  pthread_t generate_thread, receive_thread, gossiping_thread, distributing_thread;
  
  period = csize;
  chunks_per_period = chunks;
  s = s1;
 
  int fds[FDSSIZE];
  fds[0] = -1;

//  sigInit(s);
  peers_init();
  if (source_init(fname, s, loop, fds, FDSSIZE) < 0) {
    fprintf(stderr,"Cannot initialize source, exiting");
    return;
  }
  pthread_mutex_init(&cb_mutex, NULL);
  pthread_mutex_init(&topology_mutex, NULL);
  pthread_create(&receive_thread, NULL, source_receive, NULL); 
  pthread_create(&gossiping_thread, NULL, topology_sending, NULL); 
#ifndef HTTPIO
  pthread_create(&distributing_thread, NULL, chunk_sending, NULL); 
  pthread_create(&generate_thread, NULL, chunk_forging, NULL); 
#endif

#ifndef HTTPIO
  pthread_join(generate_thread, NULL);
  pthread_join(distributing_thread, NULL);
#endif
  pthread_join(receive_thread, NULL);
  pthread_join(gossiping_thread, NULL);
}
