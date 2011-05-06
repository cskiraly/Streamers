/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <chunk.h>
#include <trade_msg_la.h>

#include "output.h"
#include "measures.h"
#include "dbg.h"

#define BUFSIZE 65536*8
static int fd = -1;

void output_init(int bufsize, const char *fname)
{
  if (!fname){
    fd = STDOUT_FILENO;
  } else {
    char *c;
    int port;
    char ip[32];

    c = strchr(fname,',');
    if (c) {
      *(c++) = 0;
    }

    if (sscanf(fname,"tcp://%[0-9.]:%d", ip, &port) == 2) {

      fd = socket(AF_INET, SOCK_STREAM, 0);
      if (fd < 0) {
        fprintf(stderr,"Error creating socket\n");
      } else {
        struct sockaddr_in servaddr;

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        if (inet_aton(ip, &servaddr.sin_addr) < 0) {
          fprintf(stderr,"Error converting IP address: %s\n", ip);
          return;
        }
        if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
          fprintf(stderr,"Error connecting to %s:%d\n", ip, port);
        }
      }
    } else {
      fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC | O_NONBLOCK, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    }
  }
}

void output_deliver(const struct chunk *c)
{
  static char sendbuf[BUFSIZE];
  static int pos = 0;
  int ret;
  uint32_t size;

  size = encodeChunk(c, sendbuf + pos + sizeof(size), BUFSIZE);
  if (size <= 0) {
    fprintf(stderr,"Error encoding chunk\n");
  } else {
    *((uint32_t*)(sendbuf + pos)) = htonl(size);
    pos += sizeof(size) + size;
  }

  ret = write(fd, sendbuf, pos);
  if (ret <= 0) {
    perror("Error writing to output");
  } else {
    pos -= ret;
    memmove(sendbuf, sendbuf + ret, pos);
  }
}
