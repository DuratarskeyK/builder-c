#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <grp.h>
#include <pwd.h>
#include "init.h"

builder_config_t builder_config;

int init(char *config_path) {
	FILE *config_file;
	config_t config;

	config_file = fopen(config_path, "r");
	if(config_file == NULL) {
		printf("Fatal: Error openning config file: %s.\n", strerror(errno));
		return -1;
	}
	config_init(&config);
	if(!config_read(&config, config_file)) {
		printf("Fatal: Error parsing config file:\n");
		int line = config_error_line(&config);
		const char *text = config_error_text(&config);
		printf("Fatal: Line #%d: %s.\n", line, text);
		config_destroy(&config);
		goto err;
	}
	fclose(config_file);

	char *log_level;
	int req_res;

	req_res = config_lookup_string(&config, log_level_path, (const char **)&log_level);
	if (!req_res) {
		log_level = getenv("LOG_LEVEL");
		if (log_level == NULL) {
			log_level = (char *)default_log_level;
		}
	}
	if (init_logger(log_level) < 0) {
		printf("Fatal: Failed to initialize logger.\n");
		goto err;
	}
	register_thread("Main");

	log_printf(LOG_DEBUG, "Config parsed successfuly.\n");

	if(!thread_setup()) {
		log_printf(LOG_FATAL, "Failed to setup openssl mutexes.\n");
		goto err;
	}

	log_printf(LOG_INFO, "Starting DNS check.\n");
	if (check_dns() < 0) {
		log_printf(LOG_FATAL, "DNS check failed, can't resolve github.com.\n");
		goto err;
	} else {
		log_printf(LOG_INFO, "Successfuly resolved github.\n");
	}

	if(get_api_info(&config) < 0) {
		goto err;
	}
	log_printf(LOG_INFO, "Found api base url: %s\n", builder_config.abf_api_url);
	if (get_query_string(&config) < 0) {
		goto err;
	}
	init_api(builder_config.abf_api_url, builder_config.api_token, builder_config.query_string);

	if (init_strings(&config) < 0) {
		goto err;
	}
	if (get_platform_list(&config) < 0) {
		goto err;
	}

	if(start_statistics_thread(builder_config.query_string)) {
		log_printf(LOG_FATAL, "Failed to initialize statistics thread.\n");
		goto err;
	}

	config_destroy(&config);
	return 0;
	err:
	config_destroy(&config);
	return -1;
}

static int get_api_info(config_t *config) {
	int req_res;
	char *tmp;

	req_res = config_lookup_string(config, api_url_path, (const char **)&tmp);
	if(!req_res) {
		log_printf(LOG_FATAL, "%s must be set.\n", api_url_path);
		return -1;
	}
	builder_config.abf_api_url = strdup(tmp);

	req_res = config_lookup_string(config, file_store_url_path, (const char **)&tmp);
	if(!req_res) {
		log_printf(LOG_FATAL, "%s must be set.\n", file_store_url_path);
		return -1;
	}
	builder_config.file_store_url = strdup(tmp);

	req_res = config_lookup_string(config, build_token_path, (const char **)&tmp);
	if(!req_res) {
		tmp = getenv(build_token_env);
		if(tmp == NULL) {
			log_printf(LOG_FATAL, "%s or env variable %s must be set.\n", build_token_path, build_token_env);
			return -1;
		}
	}
	builder_config.api_token = strdup(tmp);

	return 0;
}

