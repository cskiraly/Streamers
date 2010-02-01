#ifndef STREAMING_H
#define STREAMING_H

void stream_init(int size, struct nodeID *myID);
int source_init(const char *fname, struct nodeID *myID);
void received_chunk(struct peerset *pset, struct nodeID *from, const uint8_t *buff, int len);
void send_chunk(const struct peerset *pset);
int generated_chunk(suseconds_t *delta);
struct chunkID_set *get_chunks_to_accept(struct peer *from, const struct chunkID_set *cset_off, int max_deliver);
void send_offer(const struct peerset *pset);
void send_accepted_chunks(struct peer *to, struct chunkID_set *cset_acc, int max_deliver);

#endif	/* STREAMING_H */
