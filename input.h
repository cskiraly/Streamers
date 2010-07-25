/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef INPUT_H
#define INPUT_H

#define INPUT_LOOP 0x0001

struct input_desc;

struct input_desc *input_open(const char *fname, uint16_t flags, int *fds, int fds_size);
void input_close(struct input_desc *s);

/*
 * c: chunk structure to be filled. If c->data = NULL after call, there is no new chunk
 * Returns: timeout requested till next call to the function, <0 in case of input error
 */
int input_get(struct input_desc *s, struct chunk *c);

#endif	/* INPUT_H */
