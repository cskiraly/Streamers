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
#include "io_udp.h"

#define UDP_PORT_MIN 32000
#define UDP_PORT_MAX 32003
#define UDP_PORTS_NUM_MAX 10

struct input_desc {
  int fds[UDP_PORTS_NUM_MAX + 1];
  int id;
  int interframe;
  uint64_t start_time;
};

int listen_udp(int port)
{
  struct sockaddr_in servaddr;
  int r;
  int fd;

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0) {
    return -1;
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);
  r = bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  if (r < 0) {
    close(fd);
    return -1;
  }

  fprintf(stderr,"\topened fd:%d for port:%d\n", fd, port);
  return fd;
}

struct input_desc *input_open(const char *fname, uint16_t flags, int *fds, int fds_size)
{
  struct input_desc *res;
  struct timeval tv;
  int i;

  if (! fds || fds_size <= UDP_PORT_MAX - UDP_PORT_MIN + 2) {
    fprintf(stderr, "UDP input module needs more then %d file descriptors\n", fds_size-1);
    return NULL;
  }

  res = malloc(sizeof(struct input_desc));
  if (res == NULL) {
    return NULL;
  }

  for (i = 0; UDP_PORT_MIN + i < UDP_PORT_MAX + 1; i++) {
    res->fds[i] = fds[i] = listen_udp(UDP_PORT_MIN + i);
    if (fds[i] < 0) {
      for (; i>=0 ; i--) {
        close(fds[i]);
      }
      res->fds[0] = fds[0] = -1;
      free(res);
      return NULL;
    }
  }
  res->fds[i] = fds[i] = -1;

  gettimeofday(&tv, NULL);
  res->start_time = tv.tv_usec + tv.tv_sec * 1000000ULL;
  //res->id = (res->start_time / res->interframe) % INT_MAX; //FIXME
  res->id = 1;

  return res;
}

void input_close(struct input_desc *s)
{
  int i;

  for (i = 0; UDP_PORT_MIN + i < UDP_PORT_MAX + 1; i++) {
    close(s->fds[i]);
  }
  free(s);
}

#define UDP_BUF_SIZE 65536
int input_get_udp(struct input_desc *s, struct chunk *c, int fd_index)
{
  uint8_t buf[UDP_BUF_SIZE];
  size_t buflen = UDP_BUF_SIZE;
  ssize_t msglen;
  struct timeval now;

  fprintf(stderr,"\treading on fd:%d (index:%d)\n", s->fds[fd_index], fd_index);
  msglen = recv(s->fds[fd_index], buf, buflen, MSG_DONTWAIT);
  if (msglen < 0) {
    c->data = NULL;
    return 0;
  }
  if (msglen == 0) {
    c->data = NULL;
    return 0;
  }
  fprintf(stderr,"\treceived %d bytes\n",msglen);

  c->data = malloc(sizeof(struct io_udp_header) + msglen);
  ((struct io_udp_header*)c->data)->portdiff = fd_index;
  memcpy(c->data + sizeof(struct io_udp_header), buf, msglen);
  c->size = sizeof(struct io_udp_header) + msglen;

  c->id = s->id++;
  c->attributes_size = 0;
  c->attributes = NULL;
  gettimeofday(&now, NULL);
  c->timestamp = now.tv_sec * 1000000ULL + now.tv_usec;

  return 1;
}

int input_get(struct input_desc *s, struct chunk *c)
{
  int i;

  fprintf(stderr,"input_get called\n");

  for (i = 0; UDP_PORT_MIN + i < UDP_PORT_MAX + 1; i++) {
    if (input_get_udp(s, c, i)) {
      return 999999;
    }
  }
  return 999999;
}
