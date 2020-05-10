#include <stdio.h>
#include <stdlib.h>

void *xmalloc(size_t size) {
  if (size == 0) {
    size = 1;
  }
  void *res = malloc(size);
  if (res == NULL) {
    printf("FATAL: Can't allocate enough memory for operation, aborting.\n");
    exit(100);
  }

  return res;
}