static int get_query_string(config_t *config) {
	char *tmp, *arches, *native_arches, *platforms, *platform_types;

	int supported_arches_exist = config_lookup_string(
		config,
		supported_arches_path,
		(const char **)&arches
	);
	if(!supported_arches_exist) {
		tmp = getenv(supported_arches_env);
		if(tmp != NULL) {
			arches = strdup(tmp);
			supported_arches_exist = 1;
		}
	}
	int native_arches_exist = config_lookup_string(
		config,
		native_arches_path,
		(const char **)&native_arches
	);
	if(!native_arches_exist) {
		tmp = getenv(native_arches_env);
		if(tmp != NULL) {
			native_arches = strdup(tmp);
			native_arches_exist = 1;
		}
	}
	int supported_platforms_exist = config_lookup_string(
		config,
		supported_platforms_path,
		(const char **)&platforms
	);
	if(!supported_platforms_exist) {
		tmp = getenv(supported_platforms_env);
		if(tmp != NULL) {
			platforms = strdup(tmp);
			supported_platforms_exist = 1;
		}
	}
	int supported_platform_types_exist = config_lookup_string(
		config,
		supported_platform_types_path,
		(const char **)&platform_types
	);
	if(!supported_platform_types_exist) {
		tmp = getenv(supported_platform_types_env);
		if(tmp != NULL) {
			platform_types = strdup(tmp);
			supported_platform_types_exist = 1;
		}
	}
	int len = (supported_arches_exist ? strlen(arches) : 0) +
						(native_arches_exist ? strlen(native_arches) : 0) +
						(supported_platforms_exist ? strlen(platforms) : 0) +
						(supported_platform_types_exist ? strlen(platform_types) : 0);

	char *query_string;
	if(len) {
		char *pointer;
		query_string = xmalloc(len + 80);
		pointer = query_string;
		if(supported_arches_exist) {
			pointer += sprintf(pointer, "arches=%s&", arches);
			log_printf(LOG_INFO, "Found supported arches: %s\n", arches);
		}
		if(native_arches_exist) {
			pointer += sprintf(pointer, "native_arches=%s&", native_arches);
			log_printf(LOG_INFO, "Found native arches: %s\n", native_arches);
		}
		if(supported_platforms_exist) {
			pointer += sprintf(pointer, "platforms=%s&", platforms);
			log_printf(LOG_INFO, "Found supported platforms: %s\n", platforms);
		}
		if(supported_platform_types_exist) {
			pointer += sprintf(pointer, "platform_types=%s", platform_types);
			log_printf(LOG_INFO, "Found supported platform types: %s\n", platform_types);
		}

		pointer-=1;
		if(*pointer == '&') {
			*pointer = '\0';
		}
	}
	else {
		query_string = NULL;
	}

	builder_config.query_string = query_string;
	return 0;
}

