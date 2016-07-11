#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "builder.h"

static char *uid;
static int is_busy = 0, is_ready = 0;
static pthread_mutex_t busy_access;

static void *statistics_thread(void *arg) {

}

static char char2hex(unsigned char c) {
	if(c >= 0 && c <= 9) {
		return '0' + c;
	}
	else if(c >= 10 && c <= 15) {
		return 'a' + (c - 10);
	}
	else {
		return '0';
	}
}

int start_statistics_thread() {
	unsigned char random_data[16];
	char builder_id[33];
	FILE *dev_random;

	dev_random = fopen("/dev/random", "r");
	if(dev_random == NULL) {
		dev_random = fopen("/dev/urandom", "r");
	}

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
	}
	else {
		return -1;
	}
}