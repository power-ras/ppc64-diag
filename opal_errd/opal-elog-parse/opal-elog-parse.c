/*
 * @file opal_errd_parse.c
 * Copyright (C) 2014 IBM Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <syslog.h>
#include <sys/types.h>

#include "libopalevents.h"
#include "opal-event-data.h"
#include "parse-opal-event.h"
#include "opal-elog.h"
#include "opal-esel-parse.h"

#define DEFAULT_opt_platform_dir "/var/log/opal-elog"
char *opt_platform_dir = DEFAULT_opt_platform_dir;

void print_usage(char *command)
{
	printf("%s - Parse OPAL plaform error logs\n\n", command);
	printf("Usage: %s { -d  <logid> | -e <logid> | -a | -l | -s | -h }"
			" [ -p dir | -f file]\n\n"
			"\t-a       - Display all error log entry details\n"
			"\t-d logid - Display error log entry details\n"
			"\t-e logid - Erase error log entry details (cannot be combined with -f)\n"
			"\t-l       - List all error logs\n"
			"\t-s       - List all service action logs\n"
			"\t-p dir   - Use dir as elog directory (default %s)\n"
			"\t-f file  - Specify elog by filename\n"
			"\t-h       - Print this message and exit\n",
			command, DEFAULT_opt_platform_dir);
}

static int file_filter(const struct dirent *d)
{
	struct stat sbuf;
	char filename[PATH_MAX];

	if (d->d_type == DT_DIR)
		return 0;
	if (d->d_type == DT_REG)
		return 1;

	snprintf(filename, PATH_MAX, "%s/%s", opt_platform_dir, d->d_name);
	if (stat(filename, &sbuf))
		return 0;
	if (S_ISREG(sbuf.st_mode))
		return 1;
	if (S_ISDIR(sbuf.st_mode))
		return 0;
	if (d->d_type == DT_UNKNOWN)
		return 1;

	return 0;
}

uint32_t validate_eid_str(const char *eid)
{
	char *strtol_end;
	uint32_t rc = strtoul(eid, &strtol_end, 16);

	/* strtoul parse didn't the entire eid */
	if (*strtol_end != '\0' || *eid == '\0')
		rc = 0;

	return rc;
}

char *get_elog_filename_int(uint32_t eid)
{
	struct dirent **filelist;
	char *ret_str = NULL;
	char *feid;
	int i;
	int nfiles = scandir(opt_platform_dir, &filelist, file_filter, alphasort);

	if (nfiles < 1)
		return NULL;

	for (i = 0; i < nfiles; i++) {
		if (ret_str) {
			free(filelist[i]);
			continue;
		}

		feid = strchr(filelist[i]->d_name, '-');
		if (!feid) {
			free(filelist[i]);
			continue;
		}

		feid++;
		if (eid == strtoul(feid, 0, 0))
			ret_str = strdup(filelist[i]->d_name);

		free(filelist[i]);
	}
	free(filelist);
	return ret_str;
}

char *get_elog_filename_str(const char *eid_str)
{
	int eid = validate_eid_str(eid_str);
	if (eid == 0)
		return NULL;

	return get_elog_filename_int(eid);
}

int read_elog(char path[], char **buf){

	struct stat sbuf;
	size_t bufsz;
	ssize_t readsz = 0;
	ssize_t sz = 0;
	int ret = 0;
	int platform_log_fd = -1;

	if (chdir(opt_platform_dir) < 0) {
		fprintf(stderr, "Failed to change to platform log directory"
				": %s\n", opt_platform_dir);
		return -1;
	}

	if (stat(path, &sbuf) == -1){
		fprintf(stderr, "Error accessing %s\n",path);
		return -1;
	}

	bufsz = sbuf.st_size;
	if(bufsz > OPAL_ERROR_LOG_MAX){
		fprintf(stderr, "Notice: Oversized elog encountered\n");
		if(bufsz > ELOG_BUF_MAX){
			fprintf(stderr, "Error: elog size greater than max: %zd bytes\n", bufsz);
			return -1;
		}
	}

	*buf = malloc(bufsz);
	if(!*buf){
		fprintf(stderr, "Failed to allocate buffer\n");
		return -1;
	}

	platform_log_fd = open(path, O_RDONLY);
	if (platform_log_fd < 0) {
		fprintf(stderr, "Could not open error log file : %s (%s).\n "
			"Skipping....\n", path, strerror(errno));
		ret = -1;
		goto out;
	}

	sz = 0;
	do {
		readsz = read(platform_log_fd, *buf, bufsz);
		if (readsz < 0) {
			fprintf(stderr, "Read Platform log failed\n");
			ret = -1;
			goto out;
		}
		if(!readsz){
			fprintf(stderr, "Early EOF\n");
			break;
		}
		sz += readsz;
	} while(sz != bufsz);
	ret = sz;

out:
	if (platform_log_fd != -1)
		close(platform_log_fd);

	if(ret == -1)
		free(*buf);
	return ret;

}

