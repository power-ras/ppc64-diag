/**
 * @file convert_dt_node_props.c
 *
 * Copyright (C) 2005 IBM Corporation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <dirent.h>
#include <fcntl.h>

#define MAX_IRQ_SERVERS_PER_CPU	16

static struct option long_options[] = {
	{"context",	required_argument, NULL, 'c'},
	{"from",	required_argument, NULL, 'f'},
	{"to",		required_argument, NULL, 't'},
	{"help",	no_argument,       NULL, 'h'},
	{0,0,0,0}
};

int cpu_interruptserver_to_drcindex(uint32_t, uint32_t *);
int cpu_drcindex_to_drcname(uint32_t, char *, int);
int cpu_drcindex_to_interruptserver(uint32_t, uint32_t *, int);
int cpu_drcname_to_drcindex(char *, uint32_t *);

void
print_usage(char *command) {
	printf ("Usage: %s --context <x> --from <y> --to <z> <value>\n"
		"\t--context: currently, <x> must be cpu\n"
		"\t--from and --to: allowed values for <y> and <z>:\n"
		"\t\tinterrupt-server\n\t\tdrc-index\n\t\tdrc-name\n"
		"\tif <value> is a drc-index or interrupt-server, it can be\n"
		"\tspecified in decimal, hex (with a leading 0x), or octal\n"
		"\t(with a leading 0); if it is a drc-name, it should be\n"
		"\tspecified as a string in double quotes\n\n",
		command);
	return;
}

/**
 * mem_drcindex_to_drcname
 * @brief converts drcindex of mem type to drcname
 *
 * @param drc_idx - drc index whose drc name is to be found.
 * @param drc_name - buffer for drc_name
 * @param buf_size - size of buffer.
 */
static int
mem_drcindex_to_drcname(uint32_t drc_idx, char *drc_name, int buf_size)
{
	int fd, offset=0, found=0;
	uint32_t index;
	uint8_t ch;

	if ((fd = open("/proc/device-tree/ibm,drc-indexes",
				O_RDONLY)) < 0) {
		fprintf(stderr, "error opening /proc/device-tree/"
			"ibm,drc-indexes");
		return 0;
	}

	/* skip the first one; it indicates how many are in the file */
	read(fd, &index, 4);

	while ((read(fd, &index, 4)) != 0) {
		if (index == drc_idx) {
			found = 1;
			break;
		}
		offset++;
	}
	close(fd);

	if (found) {
		if ((fd = open("/proc/device-tree/ibm,drc-names",
				O_RDONLY)) < 0) {
			fprintf(stderr, "error opening /proc/device-tree/"
				"ibm,drc-names");
			return 0;
		}

		/* skip the first one; it indicates how many are in the file */
		read(fd, &index, 4);

		while (offset > 0) {
			/* skip to (and one past) the next null char */
			do {
				if ((read(fd, &ch, 1)) != 1) {
					close(fd);
					return 0;
				}
			} while (ch != 0);
			offset--;
		}

		/* copy the drc-name at the current location to the buffer */
		while ((read(fd, &ch, 1)) == 1) {
			if (offset+1 == buf_size) {
				drc_name[offset] = '\0';
				break;
			}
			drc_name[offset++] = ch;
			if (ch == 0)
				break;
		}
	}
	close(fd);

	return found;
}

