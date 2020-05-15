#ifndef _STATISTICS_H
#define _STATISTICS_H

#include "structs.h"
#include "api_data.h"
#include "log_levels.h"

static void *statistics(void *);
static char char2hex(unsigned char);
void set_busy_status(int s, const char *build_id);
int start_statistics_thread(const char *);

extern int api_job_statistics(const char *);

extern void register_thread(const char *name);
extern void log_printf(unsigned int level, const char *message, ...);

extern char *alloc_sprintf(const char *message, ...);

extern char *xstrdup(const char *s);

extern void *xmalloc(size_t size);

#endif
