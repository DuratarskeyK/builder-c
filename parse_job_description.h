#ifndef _PARSE_JOB_DESCRIPTION_H
#define _PARSE_JOB_DESCRIPTION_H

#include "structs.h"

int parse_job_description(const char *js, char **build_id, int *ttl, char **distrib_type, char ***env);
inline static char *make_env(const char *key, const char *value);

#define COMPARE(var, str, len) (strlen(str) == (len) && !strncmp(var, str, len))

extern void *xmalloc(size_t size);

extern char *alloc_sprintf(const char *message, ...);

extern builder_config_t builder_config;

#endif
