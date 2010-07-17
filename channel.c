/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <stdlib.h>
#include <string.h>

#include "channel.h"

static char * chname = NULL;

void channel_set_name(char *ch)
{
  free(chname);
  chname = strdup(ch);
}

const char *channel_get_name()
{
  return chname;
}
