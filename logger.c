#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include "logger.h"

static unsigned int log_level;
static pthread_t last_thread;
static int log_ended_with_new_line;
static thread_config_t *thread_config;
static pthread_mutex_t logger_access;

int init_logger(const char *level) {
	log_level = LOG_SILENT + 1;
	for (unsigned int i = 0; i <= LOG_SILENT; i++) {
		if (!strcmp(level, log_levels_str[i])) {
			log_level = i;
			break;
		}
	}
	if (log_level == LOG_SILENT + 1) {
		fprintf(stderr, "LOGGER: Log level %s doesn't correspond to any known log level.\n", level);
		return -1;
	}

	pthread_mutex_init(&logger_access, NULL);
	thread_config = NULL;
	log_ended_with_new_line = 1;
	return 0;
}

static thread_config_t *new_thread_config(pthread_t tid, const char *name) {
	thread_config_t *result = (thread_config_t *)xmalloc(sizeof(thread_config_t));
	result->tid = tid;
	result->name = name;
	result->next = NULL;
	return result;
}

static thread_config_t *find_thread(pthread_t tid) {
	if (thread_config == NULL) {
		return NULL;
	}

	thread_config_t *p = thread_config;

	do {
		if (pthread_equal(p->tid, tid)) {
			return p;
		}
		if (p->next == NULL) {
			break;
		}
	} while((p = p->next));

	return NULL;
}

void register_thread(const char *name) {
	pthread_t tid = pthread_self();

	pthread_mutex_lock(&logger_access);
	if (thread_config == NULL) {
		thread_config = new_thread_config(tid, name);
	}
	thread_config_t *p = thread_config;

	do {
		if (pthread_equal(p->tid, tid)) {
			pthread_mutex_unlock(&logger_access);
			return;
		}
		if (p->next == NULL) {
			break;
		}
	} while((p = p->next));

	p->next = new_thread_config(tid, name);
	pthread_mutex_unlock(&logger_access);
}

void unregister_thread(pthread_t tid) {
	pthread_mutex_lock(&logger_access);
	if (thread_config == NULL) {
		pthread_mutex_unlock(&logger_access);    
		return;
	}
	thread_config_t *p = thread_config, *prev = NULL;

	do {
		if (pthread_equal(p->tid, tid)) {
			if (p == thread_config) {
				thread_config = p->next;
			} else {
				prev->next = p->next;
			}
			free(p);
			pthread_mutex_unlock(&logger_access);      
			return;
		}
		if (p->next == NULL) {
			break;
		}
		prev = p;
	} while((p = p->next));

	pthread_mutex_unlock(&logger_access);
}

void log_printf(unsigned int level, const char *message, ...) {
	if (log_level == LOG_SILENT || level < log_level) {
		return;
	}
	pthread_mutex_lock(&logger_access);
	pthread_t tid = pthread_self();
	thread_config_t *thread = find_thread(tid);
	const char *thread_name;
	if (thread != NULL) {
		thread_name = thread->name;
	} else {
		thread_name = "Unknown";
	}

	int size = 0;
	char *p = NULL;
	va_list ap;

	va_start(ap, message);
	size = vsnprintf(p, size, message, ap);
	va_end(ap);

	if (size < 0) {
		pthread_mutex_unlock(&logger_access);
		return;
	}

	size++;
	p = xmalloc(size);

	va_start(ap, message);
	size = vsnprintf(p, size, message, ap);
	va_end(ap);

	if (size < 0) {
		pthread_mutex_unlock(&logger_access);	
		free(p);
		return;
	}

	int continue_log;
	if (pthread_equal(tid, last_thread) && !log_ended_with_new_line) {
		continue_log = 1;
	} else {
		continue_log = 0;
	}

	last_thread = tid;
	if (p[strlen(p) - 1] != '\n') {
		log_ended_with_new_line = 0;
	} else {
		log_ended_with_new_line = 1;
	}

	time_t current_time;
	time(&current_time);
	char *time_str = ctime(&current_time);
	time_str[strlen(time_str) - 1] = '\0';
	char *start = p;
	char *new_line = strchr(start, '\n');

	if (continue_log) {
		if (new_line == NULL) {
			fputs(start, stderr);
			free(p);
			pthread_mutex_unlock(&logger_access);
			return;
		} else {
			*new_line = '\0';
			fputs(start, stderr);
			fputc('\n', stderr);
			start = new_line + 1;
			new_line = strchr(start, '\n');
		}
	}

	while(new_line) {
		*new_line = '\0';
		fprintf(stderr, "%s %s [%s] | %s\n", time_str, thread_name, log_levels_str[level], start);
		start = new_line + 1;
		new_line = strchr(start, '\n');
	}

	if (!log_ended_with_new_line) {
		fprintf(stderr, "%s %s [%s] | %s", time_str, thread_name, log_levels_str[level], start);
	}

	free(p);
	pthread_mutex_unlock(&logger_access);
}
