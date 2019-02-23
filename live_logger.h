#ifndef _LIVE_LOGGER_H
#define _LIVE_LOGGER_H

#define LIVE_LOGGER_BUFFER_SIZE 20000

#include "log_levels.h"

static void *buffer_dump(void *);
static void *read_log(void *);
int start_live_logger(const char *, int);
void stop_live_logger();

extern int api_jobs_status(const char *);
extern int api_jobs_logs(const char *, const char *);

extern void register_thread(const char *name);
extern void unregister_thread(pthread_t tid);
extern void log_printf(unsigned int level, const char *message, ...);

#endif
