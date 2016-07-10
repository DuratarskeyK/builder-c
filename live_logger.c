#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "builder.h"

static pthread_t buffer_dump_thread, read_log_thread;
static char *key, *buf;
static int cur_pos = 0, fd;
static pthread_mutex_t buf_access;

static void *buffer_dump(void *arg) {
	while(1) {
		pthread_mutex_lock(&buf_access);
		int i = 0;
		while(buf[i++] != '\n');
		api_jobs_logs(key, buf + i);
		pthread_mutex_unlock(&buf_access);
		sleep(10);
	}
}

static void *read_log(void *arg) {
	int len, d;
	char str[1025];

	while((len = read(fd, str, 1024)) > 0) {
		str[len] = 0;
		pthread_mutex_lock(&buf_access);
		if(3000 - cur_pos < len) {
			d = len - (3000 - cur_pos);
			memmove(buf, buf + d, 3000 - d);
			cur_pos -= d;
		}
		cur_pos += sprintf(buf + cur_pos, "%s", str);
		pthread_mutex_unlock(&buf_access);
	}

	return NULL;
}

int start_live_logger(const char *build_id, int read_fd) {
	pthread_attr_t attr;
	int res;

	res = pthread_attr_init(&attr);
	if(res != 0) {
		return -1;
	}
	//res = pthread_attr_setstacksize(&attr, 1<<10);

	pthread_mutex_init(&buf_access, NULL);

	key = malloc(strlen(build_id) + strlen("abfworker::rpm-worker-") + 1);
	buf = malloc(3001);
	memset(buf, 0, 3001);
	cur_pos += sprintf(buf, "\nStarting build...\n");

	sprintf(key, "abfworker::rpm-worker-%s", build_id);

	res = pthread_create(&buffer_dump_thread, &attr, &buffer_dump, NULL);

	if(res != 0) {
		return -1;
	}

	fd = read_fd;
	res = pthread_create(&read_log_thread, &attr, &read_log, NULL);

	if(res != 0) {
		return -1;
	}

	pthread_attr_destroy(&attr);

	return 0;
}

void stop_live_logger() {
	pthread_mutex_destroy(&buf_access);
	pthread_cancel(buffer_dump_thread);
	pthread_cancel(read_log_thread);
	free(key);
	free(buf);
	close(fd);
}