static int init_strings(config_t *config) {
	char *work_dir, *git_scripts_dir;

	DIR *test;
	int work_dir_exists = config_lookup_string(
		config,
		work_dir_path,
		(const char **)&work_dir
	);
	if (!work_dir_exists) {
		builder_config.work_dir = strdup(default_work_dir);
	} else {
		if (work_dir[strlen(work_dir) - 1] == '/') {
			work_dir[strlen(work_dir) - 1] = '\0';
		}
		builder_config.work_dir = strdup(work_dir);
	}
	test = opendir(builder_config.work_dir);
	if (test == NULL) {
		log_printf(LOG_FATAL, "Can't access work directory %s: %s\n", builder_config.work_dir, strerror(errno));
		return -1;
	}	else {
		closedir(test);
	}

	builder_config.output_dir = xmalloc(strlen(output_fmt) + strlen(builder_config.work_dir));
	sprintf(builder_config.output_dir, output_fmt, builder_config.work_dir);

	log_printf(LOG_DEBUG, "Work directory is %s\n", builder_config.work_dir);
	log_printf(LOG_DEBUG, "Output directory is %s\n", builder_config.output_dir);

	int git_scripts_dir_exists = config_lookup_string(
		config,
		git_scripts_dir_path,
		(const char **)&git_scripts_dir
	);
	if (!git_scripts_dir_exists) {
		builder_config.git_scripts_dir = strdup(default_git_scripts_dir);
	} else {
		if (git_scripts_dir[strlen(git_scripts_dir) - 1] == '/') {
			git_scripts_dir[strlen(git_scripts_dir) - 1] = '\0';
		}
		builder_config.git_scripts_dir = strdup(git_scripts_dir);
	}

	test = opendir(builder_config.git_scripts_dir);
	if (test == NULL) {
		log_printf(LOG_FATAL, "Can't access git scripts directory %s: %s\n", builder_config.git_scripts_dir, strerror(errno));
		return -1;
	} else {
		closedir(test);
	}

	log_printf(LOG_DEBUG, "Git scripts directory is %s\n", builder_config.git_scripts_dir);

	char hostname[128];
	if(gethostname(hostname, 128) < 0) {
		hostname[127] = '\0';
	}
	char *tmp;
	tmp = xmalloc(strlen(hostname) + strlen(hostname_payload_fmt) + 1);
	sprintf(tmp, hostname_payload_fmt, hostname);
	log_printf(LOG_DEBUG, "hostname_payload is %s\n", tmp);
	builder_config.strings.hostname_payload = tmp;

	tmp = xmalloc(strlen(move_output_cmd_fmt) + strlen(builder_config.work_dir) + strlen(builder_config.output_dir) + 1);
	sprintf(tmp, move_output_cmd_fmt, builder_config.work_dir, builder_config.output_dir);
	log_printf(LOG_DEBUG, "move_output_cmd is %s\n", tmp);
	builder_config.strings.move_output_cmd = tmp;

	tmp = xmalloc(strlen(container_data_path_fmt) + strlen(builder_config.output_dir) + 1);
	sprintf(tmp, container_data_path_fmt, builder_config.output_dir);
	log_printf(LOG_DEBUG, "container_data_path is %s\n", tmp);
	builder_config.strings.container_data_path = tmp;

	tmp = xmalloc(strlen(upload_cmd_fmt) + strlen(builder_config.api_token) + strlen(builder_config.output_dir) + 1);
	sprintf(tmp, upload_cmd_fmt, builder_config.api_token, builder_config.output_dir);
	log_printf(LOG_DEBUG, "upload_cmd is %s\n", tmp);
	builder_config.strings.upload_cmd = tmp;

	tmp = xmalloc(strlen(commit_hash_path_fmt) + strlen(builder_config.work_dir) + 1);
	sprintf(tmp, commit_hash_path_fmt, builder_config.work_dir);
	log_printf(LOG_DEBUG, "commit_hash_path is %s\n", tmp);
	builder_config.strings.commit_hash_path = tmp;

	tmp = xmalloc(strlen(fail_reason_path_fmt) + strlen(builder_config.work_dir) + 1);
	sprintf(tmp, fail_reason_path_fmt, builder_config.work_dir);
	log_printf(LOG_DEBUG, "fail_reason_path is %s\n", tmp);
	builder_config.strings.fail_reason_path = tmp;

	return 0;
}

