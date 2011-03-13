/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <grapes_msg_types.h>
#include <net_helper.h>

#ifdef WIN32
#include <winsock2.h>
#endif

#include "net_helpers.h"
#include "loop.h"
#include "output.h"
#include "channel.h"
#include "topology.h"
#include "measures.h"
#include "streamer.h"

static struct nodeID *my_sock;

const char *peername = NULL;

static const char *my_iface = NULL;
static int port = 6666;
static int srv_port;
static const char *srv_ip = "";
static int period = 40;
static int chunks_per_second = 25;
#ifdef HTTPIO
//input-http.c needs this in order to accomplish the -m multiple send_chunk()
int multiply = 3;
#else
static int multiply = 3;
#endif
static int buff_size = 50;
static int outbuff_size = 50;
static const char *fname = "/dev/stdin";
static const char *output_config;
static bool loop_input = false;
static const char *net_helper_config = "";
static const char *topo_config = "";
unsigned char msgTypes[] = {MSG_TYPE_CHUNK,MSG_TYPE_SIGNALLING};
bool chunk_log = false;
static int randomize_start = 0;

extern int NEIGHBORHOOD_TARGET_SIZE;
extern uint64_t CB_SIZE_TIME;

extern struct timeval print_tdiff;
extern struct timeval tstartdiff;

static void print_usage(int argc, char *argv[])
{
  fprintf (stderr,
    "Usage:%s [options]\n"
    "\n"
    "Peer options\n"
    "\t[-p port]: port of the remote peer to connect at during bootstrap.\n"
    "\t           Usually it is the source peer port.\n"
    "\t[-i IP]: IP address of the remote peer to connect at during bootstrap.\n"
    "\t         Usually it is the source peer IP\n"
    "\t[-C name]: set the channel name to use on the repository.\n"
    "\t           All peers should use the same channel name.\n"
    "\n"
    "\t[-b size]: set the peer Chunk Buffer size.\n"
    "\t           This is also the chunk trading window size.\n"
    "\t[-o size]: set the Output Buffer size.\n"
    "\t[-c chunks]: set the number of chunks a peer can send per seconds.\n"
    "\t             it controls the upload capacity of peer as well.\n"
    "\t[-M peers]: neighbourhood target size.\n"
    "\t[-t config]: topology config.\n"
    "\t[-P port]: local UDP port to be used by the peer.\n"
    "\t[-I iface]: local netwok interface to be used by the peer.\n"
    "\t         Useful if the host has several interfaces/addresses.\n"
    "\t[-N name]: set the name of the peer.\n"
    "\t         This name will be used when publishing in the repository.\n"
    "\t[-n options]: pass configuration options to the net-helper\n"
    "\t[--chunk_log]: print a chunk level log on stderr\n"
    "\t[-F config]: configure the output module\n"
    "\n"
    "Special Source Peer options\n"
    "\t[-m chunks]: set the number of copies the source injects in the overlay.\n"
    "\t[-f filename]: name of the video stream file to transmit.\n"
    "\t[-l]: loop the video stream.\n"
    "\n"
    "Special options\n"
    "\t[--randomize_start ms]: random wait before starting in to ms millisecs.\n"
    "\n"
    "NOTE: by deafult the peer will dump the received video on STDOUT in raw format\n"
    "      it can be played by your favourite player simply using a pipe\n"
    "      e.g., | cvlc /dev/stdin\n"
    "\n"
    "Examples:\n"
    "\n"
    "Start a source peer on port 6600:\n"
    "\n"
    "%s -l -f foreman.mpg -P 6600\n"
    "\n"
    "Start a peer connecting to the previous source, and using videolan as player:\n"
    "\n"
    "%s -i <sourceIP> -p <sourcePort> | cvlc /dev/stdin\n"
    "=======================================================\n", argv[0], argv[0], argv[0]
    );
  }



