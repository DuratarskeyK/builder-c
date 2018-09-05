#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include "builder.h"
#include "jsmn.h"

int main() {
	char *home_output = NULL;
	home_output = create_output_directory();

	int res, start, len;
	char *query_string, *abf_api_url, *api_token, *job;
	char **env;
	char hostname[128], hostname_payload[156];

	usergroup omv_mock = get_omv_uid_mock_gid();
	if(omv_mock.omv_uid == 0) {
		printf("User omv doesn't exist.\n");
		return 0;
	}
	if(omv_mock.mock_gid == 0) {
		printf("Group mock doesn't exist.\n");
		return 0;
	}

	memset(hostname, 0, 128);
	gethostname(hostname, 128);
	sprintf(hostname_payload, "{\"hostname\":\"%s\"}", hostname);

	curl_global_init(CURL_GLOBAL_ALL);

	res = process_config(&abf_api_url, &api_token, &query_string);
	switch(res) {
		case 1:
			printf("Config file builder.conf not found.\n");
			return 1;
		case 2:
			printf("Error parsing builder.conf.\n");
			return 2;
		case 3:
			printf("Missing abf api url in config.\n");
			return 3;
		case 4:
			printf("Missing api token in config.\n");
			return 4;
	}

	api_set_token(api_token);
	api_set_api_url(abf_api_url);

	if(start_statistics_thread(query_string)) {
		printf("Failed to initialize statistics thread. Moving on without it.\n");
	}

	while(1) {
		if(api_jobs_shift(&job, query_string)) {
			//getting a job failed
			sleep(10);
			continue;
		}

		char *build_id, *distrib_type;
		int ttl;

		res = parse_job_description(job, &build_id, &ttl, &distrib_type, &env);
		free(job);
		//either json is invalid or no job
		if(res < 0) {
			sleep(10);
			continue;
		}

		printf("Starting build with build_id %s\n", build_id);
		int retries = 5;
		set_busy_status(1);
		while(api_jobs_feedback(build_id, BUILD_STARTED, hostname_payload) && retries) {
			retries--;
		}

		child script = exec_build(distrib_type, (const char **)env, omv_mock);
		if(start_live_inspector(ttl, script.pid, build_id) < 0) {
			printf("Live inspector failed to start. Job canceling and timeout is unavailable.\n");
		}
		if(start_live_logger(build_id, script.read_fd) < 0) {
			printf("Live logger failed to start. Live log will be unavailable.\n");
		}

		int status, build_status;
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

		printf("Build is over with status %d\n", build_status);

		char *filename = malloc(strlen(home_output) + strlen("/container_data.json") + 1), *container_data;
		struct stat fileinfo;
		sprintf(filename, "%s/container_data.json", home_output);
		container_data = read_file(filename);
		if(container_data != NULL) {
			unlink(filename);
		}
		free(filename);

		mkdir(home_output, 0666);
		char *new_log_path = malloc(strlen(home_output) + strlen("/script_output.log") + 1);
		sprintf(new_log_path, "%s/script_output.log", home_output);
		rename("/tmp/script_output.log", new_log_path);
		free(new_log_path);

		char *upload_cmd = malloc(strlen("/bin/bash /etc/builder-c/filestore_upload.sh ") + strlen(home_output) + 22);
		sprintf(upload_cmd, "/bin/bash /etc/builder-c/filestore_upload.sh %s %s", api_token, home_output);
		system(upload_cmd);
		char *results = read_file("/tmp/results.json");
		if(results != NULL) {
			unlink("/tmp/results.json");
		}
		free(upload_cmd);

		filename = malloc(strlen(home_output) + strlen("/../commit_hash") + 1);
		sprintf(filename, "%s/../commit_hash", home_output);
		char *commit_hash = read_file(filename);
		if(commit_hash != NULL) {
			commit_hash[40] = '\0';
			unlink(filename);
		}
		free(filename);

		char *args = malloc((container_data ? strlen(container_data) : 0) + (results ? strlen(results) : 0) + 2048);
		sprintf(args, "{\"results\":%s,\"packages\":%s,\"exit_status\":%d,\"commit_hash\":\"%s\"}", (results ? results : "{}"), \
				(container_data ? container_data : "{}"), status, (commit_hash ? commit_hash : ""));

		printf("%s\n", args);
		retries = 5;
		while(api_jobs_feedback(build_id, build_status, args) && retries) {
			retries--;
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

	curl_global_cleanup();
	return 0;
}

usergroup get_omv_uid_mock_gid() {
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

char *read_file(const char *path) {
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
		return NULL;
	}
}

int process_config(char **abf_api_url, char **api_token, char **query_string) {
	FILE *config_file;
	config_t config;
	config_setting_t *platforms_list;
	const char *platform_type, *git_path, *git_branch;
	char *config_path;
	int len;

	config_file = fopen("/etc/builder-c/builder.conf", "r");
	if(config_file == NULL) {
		return 1;
	}
	config_init(&config);
	if(!config_read(&config, config_file)) {
		return 2;
	}
	fclose(config_file);

	printf("Config parsed successfuly.\n");

	const char *tmp, *arches, *native_arches, *platforms;

	int req_res;
	req_res = config_lookup_string(&config, "application.abf.api_url", &tmp);
	if(!req_res) {
		return 3;
	}
	printf("Found api base url: %s\n", tmp);
	*abf_api_url = strdup(tmp);
	req_res = config_lookup_string(&config, "application.abf.token", &tmp);
	if(!req_res) {
		tmp = getenv("BUILD_TOKEN");
		if(tmp == NULL) {
			return 4;
		}
	}
	printf("Found build token: %s\n", tmp);
	*api_token = strdup(tmp);

	int p1, p2, p3;
	p1 = config_lookup_string(&config, "application.abf.supported_arches", &arches);
	if(!p1) {
		tmp = getenv("BUILD_ARCH");
		if(tmp != NULL) {
			arches = strdup(tmp);
			p1 = 1;
		}
	}
	p2 = config_lookup_string(&config, "application.abf.native_arches", &native_arches);
	if(!p2) {
		tmp = getenv("NATIVE_ARCH");
		if(tmp != NULL) {
			native_arches = strdup(tmp);
			p2 = 1;
		}
	}
	p3 = config_lookup_string(&config, "application.abf.supported_platforms", &platforms);
	if(!p3) {
		tmp = getenv("BUILD_PLATFORM");
		if(tmp != NULL) {
			platforms = strdup(tmp);
			p3 = 1;
		}
	}
	len = (p1 ? strlen(arches) : 0) + (p2 ? strlen(native_arches) : 0) + (p3 ? strlen(platforms) : 0);
	if(len) {
		char *pointer;
		*query_string = malloc(len + 50);
		pointer = *query_string;
		if(p1) {
			pointer += sprintf(pointer, "arches=%s&", arches);
			printf("Found supported arches: %s\n", arches);
		}
		if(p2) {
			pointer += sprintf(pointer, "native_arches=%s&", native_arches);
			printf("Found native arches: %s\n", native_arches);
		}
		if(p3) {
			pointer += sprintf(pointer, "platforms=%s", platforms);
			printf("Found supported platforms: %s\n", platforms);
		}

		pointer-=1;
		if(*pointer == '&') {
			*pointer = '\0';
		}
	}
	else {
		*query_string = NULL;
	}

	platforms_list = config_lookup(&config, "application.platforms.list");
	if(platforms_list != NULL) {
		len = config_setting_length(platforms_list);
		for(int i = 0; i < len; i++) {
			int res, res2;
			platform_type = config_setting_get_string_elem(platforms_list, i);
			config_path = malloc(36 + strlen(platform_type));
			sprintf(config_path, "application.platforms.%s.git", platform_type);
			res = config_lookup_string(&config, config_path, &git_path);
			sprintf(config_path, "application.platforms.%s.branch", platform_type);
			res2 = config_lookup_string(&config, config_path, &git_branch);
			free(config_path);
			if(res == CONFIG_FALSE) {
				continue;
			}
			else if(res2 == CONFIG_FALSE) {
				load_scripts(git_path, NULL, platform_type);
			}
			else {
				load_scripts(git_path, git_branch, platform_type);
			}
		}
	}

	return 0;
}

int load_scripts(const char *git_repo, const char *git_branch, const char *platform_type) {
	char cmd[512];
	int res;

	if(git_branch != NULL) {
		sprintf(cmd, "cd /; sudo rm -rf %s; sudo git clone -b %s %s %s", platform_type, git_branch, git_repo, platform_type);
	}
	else {
		sprintf(cmd, "cd /; sudo rm -rf %s; sudo git clone %s %s", platform_type, git_repo, platform_type);
	}
	res = system((const char*)cmd);
	return res;
}

char *create_output_directory() {
	char *home = "/home/omv", *res;
	res = malloc(strlen(home) + strlen("/output") + 1);
	sprintf(res, "%s/output", home);
	return res;
}

int parse_job_description(const char *js, char **build_id, int *ttl, char **distrib_type, char ***env) {
	jsmn_parser json_parser;
	jsmntok_t *json;
	int res;

	jsmn_init(&json_parser);

	res = jsmn_parse(&json_parser, js, strlen(js), NULL, 0);

	if(res > 0) {
		json = malloc(res * sizeof(jsmntok_t));
		jsmn_init(&json_parser);
		res = jsmn_parse(&json_parser, js, strlen(js), json, res);
	}
	else {
		return -1;
	}

	int i;

	for(i = 0; i<res; i++) {
		if(!strncmp(&js[json[i].start], "worker_args", strlen("worker_args")) && json[i+2].type == JSMN_OBJECT) {
			break;
		}
	}

	if(i == res) {
		return -2;
	}

	i += 3;

	int j;
	for(j = 0; j<res; j++) {
		if(!strncmp(&js[json[j].start], "cmd_params", strlen("cmd_params")) && json[j+1].type == JSMN_OBJECT) {
			break;
		}
	}

	if(j == res) {
		return -2;
	}

	int cmd_params_len = json[j+1].size, cur_env = 0;
	//cmd_params + 6 necessary parameters + NULL
	*env = malloc((cmd_params_len + 7) * sizeof(char *));

	while(i < res) {
		int param_len = json[i].end - json[i].start;
		const char *start = &js[json[i].start];
		if(COMPARE(start, "id", param_len)) {
			*build_id = strndup(&js[json[i+1].start], json[i+1].end-json[i+1].start);
			i += 2;
		}
		else if(COMPARE(start, "distrib_type", param_len)) {
			*distrib_type = strndup(&js[json[i+1].start], json[i+1].end-json[i+1].start);
			i += 2;
		}
		else if(COMPARE(start, "time_living", param_len)) {
			char *t = strndup(&js[json[i+1].start], json[i+1].end-json[i+1].start);
			*ttl = atoi(t);
			free(t);
			i+=2;
		}
		else if(COMPARE(start, "cmd_params", param_len)) {
			//skip jsmn_object straight to the first parameter
			i += 2;

			for(j = 0; j < cmd_params_len; j++, i+=2, cur_env++) {
				char *key = strndup(&js[json[i].start], json[i].end - json[i].start);
				if(json[i+1].type == JSMN_PRIMITIVE && js[json[i+1].start] == 'n') {
					(*env)[j] = make_env(key, "");
				}
				else {
					char *value = strndup(&js[json[i+1].start], json[i+1].end - json[i+1].start);
					(*env)[j] = make_env(key, value);
					free(value);
				}

				free(key);
			}
		}
		else if(COMPARE(start, "include_repos", param_len)) {
			int repos_len = json[i+1].size;
			char *repo_names = malloc(json[i+1].end-json[i+1].start), *repo_urls = malloc(json[i+1].end-json[i+1].start);
			char *prn = repo_names, *pru = repo_urls;
			i += 2;

			int f = 0;
			for(j = 0; j < 2 * repos_len; j++, i++) {
				char *t = strndup(&js[json[i].start], json[i].end - json[i].start);
				if(!f) {
					prn += sprintf(prn, "%s ", t);
					f = 1;
				}
				else {
					pru += sprintf(pru, "%s ", t);
					f = 0;
				}
				free(t);
			}

			(*env)[cur_env] = make_env("REPO_NAMES", repo_names);
			(*env)[cur_env+1] = make_env("REPO_URL", repo_urls);
			free(repo_urls);
			free(repo_names);
			cur_env += 2;
		}
		else if(COMPARE(start, "uname", param_len)) {
			char *t = strndup(&js[json[i+1].start], json[i+1].end - json[i+1].start);
			(*env)[cur_env++] = make_env("UNAME", t);
			free(t);
			i += 2;
		}
		else if(COMPARE(start, "email", param_len)) {
			char *t = strndup(&js[json[i+1].start], json[i+1].end - json[i+1].start);
			(*env)[cur_env++] = make_env("EMAIL", t);
			free(t);
			i += 2;
		}
		else if(COMPARE(start, "platform", param_len)) {
			int platform_len = json[i+1].size;
			i += 2;
			for(j = 0; j<platform_len; i+=2, j++) {
				if(COMPARE(&js[json[i].start], "arch", json[i].end-json[i].start)) {
					char *t = strndup(&js[json[i+1].start], json[i+1].end-json[i+1].start);
					(*env)[cur_env++] = make_env("PLATFORM_ARCH", t);
					free(t);
				}
			}
		}
		else {
			i++;
		}
	}

	free(json);

	(*env)[cur_env++] = strdup("HOME=/home/omv");
	(*env)[cur_env++] = NULL;

	return cur_env;
}

char *make_env(const char *key, const char *value) {
	char *res = malloc(strlen(key) + strlen(value) + 2);

	sprintf(res, "%s=%s", key, value);

	return res;
}
