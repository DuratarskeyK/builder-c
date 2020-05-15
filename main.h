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

static const char build_started_args_fmt[] = "{\"hostname\":\"%s\",\"id\":%s,\"status\":%d}";
static const char build_completed_args_fmt[] = "{\"results\":%s,\"packages\":%s,\"exit_status\":%d,\"commit_hash\":\"%s\",\"fail_reason\":\"%s\",\"id\":%s,\"status\":%d}";

static char *read_file(const char *);

extern int init(char *config_path);

extern child_t *exec_build(const char *, char * const *);

extern int api_jobs_shift(char **);
extern int api_jobs_feedback(const char *);

extern void set_busy_status(int s, const char *build_id);

extern int start_live_logger(const char *, int);
extern void stop_live_logger();

extern int start_live_inspector(int, pid_t, const char *);
extern int stop_live_inspector();

extern void log_printf(unsigned int level, const char *message, ...);

extern int parse_job_description(const char *, char **, int *, char **, char ***);

extern int filestore_upload(char **results);

extern char *alloc_sprintf(const char *message, ...);

extern void *xmalloc(size_t size);

extern builder_config_t builder_config;

#endif
