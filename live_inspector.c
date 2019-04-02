#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "live_inspector.h"

static int stop = 0, canceled = 0;
static li_data *data = NULL;
static pthread_t li_thread;

static void *live_inspector(void *arg) {
	li_data *data = (li_data *)arg;

	register_thread("Live Inspector");

	unsigned long endtime = (unsigned long)time(NULL) + data->time_to_live;

	while(!stop) {
		unsigned long curtime = (unsigned long)time(NULL);
		int status = api_jobs_status(data->build_id);
		if(status || curtime > endtime) {
			kill(data->pid, SIGKILL);
			canceled = 1;
			break;
		}
		for (int i = 0; !stop && i < 3; i++) {
			sleep(1);
		}
	}
	free(data);
	unregister_thread(li_thread);

	return NULL;
}

int start_live_inspector(int ttl, pid_t pid, const char *bid) {
	int res;
	data = malloc(sizeof(li_data));

	data->time_to_live = ttl;
	data->pid = pid;
	data->build_id = bid;

	stop = canceled = 0;
	res = pthread_create(&li_thread, NULL, &live_inspector, data);
	if(res != 0) {
		return -1;
	}

	return 0;
}

int stop_live_inspector() {
	stop = 1;
	pthread_join(li_thread, NULL);
	stop = 0;
	return canceled;
}
