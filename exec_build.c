#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>
#include <errno.h>
#include "exec_build.h"

// 128 kbytes
#define STACK_SIZE 131072

static char *argv[] = {
	"sh",
	"-lc",
	"--",
	NULL,
	NULL
};

static int exec_init(void *arg) {
	exec_data_t *data = (exec_data_t *)arg;

	if (setgid(data->gid) < 0) {
		printf("setgid failed\n");
		return 255;
	}
	if (setuid(data->uid) < 0) {
		printf("setuid failed\n");
		return 255;
	}
	execvpe("/bin/sh", argv, data->env);

	return 255;
}

child_t *exec_build(const char *distrib_type, char * const *env) {
	pid_t pid;

	int pfd[2];
	int res = pipe(pfd);
	if (res < 0) {
		log_printf(LOG_ERROR, "Error creating pipe: %s\n", strerror(errno));
		return NULL;
	}

	unsigned int *stack = xmalloc(STACK_SIZE);
	unsigned long stack_aligned = ((unsigned long)stack + STACK_SIZE) & (~0xF);
	stack_aligned -= 0x10;

	builder_t *platform = NULL;
	for (int i = 0; i < builder_config.builder_scripts_len; i++) {
		if (!strcmp(builder_config.builder_scripts[i].type, distrib_type)) {
			platform = &(builder_config.builder_scripts[i]);
			break;
		}
	}

	if (platform == NULL) {
		log_printf(LOG_ERROR, "No build scripts for platform type %s.\n", distrib_type);
		return NULL;
	}

	if (platform->is_git) {
		log_printf(LOG_INFO, "Updating build scripts.\n");
		char *update_cmd = alloc_sprintf(update_scripts_cmd, builder_config.git_scripts_dir, platform->type, platform->branch);
		char *output;
		int exit_code = system_with_output(update_cmd, &output);
		if (exit_code < 0) {
			log_printf(LOG_ERROR, "Error starting command %s\n", update_cmd);
		} else if (!WIFEXITED(exit_code) || WEXITSTATUS(exit_code) != 0) {
			log_printf(LOG_ERROR, "Error updating git scripts. Output:\n%s\n", output);
		}
		if (output != NULL) {
			free(output);
		}
		free(update_cmd);
	}

	char *cmd = alloc_sprintf(cmd_fmt, platform->cmd, pfd[1], pfd[1]);
	argv[3] = cmd;

	exec_data_t *data = xmalloc(sizeof(exec_data_t));
	data->env = env;
	data->uid = platform->run_as_uid;
	data->gid = platform->run_as_gid;

	pid = clone(exec_init, (unsigned char *)stack_aligned, CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, data);
	if (pid < 0) {
		log_printf(LOG_ERROR, "Error starting %s: %s\n", cmd, strerror(errno));
		return NULL;
	}

	free(cmd);
	free(data);
	free(stack);
	close(pfd[1]);

	child_t *ret = xmalloc(sizeof(child_t));
	ret->pid = pid;
	ret->read_fd = pfd[0];

	return ret;
}
