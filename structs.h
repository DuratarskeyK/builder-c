#ifndef _BUILDER_H
#define _BUILDER_H
#include <unistd.h>

typedef struct {
	int time_to_live;
	pid_t pid;
	const char *build_id;
} li_data;

typedef struct {
	pid_t pid;
	int read_fd;
} child_t;

typedef struct
{
	char *hostname_payload;
	char *move_output_cmd;
	char *container_data_path;
	char *upload_cmd;
	char *commit_hash_path;
	char *fail_reason_path;
} strings_t;

typedef struct {
	char *type;
	char *cmd;
	uid_t run_as_uid;
	gid_t run_as_gid;
} builder_t;

typedef struct {
	char *abf_api_url;
	char *query_string;
	char *api_token;

	char *file_store_url;

	char *work_dir;
	char *output_dir;
	char *git_scripts_dir;

	strings_t strings;
	
	int builder_scripts_len;
	builder_t *builder_scripts;
} builder_config_t;

#endif
