/*
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2010-2011 Luca Abeni
 *
 * This file is part of PeerStreamer.
 *
 * PeerStreamer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PeerStreamer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PeerStreamer.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <stdint.h>

#define MSG_TYPE_STREAMER_TOPOLOGY   0x22

int peers_init(void);
struct peerset *get_peers(void);
void update_peers(struct nodeID *from, const uint8_t *buff, int len);
struct peer *nodeid_to_peer(const struct nodeID* id, int reg);
int topoAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size);
int topologyInit(struct nodeID *myID, const char *config);
void topologyShutdown(void);

#endif	/* TOPOLOGY_H */