static int get_platform_list(config_t *config) {
	config_setting_t *platforms_list = config_lookup(config, platforms_path);
	if (platforms_list == NULL) {
		log_printf(LOG_FATAL, "No scripts found in the config.\n");
		return -1;
	}
	if (config_setting_type(platforms_list) != CONFIG_TYPE_GROUP) {
		log_printf(LOG_FATAL, "%s must be a group.\n", platforms_path);
		return -1;
	}

	int len = config_setting_length(platforms_list);
	builder_config.builder_scripts_len = len;

	builder_config.builder_scripts = xmalloc(sizeof(builder_t) * len);
	for (int i = 0; i < len; i++) {
		config_setting_t *platform = config_setting_get_elem(platforms_list, i);
		char *name = config_setting_name(platform);
		if (config_setting_type(platforms_list) != CONFIG_TYPE_GROUP) {
			log_printf(LOG_FATAL, "%s must be a group.\n", name);
			return -1;
		}
		builder_config.builder_scripts[i].type = strdup(name);
		const char *git, *path;
		int git_exists = config_setting_lookup_string(platform, "git", &git);
		int path_exists = config_setting_lookup_string(platform, "path", &path);
		
		char *platform_path;
		if (git_exists && path_exists) {
			log_printf(LOG_FATAL, "Platform %s: Only one of git, path is expected.\n", name);
			return -1;
		} else if(git_exists && !path_exists) {
			const char *branch;
			int branch_exists = config_setting_lookup_string(platform, "branch", &branch);
			if (!branch_exists) {
				branch = default_branch;
			}
			if (load_scripts(git, branch, name) < 0) {
				return -1;
			}
			platform_path = xmalloc(strlen(builder_config.git_scripts_dir) + strlen(name) + 2);
			sprintf(platform_path, "%s/%s", builder_config.git_scripts_dir, name);
			builder_config.builder_scripts[i].is_git = 1;
			builder_config.builder_scripts[i].branch = strdup(branch);
		} else if(!git_exists && path_exists) {
			platform_path = strdup(path);
			builder_config.builder_scripts[i].is_git = 0;
		} else {
			log_printf(LOG_FATAL, "Platform %s: One of git, path is expected.\n", name);
			return -1;
		}

		const char *script_name, *interpreter;
		int script_name_exists = config_setting_lookup_string(platform, "script_name", &script_name);
		if (!script_name_exists) {
			script_name = default_script_name;
		}
		int interpreter_exists = config_setting_lookup_string(platform, "interpreter", &interpreter);
		if (!interpreter_exists) {
			interpreter = default_interpreter;
		}
		char *script_path = xmalloc(strlen(platform_path) + 1 + strlen(script_name) + 1);
		sprintf(script_path, "%s/%s", platform_path, script_name);
		char *real_script_path = realpath(script_path, NULL);
		if (!real_script_path) {
			log_printf(LOG_FATAL, "Path %s: %s\n", script_path, strerror(errno));
			return -1;
		}
		char *cmd = xmalloc(strlen(interpreter) + 1 + strlen(real_script_path) + 1);
		sprintf(cmd, "%s %s", interpreter, real_script_path);
		builder_config.builder_scripts[i].cmd = cmd;
		free(script_path);
		free(real_script_path);
		free(platform_path);
		log_printf(LOG_DEBUG, "Platform %s: command is %s\n", name, cmd);

		const char *run_as_user, *run_as_group;
		int run_as_user_exists = config_setting_lookup_string(platform, "run_as_user", &run_as_user);
		if (!run_as_user_exists) {
			run_as_user = default_run_as_user;
		}
		int run_as_group_exists = config_setting_lookup_string(platform, "run_as_group", &run_as_group);
		if (!run_as_group_exists) {
			run_as_group = default_run_as_group;
		}
		struct passwd *pwd = getpwnam(run_as_user);
		if (pwd == NULL) {
			log_printf(LOG_FATAL, "Platform %s: Error retrieving user %s: %s\n", name, run_as_user, strerror(errno));
			return -1;
		}
		builder_config.builder_scripts[i].run_as_uid = pwd->pw_uid;
		struct group *grp = getgrnam(run_as_group);
		if (grp == NULL) {
			log_printf(LOG_FATAL, "Platform %s: Error retrieving group %s: %s\n", name, run_as_group, strerror(errno));
			return -1;
		}
		builder_config.builder_scripts[i].run_as_gid = grp->gr_gid;
		log_printf(LOG_DEBUG, "Platform %s: run script as uid %d, gid %d\n", name, pwd->pw_uid, grp->gr_gid);
	}
	return 0;
}

static int load_scripts(const char *git_repo, const char *git_branch, const char *platform_type) {
  size_t len = strlen(clone_cmd) + strlen(builder_config.git_scripts_dir) + strlen(git_repo) + strlen(git_branch) + strlen(platform_type);
	char *cmd = xmalloc(len);
	int res;

	sprintf(cmd, clone_cmd, builder_config.git_scripts_dir, platform_type, git_branch, git_repo, platform_type);
  char *output;
	res = system_with_output(cmd, &output);
	if (res < 0) {
		log_printf(LOG_FATAL, "Failed to start command %s.\n", cmd);
		if (output != NULL) {
			free(output);
		}
		return -1;
	}
  int success = 0;
  if (!WIFEXITED(res) || WEXITSTATUS(res) != 0) {
    log_printf(LOG_FATAL, "Error cloning %s, git output:\n", git_repo);
    log_printf(LOG_FATAL, output);
    success = -1;
  }
  free(cmd);
  free(output);
	return success;
}
