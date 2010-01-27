#ifndef STREAMING_H
#define STREAMING_H

void stream_init(int size, struct nodeID *myID);
int source_init(const char *fname, struct nodeID *myID);
void received_chunk(const uint8_t *buff, int len);
void send_chunk(const struct nodeID **neighbours, int n);
void generated_chunk(void);

#endif	/* STREAMING_H */
