/**
 * @file convert_dt_node_props.c
 *
 * Copyright (C) 2005 - 2016 IBM Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <dirent.h>
#include <fcntl.h>

#include "platform.h"

#define MAX_IRQ_SERVERS_PER_CPU	16

static struct option long_options[] = {
	{"context",	required_argument, NULL, 'c'},
	{"from",	required_argument, NULL, 'f'},
	{"to",		required_argument, NULL, 't'},
	{"help",	no_argument,       NULL, 'h'},
	{0,0,0,0}
};


/*
 * The original version of this code processed 4 property arrays whose value
 * at any specific array index described a single system entity.
 *
 *   ibm,drc-names: <num entries>; Array of name <encode-string>
 *   ibm,drc-types: <num entries>; Array of entity 'type' <encode-string>
 *   ibm,drc-indexes: <num entries>; Array of <encode-int> values
 *   ibm,drc-power-domains: Array of <encode-int> values
 *
 * In a later version of the system firmware, a compressed representation
 * of this information property was added named 'ibm,drc-info'.  Here is
 * a summary of the representation of the property.
 *
 * <int word: num drc-info entries>
 * Each entry has the following fields in sequential order:
 *     <encode-string: drc-type e.g. "MEM" "PHB" "CPU">
 *     <encode-string: drc-name-prefix >
 *     <encode-int:    drc-index-start >
 *     <encode-int:    drc-name-suffix-start >
 *     <encode-int:    number-sequential-elements >
 *     <encode-int:    sequential-increment >
 *     <encode-int:    drc-power-domain >
 * Each entry describes a subset of all of the 'ibm,drc-info'
 * values.
 */

#define DRC_TYPE_LEN		16
#define DRC_NAME_LEN		256   /* Worst case length to expect */

/*
 * Association between older and newer 'drc info' structions
 * used to drive search routines.
 */
struct drc_info_search_config {
	char *drc_type;		/* device kind sought e.g. "MEM" "PHB" "CPU" */
	char *v1_tree_address;
	char *v1_tree_name_address;
	char *v2_tree_address;
};

/*
 * Configuration of 'drc info' structures for memory-to-name association
 */
static struct drc_info_search_config mem_to_name = {
	"MEM",
	"/proc/device-tree/ibm,drc-indexes",
	"/proc/device-tree/ibm,drc-names",
	"/proc/device-tree/ibm,drc-info",
};

/*
 * Configuration of 'drc info' structures for cpu-to-name association
 */
static struct drc_info_search_config cpu_to_name = {
	"CPU",
	"/proc/device-tree/cpus/ibm,drc-indexes",
	"/proc/device-tree/cpus/ibm,drc-names",
	"/proc/device-tree/cpus/ibm,drc-info",
};

/*
 * Function prototypes supporting search across the 'drc info' structures
 */
static int search_drcindex_to_drcname(struct drc_info_search_config *,
					uint32_t, char *, int);
static int search_drcname_to_drcindex(struct drc_info_search_config *,
					char *, uint32_t *);

static int mem_drcindex_to_drcname(uint32_t, char *, int);

int cpu_interruptserver_to_drcindex(uint32_t, uint32_t *);
int cpu_drcindex_to_drcname(uint32_t, char *, int);
int cpu_drcindex_to_interruptserver(uint32_t, uint32_t *, int);
int cpu_drcname_to_drcindex(char *, uint32_t *);


static void
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
}

static int
read_char_name(int fd, char *propname, int buf_size) {
	int offset = 0;
	uint8_t ch;

	/* Copy the property string at the current location to the buffer */
	while ((read(fd, &ch, 1)) == 1) {
		if (propname) {
			if (offset+1 == buf_size) {
				propname[offset] = '\0';
				break;
			}
			propname[offset++] = ch;
		} else {
			offset++;
		}
		if (ch == 0)
			break;
	}

	return offset;
}

static int
read_uint32(int fd, uint32_t *retval) {
	uint32_t val;
	int rc = 0;

	rc = read(fd, &val, 4);
	if (rc < 4) {
		perror("Error: read_uint32: ");
		return -1;
	}

	(*retval) = be32toh(val);
	return 0;
}

