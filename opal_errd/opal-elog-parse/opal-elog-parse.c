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

#include "libopalevents.h"
#include "platform.c"

#define PLATFORM_LOG		"/var/log/platform"
#define ELOG_ID_OFFESET         0x2c
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
	printf("Usage: %s { -d  <entryid> | -l  | -s | -h }\n"
		"\t-d: Display error log entry details\n"
		"\t-l: list all error logs\n"
		"\t-s: list all call home logs\n"
		"\t-h: print the usage\n", command);
}

/* parse error log entry passed by user */
int elogdisplayentry(uint32_t eid)
{
	char logid[4];
	int len = 0;
	int ret = 0;
	static int pos;
	char buffer[OPAL_ERROR_LOG_MAX];
	platform_log_fd = open(PLATFORM_LOG, O_RDONLY);
	if (platform_log_fd <= 0) {
		fprintf(stderr, "Could not open error log file at either %s or "
		       " %s\nThe errorlog daemon cannot continue and will "
		       "exit", PLATFORM_LOG, strerror(errno));
		close(platform_log_fd);
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
		memcpy(logid, (buffer+ELOG_ID_OFFESET), 4);
		if (*(int *)logid == eid) {
			parse_opal_event(buffer, len);
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
	char *parse = "NONE";
	char logid[4];
	char src[4];
	char buffer[OPAL_ERROR_LOG_MAX];
	char severity;
	static int pos;
	uint32_t action;
	platform_log_fd = open(PLATFORM_LOG, O_RDONLY);
	if (platform_log_fd <= 0) {
		fprintf(stderr, "Could not open error log file at either %s or "
		       " %s\nThe errorlog daemon cannot continue and will "
		       "exit", PLATFORM_LOG, strerror(errno));
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
		memcpy(logid, (buffer+ELOG_ID_OFFESET), 4);
		memcpy(src, (buffer + ELOG_SRC_OFFSET), 8);
		severity = buffer[ELOG_SEVERITY_OFFSET];
		action = *(uint32_t *)(buffer + ELOG_ACTION_OFFSET);
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
		}
		if (service_flag != 1)
			printf("| 0x%08X   %-36.36s %8.8s |\n", *(int *)logid, parse, src);
		else if ((action == ELOG_ACTION_FLAG) && (service_flag == 1))
			/* list only service action logs */
			printf("| 0x%08X   %-36.36s %8.8s |\n", *(int *)logid, parse, src);
	}
	close(platform_log_fd);
	return ret;
}

int main(int argc, char *argv[])
{
	uint32_t eid = 0;
	int opt = 0, ret = 0;
	int platform = 0;

	platform = get_platform();
	if (platform != PLATFORM_POWERKVM) {
		fprintf(stderr, "%s: is not supported on the %s platform\n",
					argv[0], __power_platform_name(platform));
		return -1;
	}

	while ((opt = getopt(argc, argv, "d:lsh")) != -1) {
		switch (opt) {
		case 'l':
			ret = eloglist(0);
			break;
		case 'd':
			eid = strtoul(argv[2], 0, 0);
			ret = elogdisplayentry(eid);
			break;
		case 's':
			ret = eloglist(1);
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
	return ret;
}
