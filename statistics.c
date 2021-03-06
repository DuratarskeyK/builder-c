#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "statistics.h"

static char *uid, *last_build_id = NULL;
static int is_busy = 0;
static pthread_mutex_t busy_access;
static pthread_t statistics_thread;

static void *statistics(void *arg) {
	char hostname[128];
	char *query_string = (char *)arg;
	memset(hostname, 0, 128);
	gethostname(hostname, 128);

	register_thread("Statistics");
	log_printf(LOG_DEBUG, "Statistics thread started\n");
	while(1) {
		pthread_mutex_lock(&busy_access);
		char *payload = alloc_sprintf(API_STATISTICS_PAYLOAD, uid, is_busy, hostname, (query_string ? query_string : ""), (last_build_id == NULL ? "" : last_build_id));
		api_job_statistics(payload);
		pthread_mutex_unlock(&busy_access);
		free(payload);
		sleep(10);
	}

	return NULL;
}

static char char2hex(unsigned char c) {
	if(c <= 9) {
		return '0' + c;
	}
	else if(c <= 15) {
		return 'a' + (c - 10);
	}
	return '0';
}

void set_busy_status(int s, const char *build_id) {
	pthread_mutex_lock(&busy_access);
	is_busy = s ? 1 : 0;
	if (is_busy) {
		log_printf(LOG_DEBUG, "Builder status is busy\n");
	} else {
		log_printf(LOG_DEBUG, "Builder is accepting jobs\n");
	}
	if (build_id != NULL) {
		if (last_build_id != NULL) {
			free(last_build_id);
		}
		last_build_id = xstrdup(build_id);
	}
	pthread_mutex_unlock(&busy_access);
}

int start_statistics_thread(const char *query_string) {
	char builder_id[33];
	char random_data[16];

	int res;

	FILE *urandom = fopen("/dev/urandom", "r");
	if (urandom == NULL) {
		log_printf(LOG_FATAL, "Can't open /dev/urandom for read: %s\n", strerror(errno));
		return -1;
	}
	size_t read_bytes = fread(random_data, 1, 16, urandom);
	if (read_bytes < 16) {
		log_printf(LOG_FATAL, "Couldn't read 16 bytes from /dev/urandom\n");
		fclose(urandom);
		return -1;
	}
	fclose(urandom);

	for(int i = 0; i<16; i++) {
		int rnd = random_data[i];
		builder_id[2 * i] = char2hex(rnd & 0x0F);
		builder_id[2 * i + 1] = char2hex((rnd >> 4) & 0x0F);
	}
	builder_id[32] = '\0';
	uid = xstrdup(builder_id);
	log_printf(LOG_DEBUG, "Builder ID is %s\n", uid);

	pthread_mutex_init(&busy_access, NULL);

	res = pthread_create(&statistics_thread, NULL, &statistics, (void *)query_string);

	if(res) {
		return -1;
	}

	return 0;
}