static int
search_drcindex_to_drcname_v1(struct drc_info_search_config *sr,
				uint32_t drc_idx, char *drc_name, int buf_size)
{
	struct stat sbuf;
	int fd, offset=0, found=0;
	uint32_t index, num = 0;
	uint8_t ch;

	if (stat(sr->v1_tree_address, &sbuf) < 0) {
		fprintf(stderr, "Error: property %s not found",
			sr->v1_tree_name_address);
		return 0;
	}

	fd = open(sr->v1_tree_address, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error opening %s:\n%s\n",
			sr->v1_tree_address, strerror(errno));
		return 0;
	}

	/* skip this counter value; not needed to process the property here */
	if (read_uint32(fd, &num) < 0)
		goto err;

	while ((read(fd, &index, 4)) != 0) {
		if (be32toh(index) == drc_idx) {
			found = 1;
			break;
		}
		offset++;
	}
	close(fd);

	if (found) {
		fd = open(sr->v1_tree_name_address, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Error opening %s:\n%s\n",
				sr->v1_tree_name_address, strerror(errno));
			return 0;
		}

		/* skip the first one; it indicates how many are in the file */
		if (read_uint32(fd, &index) < 0)
			goto err;

		while (offset > 0) {
			/* skip to (and one past) the next null char */
			do {
				if ((read(fd, &ch, 1)) != 1)
					goto err;
			} while (ch != 0);
			offset--;
		}

		/* copy the drc-name at the current location to the buffer */
		if (!read_char_name(fd, drc_name, buf_size))
			goto err;
	}
	close(fd);

	return found;

err:
	close(fd);
	return 0;
}

static int
search_drcindex_to_drcname(struct drc_info_search_config *sr, uint32_t drc_idx,
			char *drc_name, int buf_size)
{
	struct stat sbuf;
	int fd, found = 0;
	uint32_t num;
	int i;

	if (stat(sr->v2_tree_address, &sbuf))
		return search_drcindex_to_drcname_v1(sr, drc_idx, drc_name,
							buf_size);

	fd = open(sr->v2_tree_address, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error opening %s:\n%s\n",
			sr->v2_tree_address, strerror(errno));
		return 0;
	}

	/* drc-info: need the first word to iterate over the subsets */
	if (read_uint32(fd, &num) < 0)
		goto err;

	for (i = 0; i < num; i++) {
		char drc_type[DRC_TYPE_LEN];
		char drc_name_base[DRC_NAME_LEN];
		uint32_t drc_start_index, drc_end_index;
		uint32_t drc_name_start_index;
		uint32_t num_seq_elems;
		uint32_t seq_incr;
		uint32_t power_domain;
		int ndx_delta;

		if (!read_char_name(fd, drc_type, sizeof(drc_type)))
			goto err;
		if (!read_char_name(fd, drc_name_base, sizeof(drc_name_base)))
			goto err;

		if (read_uint32(fd, &drc_start_index) < 0)
			goto err;
		if (read_uint32(fd, &drc_name_start_index) < 0)
			goto err;
		if (read_uint32(fd, &num_seq_elems) < 0)
			goto err;
		if (read_uint32(fd, &seq_incr) < 0)
			goto err;
		if (read_uint32(fd, &power_domain) < 0)
			goto err;

		/* Drc-type sought match current entry? */
		if (strcmp(drc_type, sr->drc_type))
			continue;

		drc_end_index = drc_start_index +
				((num_seq_elems-1) * seq_incr);

		if (drc_idx < drc_start_index)
			continue;
		if (drc_idx > drc_end_index)
			continue;
		ndx_delta = drc_idx - drc_start_index;

		snprintf(drc_name, buf_size, "%s%d", drc_name_base,
			drc_name_start_index + ndx_delta);
		found = 1;
		break;
	}

err:
	close(fd);
	return found;
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
	return search_drcindex_to_drcname(&mem_to_name, drc_idx, drc_name,
					buf_size);
}

/**
 * cpu_drcindex_to_drcname
 * @brief converts drcindex of cpu type to drcname
 *
 * @param drc_idx - drc index whose drc name is to be found.
 * @param drc_name - buffer for drc_name
 * @param buf_size - size of buffer.
 */
int
cpu_drcindex_to_drcname(uint32_t drc_idx, char *drc_name, int buf_size)
{
	return search_drcindex_to_drcname(&cpu_to_name, drc_idx, drc_name,
					buf_size);
}

