#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "live_logger.h"

static pthread_t buffer_dump_thread, read_log_thread;
static int stop, fd;
static char *buf;
static FILE *flog;
static pthread_mutex_t buf_access;

static void *buffer_dump(void *arg) {
	char *build_id = (char *)arg;
	char *key = malloc(strlen(build_id) + strlen("abfworker::rpm-worker-") + 1);
	sprintf(key, "abfworker::rpm-worker-%s", build_id);

	register_thread("Livelogger");
	while(!stop) {
		pthread_mutex_lock(&buf_access);
		api_jobs_logs(key, buf);
		pthread_mutex_unlock(&buf_access);
		sleep(10);
	}
	free(key);
	unregister_thread(buffer_dump_thread);

	return NULL;
}

static void *read_log(__attribute__((unused)) void *arg) {
	int len, d, cur_pos;
	char str[1025];

	cur_pos = 18;

	register_thread("LOG");
	while((len = read(fd, str, 1024)) > 0) {
		str[len] = 0;
		log_printf(LOG_INFO, "%s", str);
		if(flog != NULL) {
			fwrite(str, len, 1, flog);
		}
		pthread_mutex_lock(&buf_access);
		if(LIVE_LOGGER_BUFFER_SIZE - cur_pos < len) {
			d = len - (LIVE_LOGGER_BUFFER_SIZE - cur_pos);
			memmove(buf, buf + d, LIVE_LOGGER_BUFFER_SIZE - d);
			cur_pos -= d;
		}
		cur_pos += sprintf(buf + cur_pos, "%s", str);
		pthread_mutex_unlock(&buf_access);
		if(stop) {
			break;
		}
	}
	if(flog != NULL) {
		fclose(flog);
		flog = NULL;
	}
	close(fd);
	unregister_thread(read_log_thread);

	return NULL;
}

int start_live_logger(char *build_id, int read_fd) {
	pthread_attr_t attr;
	int res;

	stop = 0;

	res = pthread_attr_init(&attr);
	if(res != 0) {
		return -1;
	}
	pthread_mutex_init(&buf_access, NULL);

	buf = malloc(LIVE_LOGGER_BUFFER_SIZE + 1);
	memset(buf, 0, LIVE_LOGGER_BUFFER_SIZE + 1);
	sprintf(buf, "Starting build...\n");

	res = pthread_create(&buffer_dump_thread, &attr, &buffer_dump, (void *)build_id);
	if(res != 0) {
		pthread_mutex_destroy(&buf_access);
		pthread_attr_destroy(&attr);
		return -1;
	}

	flog = fopen("/tmp/script_output.log", "w");
	if (flog == NULL) {
		log_printf(LOG_ERROR, "Can't open /tmp/script_output.log, error: %s\n", strerror(errno));
	}

	fd = read_fd;
	res = pthread_create(&read_log_thread, &attr, &read_log, NULL);
	if(res != 0) {
		pthread_mutex_destroy(&buf_access);
		pthread_attr_destroy(&attr);
		pthread_cancel(buffer_dump_thread);
		return -1;
	}

	pthread_attr_destroy(&attr);

	return 0;
}

void stop_live_logger() {
	stop = 1;
	pthread_join(buffer_dump_thread, NULL);
	pthread_join(read_log_thread, NULL);
	pthread_mutex_destroy(&buf_access);
	free(buf);
	stop = 0;
}
