/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef CHUNKBUFFER_HELPERS_H
#define CHUNKBUFFER_HELPERS_H

#include <chunkbuffer.h>

inline struct chunk_buffer *cb_init(const char *config) {
  return chbInit(config);
}

inline int cb_add_chunk(struct chunk_buffer *cb, const struct chunk *c) {
  return chbAddChunk(cb, c);
}

inline struct chunk *cb_get_chunks(const struct chunk_buffer *cb, int *n) {
 return chbGetChunks(cb, n);
}

inline int cb_clear(struct chunk_buffer *cb) {
 return chbClear(cb);
}

inline const struct chunk *cb_get_chunk(const struct chunk_buffer *cb, int id) {
  return chbGetChunk(cb, id);
}

#endif	/* CHUNKBUFFER_HELPERS_H */
