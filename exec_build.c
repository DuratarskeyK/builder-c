#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>
#include "exec_build.h"

// 128 kbytes
#define STACK_SIZE 131072

static char *argv[] = {
	"bash",
	"-lc",
	"--",
	NULL,
	NULL
};

static int exec_init(void *arg) {
	exec_data_t *data = (exec_data_t *)arg;

	setgid(data->omv_mock.mock_gid);
	setuid(data->omv_mock.omv_uid);
	execvpe("/bin/bash", argv, data->env);

	return 255;
}

child exec_build(const char *distrib_type, char * const *env, usergroup omv_mock) {
	pid_t pid;
	unsigned int *stack = malloc(STACK_SIZE);
	int pfd[2];
	child ret;
	pipe(pfd);

	// clean output directory
	system("rm -rf /home/omv/output/*");

	exec_data_t *data = malloc(sizeof(exec_data_t));
	data->env = env;
	data->omv_mock = omv_mock;
	unsigned long stack_aligned = ((unsigned long)stack + STACK_SIZE) & (~0xF);
	stack_aligned -= 0x10;

	char *cmd = malloc(strlen(cmd_fmt) + strlen(distrib_type) + 128);
	sprintf(cmd, cmd_fmt, distrib_type, pfd[1], pfd[1]);
	argv[3] = cmd;

	pid = clone(exec_init, (unsigned char *)stack_aligned, CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, data);

	free(cmd);
	free(data);
	free(stack);
	close(pfd[1]);

	ret.pid = pid;
	ret.read_fd = pfd[0];

	return ret;
}