static void cmdline_parse(int argc, char *argv[])
{
  int o;

  int option_index = 0;
  static struct option long_options[] = {
        {"chunk_log", no_argument, 0, 0},
        {"measure_start", required_argument, 0, 0},
        {"measure_every", required_argument, 0, 0},
        {"playout_limit", required_argument, 0, 0},
        {"randomize_start", required_argument, 0, 0},
	{0, 0, 0, 0}
  };

    while ((o = getopt_long (argc, argv, "b:o:c:p:i:P:I:f:F:m:lC:N:n:M:t:",long_options, &option_index)) != -1) { //use this function to manage long options
    switch(o) {
      case 0: //for long options
        if( strcmp( "chunk_log", long_options[option_index].name ) == 0 ) { chunk_log = true; }
        if( strcmp( "measure_start", long_options[option_index].name ) == 0 ) { tstartdiff.tv_sec = atoi(optarg); }
        if( strcmp( "measure_every", long_options[option_index].name ) == 0 ) { print_tdiff.tv_sec = atoi(optarg); }
        if( strcmp( "playout_limit", long_options[option_index].name ) == 0 ) { CB_SIZE_TIME = atoi(optarg); }
        if( strcmp( "randomize_start", long_options[option_index].name ) == 0 ) { randomize_start = atoi(optarg); }
        break;
      case 'b':
        buff_size = atoi(optarg);
        break;
      case 'o':
        outbuff_size = atoi(optarg);
        break;
        case 'c':
        chunks_per_second = atoi(optarg);
        break;
      case 'm':
        multiply = atoi(optarg);
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
      case 'f':
        fname = strdup(optarg);
        break;
      case 'F':
        output_config = strdup(optarg);
        break;
      case 'l':
        loop_input = true;
        break;
      case 'C':
        channel_set_name(optarg);
        break;
      case 'n':
        net_helper_config = strdup(optarg);
        break;
      case 'N':
        peername = strdup(optarg);
        break;
      case 'M':
        NEIGHBORHOOD_TARGET_SIZE = atoi(optarg);
        break;
      case 't':
        topo_config = strdup(optarg);
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);
        print_usage(argc, argv);

        exit(-1);
    }
  }

  if (!channel_get_name()) {
    channel_set_name("generic");
  }

  if (argc <= 1) {
    print_usage(argc, argv);
    fprintf(stderr, "Trying to start a source with default parameters, reading from %s\n", fname);
  }
}

const struct nodeID *get_my_addr(void)
{
  return my_sock;
}

static void init_rand(const char * s)
{
  long i, l, x;
  struct timeval t;

  gettimeofday(&t, NULL);

  x = 1;
  l = strlen(s);
  for (i = 0; i < l; i++) {
    x *= s[i];
  }

  srand(t.tv_usec * t.tv_sec * x);
}

static struct nodeID *init(void)
{
  int i;
  struct nodeID *myID;
  char *my_addr;

  if (my_iface) {
    my_addr = iface_addr(my_iface);
  } else {
    my_addr = default_ip_addr();
  }

  if (my_addr == NULL) {
    fprintf(stderr, "Cannot find network interface %s\n", my_iface);

    return NULL;
  }
  for (i=0;i<2;i++)
	  bind_msg_type(msgTypes[i]);
  myID = net_helper_init(my_addr, port, net_helper_config);
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);
    free(my_addr);

    return NULL;
  }
  free(my_addr);
  fprintf(stderr, "My network ID is: %s\n", node_addr(myID));

  init_rand(node_addr(myID));

  topologyInit(myID, topo_config);

  return myID;
}

void leave(int sig) {
  end_measures();
  exit(sig);
}

static void random_wait(int max) {
    struct timespec t;
    uint64_t ms;

    ms = (rand()/(RAND_MAX + 1.0)) * max;
    t.tv_sec = ms / 1000000;
    t.tv_nsec = (ms % 1000000) * 1000;
    nanosleep(&t, NULL);
}

int main(int argc, char *argv[])
{

  (void) signal(SIGTERM,leave);

  cmdline_parse(argc, argv);

  my_sock = init();
  if (my_sock == NULL) {
    fprintf(stderr, "Cannot initialize streamer, exiting!\n");
    return -1;
  }
  if (srv_port != 0) {
    struct nodeID *srv;

    //random wait a bit before starting
    if (randomize_start) random_wait(randomize_start);

    output_init(outbuff_size, output_config);

    srv = create_node(srv_ip, srv_port);
    if (srv == NULL) {
      fprintf(stderr, "Cannot resolve remote address %s:%d\n", srv_ip, srv_port);

      return -1;
    }
    topoAddNeighbour(srv, NULL, 0);

    loop(my_sock, 1000000 / chunks_per_second, buff_size);
  } else {
    source_loop(fname, my_sock, period * 1000, multiply, loop_input);
  }
  return 0;
}

int am_i_source()
{
  return (srv_port == 0);
}

int get_cb_size()
{
  return buff_size;
}
