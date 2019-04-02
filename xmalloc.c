#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void *xmalloc(size_t size) {
  errno = 0;
  if (size == 0) {
    size = 1;
  }
  void *res = malloc(size);
  if (res == NULL && errno == ENOMEM) {
    printf("FATAL: Can't allocate enough memory for opeartion, aborting.\n");
    exit(1);
  }

  return res;
}