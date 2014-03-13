/*
 * @file opal_errd_parse.c
 * Copyright (C) 2014 IBM Corporation
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

#include "libopalevents.h"

#define DEFAULT_opt_platform_log "/var/log/platform"
char *opt_platform_log = DEFAULT_opt_platform_log;
int opt_display_all = 0;

#define ELOG_ID_OFFSET          0x2c
#define ELOG_SRC_OFFSET         0x78
#define ELOG_SEVERITY_OFFSET    0x3a
#define OPAL_ERROR_LOG_MAX      16384
#define ELOG_ACTION_OFFSET      0x42
#define ELOG_ACTION_FLAG        0xa8000000

/* Severity of the log */
#define OPAL_INFORMATION_LOG    0x00
#define OPAL_RECOVERABLE_LOG    0x10
#define OPAL_PREDICTIVE_LOG     0x20
#define OPAL_UNRECOVERABLE_LOG  0x40

static int platform_log_fd = -1;

void print_usage(char *command)
{
	printf("Usage: %s { -d  <entryid> | -l  | -s | -h } [ -f file ]\n"
		"\t-d: Display error log entry details\n"
	        "\t-a: Display all error log entry details\n"
		"\t-l: list all error logs\n"
		"\t-s: list all call home logs\n"
	        "\t-f file: use file as platform log file (default %s)\n"
	        "\t-h: print the usage\n", command, DEFAULT_opt_platform_log);
}

/* parse error log entry passed by user */
int elogdisplayentry(uint32_t eid)
{
	uint32_t logid;
	int len = 0;
	int ret = 0;
	static int pos;
	char buffer[OPAL_ERROR_LOG_MAX];
	platform_log_fd = open(opt_platform_log, O_RDONLY);
	if (platform_log_fd <= 0) {
		fprintf(stderr, "Could not open error log file at either %s or "
		       " %s\nThe error log parse tool cannot continue and will "
		       "exit", opt_platform_log, strerror(errno));
		close(platform_log_fd);
		return -1;
	}
	while (lseek(platform_log_fd, pos, 0) >= 0) {
		memset(buffer, 0, sizeof(buffer));
		len = read(platform_log_fd, (char *)buffer, OPAL_ERROR_LOG_MAX);
		if (len == 0) {
			printf ("Read Completed\n");
			break;
		} else if (len < 0) {
			fprintf(stderr, "Read Platform log failed\n");
			ret = -1;
			break;
		}
		pos = pos + len;
		logid = be32toh(*(uint32_t*)(buffer+ELOG_ID_OFFSET));
		if (opt_display_all || logid == eid) {
			ret = parse_opal_event(buffer, len);
			if (!opt_display_all)
				break;
		}
	}
	close(platform_log_fd);
	return ret;
}

/* list all the error logs */
int eloglist(uint32_t service_flag)
{
	int len = 0, ret = 0;
	char *parse;
	uint32_t logid;
	char src[4];
	char buffer[OPAL_ERROR_LOG_MAX];
	char severity;
	static int pos;
	uint32_t action;
	platform_log_fd = open(opt_platform_log, O_RDONLY);
	if (platform_log_fd <= 0) {
		fprintf(stderr, "Could not open error log file at either %s or "
		       " %s\nThe error log parse tool cannot continue and will "
		       "exit", opt_platform_log, strerror(errno));
		close(platform_log_fd);
	}
	printf("|-------------------------------------------------------------|\n");
	printf("| Entry Id      Event Severity                       SRC      |\n");
	printf("|-------------------------------------------------------------|\n");
	while (lseek(platform_log_fd, pos, 0) >= 0) {
		memset(buffer, 0, sizeof(buffer));
		len = read(platform_log_fd, (char *)buffer, OPAL_ERROR_LOG_MAX);
		if (len == 0) {
			printf("Read Completed\n");
			break;
		} else if (len < 0) {
			fprintf(stderr, "Read Platform log failed\n");
			ret = -1;
			break;
		}
		pos = pos + len;
		logid = be32toh(*(uint32_t*)(buffer+ELOG_ID_OFFSET));
		memcpy(src, (buffer + ELOG_SRC_OFFSET), 8);
		severity = buffer[ELOG_SEVERITY_OFFSET];
		action = be32toh(*(uint32_t *)(buffer + ELOG_ACTION_OFFSET));
		switch (severity) {
		case OPAL_INFORMATION_LOG:
			parse = "Informational Error";
			break;
		case OPAL_RECOVERABLE_LOG:
			parse = "Recoverable Error";
			break;
		case OPAL_PREDICTIVE_LOG:
			parse = "Predictive Error";
			break;
		case OPAL_UNRECOVERABLE_LOG:
			parse = "Unrecoverable Error";
			break;
		default:
			parse = "NONE";
			break;
		}
		if (service_flag != 1)
			printf("| 0x%08X   %-36.36s %8.8s |\n", logid, parse, src);
		else if ((action == ELOG_ACTION_FLAG) && (service_flag == 1))
			/* list only service action logs */
			printf("| 0x%08X   %-36.36s %8.8s |\n", logid, parse, src);
	}
	close(platform_log_fd);
	return ret;
}

int main(int argc, char *argv[])
{
	uint32_t eid = 0;
	int opt = 0, ret = 0;
	char do_operation = '\0';
	const char *eid_opt;

	while ((opt = getopt(argc, argv, "ad:lshf:")) != -1) {
		switch (opt) {
		case 'd':
			eid_opt = optarg;
			/* fallthrough */
		case 'l':
		case 's':
			if (do_operation != '\0') {
				fprintf(stderr, "Only one operation "
					"(-d | -l | -s) can "
					"be selected at any one time.\n");
				print_usage(argv[0]);
				return -1;
			}
			do_operation = opt;
			break;
		case 'a':
			opt_display_all = 1;
			do_operation = opt;
			break;
		case 'f':
			opt_platform_log = optarg;
			break;
		case 'h':
			print_usage(argv[0]);
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
	if (argc == 1) {
		fprintf(stderr, "No parameters are specified\n");
		print_usage(argv[0]);
		ret = -1;
	}

	switch (do_operation) {
	case 'l':
		ret = eloglist(0);
		break;
	case 'd':
		eid = strtoul(eid_opt, 0, 0);
		/* fallthrough */
	case 'a':
		ret = elogdisplayentry(eid);
		break;
	case 's':
		ret = eloglist(1);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}
