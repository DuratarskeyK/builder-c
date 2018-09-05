#ifndef _BUILDER_H
#define _BUILDER_H
	//for pid_t
	#include <unistd.h>

	#define BUILD_COMPLETED 0
	#define BUILD_FAILED    1
	#define BUILD_PENDING   2
	#define BUILD_STARTED   3
	#define BUILD_CANCELED  4
	#define TESTS_FAILED    5

	//main.c
	#define COMPARE(var, str, len) (strlen(str) == (len) && !strncmp(var, str, len))

	//data to pass to live inspector thread
	typedef struct {
		int time_to_live;
		pid_t pid;
		const char *build_id;
	} li_data;

	typedef struct {
		uid_t omv_uid;
		gid_t mock_gid;
	} usergroup;

	usergroup get_omv_uid_mock_gid();
	char *read_file(const char *);
	int process_config(char **, char **, char **);
	int load_scripts(const char *, const char *, const char *);
	char *create_output_directory();
	int parse_job_description(const char *, char **, int *, char **, char ***);
	char *make_env(const char *, const char *);

	//statistics.h
	static void *statistics(void *);
	static char char2hex(unsigned char);
	void set_busy_status(int s);
	int start_statistics_thread(const char *);

	//live_logger.c
	static void *buffer_dump(void *);
	static void *read_log(void *);
	int start_live_logger(const char *, int);
	void stop_live_logger();

	//live_inspector.c
	static void *live_inspector(void *);
	int start_live_inspector(int, pid_t, const char *);
	void stop_live_inspector();

	//api.c
	#define OBSERVER_QUEUE "rpm_worker_observer"
	#define OBSERVER_CLASS "AbfWorker::RpmWorkerObserver"
	#define API_JOBS_SHIFT "/jobs/shift"
	#define API_JOBS_FEEDBACK "/jobs/feedback"
	#define API_JOBS_STATUS "/jobs/status"
	#define API_JOBS_LOGS "/jobs/logs"
	#define API_JOBS_STATISTICS "/jobs/statistics"
	#define API_LOGS_PAYLOAD "{\"name\":\"%s\",\"logs\":\"%s\"}"
	#define API_STATUS_QUERYSTRING "?key=abfworker::rpm-worker-%s::live-inspector"
	#define API_STATISTICS_PAYLOAD "{\"uid\":\"%s\",\"worker_count\":1,\"busy_workers\":%d,\"host\":\"%s\",\"query_string\":\"%s\"}"
	static int curl_get(const char *, char **);
	static int curl_put(const char *, const char *);
	void api_set_token(const char *);
	void api_set_api_url(const char *);
	int api_job_statistics(const char *);
	int api_jobs_logs(const char *, const char *);
	int api_jobs_shift(char **, const char *);
	int api_jobs_feedback(const char *, int, const char *);
	int api_jobs_status(const char *);
	//exec_build.c
	typedef struct {
		const char *distrib_type, **env;
		int write_fd;
		usergroup omv_mock;
	} exec_data;
	typedef struct {
		char *stack;
		pid_t pid;
		int read_fd;
	} child;
	child exec_build(const char *, const char **, usergroup);
#endif