int
main(int argc, char *argv[]) {
	int option_index, rc, i;
	char *context=NULL, *from=NULL, *to=NULL;
	uint32_t interruptserver, drcindex;
	unsigned long drc_tmp_idx;
	uint32_t intservs_array[MAX_IRQ_SERVERS_PER_CPU];
	char drcname[20];

	for (;;) {
		option_index = 0;
		rc = getopt_long(argc, argv, "hc:f:t:", long_options,
				&option_index);

		if (rc == -1)
			break;
		switch (rc) {
		case 'h':
			print_usage(argv[0]);
			return 0;
		case 'c':	/* context */
			context = optarg;
			break;
		case 'f':	/* from */
			from = optarg;
			break;
		case 't':	/* to */
			to = optarg;
			break;
		case '?':
			print_usage(argv[0]);
			return -1;
			break;
		default:
			printf("huh?\n");
			break;
		}
	}

	if (!context) {
		fprintf(stderr, "--context not specified\n");
		return 1;
	}
	if (!from) {
		fprintf(stderr, "--from not specified\n");
		return 2;
	}
	if (!to) {
		fprintf(stderr, "--to not specified\n");
		return 3;
	}

	/*
	 * In the cpu context, we can convert between drc name, drc index,
	 * and interrupt server (AKA logical CPU ID).
	 */
	if (!strcmp(context, "cpu")) {
		if (!strcmp(from, "interrupt-server")) {
			interruptserver = strtol(argv[argc-1], NULL, 0);
			if (!strcmp(to, "drc-index")) {
				if (!cpu_interruptserver_to_drcindex(
						interruptserver, &drcindex)) {
					fprintf(stderr, "could not find the "
						"drc-index corresponding to "
						"interrupt-server 0x%08x\n",
						interruptserver);
					return 4;
				}
				printf("0x%08x\n", drcindex);
			}
			else if (!strcmp(to, "drc-name")) {
				if (!cpu_interruptserver_to_drcindex(
						interruptserver, &drcindex)) {
					fprintf(stderr, "could not find the "
						"drc-index corresponding to "
						"interrupt-server 0x%08x\n",
						interruptserver);
					return 4;
				}
				if (!cpu_drcindex_to_drcname(drcindex,
						drcname, 20)) {
					fprintf(stderr, "could not find the "
						"drc-name corresponding to "
						"drc-index 0x%08x\n", drcindex);
					return 4;
				}
				printf("%s\n", drcname);
			}
			else {
				fprintf(stderr, "invalid --to flag: %s\n", to);
				return 3;
			}
		}
		else if (!strcmp(from, "drc-index")) {
			drcindex = strtol(argv[argc-1], NULL, 0);
			if (!strcmp(to, "drc-name")) {
				if (!cpu_drcindex_to_drcname(drcindex,
						drcname, 20)) {
					fprintf(stderr, "could not find the "
						"drc-name corresponding to "
						"drc-index 0x%08x\n", drcindex);
					return 4;
				}
				printf("%s\n", drcname);
			}
			else if (!strcmp(to, "interrupt-server")) {
				rc = cpu_drcindex_to_interruptserver(drcindex,
						intservs_array,
						MAX_IRQ_SERVERS_PER_CPU);
				if (!rc) {
					fprintf(stderr, "could not find the "
						"interrupt-server corresponding"
						" to drc-index 0x%08x\n",
						drcindex);
					return 4;
				}
				if (rc > MAX_IRQ_SERVERS_PER_CPU)
					fprintf(stderr, "warning: only the "
						"first %d servers in the list "
						"of %d were returned; increase "
						"MAX_IRQ_SERVERS_PER_CPU to "
						"retrieve the entire list\n",
						MAX_IRQ_SERVERS_PER_CPU, rc);
				i = 0;
				while (i < rc)
					printf("0x%08x\n", intservs_array[i++]);
			}
			else {
				fprintf(stderr, "invalid --to flag: %s\n", to);
				return 3;
			}
		}
		else if (!strcmp(from, "drc-name")) {
			strncpy(drcname, argv[argc-1], 20);
			if (!strcmp(to, "drc-index")) {
				if (!cpu_drcname_to_drcindex(drcname,
						&drcindex)) {
					fprintf(stderr, "could not find the "
						"drc-index corresponding to "
						"drc-name %s\n", drcname);
					return 4;
				}
				printf("0x%08x\n", drcindex);
			}
			else if (!strcmp(to, "interrupt-server")) {
				if (!cpu_drcname_to_drcindex(drcname,
						&drcindex)) {
					fprintf(stderr, "could not find the "
						"drc-index corresponding to "
						"drc-name %s\n", drcname);
					return 4;
				}
				rc = cpu_drcindex_to_interruptserver(drcindex,
						intservs_array,
						MAX_IRQ_SERVERS_PER_CPU);
				if (!rc) {
					fprintf(stderr, "could not find the "
						"interrupt-server corresponding"
						" to drc-index 0x%08x\n",
						drcindex);
					return 4;
				}
				if (rc > MAX_IRQ_SERVERS_PER_CPU)
					fprintf(stderr, "warning: only the "
						"first %d servers in the list "
						"of %d were returned; increase "
						"MAX_IRQ_SERVERS_PER_CPU to "
						"retrieve the entire list\n",
						MAX_IRQ_SERVERS_PER_CPU, rc);
				i = 0;
				while (i < rc)
					printf("0x%08x\n", intservs_array[i++]);
			}
			else {
				fprintf(stderr, "invalid --to flag: %s\n", to);
				return 3;
			}
		}
		else {
			fprintf(stderr, "invalid --from flag: %s\n", from);
			return 2;
		}
	}
	else if (!strcmp(context, "mem")) {
		if (!strcmp(from, "drc-index")) {
			drc_tmp_idx = strtoul(argv[argc-1], NULL, 0);
			if (!strcmp(to, "drc-name")) {
				if (!mem_drcindex_to_drcname(drc_tmp_idx,
						drcname, 20)) {
					fprintf(stderr, "could not find the "
						"drc-name corresponding to "
						"drc-index 0x%08x\n", drcindex);
					return 4;
				}
				printf("%s\n", drcname);
			}
		}
		else {
			fprintf(stderr, "invalid --to flag: %s\n", to);
			return 3;
		}
	}
	else {
		fprintf(stderr, "invalid --context flag: %s\n", context);
		return 1;
	}

	return 0;
}

