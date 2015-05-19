/*
 * Copyright (C) 2015 IBM Corporation.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include "utils.h"

static int process_child(char *argv[], int pipefd[])
{
	int	nullfd;

	close(pipefd[0]);

	/* stderr to /dev/null redirection */
	nullfd = open("/dev/null", O_WRONLY);
	if (nullfd == -1) {
		fprintf(stderr, "%s : %d - failed to open "
				"\'/dev/null\' for redirection : %s\n",
				__func__, __LINE__, strerror(errno));
		close(pipefd[1]);
		return -1;
	}

	if (dup2(nullfd, STDERR_FILENO) == -1) {
		fprintf(stderr, "%s : %d - failed to redirect "
				"\'/dev/null\' to stderr : %s\n",
				__func__, __LINE__, strerror(errno));
		goto err_out;
	}

	/* redirect stdout to write-end of the pipe */
	if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
		fprintf(stderr, "%s : %d - failed to redirect "
				"pipe write fd to to stdout : %s\n",
				__func__, __LINE__, strerror(errno));
		goto err_out;
	}

	execve(argv[0], argv, NULL);
	/* some failure in exec */

err_out:
	close(pipefd[1]);
	close(nullfd);
	return -1;
}

/*
 * This function mimics popen(3).
 *
 * Returns:
 *   NULL, if fork(2), pipe(2) and dup2(3) calls fail
 *
 * Note:
 *   fclose(3) function shouldn't be used to close the stream
 *   returned here, since it doesn't wait for the child to exit.
 */
FILE *spopen(char *argv[], pid_t *ppid)
{
	FILE	*fp = NULL;
	int	pipefd[2];
	pid_t	cpid;

	if (argv == NULL)
		return fp;

	if (access(argv[0], F_OK|X_OK) != 0) {
		fprintf(stderr, "%s : The command \"%s\" is not executable.\n",
			__func__, argv[0]);
		return fp;
	}

	if (pipe(pipefd) == -1) {
		fprintf(stderr, "%s : %d - failed in pipe(), error : %s\n",
			__func__, __LINE__, strerror(errno));
		return NULL;
	}

	cpid = fork();
	switch (cpid) {
	case -1:
		/* Still in parent; Failure in fork() */
		fprintf(stderr, "%s : %d - fork() failed, error : %s\n",
			__func__, __LINE__, strerror(errno));
		close(pipefd[0]);
		close(pipefd[1]);
		return NULL;
	case  0: /* Code executed by child */
		if (process_child(argv, pipefd) == -1) {
			fprintf(stderr, "%s : %d - Error occured while "
					"processing write end of the pipe "
					"(in child).", __func__, __LINE__);
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	default: /* Code executed by parent */
		/* store the child pid for pclose() */
		*ppid = cpid;

		close(pipefd[1]);

		fp = fdopen(pipefd[0], "r");
		if (fp == NULL) {
			fprintf(stderr, "%s : %d - fdopen() error : %s\n",
				__func__, __LINE__, strerror(errno));
			close(pipefd[0]);
			return NULL;
		}
		break;
	}

	return fp;
}

/*
 * This function closely mimics pclose(3).
 * Returns :
 *   On success :  exit status of the command as returned by waitpid(2),
 *   On failure : -1 if waitpid(2) returns an error.
 *   If it cannot obtain the child status, errno is set to ECHILD.
 */
int spclose(FILE *stream, pid_t cpid)
{
	int	status;
	pid_t	pid;

	/*
	 * Close the stream, fclose() takes care of closing
	 * the underlying fd.
	 */
	if (fclose(stream) == EOF) {
		fprintf(stderr, "%s : %d - Failed in fclose() : %s\n",
			__func__, __LINE__, strerror(errno));
		return -1;
	}

	/* Wait for the child to exit */
	do {
		pid = waitpid(cpid, &status, 0);
	} while (pid == -1 && errno == EINTR);

	/* Couldn't obtain child status */
	if (status == -1)
		errno = SIGCHLD;

	return status;
}
