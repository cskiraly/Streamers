#ifndef IO_UDP_HEADER
#define IO_UDP_HEADER

struct io_udp_header {
  uint8_t portdiff;
} __attribute__((packed));

#endif