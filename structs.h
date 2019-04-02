#ifndef _BUILDER_H
#define _BUILDER_H
#include <unistd.h>

typedef struct {
	int time_to_live;
	pid_t pid;
	const char *build_id;
} li_data;

typedef struct {
	uid_t omv_uid;
	gid_t mock_gid;
} usergroup;

typedef struct {
	pid_t pid;
	int read_fd;
} child;

#endif