int
cpu_interruptserver_to_drcindex(uint32_t int_serv, uint32_t *drc_idx) {
	DIR *dir;
	struct dirent *entry;
	int found=0, fd;
	char buffer[1024];
	uint32_t temp;

	dir = opendir("/proc/device-tree/cpus");

	while ((entry = readdir(dir)) != NULL) {
		if (!strncmp(entry->d_name, "PowerPC,POWER", 13)) {
			snprintf(buffer, 1024, "/proc/device-tree/cpus/%s/"
				"ibm,ppc-interrupt-server#s", entry->d_name);
			if ((fd = open(buffer, O_RDONLY)) < 0) {
				fprintf(stderr, "error opening %s\n", buffer);
				goto cleanup;
			}
			while ((read(fd, &temp, 4)) != 0) {
				if (temp == int_serv) {
					close(fd);
					snprintf(buffer, 1024, "/proc/device-"
						"tree/cpus/%s/ibm,my-drc-index",
						entry->d_name);
					if ((fd = open(buffer, O_RDONLY)) < 0) {
						fprintf(stderr, "error opening"
							" %s\n", buffer);
						goto cleanup;
					}
					if ((read(fd, drc_idx, 4)) == 4)
						found = 1;
					close(fd);
					break;
				}
			}
			close(fd);
		}
	}

cleanup:
	closedir(dir);
	return found;
}

