#ifndef _MAIN_H
#define _MAIN_H

#include "structs.h"
#include "api_data.h"
#include "build_statuses.h"
#include "jsmn.h"

#define COMPARE(var, str, len) (strlen(str) == (len) && !strncmp(var, str, len))

usergroup get_omv_uid_mock_gid();
char *read_file(const char *);
int process_config(char **, char **, char **);
int load_scripts(const char *, const char *, const char *);
char *create_output_directory();
int parse_job_description(const char *, char **, int *, char **, char ***);
char *make_env(const char *, const char *);

extern child exec_build(const char *, const char **, usergroup);

extern void api_set_token(const char *);
extern void api_set_api_url(const char *);

extern int start_statistics_thread(const char *);
extern void set_busy_status(int s);

extern int api_jobs_shift(char **, const char *);
extern int api_jobs_feedback(const char *, int, const char *);

extern int start_live_logger(const char *, int);
extern void stop_live_logger();

extern int start_live_inspector(int, pid_t, const char *);
extern void stop_live_inspector();

#endif
