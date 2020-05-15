#ifndef _MEM_FUNCTIONS_H
#define _MEM_FUNCTIONS_H

#include <sys/types.h>

static const char no_mem_msg[] = "FATAL: Can't allocate enough memory for operation, aborting.\n";

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);

#endif
