#ifndef _SYSTEM_WITH_OUTPUT_H
#define _SYSTEM_WITH_OUTPUT_H

int system_with_output(const char *cmd, char **output);

extern void *xmalloc(size_t size);
extern void *xrealloc(void *ptr, size_t size);

#endif