void print_elog_summary(char *buffer, int bufsz, uint32_t service_flag)
{
	const char *parse;
	uint32_t logid;
	char src[ELOG_SRC_SIZE + 1];
	char severity;
	uint16_t action;
	struct opal_datetime date_time_in, date_time_out;
	uint8_t creator_id;
	int plus;
	logid = be32toh(*(uint32_t*)(buffer+ELOG_ID_OFFSET));
	memcpy(src, (buffer + ELOG_SRC_OFFSET), sizeof(src));
	src[ELOG_SRC_SIZE] = '\0';
	severity = buffer[ELOG_SEVERITY_OFFSET];
	action = be16toh(*(uint16_t *)(buffer + ELOG_ACTION_OFFSET));
	plus = ((action & ELOG_ACTION_FLAG_SERVICE) == ELOG_ACTION_FLAG_SERVICE);
	parse = get_severity_desc(severity & 0xF0);
	/* & with 0xF0 to get only the category of severity, not the full description */

	date_time_in = *(const struct opal_datetime *)(buffer + ELOG_COMMIT_TIME_OFFSET);
	date_time_out = parse_opal_datetime(date_time_in);
	creator_id = buffer[ELOG_CREATOR_ID_OFFSET];
	if (service_flag != 1 || plus)
		printf("|%08X %04u-%02u-%02u %02u:%02u:%02u %8.8s %c %-17.17s %-20.20s|\n",
		       logid, date_time_out.year, date_time_out.month,
		       date_time_out.day, date_time_out.hour,
		       date_time_out.minutes, date_time_out.seconds,
		       src, (plus && !service_flag) ? '+' : ' ', get_creator_name(creator_id), parse);
}

/* parse error log entry from file */
int elogdisplayfile(char *elog_path, uint32_t eid, int display_all)
{
	uint32_t logid;
	int ret = 0;
	char *buffer;
	ssize_t sz = 0;
	int offset = ELOG_ID_OFFSET;

	sz = read_elog(elog_path, &buffer);
	if(sz < 0) {
		return -1;
	/* Make sure we read minimum data needed in this function */
	} else if (sz < (ELOG_ID_OFFSET + sizeof(logid))){
		fprintf(stderr, "Partially read elog, cannot parse\n");
		free(buffer);
		return -1;
	}

	/* Looking for the logid won't work if the eSEL header isn't accounted for. */
	if (is_esel_header(buffer))
		offset += sizeof(struct esel_header);

	logid = be32toh(*(uint32_t*)(buffer+offset));
	if (display_all || logid == eid) {
		ret = parse_opal_event(buffer, sz);
	} else {
		fprintf(stderr, "EID %u does not match %s\n",
			eid, elog_path);
		ret = -1;
	}
	free(buffer);

	return ret;
}

/* parse error log entry passed by user */
int elogdisplayentry(uint32_t eid, int display_all)
{
	uint32_t logid;
	int ret = 0;
	char *buffer;
	struct dirent **filelist;
	int nfiles;
	ssize_t sz = 0;
	int i;
	int done = 0;
	int offset = ELOG_ID_OFFSET;

	nfiles = scandir(opt_platform_dir, &filelist,
			 file_filter, alphasort);
	if (nfiles < 0){
		fprintf(stderr, "Error accessing directory: %s\n",opt_platform_dir);
		return -1;
	}
	if (nfiles == 0){
		fprintf(stderr,"0 files found in directory: %s\n",opt_platform_dir);
		return -1;
	}
	for (i = 0; i < nfiles; i++){
		if(done){
			free(filelist[i]);
			continue;
		}

		sz = read_elog(filelist[i]->d_name, &buffer);

		if(sz < 0) {
			free(filelist[i]);
			continue;
		/* Make sure we read minimum data needed in this function */
		} else if (sz < (ELOG_ID_OFFSET + sizeof(logid))){
			fprintf(stderr, "Partially read elog, cannot parse\n");
			free(filelist[i]);
			free(buffer);
			continue;
		}

		if (is_esel_header(buffer))
			offset += sizeof(struct esel_header);

		logid = be32toh(*(uint32_t*)(buffer+offset));
		if (display_all || logid == eid) {
			ret = parse_opal_event(buffer, sz);
			if (!display_all){
				done = 1;
			}
		}

		free(buffer);
		free(filelist[i]);
	}
	free(filelist);

	return ret;
}

