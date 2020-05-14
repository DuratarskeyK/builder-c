#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "alloc_sprintf.h"

char *alloc_sprintf(const char *message, ...) {
  char *res = NULL;
  va_list ap;
  int size = 0;

	va_start(ap, message);
  size = vsnprintf(res, 0, message, ap);
	va_end(ap);

  size++;

  res = xmalloc(size);

	va_start(ap, message);
  size = vsnprintf(res, size, message, ap);
  if (size < 0) {
    free(res);
    return NULL;
  }
  va_end(ap);

  return res;
}
