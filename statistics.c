#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "statistics.h"

static char *uid;
static int is_busy = 0, is_ready = 0;
static pthread_mutex_t busy_access;
static pthread_t statistics_thread;

extern int api_job_statistics(const char *);

static void *statistics(void *arg) {
	char hostname[128];
	char *query_string = (char *)arg;
	memset(hostname, 0, 128);
	gethostname(hostname, 128);

	while(1) {
		// uid length plus hostname plus busy workers plus \0
		// 162 = 32 + 128 + 1 + 1
		char *payload;
		if(query_string) {
			payload = malloc(strlen(API_STATISTICS_PAYLOAD) + strlen(query_string) + 162);
		}
		else {
			payload = malloc(strlen(API_STATISTICS_PAYLOAD) + 162);
		}
		pthread_mutex_lock(&busy_access);
		sprintf(payload, API_STATISTICS_PAYLOAD, uid, is_busy, hostname, (query_string ? query_string : ""));
		pthread_mutex_unlock(&busy_access);
		api_job_statistics(payload);
		free(payload);
		sleep(10);
	}

	return NULL;
}

static char char2hex(unsigned char c) {
	if(c <= 9) {
		return '0' + c;
	}
	else if(c >= 10 && c <= 15) {
		return 'a' + (c - 10);
	}
	else {
		return '0';
	}
}

void set_busy_status(int s) {
	if(is_ready) {
		pthread_mutex_lock(&busy_access);
	}

	is_busy = s ? 1 : 0;

	if(is_ready) {
		pthread_mutex_unlock(&busy_access);
	}
}

int start_statistics_thread(const char *query_string) {
	unsigned char random_data[16];
	char builder_id[33];
	int res;
	pthread_attr_t attr;

	res = pthread_attr_init(&attr);
	if(res != 0) {
		return -1;
	}

	FILE *dev_random;

	dev_random = fopen("/dev/urandom", "r");

	if(dev_random) {
		fread(random_data, 16, 1, dev_random);
		fclose(dev_random);
		for(int i = 0; i<16; i++) {
			builder_id[2 * i] = char2hex(random_data[i] & 0x0F);
			builder_id[2 * i + 1] = char2hex((random_data[i] >> 4) & 0x0F);
		}
		builder_id[32] = '\0';
		uid = strdup(builder_id);

		pthread_mutex_init(&busy_access, NULL);
		is_ready = 1;

		res = pthread_create(&statistics_thread, &attr, &statistics, (void *)query_string);

		if(res) {
			pthread_attr_destroy(&attr);
			return -1;
		}

		pthread_attr_destroy(&attr);
		return 0;
	}
	else {
		return -1;
	}
}
