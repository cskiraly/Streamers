/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef OUTPUT_H
#define OUTPUT_H

struct chunk;

void output_init(int bufsize);
void output_deliver(const struct chunk *c);

#endif	/* OUTPUT_H */
