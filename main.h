#ifndef _MAIN_H
#define _MAIN_H

#include "structs.h"
#include "build_statuses.h"
#include "log_levels.h"

static const char *build_statuses_str[] = {"BUILD_COMPLETED",
                                           "BUILD_FAILED",
                                           "BUILD_PENDING",
                                           "BUILD_STARTED",
                                           "BUILD_CANCELED",
                                           "TESTS_FAILED"};

static const char hostname_payload_fmt[] = "{\"hostname\":\"%s\"}";
static const char move_output_cmd_fmt[] = "mv /tmp/script_output.log %s/script_output.log";
static const char container_data_path_fmt[] = "%s/container_data.json";
static const char commit_hash_path_fmt[] = "%s/../commit_hash";
static const char upload_cmd_fmt[] = "/bin/bash /etc/builder-c/filestore_upload.sh %s %s";
static const char fail_reason_path_fmt[] = "%s/../build_fail_reason.log";

static const char build_completed_args_fmt[] = "{\"results\":%s,\"packages\":%s,\"exit_status\":%d,\"commit_hash\":\"%s\",\"fail_reason\":\"%s\"}";

static void init_strings(const char *api_token);
static usergroup get_omv_uid_mock_gid();
static char *read_file(const char *);

extern child exec_build(const char *, const char **, usergroup);

extern void init_api(const char *url, const char *api_token);
extern int api_jobs_shift(char **, const char *);
extern int api_jobs_feedback(const char *, int, const char *);

extern int start_statistics_thread(const char *);
extern void set_busy_status(int s, const char *build_id);

extern int start_live_logger(const char *, int);
extern void stop_live_logger();

extern int start_live_inspector(int, pid_t, const char *);
extern int stop_live_inspector();

extern int init_logger(const char *level);
extern void register_thread(const char *name);
extern void log_printf(unsigned int level, const char *message, ...);

extern int check_dns();

extern int process_config(char **abf_api_url, char **api_token, char **query_string);

extern int system_with_output(const char *cmd, char **output);

extern int parse_job_description(const char *, char **, int *, char **, char ***);

#endif
