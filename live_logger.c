#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "live_logger.h"

static pthread_t buffer_dump_thread, read_log_thread;
static int stop, log_read_fd;
static char *buf;
static FILE *flog;
static pthread_mutex_t buf_access;

static void *buffer_dump(void *arg) {
	char *build_id = (char *)arg;
	char *key = xmalloc(strlen(build_id) + strlen("abfworker::rpm-worker-") + 1);
	sprintf(key, "abfworker::rpm-worker-%s", build_id);

	register_thread("Livelogger");
	while(!stop) {
		pthread_mutex_lock(&buf_access);
		api_jobs_logs(key, buf);
		pthread_mutex_unlock(&buf_access);
		for (int i = 0; !stop && i < 10; i++) {
			sleep(1);
		}
	}
	free(key);
	unregister_thread(buffer_dump_thread);

	return NULL;
}

static void *read_log(__attribute__((unused)) void *arg) {
	int len, d, cur_pos;
	char str[1025];

	cur_pos = strlen(start_build_str);

	register_thread("LOG");
	while((len = read(log_read_fd, str, 1024)) > 0) {
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
	}
	if(flog != NULL) {
		fclose(flog);
		flog = NULL;
	}
	close(log_read_fd);
	unregister_thread(read_log_thread);

	return NULL;
}

int start_live_logger(char *build_id, int read_fd) {
	int res;

	stop = 0;

	pthread_mutex_init(&buf_access, NULL);

	buf = xmalloc(LIVE_LOGGER_BUFFER_SIZE + 1);
	memset(buf, 0, LIVE_LOGGER_BUFFER_SIZE + 1);
	sprintf(buf, "%s", start_build_str);

	res = pthread_create(&buffer_dump_thread, NULL, &buffer_dump, (void *)build_id);
	if(res != 0) {
		pthread_mutex_destroy(&buf_access);
		return -1;
	}

	char *script_output_path = xmalloc(strlen(builder_config.work_dir) + 1 + strlen("script_output.log") + 1);
	sprintf(script_output_path, "%s/script_output.log", builder_config.work_dir);
	flog = fopen(script_output_path, "w");
	if (flog == NULL) {
		log_printf(LOG_ERROR, "Can't open %s, error: %s\n", script_output_path, strerror(errno));
	}
	free(script_output_path);

	log_read_fd = read_fd;
	res = pthread_create(&read_log_thread, NULL, &read_log, NULL);
	if(res != 0) {
		pthread_mutex_destroy(&buf_access);
		pthread_cancel(buffer_dump_thread);
		return -1;
	}

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
