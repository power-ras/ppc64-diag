#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "utils.h"

int main(int argc, char *argv[])
{
	int	status;
	FILE	*fp;
	pid_t	cpid;
	char	buff[128] = {};

	if (argc < 2) {
		fprintf(stderr, "Usage : ./spopen <cmd> <cmd-arg0> <cmd-arg1>"
			" ...\n");
		exit(1);
	}


	fp = spopen(argv + 1, &cpid);
	if (fp == NULL) {
		fprintf(stderr, "spopen() failed : %s\n", strerror(errno));
		exit(1);
	}

	while (fgets(buff, 128, fp) != NULL)
		printf("%s", buff);

	status = spclose(fp, cpid);
	if (status == -1) {
		fprintf(stderr, "Failed in spclose() : %s.\n", strerror(errno));
		exit(1);
	}

	fprintf(stdout, "Child exited with status : %d.\n", status);
	exit(0);
}
