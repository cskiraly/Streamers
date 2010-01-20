#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <chunk.h>

#define SIZE 8

static int next_chunk;
static int outfd;
static int buff_size = SIZE;

static struct {
  void *data;
  int size;
} buff[SIZE];

void buffer_flush(int id)
{
  int i = id % buff_size;

  while(buff[i].data) {
    write(outfd, buff[i].data, buff[i].size);
    free(buff[i].data);
    buff[i].data = NULL;
    i = (i + 1) % buff_size;
    next_chunk++;
    if (i == id) {
      break;
    }
  }
}

void output_deliver(const struct chunk *c)
{
  if (c->id == next_chunk) {
    write(outfd, c->data, c->size);
    next_chunk++;
    buffer_flush(next_chunk);
  } else {
    buffer_flush(c->id);
    buff[c->id % buff_size].data = malloc(c->size);
    memcpy(buff[c->id % buff_size].data, c->data, c->size);
    buff[c->id % buff_size].size = c->size;
  }
}
