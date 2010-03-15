#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "out-stream.h"
#include "dbg.h"

static int outfd = 1;

void chunk_write(int id, const uint8_t *data, int size)
{
  const int header_size = 1 + 2 + 2 + 1 + 2; // 1 Frame type + 2 width + 2 height + 1 number of frames + 2 frame len
  int width, height, frames;
  int i;

  if (data[0] != 1) {
    fprintf(stderr, "Error! Non video chunk!!!\n");
    return;
  }
  width = data[1] << 8 | data[2];
  height = data[3] << 8 | data[4];
  dprintf("Frame size: %dx%d\n", width, height);
  frames = data[5];
  for (i = 0; i < frames; i++) {
    dprintf("Frame %d has size %d\n", i, data[6 + 2 * i] << 8 | data[7 + 2 * i]);
  }
#ifdef DEBUG
#define buff_size 8 // HACK!
  fprintf(stderr, "\tOut Chunk[%d] - %d: %s\n", id, id % buff_size, data + header_size);
#else
  write(outfd, data + header_size, size - header_size);
#endif
}
