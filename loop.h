/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef LOOP_H
#define LOOP_H

void loop(struct nodeID *s, int period, int buff_size);
void source_loop(const char *fname, struct nodeID *s, int csize, int chunks, bool loop, int buff_size);

#endif	/* LOOP_H */
