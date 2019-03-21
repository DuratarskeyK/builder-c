#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include <string.h>
#include <errno.h>
#include "config.h"

int process_config(char **abf_api_url, char **api_token, char **query_string) {
	FILE *config_file;
	config_t config;

	config_file = fopen("/etc/builder-c/builder.conf", "r");
	if(config_file == NULL) {
		log_printf(LOG_FATAL, "Error openning config file: %s.\n", strerror(errno));
		return -1;
	}
	config_init(&config);
	if(!config_read(&config, config_file)) {
		log_printf(LOG_FATAL, "Error parsing config file:\n");
		int line = config_error_line(&config);
		const char *text = config_error_text(&config);
		log_printf(LOG_FATAL, "Line #%d: %s.\n", line, text);
		config_destroy(&config);
		return -1;
	}
	fclose(config_file);

	log_printf(LOG_INFO, "Config parsed successfuly.\n");

	char *tmp, *arches, *native_arches, *platforms, *platform_types;

	int req_res;
	req_res = config_lookup_string(&config, api_url_path, (const char **)&tmp);
	if(!req_res) {
		log_printf(LOG_FATAL, "%s must be set.\n", api_url_path);
		config_destroy(&config);
		return -1;
	}
	*abf_api_url = strdup(tmp);
	req_res = config_lookup_string(&config, build_token_path, (const char **)&tmp);
	if(!req_res) {
		tmp = getenv(build_token_env);
		if(tmp == NULL) {
			log_printf(LOG_FATAL, "%s or env variable %s must be set.\n", build_token_path, build_token_env);
  		config_destroy(&config);
			return -1;
		}
	}
	*api_token = strdup(tmp);

	log_printf(LOG_INFO, "Found api base url: %s\n", *abf_api_url);
	log_printf(LOG_INFO, "Found build token: [REDACTED]\n");

	int supported_arches_exist = config_lookup_string(&config, supported_arches_path, (const char **)&arches);
	if(!supported_arches_exist) {
		tmp = getenv(supported_arches_env);
		if(tmp != NULL) {
			arches = strdup(tmp);
			supported_arches_exist = 1;
		}
	}
	int native_arches_exist = config_lookup_string(&config, native_arches_path, (const char **)&native_arches);
	if(!native_arches_exist) {
		tmp = getenv(native_arches_env);
		if(tmp != NULL) {
			native_arches = strdup(tmp);
			native_arches_exist = 1;
		}
	}
	int supported_platforms_exist = config_lookup_string(&config, supported_platforms_path, (const char **)&platforms);
	if(!supported_platforms_exist) {
		tmp = getenv(supported_platforms_env);
		if(tmp != NULL) {
			platforms = strdup(tmp);
			supported_platforms_exist = 1;
		}
	}
	int supported_platform_types_exist = config_lookup_string(&config, supported_platform_types_path, (const char **)&platform_types);
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

	if(len) {
		char *pointer;
		*query_string = malloc(len + 80);
		pointer = *query_string;
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
		*query_string = NULL;
	}

	config_setting_t *platforms_list = config_lookup(&config, platforms_list_path);
	if(platforms_list != NULL) {
		len = config_setting_length(platforms_list);
		for(int i = 0; i < len; i++) {
			const char *platform_type, *git_path, *git_branch;
			int git_exists, branch_exists;
			platform_type = config_setting_get_string_elem(platforms_list, i);

			char *config_path = malloc(strlen(platform_branch_path) + strlen(platform_type));
			sprintf(config_path, platform_git_path, platform_type);
			git_exists = config_lookup_string(&config, config_path, &git_path);

			sprintf(config_path, platform_branch_path, platform_type);
			branch_exists = config_lookup_string(&config, config_path, &git_branch);

			free(config_path);
			if(git_exists == CONFIG_FALSE) {
				continue;
			}
			else {
        log_printf(LOG_INFO, "Cloning scripts for platform type %s from %s with branch %s\n",
        platform_type, git_path, (branch_exists ? git_branch : "master"));
				if(!load_scripts(git_path, (branch_exists ? git_branch : NULL), platform_type)) {
        	config_destroy(&config);
          return -1;
        }
			}
		}
	}

	config_destroy(&config);
	return 0;
}

static int load_scripts(const char *git_repo, const char *git_branch, const char *platform_type) {
  size_t len = strlen(clone_branch_cmd) + strlen(git_repo) + strlen(platform_type);
  if (git_branch) {
    len += strlen(git_branch);
  }
	char *cmd = malloc(len);
	int res;

	if(git_branch != NULL) {
		sprintf(cmd, clone_branch_cmd, platform_type, git_branch, git_repo, platform_type);
	}
	else {
		sprintf(cmd, clone_cmd, platform_type, git_repo, platform_type);
	}
  char *output;
	res = system_with_output(cmd, &output);
  int success = 1;
  if (!WIFEXITED(res) || WEXITSTATUS(res) != 0) {
    log_printf(LOG_FATAL, "Error cloning %s, git output:\n", git_repo);
    log_printf(LOG_FATAL, output);
    success = 0;
  }
  free(cmd);
  free(output);
	return success;
}
