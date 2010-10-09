/*
 *  Copyright (c) 2009 Alessandro Russo
 *  Copyright (c) 2009 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef CHUNK_SIGNALING_H
#define CHUNK_SIGNALING_H

int sigParseData(const struct nodeID *from_id, uint8_t *buff, int buff_len);

#endif