/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef STREAMING_H
#define STREAMING_H

#include <stdbool.h>

void stream_init(int size, struct nodeID *myID);
int source_init(const char *fname, struct nodeID *myID, bool loop, int *fds, int fds_size);
void received_chunk(struct nodeID *from, const uint8_t *buff, int len);
void send_chunk();
struct chunk *generated_chunk(suseconds_t *delta);
int add_chunk(struct chunk *c);
struct chunkID_set *get_chunks_to_accept(struct peer *from, const struct chunkID_set *cset_off, int max_deliver, int trans_id);
void send_offer();
void send_accepted_chunks(struct nodeID *to, struct chunkID_set *cset_acc, int max_deliver, int trans_id);
void send_bmap(struct nodeID *to);

#endif	/* STREAMING_H */
