/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "out-stream.h"
#include "dbg.h"
#include "payload.h"
#include "io_udp.h"

static int outfd = -1;
#define PLAYER_IP "127.0.0.1"
#define PLAYER_PORT 32500

int output_stream_init()
{
  int fd;

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0) {
    fprintf(stderr, "output socket: can't open\n");
    return -1;
  }

  return fd;
}

int out_stream_init(const char *config)
{
  return 1;
}

void chunk_write(int id, const uint8_t *data, int size)
{
  struct sockaddr_in si_other;

  if (outfd < 0) {
    outfd = output_stream_init();
    if (outfd < 0) {
      fprintf(stderr, "Error: can't open output socket\n");
      exit(1);
    }
  }

  bzero(&si_other, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons((PLAYER_PORT + ((const struct io_udp_header*)data)->portdiff));
  if (inet_aton(PLAYER_IP, &si_other.sin_addr) == 0) {
     fprintf(stderr, " output socket: inet_aton() failed\n");
     return;
  }

  sendto(outfd, data + sizeof (struct io_udp_header), size - sizeof (struct io_udp_header), 0, (struct sockaddr *) &si_other, sizeof(si_other));
}
