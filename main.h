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

#endif
