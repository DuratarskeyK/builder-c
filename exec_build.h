#ifndef _EXEC_BUILD_H
#define _EXEC_BUILD_H

#include "structs.h"
#include "log_levels.h"

static const char cmd_fmt[] = "%s 1>&%d 2>&%d";

typedef struct {
	char * const *env;
	gid_t gid;
	uid_t uid;
} exec_data_t;

child_t *exec_build(const char *, char * const *);

extern void *xmalloc(size_t size);

extern void log_printf(unsigned int level, const char *message, ...);

extern builder_config_t builder_config;

#endif
