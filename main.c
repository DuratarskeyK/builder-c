#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include "main.h"

static const char home_output[] = "/home/omv/output";
static char *container_data_path, *upload_cmd, *commit_hash_path;
static char *hostname_payload, *move_output_cmd;

int main() {
	int res, try, retries;
	char *query_string, *abf_api_url, *api_token;

	char *log_level = getenv("LOG_LEVEL");
	if (init_logger(log_level == NULL ? "INFO" : log_level) < 0) {
		return 2;
	}
	register_thread("Main");

	log_printf(LOG_INFO, "Starting DNS check...\n");
	res = check_dns();
	if (res < 0) {
		log_printf(LOG_FATAL, "Check DNS failed, can't resolve github.com. Exiting...\n");
		return 6;
	} else {
		log_printf(LOG_INFO, "Successfuly resolved github. Continuing.\n");
	}

	usergroup omv_mock = get_omv_uid_mock_gid();
	if(omv_mock.omv_uid == 0) {
		log_printf(LOG_FATAL, "User omv doesn't exist. Exiting...\n");
		return 7;
	}
	if(omv_mock.mock_gid == 0) {
		log_printf(LOG_FATAL, "Group mock doesn't exist. Exiting...\n");
		return 8;
	}

	res = process_config(&abf_api_url, &api_token, &query_string);
	if (res < 0) {
		log_printf(LOG_FATAL, "Failed to process config, exiting.\n");
		return 1;
	}

	init_strings(api_token);

	init_api(abf_api_url, api_token);

	if(start_statistics_thread(query_string)) {
		log_printf(LOG_FATAL, "Failed to initialize statistics thread. Exiting...\n");
		return 5;
	}

	while(1) {
		char *job;
		if(api_jobs_shift(&job, query_string)) {
			//sleep(10);
			continue;
		}

		char *build_id, *distrib_type;
		char **env;
		int ttl;

		res = parse_job_description(job, &build_id, &ttl, &distrib_type, &env);
		free(job);
		if(res < 0) {
			sleep(10);
			continue;
		}

		log_printf(LOG_INFO, "Starting build with build_id %s.\n", build_id);
		log_printf(LOG_INFO, "Sending build start notification to ABF.\n");
		retries = 5;
		try = 0;
		set_busy_status(1);
		int build_start_success = 0;
		while(retries) {
			log_printf(LOG_INFO, "Try #%d: Sending data to ABF.\n", try + 1);
			if(!api_jobs_feedback(build_id, BUILD_STARTED, hostname_payload)) {
				log_printf(LOG_INFO, "Try #%d: Data sent.\n", try + 1);
				build_start_success = 1;
				break;
			} else {
				log_printf(LOG_ERROR, "Try #%d: Failed to send data to ABF. Sleeping for %d seconds and retrying...\n", try + 1, 1 << try);
			}
			retries--;
			sleep(1 << try);
			try++;
		}

		if (!build_start_success) {
			log_printf(LOG_ERROR, "Failed to send build start to ABF, aborting build.\n");
			free(build_id);
			free(distrib_type);
			for(int i = 0; i < res; i++) {
				free(env[i]);
			}
			free(env);
			continue;
		}

		child script = exec_build(distrib_type, (const char **)env, omv_mock);
		if(start_live_inspector(ttl, script.pid, build_id) < 0) {
			log_printf(LOG_WARN, "Live inspector failed to start. Job canceling and timeout is unavailable.\n");
		}
		if(start_live_logger(build_id, script.read_fd) < 0) {
			log_printf(LOG_WARN, "Live logger failed to start. Live log will be unavailable.\n");
		}

		int status, build_status = BUILD_COMPLETED;
		waitpid(script.pid, &status, 0);
		stop_live_inspector();
		stop_live_logger();

		if(WIFEXITED(status)) {
			build_status = WEXITSTATUS(status);
		}
		else if(WIFSIGNALED(status)) {
			build_status = BUILD_FAILED;
		}

		free(script.stack);
		for(int i = 0; i < res; i++) {
			free(env[i]);
		}
		free(env);
		free(distrib_type);

		log_printf(LOG_INFO, "Build is over with status %s.\n", build_statuses_str[build_status]);

		mkdir(home_output, 0666);
		system(move_output_cmd);

		char *container_data = read_file(container_data_path);
		if(container_data != NULL) {
			unlink(container_data_path);
		}

		system(upload_cmd);
		char *results = read_file("/tmp/results.json");
		if(results != NULL) {
			unlink("/tmp/results.json");
		}

		char *commit_hash = read_file(commit_hash_path);
		if(commit_hash != NULL) {
			commit_hash[40] = '\0';
			unlink(commit_hash_path);
		}

		char *args = malloc((container_data ? strlen(container_data) : 0) + (results ? strlen(results) : 0) + 2048);
		sprintf(args, build_completed_args_fmt, (results ? results : "[]"), \
				(container_data ? container_data : "{}"), status, (commit_hash ? commit_hash : ""));

		retries = 5;
		try = 0;
		while(retries) {
			log_printf(LOG_INFO, "Try #%d: Sending data to ABF\n", try + 1);
			if(!api_jobs_feedback(build_id, build_status, args)) {
				log_printf(LOG_INFO, "Data sent.\n");
				break;
			} else {
				log_printf(LOG_ERROR, "Failed to send data to ABF. Sleeping for %d seconds and retrying...\n", 1 << try);
			}
			retries--;
			sleep(1 << try);
			try++;
		}

		set_busy_status(0);
		free(args);
		free(build_id);

		if(commit_hash) {
			free(commit_hash);
		}
		if(results) {
			free(results);
		}
		if(container_data) {
			free(container_data);
		}
	}

	return 0;
}

static usergroup get_omv_uid_mock_gid() {
	usergroup ret;
	ret.omv_uid = ret.mock_gid = 0;

	struct passwd *omv = getpwnam("omv");
	if(omv == NULL) {
		return ret;
	}
	ret.omv_uid = omv->pw_uid;
	struct group *mock = getgrnam("mock");
	if(mock == NULL) {
		return ret;
	}
	ret.mock_gid = mock->gr_gid;

	return ret;
}

static char *read_file(const char *path) {
	FILE *fn = fopen(path, "r");
	struct stat fileinfo;

	if(fn != NULL && !fstat(fileno(fn), &fileinfo)) {
		char *res = malloc(fileinfo.st_size + 1);
		fread(res, fileinfo.st_size, 1, fn);
		res[fileinfo.st_size] = '\0';
		fclose(fn);
		return res;
	}
	else {
		log_printf(LOG_ERROR, "Failed to read file %s. Error: %s\n", path, strerror(errno));
		return NULL;
	}
}

static void init_strings(const char *api_token) {
	char hostname[128];
	gethostname(hostname, 128);
	hostname_payload = malloc(strlen(hostname) + strlen(hostname_payload_fmt));
	sprintf(hostname_payload, hostname_payload_fmt, hostname);

	move_output_cmd = malloc(strlen(move_output_cmd_fmt) + strlen(home_output));
	sprintf(move_output_cmd, move_output_cmd_fmt, home_output);

	container_data_path = malloc(strlen(container_data_path_fmt) + strlen(home_output));
	sprintf(container_data_path, container_data_path_fmt, home_output);

	upload_cmd = malloc(strlen(upload_cmd_fmt) + strlen(api_token) + strlen(home_output));
	sprintf(upload_cmd, upload_cmd_fmt, api_token, home_output);

	commit_hash_path = malloc(strlen(commit_hash_path_fmt) + strlen(home_output));
	sprintf(commit_hash_path, commit_hash_path_fmt, home_output);
}
