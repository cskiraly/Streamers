void stream_init(int size);
void received_chunk(const uint8_t *buff, int len);
void send_chunk(const struct nodeID **neighbours, int n);
void generated_chunk(void);
