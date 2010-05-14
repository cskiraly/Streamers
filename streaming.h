#ifndef STREAMING_H
#define STREAMING_H

#include <stdbool.h>

void stream_init(int size, struct nodeID *myID);
int source_init(const char *fname, struct nodeID *myID, bool loop);
void received_chunk(struct nodeID *from, const uint8_t *buff, int len);
void send_chunk();
int generated_chunk(suseconds_t *delta);
struct chunkID_set *get_chunks_to_accept(struct peer *from, const struct chunkID_set *cset_off, int max_deliver, int trans_id);
void send_offer();
void send_accepted_chunks(struct peer *to, struct chunkID_set *cset_acc, int max_deliver, int trans_id);
void send_bmap(struct peer *to);

#endif	/* STREAMING_H */
