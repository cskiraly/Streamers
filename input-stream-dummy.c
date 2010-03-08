/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "input-stream.h"

static struct input_stream {
} fake_descriptor;

struct input_stream *input_stream_open(const char *fname, int *period, uint16_t flags)
{
  *period = 40000;
  return &fake_descriptor;
}

void input_stream_close(struct input_stream *dummy)
{
}

uint8_t *chunkise(struct input_stream *dummy, int id, int *size, uint64_t *ts)
{
  static char buff[80];

  sprintf(buff, "Chunk %d", id);
  *ts = 40 * id * 1000;
  *size = strlen(buff) + 1;

  return strdup(buff);
}
