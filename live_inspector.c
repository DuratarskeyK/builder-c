#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "builder.h"

static li_data *data = NULL;
static pthread_t li_thread;

static void *live_inspector(void *arg) {
	li_data *data = (li_data *)arg;
	int t;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &t);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &t);

	unsigned long endtime = (unsigned long)time(NULL) + data->time_to_live;

	while(1) {
		unsigned long curtime = (unsigned long)time(NULL);
		int status = api_jobs_status(data->build_id);
		if(status || curtime > endtime) {
			kill(data->pid, SIGTERM);
		}
		sleep(10);
	}

	return NULL;
}

int start_live_inspector(int ttl, pid_t pid, const char *bid) {
	pthread_attr_t attr;
	int res;
	data = malloc(sizeof(li_data));

	data->time_to_live = ttl;
	data->pid = pid;
	data->build_id = bid;

	res = pthread_attr_init(&attr);
	if(res != 0) {
		return -1;
	}

	res = pthread_create(&li_thread, &attr, &live_inspector, data);

	if(res != 0) {
		return -1;
	}

	pthread_attr_destroy(&attr);

	return 0;
}

void stop_live_inspector() {
	pthread_cancel(li_thread);
	free(data);
}