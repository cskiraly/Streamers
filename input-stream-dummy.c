#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "input-stream.h"

static struct input_stream {
} fake_descriptor;

struct input_stream *input_stream_open(const char *fname)
{
  return &fake_descriptor;
}

void input_stream_close(struct input_stream *dummy)
{
}

uint8_t *chunkise(struct input_stream *dummy, int id, int *size, uint64_t *ts)
{
  static char buff[80];

  sprintf(buff, "Chunk %d", id);
  *ts = 40 * id;
  *size = strlen(buff) + 1;

  return strdup(buff);
}