static int
search_drcname_to_drcindex_v1(struct drc_info_search_config *sr,
				char *drc_name, uint32_t *drc_idx)
{
	struct stat sbuf;
	int fd, offset = 0, found = 0;
	uint32_t index, num = 0;
	char buffer[DRC_NAME_LEN];

	if (stat(sr->v1_tree_address, &sbuf) < 0) {
		fprintf(stderr, "Error: property %s not found",
			sr->v1_tree_name_address);
		return 0;
	}
	fd = open(sr->v1_tree_name_address, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error opening %s:\n%s\n",
			sr->v1_tree_name_address, strerror(errno));
		return 0;
	}

	/* skip the first prop word; it indicates how many are in the file */
	if (read(fd, &num, 4) != 4) {
		close(fd);
		return 0;
	}

	do {
		if (!read_char_name(fd, buffer, sizeof(buffer))) {
			close(fd);
			return 0;
		}
		if (!strcmp(buffer, drc_name)) {
			found = 1;
			break;
		}
		offset++;
	} while (!found);
	close(fd);

	if (found) {
		fd = open(sr->v1_tree_address, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Error opening %s:\n%s\n",
				sr->v1_tree_address, strerror(errno));
			return 0;
		}

		/*
		 * skip the first one (indicating the number of entries)
		 * + offset number of indices
		 */
		if (lseek(fd, (1 + offset) * 4, SEEK_SET) !=
				(1 + offset) * 4) {
			close(fd);
			return 0;
		}

		if ((read(fd, &index, 4)) == 4)
			*drc_idx = be32toh(index);
		else
			found = 0;

		close(fd);
	}

	return found;
}

static int
search_drcname_to_drcindex(struct drc_info_search_config *sr, char *drc_name,
			uint32_t *drc_idx)
{
	struct stat sbuf;
	int fd, i, found = 0;
	uint32_t num = 0;

	if (stat(sr->v2_tree_address, &sbuf) < 0)
		return search_drcname_to_drcindex_v1(sr, drc_name, drc_idx);

	fd = open(sr->v2_tree_address, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error: property %s not found",
			sr->v2_tree_address);
		return 0;
	}

	/*
	 * Now we process the entries of the 'ibm,drc-info' property.
	 */
	if (read_uint32(fd, &num) < 0)
		goto err;

	for (i = 0; i < num; i++) {
		char drc_type[DRC_TYPE_LEN];
		char drc_name_base[DRC_NAME_LEN];
		char name_compare[DRC_NAME_LEN];
		uint32_t drc_start_index;
		uint32_t drc_name_start_index;
		uint32_t num_seq_elems;
		uint32_t seq_incr;
		uint32_t power_domain;
		uint32_t read_idx, ndx, tst_drc_idx;
		int rc;

		if (!read_char_name(fd, drc_type, sizeof(drc_type)))
			goto err;
		if (!read_char_name(fd, drc_name_base, sizeof(drc_name_base)))
			goto err;

		if (read_uint32(fd, &drc_start_index) < 0)
			goto err;
		if (read_uint32(fd, &drc_name_start_index) < 0)
			goto err;
		if (read_uint32(fd, &num_seq_elems) < 0)
			goto err;
		if (read_uint32(fd, &seq_incr) < 0)
			goto err;
		if (read_uint32(fd, &power_domain) < 0)
			goto err;

		/* Drc-type sought match current entry? */
		if (strcmp(drc_type, sr->drc_type))
			continue;

		strcat(drc_name_base, "%d");
		rc = sscanf(drc_name, drc_name_base, &read_idx);
		if (rc)
			continue;

		/* Make sure that we can convert to/from the name */
		ndx = ((read_idx - drc_name_start_index) / seq_incr);
		tst_drc_idx = drc_name_start_index +
			      ((ndx-drc_name_start_index)*seq_incr);
		snprintf(name_compare, DRC_NAME_LEN, drc_name_base,
			tst_drc_idx);
		if (strcmp(drc_name, name_compare))
			continue;

		*drc_idx = drc_start_index + ndx;
		found = 1;
		break;
	}

err:
	close(fd);
	return found;
}

/**
 * cpu_drcname_to_drcindex
 * @brief converts mem type drcname to drcindex
 *
 * @param drc_idx - drc index whose drc name is to be found.
 * @param drc_name - buffer for drc_name
 */
