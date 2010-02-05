#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "out-stream.h"
#include "dbg.h"

static int outfd = 1;

void chunk_write(int id, const uint8_t *data, int size)
{
#ifdef DEBUGOUT
#define buff_size 8 // HACK!
    fprintf(stderr, "\tOut Chunk[%d] - %d: %s\n", id, id % buff_size, data);
#else
    write(outfd, data, size);
#endif
}
