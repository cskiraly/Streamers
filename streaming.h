#ifndef STREAMING_H
#define STREAMING_H

#include <stdbool.h>

void stream_init(int size, struct nodeID *myID);
int source_init(const char *fname, struct nodeID *myID, bool loop);
void received_chunk(struct peerset *pset, struct nodeID *from, const uint8_t *buff, int len);
void send_chunk(const struct peerset *pset);
int generated_chunk(suseconds_t *delta);

#endif	/* STREAMING_H */
