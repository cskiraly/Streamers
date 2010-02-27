/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include "chunklock.h"

static struct chunkID_set *lock_set;

void chunk_lock(int chunkid,struct peer *from){
  if (!lock_set) lock_set = chunkID_set_init(16);

  chunkID_set_add_chunk(lock_set, chunkid);
}

void chunk_unlock(int chunkid){
  if (!lock_set) return;
  chunkID_set_remove_chunk(lock_set, chunkid);
}

int chunk_islocked(int chunkid){
  int r;

  if (!lock_set) return 0;
  r = chunkID_set_check(lock_set, chunkid);
  return (r >= 0);
}
