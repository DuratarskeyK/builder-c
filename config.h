#ifndef _CONFIG_H
#define _CONFIG_H

#include "log_levels.h"

static const char api_url_path[] = "application.abf.api_url";
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

static const char platforms_list_path[] = "application.platforms.list";
static const char platform_git_path[] = "application.platforms.%s.git";
static const char platform_branch_path[] = "application.platforms.%s.branch";

static const char clone_cmd[] = "cd /; sudo rm -rf %s; sudo git clone %s %s";
static const char clone_branch_cmd[] = "cd /; sudo rm -rf %s; sudo git clone -b %s %s %s";

int process_config(char **abf_api_url, char **api_token, char **query_string);
static int load_scripts(const char *git_repo, const char *git_branch, const char *platform_type);

extern void log_printf(unsigned int level, const char *message, ...);
extern int system_with_output(const char *cmd, char **output);

extern void *xmalloc(size_t size);

#endif