int
cpu_drcname_to_drcindex(char *drc_name, uint32_t *drc_idx)
{
	return search_drcname_to_drcindex(&cpu_to_name, drc_name, drc_idx);
}


int
main(int argc, char *argv[]) {
	int option_index, rc, i;
	int platform = 0;
	char *context=NULL, *from=NULL, *to=NULL;
	uint32_t interruptserver, drcindex;
	unsigned long drc_tmp_idx;
	uint32_t intservs_array[MAX_IRQ_SERVERS_PER_CPU];
	char drcname[DRC_NAME_LEN];

	platform = get_platform();
	switch (platform) {
	case PLATFORM_UNKNOWN:
	case PLATFORM_POWERNV:
		fprintf(stderr, "%s: is not supported on the %s platform\n",
				argv[0], __power_platform_name(platform));
		return -1;
	}

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
						drcname, DRC_NAME_LEN)) {
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
						drcname, DRC_NAME_LEN)) {
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
			strncpy(drcname, argv[argc-1], DRC_NAME_LEN - 1);
			drcname[DRC_NAME_LEN - 1] = '\0';
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
						drcname, DRC_NAME_LEN)) {
					fprintf(stderr, "could not find the "
						"drc-name corresponding to "
						"drc-index 0x%08lx\n",
						drc_tmp_idx);
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

	while (!found && (entry = readdir(dir)) != NULL) {
		if (!strncmp(entry->d_name, "PowerPC,POWER", 13)) {
			snprintf(buffer, 1024, "/proc/device-tree/cpus/%s/"
				"ibm,ppc-interrupt-server#s", entry->d_name);
			if ((fd = open(buffer, O_RDONLY)) < 0) {
				fprintf(stderr, "Error: error opening %s:\n"
					"%s\n", buffer, strerror(errno));
				goto cleanup;
			}

			while (read_uint32(fd, &temp) == 0) {
				if (temp == int_serv) {
					close(fd);
					snprintf(buffer, 1024, "/proc/device-"
						"tree/cpus/%s/ibm,my-drc-index",
						entry->d_name);
					if ((fd = open(buffer, O_RDONLY)) < 0) {
						fprintf(stderr, "Error "
							"opening %s:\n%s\n",
							buffer,
							strerror(errno));
						goto cleanup;
					}
					if ((read(fd, &temp, 4)) == 4)  {
						*drc_idx = be32toh(temp);
						found = 1;
					} /* if read */
					break;
				} /* if be32toh  */
			} /* while */
			close(fd);
		}
	} /* readdir */

cleanup:
	closedir(dir);
	return found;
}

/* returns # of interrupt-server numbers found, rather than a boolean value */
int
cpu_drcindex_to_interruptserver(uint32_t drc_idx, uint32_t *int_servs,
		int array_elements) {
	DIR *dir;
	struct dirent *entry;
	int intr_fd, drc_fd, found=0;
	char buffer[1024];
	uint32_t temp;

	dir = opendir("/proc/device-tree/cpus");
	if (!dir)
		return 0;

	while (!found && (entry = readdir(dir)) != NULL) {
		if (!strncmp(entry->d_name, "PowerPC,POWER", 13)) {
			snprintf(buffer, 1024, "/proc/device-tree/cpus/%s/"
				"ibm,my-drc-index", entry->d_name);
			if ((drc_fd = open(buffer, O_RDONLY)) < 0) {
				fprintf(stderr, "Error opening %s:\n"
					"%s\n", buffer, strerror(errno));
				closedir(dir);
				return 0;
			}

			while (read_uint32(drc_fd, &temp) == 0) {
				if (temp == drc_idx) {
					snprintf(buffer, 1024, "/proc/device-"
						"tree/cpus/%s/"
						"ibm,ppc-interrupt-server#s",
						entry->d_name);
					if ((intr_fd = open(buffer, O_RDONLY)) < 0) {
						fprintf(stderr, "Error "
							"opening %s:\n%s\n",
							buffer,
							strerror(errno));
						close(drc_fd);
						closedir(dir);
						return 0;
					}
					while (found < array_elements &&
						(read(intr_fd, &temp, 4)) == 4)
							int_servs[found++] = temp;
					close(intr_fd);
					break;
				}
			}
			close(drc_fd);
		}
	}

	closedir(dir);
	return found;
}
