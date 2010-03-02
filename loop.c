/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <net_helper.h>
#include <topmanager.h>
#include <msg_types.h>

#include "streaming.h"
#include "loop.h"
#include "dbg.h"

#define BUFFSIZE 64 * 1024
static struct timeval period = {0, 500000};
static struct timeval tnext;

void tout_init(struct timeval *tv)
{
  struct timeval tnow;

  if (tnext.tv_sec == 0) {
    gettimeofday(&tnext, NULL);
  }
  gettimeofday(&tnow, NULL);
  if(timercmp(&tnow, &tnext, <)) {
    timersub(&tnext, &tnow, tv);
  } else {
    *tv = (struct timeval){0, 0};
  }
}

void loop(struct nodeID *s, int csize, int buff_size)
{
  int done = 0;
  static uint8_t buff[BUFFSIZE];
  int cnt = 0;
  
  period.tv_sec = csize / 1000000;
  period.tv_usec = csize % 1000000;
  
  topParseData(NULL, 0);
  stream_init(buff_size, s);
  while (!done) {
    int len, res;
    struct timeval tv;

    tout_init(&tv);
    res = wait4data(s, tv);
    if (res > 0) {
      struct nodeID *remote;

      len = recv_from_peer(s, &remote, buff, BUFFSIZE);
      switch (buff[0] /* Message Type */) {
        case MSG_TYPE_TOPOLOGY:
          topParseData(buff, len);
          break;
        case MSG_TYPE_CHUNK:
          received_chunk(buff, len);
          break;
        default:
          fprintf(stderr, "Unknown Message Type %x\n", buff[0]);
      }
      nodeid_free(remote);
    } else {
      const struct nodeID **neighbours;
      int n;
      struct timeval tmp;

      neighbours = topGetNeighbourhood(&n);
      send_chunk(neighbours, n);
      if (cnt++ % 10 == 0) {
        topParseData(NULL, 0);
      }
      timeradd(&tnext, &period, &tmp);
      tnext = tmp;
    }
  }
}

void source_loop(const char *fname, struct nodeID *s, int csize, int chunks)
{
  int done = 0;
  static uint8_t buff[BUFFSIZE];
  int cnt = 0;

  source_init(fname, s);
  while (!done) {
    int len, res;
    struct timeval tv;

    tout_init(&tv);
    res = wait4data(s, tv);
    if (res > 0) {
      struct nodeID *remote;

      len = recv_from_peer(s, &remote, buff, BUFFSIZE);
      switch (buff[0] /* Message Type */) {
        case MSG_TYPE_TOPOLOGY:
          fprintf(stderr, "Top Parse\n");
          topParseData(buff, len);
          break;
        case MSG_TYPE_CHUNK:
          fprintf(stderr, "Some dumb peer pushed a chunk to me!\n");
          break;
        default:
          fprintf(stderr, "Bad Message Type %x\n", buff[0]);
      }
      nodeid_free(remote);
    } else {
      const struct nodeID **neighbours;
      int i, n, res;
      struct timeval tmp, d;

      d.tv_sec = 0;
      res = generated_chunk(&d.tv_usec);
      if (res) {
        neighbours = topGetNeighbourhood(&n);
        for (i = 0; i < chunks; i++) {
dprintf(" T: %lld\n", tnext.tv_sec * 1000 + tnext.tv_usec / 1000);
          send_chunk(neighbours, n);
        }
        if (cnt++ % 10 == 0) {
          topParseData(NULL, 0);
        }
      }
      timeradd(&tnext, &d, &tmp);
      tnext = tmp;
    }
  }
}
