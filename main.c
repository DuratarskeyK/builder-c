#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "main.h"

int main(int argc, char **argv) {
	int res, try, retries;

	char *config_path = "/etc/builder-c/builder.conf";
	if (argc == 2) {
		config_path = argv[1];
	}
	res = init(config_path);
	if (res < 0) {
		return 1;
	}

	while(1) {
		char *job;
		if(api_jobs_shift(&job)) {
			sleep(10);
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
		retries = 5;
		try = 0;
		set_busy_status(1, build_id);
		int build_start_success = 0;
		while(retries) {
			log_printf(LOG_INFO, "Try #%d: Sending data to ABF.\n", try + 1);
			if(!api_jobs_feedback(build_id, BUILD_STARTED, builder_config.strings.hostname_payload)) {
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
			set_busy_status(0, NULL);
			continue;
		}

		mkdir(builder_config.output_dir, 0777);
		child_t *script = exec_build(distrib_type, (char * const *)env);
		int status, build_status = BUILD_COMPLETED, canceled = 0, exit_code = 0;
		if (script == NULL) {
			exit_code = 255;
			build_status = BUILD_FAILED;
		} else {
			int live_inspector_started = 1;
			log_printf(LOG_DEBUG, "Starting live inspector, build's time to live is %d seconds.\n", ttl);
			if(start_live_inspector(ttl, script->pid, build_id) < 0) {
				live_inspector_started = 0;
				log_printf(LOG_WARN, "Live inspector failed to start. Job canceling and timeout will be unavailable.\n");
			}

			int live_logger_started = 1;
			log_printf(LOG_DEBUG, "Starting live logger.\n");
			if(start_live_logger(build_id, script->read_fd) < 0) {
				live_logger_started = 0;
				log_printf(LOG_WARN, "Live logger failed to start. Live log will be unavailable.\n");
			}

			waitpid(script->pid, &status, 0);
			if (live_inspector_started) {
				canceled = stop_live_inspector();
			}
			if (live_logger_started) {
				stop_live_logger();
			}

			if (canceled) {
				build_status = BUILD_CANCELED;
			}
			else if(WIFEXITED(status)) {
				exit_code = WEXITSTATUS(status);
				switch(exit_code) {
					case 0:
						build_status = BUILD_COMPLETED;
						break;
					case 5:
						build_status = TESTS_FAILED;
						break;
					default:
						build_status = BUILD_FAILED;
				}
			}
			else if(WIFSIGNALED(status)) {
				exit_code = 255;
				build_status = BUILD_FAILED;
			}
			free(script);
		}

		for(int i = 0; i < res; i++) {
			free(env[i]);
		}
		free(env);
		free(distrib_type);

		log_printf(LOG_INFO, "Build is over with status %s.\n", build_statuses_str[build_status]);

		res = system(builder_config.strings.move_output_cmd);

		char *container_data = read_file(builder_config.strings.container_data_path);
		char *commit_hash = read_file(builder_config.strings.commit_hash_path);
		if(commit_hash != NULL) {
			commit_hash[40] = '\0';
		}

		res = system(builder_config.strings.upload_cmd);
		char *results = read_file("/tmp/results.json");

		char *fail_reason = NULL;
		if (build_status == BUILD_FAILED) {
			fail_reason = read_file(builder_config.strings.fail_reason_path);
			if (fail_reason != NULL) {
				for (unsigned int i = 0; i < strlen(fail_reason); i++) {
					if (!isprint(fail_reason[i]) || fail_reason[i] == '"') {
						fail_reason[i] = ' ';
					}
				}
			}
		}

		char *args = xmalloc((container_data ? strlen(container_data) : 0) + (results ? strlen(results) : 0) + 2048);
		sprintf(args, build_completed_args_fmt, (results ? results : "[]"), \
				(container_data ? container_data : "{}"), exit_code, (commit_hash ? commit_hash : ""),
				(fail_reason ? fail_reason : ""));

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

		set_busy_status(0, NULL);
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
		if(fail_reason) {
			free(fail_reason);
		}
	}

	return 0;
}

static char *read_file(const char *path) {
	FILE *fn = fopen(path, "r");
	struct stat fileinfo;

	if(fn != NULL && !fstat(fileno(fn), &fileinfo)) {
		int len = fileinfo.st_size;
		char *res = xmalloc(len + 1);
		int read_size = fread(res, 1, len, fn);
		if (read_size < len && ferror(fn)) {
			log_printf(LOG_ERROR, "Encountered error while reading file %s\n", path);
		}
		res[read_size] = '\0';
		fclose(fn);
		unlink(path);
		return res;
	}
	else {
		if (errno != ENOENT) {
			log_printf(LOG_ERROR, "Failed to read file %s. Error: %s\n", path, strerror(errno));
		}
		return NULL;
	}
}
