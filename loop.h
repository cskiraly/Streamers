#ifndef LOOP_H
#define LOOP_H

void loop(struct nodeID *s, int period, int buff_size);
void source_loop(const char *fname, struct nodeID *s, int csize, int chunks);

#endif	/* LOOP_H */
