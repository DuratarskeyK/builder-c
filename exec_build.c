#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sched.h>
#include <signal.h>
#include "exec_build.h"

static void term_handler(__attribute__((unused)) int signum) {
	kill(-1, SIGTERM);
	wait(NULL);
	umount("/proc");
	exit(BUILD_CANCELED);
}

static int exec_init(void *arg) {
	exec_data *data = (exec_data *)arg;

	//remount proc and clear build dir
	mount("", "/proc", "proc", MS_PRIVATE, "");
	system("rm -rf /home/omv/output/*");
	pid_t pid = fork();

	if(pid > 0) {
		int status = 0;
		int build_status = BUILD_FAILED;
		struct sigaction sig;
		sigset_t sigset;
		sigemptyset(&sigset);

		sig.sa_handler = &term_handler;
		sig.sa_mask = sigset;
		sig.sa_flags = 0;

		sigaction(SIGTERM, &sig, NULL);

		waitpid(pid, &status, 0);

		if(WIFEXITED(status)) {
			status = WEXITSTATUS(status);
			if(status == 5) {
				build_status = TESTS_FAILED;
			}
			else if(status) {
				build_status = BUILD_FAILED;
			}
			else {
				build_status = BUILD_COMPLETED;
			}
		}

		umount("/proc");
		return build_status;
	}
	else {
		setgid(data->omv_mock.mock_gid);
		setuid(data->omv_mock.omv_uid);
		dup2(data->write_fd, 1);
		dup2(data->write_fd, 2);
		close(data->write_fd);
		char *script_path = malloc(strlen(data->distrib_type) + strlen("//build-rpm.sh") + 1);
		sprintf(script_path, "/%s/build-rpm.sh", data->distrib_type);
		execle("/bin/bash", "bash", "-lc", "--", script_path, NULL, data->env);
	}

	return 0;
}

child exec_build(const char *distrib_type, const char **env, usergroup omv_mock) {
	pid_t pid;
	unsigned int *stack = malloc(1048576 * 2);
	int pfd[2];
	child ret;
	pipe(pfd);

	exec_data *data = malloc(sizeof(exec_data));
	data->distrib_type = distrib_type;
	data->env = env;
	data->write_fd = pfd[1];
	data->omv_mock = omv_mock;
	unsigned long stack_aligned = ((unsigned long)stack + 1048576 * 2) & (~0xF);
	stack_aligned -= 0x10;
	pid = clone(exec_init, (unsigned char *)stack_aligned, CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, data);
	free(data);
	close(pfd[1]);
	ret.pid = pid;
	ret.read_fd = pfd[0];
	ret.stack = stack;

	return ret;
}
