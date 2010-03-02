#ifndef STREAMING_H
#define STREAMING_H

#include <stdbool.h>

void stream_init(int size, struct nodeID *myID);
int source_init(const char *fname, struct nodeID *myID, bool loop);
void received_chunk(const uint8_t *buff, int len);
void send_chunk(const struct nodeID **neighbours, int n);
int generated_chunk(suseconds_t *delta);

#endif	/* STREAMING_H */
