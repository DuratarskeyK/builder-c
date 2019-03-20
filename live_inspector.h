#ifndef _LIVE_INSPECTOR_H
#define _LIVE_INSPECTOR_H

#include "structs.h"

static void *live_inspector(void *);
int start_live_inspector(int, pid_t, const char *);
int stop_live_inspector();

extern int api_jobs_status(const char *);

extern void register_thread(const char *name);
extern void unregister_thread(pthread_t tid);
extern void log_printf(unsigned int level, const char *message, ...);

#endif
