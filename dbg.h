/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef DBG_H
#define DBG_H

#ifdef DEBUG

#include <stdio.h>

int dtprintf2(const char *format, ...);

#include <stdio.h>
#define dprintf(...) fprintf(stderr,__VA_ARGS__)
#define dtprintf(...) dtprintf2(__VA_ARGS__)
#else
#define dprintf(...)
#define dtprintf(...)
#endif

#endif	/* DBG_H */