int
cpu_drcindex_to_drcname(uint32_t drc_idx, char *drc_name, int buf_size) {
	int fd, offset=0, found=0;
	uint32_t index;
	uint8_t ch;

	if ((fd = open("/proc/device-tree/cpus/ibm,drc-indexes",
				O_RDONLY)) < 0) {
		fprintf(stderr, "error opening /proc/device-tree/cpus/"
			"ibm,drc-indexes");
		return 0;
	}

	/* skip the first one; it indicates how many are in the file */
	read(fd, &index, 4);

	while ((read(fd, &index, 4)) != 0) {
		if (index == drc_idx) {
			found = 1;
			break;
		}
		offset++;
	}
	close(fd);

	if (found) {
		if ((fd = open("/proc/device-tree/cpus/ibm,drc-names",
				O_RDONLY)) < 0) {
			fprintf(stderr, "error opening /proc/device-tree/cpus/"
				"ibm,drc-names");
			return 0;
		}

		/* skip the first one; it indicates how many are in the file */
		read(fd, &index, 4);

		while (offset > 0) {
			/* skip to (and one past) the next null char */
			do {
				if ((read(fd, &ch, 1)) != 1) {
					close(fd);
					return 0;
				}
			} while (ch != 0);
			offset--;
		}

		/* copy the drc-name at the current location to the buffer */
		while ((read(fd, &ch, 1)) == 1) {
			if (offset+1 == buf_size) {
				drc_name[offset] = '\0';
				break;
			}
			drc_name[offset++] = ch;
			if (ch == 0)
				break;
		}
	}
	close(fd);

	return found;
}

/* returns # of interrupt-server numbers found, rather than a boolean value */
int
cpu_drcindex_to_interruptserver(uint32_t drc_idx, uint32_t *int_servs,
		int array_elements) {
	DIR *dir;
	struct dirent *entry;
	int fd, found=0;
	char buffer[1024];
	uint32_t temp;

	dir = opendir("/proc/device-tree/cpus");

	while ((entry = readdir(dir)) != NULL) {
		if (!strncmp(entry->d_name, "PowerPC,POWER", 13)) {
			snprintf(buffer, 1024, "/proc/device-tree/cpus/%s/"
				"ibm,my-drc-index", entry->d_name);
			if ((fd = open(buffer, O_RDONLY)) < 0) {
				fprintf(stderr, "error opening %s\n", buffer);
				closedir(dir);
				return 0;
			}
			while ((read(fd, &temp, 4)) != 0) {
				if (temp == drc_idx) {
					close(fd);
					snprintf(buffer, 1024, "/proc/device-"
						"tree/cpus/%s/"
						"ibm,ppc-interrupt-server#s",
						entry->d_name);
					if ((fd = open(buffer, O_RDONLY)) < 0) {
						fprintf(stderr, "error opening"
							" %s\n", buffer);
						closedir(dir);
						return 0;
					}
					while ((read(fd, &temp, 4)) == 4) {
						if (found <= array_elements)
							int_servs[found] = temp;
						found++;
					}
					break;
				}
			}
			close(fd);
		}
	}

	return found;
}

int
cpu_drcname_to_drcindex(char *drc_name, uint32_t *drc_idx) {
	int fd, offset=0, found=0, i, rc;
	uint32_t index;
	uint8_t ch;
	char buffer[64];

	if ((fd = open("/proc/device-tree/cpus/ibm,drc-names",
				O_RDONLY)) < 0) {
		fprintf(stderr, "error opening /proc/device-tree/cpus/"
			"ibm,drc-names");
		return 0;
	}

	/* skip the first one; it indicates how many are in the file */
	read(fd, &index, 4);

	do {
		i = 0;
		while ((rc = read(fd, &ch, 1)) == 1) {
			buffer[i++] = ch;
			if (ch == 0)
				break;
		}
		if (!strcmp(buffer, drc_name)) {
			found = 1;
			break;
		}
		offset++;
	} while (rc);
	close(fd);

	if (found) {
		if ((fd = open("/proc/device-tree/cpus/ibm,drc-indexes",
				O_RDONLY)) < 0) {
			fprintf(stderr, "error opening /proc/device-tree/cpus/"
				"ibm,drc-indexes");
			return 0;
		}

		/* skip the first one; it indicates how many are in the file */
		read(fd, &index, 4);

		while (offset > 0) {
			if ((read(fd, &index, 4)) != 4) {
				close(fd);
				return 0;
			}
			offset--;
		}

		if ((read(fd, drc_idx, 4)) != 4)
			found = 0;
	}
	close(fd);

	return found;
}

