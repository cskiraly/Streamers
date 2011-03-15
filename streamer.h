/*
 *  Copyright (c) 2011 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef STREAMER_H
#define STREAMER_H

#include <net_helper.h>

const struct nodeID *get_my_addr(void);
int am_i_source();
int get_cb_size();
int get_chunks_per_sec();

#endif //STREAMER_H
