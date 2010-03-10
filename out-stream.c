#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "out-stream.h"
#include "dbg.h"

static int outfd = 1;

void chunk_write(int id, const uint8_t *data, int size)
{
  const int header_size = 1 + 2 + 2 + 1 + 2; // 1 Frame type + 2 width + 2 height + 1 number of frames + 2 frame len
#ifdef DEBUG
#define buff_size 8 // HACK!
  fprintf(stderr, "\tOut Chunk[%d] - %d: %s\n", id, id % buff_size, data + header_size);
#else
  write(outfd, data + header_size, size - header_size);
#endif
}
