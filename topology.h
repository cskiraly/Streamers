/*
 *  Copyright (c) 2010 Csaba Kiraly
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdint.h>

int peers_init();
struct peerset *get_peers();
void update_peers(struct nodeID *from, const uint8_t *buff, int len);
struct peer *nodeid_to_peer(const struct nodeID* id, int reg);
