#ifndef INPUT_STREAM_H
#define INPUT_STREAM_H

struct input_stream;

struct input_stream *input_stream_open(const char *fname);
void input_stream_close(struct input_stream *dummy);
uint8_t *chunkise(struct input_stream *dummy, int id, int *size, uint64_t *ts);

#endif	/* INPUT_STREAM_H */
