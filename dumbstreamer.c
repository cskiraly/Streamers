/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <net_helper.h>
#include <topmanager.h>

#include "net_helpers.h"
#include "loop.h"

static const char *my_iface = "eth0";
static int port = 6666;
static int srv_port;
static const char *srv_ip = "";
static int period = 500;
static int chunks_per_second = 4;
static int buff_size = 8;
static const char *fname = "input.mpg";

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "b:c:t:p:i:P:I:")) != -1) {
    switch(o) {
      case 'b':
        buff_size = atoi(optarg);
        break;
      case 'c':
        chunks_per_second = atoi(optarg);
        break;
      case 't':
        period = atoi(optarg);
        break;
      case 'p':
        srv_port = atoi(optarg);
        break;
      case 'i':
        srv_ip = strdup(optarg);
        break;
      case 'P':
        port =  atoi(optarg);
        break;
      case 'I':
        my_iface = strdup(optarg);
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);

        exit(-1);
    }
  }
}

static struct nodeID *init(void)
{
  struct nodeID *myID;
  char *my_addr = iface_addr(my_iface);

  myID = net_helper_init(my_addr, port);
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);
    free(my_addr);

    return NULL;
  }
  free(my_addr);
  topInit(myID);

  return myID;
}


int main(int argc, char *argv[])
{
  struct nodeID *my_sock;

  cmdline_parse(argc, argv);

  my_sock = init();
  if (my_sock == NULL) {
    return -1;
  }
  if (srv_port != 0) {
    struct nodeID *srv;

    srv = create_node(srv_ip, srv_port);
    if (srv == NULL) {
      fprintf(stderr, "Cannot resolve remote address %s:%d\n", srv_ip, srv_port);

      return -1;
    }
    topAddNeighbour(srv);

    loop(my_sock, 1000000 / chunks_per_second, buff_size);
  }

  source_loop(fname, my_sock, period * 1000, chunks_per_second * period / 1000);

  return 0;
}