/* print summary of specified file */
int elog_summary(char *elog_path, uint32_t service_flag)
{
	int ret = 0;
	char *buffer;
	ssize_t sz = 0;

	printf("|------------------------------------------------------------------------------|\n");
	printf("|ID       Date       Time     SRC        Creator           Event Severity      |\n");
	printf("|------------------------------------------------------------------------------|\n");

	sz = read_elog(elog_path, &buffer);
	if (sz < 0)
		return -1;

	if (sz < ELOG_MIN_READ_OFFSET) {
		fprintf(stderr, "Partially read elog, cannot parse\n");
		ret = -1;
	} else {
		/* If the file is an eSEL, we need to ignore the header. */
		if (is_esel_header(buffer))
			print_elog_summary(buffer + sizeof(struct esel_header),
					   sz, service_flag);
		else
			print_elog_summary(buffer, sz, service_flag);
	}

	if (!ret)
		printf("|------------------------------------------------------------------------------|\n");

	free(buffer);
	return ret;
}

/* list all the error logs */
int eloglist(uint32_t service_flag)
{
	char *buffer;
	struct dirent **filelist;
	int nfiles;
	ssize_t sz = 0;
	int i;

	printf("|------------------------------------------------------------------------------|\n");
	printf("|ID       Date       Time     SRC        Creator           Event Severity      |\n");
	printf("|------------------------------------------------------------------------------|\n");

	nfiles = scandir(opt_platform_dir, &filelist,
			 file_filter, alphasort);

	if (nfiles < 0){
		fprintf(stderr,"Error accessing directory: %s\n",opt_platform_dir);
		return -1;
	}
	if (nfiles == 0){
		fprintf(stderr,"0 files found in directory: %s\n",opt_platform_dir);
		return -1;
	}

	for (i = 0; i < nfiles; i++){
		sz = read_elog(filelist[i]->d_name, &buffer);
		if (sz < 0){
			free(filelist[i]);
			continue;
		} else if (sz < ELOG_MIN_READ_OFFSET) {
			fprintf(stderr, "Partially read elog, cannot parse\n");
		} else {
			/* If the file is an eSEL, we need to ignore the header */
			if (is_esel_header(buffer))
				print_elog_summary(buffer+sizeof(struct esel_header),
						   sz, service_flag);
			else
				print_elog_summary(buffer, sz, service_flag);
		}

		free(buffer);
		free(filelist[i]);
	}
	free(filelist);

	printf("|------------------------------------------------------------------------------|\n");

	return 0;
}

int delete_elog(const char *eid)
{
	int error = -1;
	char *f_name = get_elog_filename_str(eid);
	if (f_name) {
		error = chdir(opt_platform_dir);
		if (!error)
			error = remove(f_name);
		free(f_name);
	}
	return error;
}

int main(int argc, char *argv[])
{
	uint32_t eid = 0;
	int opt = 0, ret = 0;
	int arg_cnt = 0;
	char do_operation = '\0';
	const char *eid_opt = NULL;
	char *elog_path = NULL;
	int opt_display_file = 0;
	int opt_display_all = 0;

	while ((opt = getopt(argc, argv, "ad:lshf:p:e:")) != -1) {
		switch (opt) {
		case 'e':
		case 'd':
			eid_opt = optarg;
			eid = validate_eid_str(eid_opt);
			if (eid == 0) {
				fprintf(stderr, "Invalid logid '%s'\n", optarg);
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			/* fallthrough */
		case 'l':
		case 's':
			arg_cnt++;
			do_operation = opt;
			break;
		case 'a':
			arg_cnt++;
			opt_display_all = 1;
			do_operation = opt;
			break;
		case 'f':
			elog_path = optarg;
			opt_display_file = 1;
			break;
		case 'p':
			opt_platform_dir = optarg;
			break;
		case 'h':
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
		default:
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (arg_cnt > 1) {
		fprintf(stderr, "Only one operation (-d | -a | -l | -s | -e) "
			"can be selected at any one time.\n");
		print_usage(argv[0]);
		return -1;
	}

	if (do_operation == 'e' && opt_display_file) {
		fprintf(stderr, "Cannot combine -e and -f flags\n");
		print_usage(argv[0]);
		return -1;
	}

	switch (do_operation) {
	case 'l':
		if(opt_display_file){
			ret = elog_summary(elog_path,0);
		} else {
			ret = eloglist(0);
		}
		break;
	case 'e':
		ret = delete_elog(eid_opt);
		break;
	case 'd':
		/* fallthrough */
	case 'a':
		if(opt_display_file){
			ret = elogdisplayfile(elog_path,eid,opt_display_all);
		} else {
			ret = elogdisplayentry(eid,opt_display_all);
		}
		break;
	case 's':
		if(opt_display_file){
			ret = elog_summary(elog_path,1);
		} else {
			ret = eloglist(1);
		}
		break;
	default:
		fprintf(stderr, "No operation specified\n");
		print_usage(argv[0]);
		ret = -1;
		break;
	}

	return ret;
}
