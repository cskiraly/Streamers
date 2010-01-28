void stream_init(int size, struct nodeID *myID);
int source_init(const char *fname, struct nodeID *myID);
void received_chunk(const uint8_t *buff, int len);
void send_chunk(const struct peerset *pset);
void generated_chunk(void);
