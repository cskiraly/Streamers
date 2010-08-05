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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "out-stream.h"
#include "dbg.h"
#include "payload.h"
#include "io_udp.h"

static int outfd = -1;
static char ip[16];
static int base_port;
static int ports;

static int config_parse(const char *config)
{
  int res, i;

  res = sscanf(config, "udp://%15[0-9.]:%d%n", ip, &base_port, &i);
  if (res < 2) {
    fprintf(stderr,"error parsing output config: %s\n", config);
    return -1;
  }
  if (*(config + i) == 0) {
    ports = INT_MAX;
  } else {
    int max_port;
    res = sscanf(config + i, "-%d", &max_port);
    if (res == 1) {
      ports = max_port - base_port + 1;
      if (ports <=0) {
        fprintf(stderr,"error parsing output config: negative port range %s\n", config + i);
        return -2;
      }
    } else {
      fprintf(stderr,"error parsing output config: bad port range %s\n", config + i);
      return -3;
    }
  }

  return 1;
}


int out_stream_init(const char *config)
{
  int res;

  if (!config) {
    fprintf(stderr, "udp output not configured\n");
    return -1;
  }

  outfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (outfd < 0) {
    fprintf(stderr, "output socket: can't open\n");
    return -2;
  }

  res = config_parse(config);
  if (res < 0) {
    close(outfd);
    return -3;
  }

  return outfd;
}

void packet_write(const uint8_t *data, int size)
{
  struct sockaddr_in si_other;

  bzero(&si_other, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons((base_port + ((const struct io_udp_header*)data)->portdiff));
  if (inet_aton(ip, &si_other.sin_addr) == 0) {
     fprintf(stderr, " output socket: inet_aton() failed\n");
     return;
  }

  sendto(outfd, data + sizeof (struct io_udp_header), size - sizeof (struct io_udp_header), 0, (struct sockaddr *) &si_other, sizeof(si_other));
}

void chunk_write(int id, const uint8_t *data, int size)
{
  int i = 0;

  while (i < size) {
    int psize = ((const struct io_udp_header*)(data + i))->size;
    packet_write(data + i, sizeof(struct io_udp_header)  + psize);
    i += sizeof(struct io_udp_header) + psize;
  }
}
