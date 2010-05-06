#include <stdlib.h>

#include "channel.h"

static const char * chname = NULL;

void channel_set_name(char *ch)
{
  free(chname);
  chname = strdup(ch);
}

const char *channel_get_name()
{
  return chname;
}
