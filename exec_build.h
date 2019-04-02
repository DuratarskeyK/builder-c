#ifndef _EXEC_BUILD_H
#define _EXEC_BUILD_H

#include "structs.h"

static const char cmd_fmt[] = "/bin/bash /%s/build-rpm.sh 1>&%d 2>&%d";

typedef struct {
	char * const *env;
	usergroup omv_mock;
} exec_data_t;

child exec_build(const char *, char * const *, usergroup);

#endif
