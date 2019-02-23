#ifndef _LOGGER_H
#define _LOGGER_H
#include <pthread.h>
#include "log_levels.h"

const char *log_levels_str[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "SILENT"};

typedef struct thread_config {
	pthread_t tid;
	const char *name;
	struct thread_config *next;
} thread_config_t;

int init_logger(const char *level);
void register_thread(const char *name);
void unregister_thread(pthread_t tid);
void log_printf(unsigned int level, const char *message, ...);

#endif
