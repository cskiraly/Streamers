/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <chunk.h>

#include "input.h"
#include "input-stream.h"
#include "dbg.h"

#define UDP_PORT 32000

struct input_desc {
  int fd;
  int id;
  int interframe;
  uint64_t start_time;
};

struct input_desc *input_open(const char *fname, uint16_t flags, int *fds, int fds_size)
{
  struct input_desc *res;
  struct timeval tv;
  struct sockaddr_in servaddr;
  int r;

  if (! fds || fds_size <= 2) {
    fprintf(stderr, "UDP input module needs more then %d file descriptors\n", fds_size-1);
    return NULL;
  }

  res = malloc(sizeof(struct input_desc));
  if (res == NULL) {
    return NULL;
  }

  res->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (res->fd < 0) {
    free(res);
    return NULL;
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(UDP_PORT);
  r = bind(res->fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  if (r < 0) {
    close(res->fd);
    free(res);
    return NULL;
  }

  gettimeofday(&tv, NULL);
  res->start_time = tv.tv_usec + tv.tv_sec * 1000000ULL;
  //res->id = (res->start_time / res->interframe) % INT_MAX; //FIXME
  res->id = 1;

  *fds = res->fd;
  *(fds + 1) = -1;

  return res;
}

void input_close(struct input_desc *s)
{
  close(s->fd);
  free(s);
}

int input_get_fd(struct input_desc *s)
{
  return s->fd;
}

#define UDP_BUF_SIZE 65536
int input_get(struct input_desc *s, struct chunk *c)
{
  uint8_t buf[UDP_BUF_SIZE];
  size_t buflen = UDP_BUF_SIZE;
  ssize_t msglen;
  struct timeval now;

  fprintf(stderr,"input_get called\n");
  msglen = recv(s->fd, buf, buflen, MSG_DONTWAIT);
  if (msglen < 0) {
    c->data = NULL;
    return 999999;
  }
  if (msglen == 0) {
    c->data = NULL;
    return 999999;
  }
  fprintf(stderr,"\treceived %d bytes\n",msglen);

  c->data = malloc(msglen);
  memcpy(c->data, buf, msglen);
  c->size = msglen;

  c->id = s->id++;
  c->attributes_size = 0;
  c->attributes = NULL;
  gettimeofday(&now, NULL);
  c->timestamp = now.tv_sec * 1000000ULL + now.tv_usec;

  return 999999;
}
