#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>

static struct timeval tnow;

int dtprintf2(const char *format, ...)
{
  va_list ap;
  int ret;
  
  gettimeofday(&tnow, NULL);
  fprintf(stderr, "%ld.%03ld ", tnow.tv_sec, tnow.tv_usec/100);

  va_start (ap, format);
  ret = vfprintf(stderr, format, ap);
  va_end (ap);
  
  return ret;
}
