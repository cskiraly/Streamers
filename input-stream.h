/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef INPUT_STREAM_H
#define INPUT_STREAM_H

struct input_stream;

struct input_stream *input_stream_open(const char *fname, int *period, uint16_t flags);
void input_stream_close(struct input_stream *dummy);
uint8_t *chunkise(struct input_stream *dummy, int id, int *size, uint64_t *ts);

#endif	/* INPUT_STREAM_H */
