#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "out-stream.h"
#include "dbg.h"

static int outfd = 1;

#define FRAME_HEADER_SIZE (3 + 4 + 1)

void chunk_write(int id, const uint8_t *data, int size)
{
  const int header_size = 1 + 2 + 2 + 2 + 2 + 1; // 1 Frame type + 2 width + 2 height + 2 frame rate num + 2 frame rate den + 1 number of frames
  int width, height, frame_rate_n, frame_rate_d, frames;
  int i;

  if (data[0] != 1) {
    fprintf(stderr, "Error! Non video chunk: %x!!!\n", data[0]);
    return;
  }
  width = data[1] << 8 | data[2];
  height = data[3] << 8 | data[4];
  frame_rate_n = data[5] << 8 | data[6];
  frame_rate_d = data[7] << 8 | data[8];
  dprintf("Frame size: %dx%d -- Frame rate: %d / %d\n", width, height, frame_rate_n, frame_rate_d);
  frames = data[9];
  for (i = 0; i < frames; i++) {
    int frame_size;

    frame_size = data[header_size + FRAME_HEADER_SIZE * i];
    frame_size = frame_size << 8;
    frame_size |= data[header_size + FRAME_HEADER_SIZE * i + 1];
    frame_size = frame_size << 8;
    frame_size |= data[header_size + FRAME_HEADER_SIZE * i + 2];
    dprintf("Frame %d has size %d\n", i, frame_size);
  }
#ifdef DEBUGOUT
#define buff_size 8 // HACK!
  fprintf(stderr, "\tOut Chunk[%d] - %d: %s\n", id, id % buff_size, data + header_size + frames * FRAME_HEADER_SIZE);
#else
  write(outfd, data + header_size + frames * FRAME_HEADER_SIZE, size - header_size - frames * FRAME_HEADER_SIZE);
#endif
}
