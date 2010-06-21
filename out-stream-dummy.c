/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "out-stream.h"
#include "dbg.h"
#include "payload.h"

static int outfd = 1;

void chunk_write(int id, const uint8_t *data, int size)
{
  const int header_size = VIDEO_PAYLOAD_HEADER_SIZE;
  int width, height, frame_rate_n, frame_rate_d, frames;
  int i;
  uint8_t codec;

  payload_header_parse(data, &codec, &width, &height, &frame_rate_n, &frame_rate_d);
  if (codec != 1) {
    fprintf(stderr, "Error! Non video chunk: %x!!!\n", codec);
    return;
  }
  dprintf("Frame size: %dx%d -- Frame rate: %d / %d\n", width, height, frame_rate_n, frame_rate_d);
  frames = data[9];
  for (i = 0; i < frames; i++) {
    int frame_size;
    int32_t pts, dts;

    frame_header_parse(data, &frame_size, &pts, &dts);
    dprintf("Frame %d has size %d\n", i, frame_size);
  }
#ifdef DEBUGOUT
#define buff_size 8 // HACK!
  fprintf(stderr, "\tOut Chunk[%d] - %d: %s\n", id, id % buff_size, data + header_size + frames * FRAME_HEADER_SIZE);
#else
  write(outfd, data + header_size + frames * FRAME_HEADER_SIZE, size - header_size - frames * FRAME_HEADER_SIZE);
#endif
}
