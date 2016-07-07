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
#include "builder.h"

static void term_handler(int signum) {
	exit(BUILD_CANCELED);
}

static int exec_init(void *arg) {
	exec_data *data = (exec_data *)arg;

	//remount proc and clear build dir
	mount("", "/proc", "proc", MS_NOEXEC | MS_NOSUID | MS_NODEV, "");
	pid_t pid = fork();

	if(pid > 0) {
		int status, build_status;
		struct sigaction sig;
		sigset_t sigset;
		sigemptyset(&sigset);

		sig.sa_handler = &term_handler;
		sig.sa_mask = sigset;
		sig.sa_flags = 0;

		sigaction(SIGTERM, &sig, NULL);

		wait(&status);

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

		return build_status;
	}
	else {
		setuid(1000);
		setgid(1000);
		const char *logfile_name = "/home/omv/script_output.log";
		int fd = open(logfile_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
		char *script_path = malloc(strlen(data->distrib_type) + strlen("//build-rpm.sh") + 1);
		sprintf(script_path, "/%s/build-rpm.sh", data->distrib_type);
		execle("/bin/bash", "bash", "-c", script_path, NULL, data->env);
	}

	return 0;
}

pid_t exec_build(const char *distrib_type, const char **env) {
	pid_t pid;
	char *stack = malloc(1048576);
	exec_data *data = malloc(sizeof(exec_data));
	data->distrib_type = distrib_type;
	data->env = env;
	pid = clone(exec_init, stack + 1048572, CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, data);

	return pid;
}