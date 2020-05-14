#ifndef _INIT_H
#define _INIT_H

#include "log_levels.h"
#include "structs.h"

static const char api_url_path[] = "application.abf.api_url";
static const char file_store_url_path[] = "application.abf.file_store_url";
static const char build_token_path[] = "application.abf.token";
static const char build_token_env[] = "BUILD_TOKEN";

static const char supported_arches_path[] = "application.abf.supported_arches";
static const char supported_arches_env[] = "BUILD_ARCH";

static const char native_arches_path[] = "application.abf.native_arches";
static const char native_arches_env[] = "NATIVE_ARCH";

static const char supported_platforms_path[] = "application.abf.supported_platforms";
static const char supported_platforms_env[] = "BUILD_PLATFORM";

static const char supported_platform_types_path[] = "application.abf.supported_platform_types";
static const char supported_platform_types_env[] = "BUILD_PLATFORM_TYPE";

static const char log_level_path[] = "application.logger.level";
static const char log_level_env[] = "LOG_LEVEL";
static const char default_log_level[] = "INFO";

static const char work_dir_path[] = "application.builder.work_dir";
static const char default_work_dir[] = "/home/omv";
static const char git_scripts_dir_path[] = "application.builder.git_scripts";
static const char default_git_scripts_dir[] = "/etc/builder-c";

static const char output_fmt[] = "%s/output";
static const char hostname_payload_fmt[] = "{\"hostname\":\"%s\"}";
static const char move_output_cmd_fmt[] = "mv %s/script_output.log %s/script_output.log";
static const char container_data_path_fmt[] = "%s/container_data.json";
static const char commit_hash_path_fmt[] = "%s/commit_hash";
static const char upload_cmd_fmt[] = "/bin/bash /etc/builder-c/filestore_upload.sh %s %s";
static const char fail_reason_path_fmt[] = "%s/build_fail_reason.log";

static const char platforms_path[] = "application.platforms";
static const char default_branch[] = "master";
static const char default_interpreter[] = "/usr/bin/python3";
static const char default_script_name[] = "build-rpm.py";
static const char default_run_as_user[] = "omv";
static const char default_run_as_group[] = "mock";

static const char clone_cmd[] = "cd %s; sudo rm -rf %s; sudo git clone -b %s %s %s";

int init(char *config_path);
static int get_query_string(config_t *config);
static int get_api_info(config_t *config);
static int get_query_string(config_t *config);
static int init_strings(config_t *config);
static int get_platform_list(config_t *config);
static int load_scripts(const char *git_repo, const char *git_branch, const char *platform_type);

extern void init_api(const char *url, const char *tok, const char *qs);

extern int start_statistics_thread(const char *);

extern int check_dns();

extern int thread_setup();

extern int init_logger(const char *level);
extern void register_thread(const char *name);
extern void log_printf(unsigned int level, const char *message, ...);

extern int system_with_output(const char *cmd, char **output);

extern void *xmalloc(size_t size);

#endif
