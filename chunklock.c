/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "chunklock.h"

#include "net_helper.h"

static struct timeval toutdiff = {0, 300000};



struct lock {
  int chunkid;
  struct nodeID *peer;
  struct timeval timestamp;
};

struct lock *locks;
static size_t lsize, lcount=0;
#define LSIZE_INCREMENT 10

void locks_init()
{
  if (!locks) {
    lsize = LSIZE_INCREMENT;
    locks = malloc(sizeof(struct lock) * lsize);
    lcount = 0;
  }

  if (lcount == lsize) {
    lsize += LSIZE_INCREMENT;
    locks = realloc(locks , sizeof(struct lock) * lsize);
  }

  if (!locks) {
    fprintf(stderr, "Error allocating memory for locks!\n");
    exit(EXIT_FAILURE);
  }
}

int chunk_lock_timed_out(struct lock *l)
{
  struct timeval tnow,tout;
  gettimeofday(&tnow, NULL);
  timeradd(&l->timestamp, &toutdiff, &tout);

  return timercmp(&tnow, &tout, >);
}

void chunk_lock_remove(struct lock *l){
  nodeID_free(l->peer);
  memmove(l, l+1, sizeof(struct lock) * (locks+lcount-l-1));
  lcount--;
}

void chunk_locks_cleanup(){
  int i;

  for (i=lcount-1; i>=0; i--) {
    if (chunk_lock_timed_out(locks+i)) {
      chunk_lock_remove(locks+i);
    }
  }
}

void chunk_lock(int chunkid,struct peer *from){
  locks_init();
  locks[lcount].chunkid = chunkid;
  locks[lcount].peer = nodeid_dup(from->id);
  gettimeofday(&locks[lcount].timestamp,NULL);
  lcount++;
}

void chunk_unlock(int chunkid){
  int i;

  for (i=0; i<lcount; i++) {
    if (locks[i].chunkid == chunkid) {
      chunk_lock_remove(locks+i);
      break;
    }
  }
}

int chunk_islocked(int chunkid){
  int i;

  chunk_locks_cleanup();

  for (i=0; i<lcount; i++) {
    if (locks[i].chunkid == chunkid) {
      return 1;
    }
  }
  return 0;
}
