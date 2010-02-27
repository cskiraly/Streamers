#ifndef CHUNKLOCK_H
#define CHUNKLOCK_H

#include <peer.h>

void chunk_lock(int chunkid,struct peer *from);
void chunk_unlock(int chunkid);
int chunk_islocked(int chunkid);

#endif //CHUNKLOCK_H
