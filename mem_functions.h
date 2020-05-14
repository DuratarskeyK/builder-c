#ifndef _MEM_FUNCTIONS_H
#define _MEM_FUNCTIONS_H

#include <sys/types.h>

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);

#endif