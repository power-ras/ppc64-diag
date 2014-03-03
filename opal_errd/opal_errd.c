/**
 * @file	elog-daemon.c
 * @brief	Daemon to read/parse OPAL error/events
 *
 * Copyright (C) 2014 IBM Corporation
 */

/*
 * This file supports
 *   1. Reading OPAL platform logs from sysfs
 *   2. Writing OPAL platform logs to /var/log/platform
 *   3. ACK platform log
 *   4. Parsing required fields from log and write to syslog
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>

#include "platform.c"

#define SYSFS_ELOG	"/sys/firmware/opal/opal_elog"
#define SYSFS_ELOG_ACK	"/sys/firmware/opal/opal_elog_ack"
#define PLATFORM_LOG	"/var/log/platform"

#define EXTRACT_OPAL_DUMP_CMD	"/usr/sbin/extract_opal_dump"

/*
 * As per PEL v6 (defined in PAPR spec) fixed offset for
 * error log information.
 */
#define OPAL_ERROR_LOG_MAX	16384
#define ELOG_ID_SIZE		4
#define ELOG_SRC_SIZE		8

#define ELOG_ID_OFFESET		0x2c
#define ELOG_DATE_OFFSET	0x8
#define ELOG_TIME_OFFSET	0xc
#define ELOG_SRC_OFFSET		0x78
#define ELOG_SEVERITY_OFFSET	0x3a
#define ELOG_SUBSYSTEM_OFFSET	0x38
#define ELOG_ACTION_OFFSET	0x42

/* Severity of the log */
#define OPAL_INFORMATION_LOG	0x00
#define OPAL_RECOVERABLE_LOG	0x10
#define OPAL_PREDICTIVE_LOG	0x20
#define OPAL_UNRECOVERABLE_LOG	0x40

#define ELOG_ACTION_FLAG	0xa8000000

static int sysfs_elog_fd = -1;
static int sysfs_elog_id_fd = -1;
static int platform_log_fd = -1;

volatile int terminate;

static void close_files(void)
{
	close(sysfs_elog_fd);
	close(platform_log_fd);
	close(sysfs_elog_id_fd);
}

/* Open error log and platform log files */
static int init_files(void)
{
	/* Open Error/event log file */
	sysfs_elog_fd = open(SYSFS_ELOG, O_RDONLY);
	if (sysfs_elog_fd <= 0) {
		syslog(LOG_NOTICE, "Could not open error log file %s: %s\n"
		       "The opal_errd daemon cannot continue and will exit.\n",
		       SYSFS_ELOG, strerror(errno));
		return -1;
	}

	/* Open log ACK file*/
	sysfs_elog_id_fd = open(SYSFS_ELOG_ACK, O_WRONLY);
	if (sysfs_elog_id_fd <= 0) {
		syslog(LOG_NOTICE, "Could not open ACK file %s: %s\n"
		       "The opal_errd daemon cannot continue and will exit.\n",
		       SYSFS_ELOG_ACK, strerror(errno));
		close(sysfs_elog_fd);
		return -1;
	}

	/* Next, open /var/log/platform with 0640 mode */
	platform_log_fd = open(PLATFORM_LOG, O_RDWR | O_SYNC | O_CREAT,
			       S_IRUSR | S_IWUSR | S_IRGRP);
	if (platform_log_fd <= 0) {
		syslog(LOG_NOTICE, "Could not open platform log file %s: %s\n"
		       "The opal_errd daemon cannot continue and will exit.\n",
		       PLATFORM_LOG, strerror(errno));
		close(sysfs_elog_fd);
		close(platform_log_fd);
		return -1;
	}

	return 0;
}

/* Parse required fields from error log */
static int parse_log(char *buffer)
{
	char logid[ELOG_ID_SIZE];
	char src[ELOG_SRC_SIZE];
	uint8_t severity;
	uint8_t subsysid;
	uint32_t action;
	char *parse = "NONE";
	char *parse_action = "NONE";
	char *failingsubsys = "Not Applicable";

	memcpy(logid, (buffer + ELOG_ID_OFFESET), ELOG_ID_SIZE);
	memcpy(src, (buffer + ELOG_SRC_OFFSET), ELOG_SRC_SIZE);
	subsysid = buffer[ELOG_SUBSYSTEM_OFFSET];
	severity = buffer[ELOG_SEVERITY_OFFSET];

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

	action = *(uint32_t *)(buffer + ELOG_ACTION_OFFSET);
	if (action == ELOG_ACTION_FLAG)
		parse_action = "Service action and call home required";
	else
		parse_action = "No service action required";

	if ((subsysid >= 0x10) && (subsysid <= 0x1F))
		failingsubsys = "Processor, including internal cache";
	else if ((subsysid >= 0x20) && (subsysid <= 0x2F))
		failingsubsys = "Memory, including external cache";
	else if ((subsysid >= 0x30) && (subsysid <= 0x3F))
		failingsubsys = "I/O (hub, bridge, bus";
	else if ((subsysid >= 0x40) && (subsysid <= 0x4F))
		failingsubsys = "I/O adapter, device and peripheral";
	else if ((subsysid >= 0x50) && (subsysid <= 0x5F))
		failingsubsys = "CEC Hardware";
	else if ((subsysid >= 0x60) && (subsysid <= 0x6F))
		failingsubsys = "Power/Cooling System";
	else if ((subsysid >= 0x70) && (subsysid <= 0x79))
		failingsubsys = "Other Subsystems";
	else if ((subsysid >= 0x7A) && (subsysid <= 0x7F))
		failingsubsys = "Surveillance Error";
	else if ((subsysid >= 0x80) && (subsysid <= 0x8F))
		failingsubsys = "Platform Firmware";
	else if ((subsysid >= 0x90) && (subsysid <= 0x9F))
		failingsubsys = "Software";
	else if ((subsysid >= 0xA0) && (subsysid <= 0xAF))
		failingsubsys = "External Environment";

	syslog(LOG_NOTICE, "LID[%x]::SRC[%s]::%s::%s::%s\n",
	       *(uint32_t *)logid, src, failingsubsys, parse, parse_action);

	return 0;
}

/**
 * Check platform dump
 *
 * FIXME:
 *   Presently we are calling dump extractor for every error/event.
 *   We have to parse the error/event and call dump extractor only
 *   for dump available event.
 */
static void check_platform_dump(void)
{
	int rc;
	struct stat sbuf;

	if (stat(EXTRACT_OPAL_DUMP_CMD, &sbuf) < 0) {
		syslog(LOG_NOTICE, "The command \"%s\" does not exist.\n",
		       EXTRACT_OPAL_DUMP_CMD);
		return;
	}

	rc = system(EXTRACT_OPAL_DUMP_CMD);
	if (rc) {
		syslog(LOG_NOTICE, "Failed to execute platform dump "
		       "extractor (%s).\n", EXTRACT_OPAL_DUMP_CMD);
		return;
	}
}

/* Read logs from opal sysfs interface */
static int read_elog_events(void)
{
	ssize_t len;
	uint32_t elog_id;
	int rc = 0;
	char buffer[OPAL_ERROR_LOG_MAX];

	memset(buffer, 0, sizeof(buffer));

	/* Read log from OPAL log file */
	len = read(sysfs_elog_fd, (char *)buffer, OPAL_ERROR_LOG_MAX);

	/* Received SIGTERM? */
	if (terminate && len <= 0)
		return 0;

	if (len <= 0) {
		syslog(LOG_NOTICE, "Could not read error log file (%s).\n",
		       SYSFS_ELOG);
		return -1;
	}

	/* Write log to /var/log/platform */
	rc = write(platform_log_fd, buffer, len);
	if (rc <= 0) {
		syslog(LOG_NOTICE, "Could not write to platform log "
		       "file (%s).\n", PLATFORM_LOG);
		return -1;
	}

	/* ACK log (write elog id to ACK file) */
	elog_id = *(uint32_t *)(buffer + ELOG_ID_OFFESET);
	rc = write(sysfs_elog_id_fd, &elog_id, sizeof(elog_id));
	if (rc < 0) {
		syslog(LOG_NOTICE, "Failed to ACK log ID : %x\n", elog_id);
		return -1;
	}

	/* Check platform dump */
	check_platform_dump();

	/* Parse and write required fields to syslog */
	rc = parse_log(buffer);

	return rc;
}

static void sigterm_handler(int sig)
{
	terminate = 1;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	int platform = 0;
	struct sigaction sigact;

	platform = get_platform();
	if (platform != PLATFORM_POWERKVM) {
		fprintf(stderr, "%s: is not supported on the %s platform\n",
					argv[0], __power_platform_name(platform));
		return -1;
	}

	/* syslog initialization */
	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog("ELOG", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	/* Convert the opal_errd process to a daemon. */
	rc = daemon(0, 0);
	if (rc) {
		syslog(LOG_NOTICE, "Cannot daemonize opal_errd, "
		       "opal_errd cannot continue.\n");
		closelog();
		return rc;
	}

	/* open sysfs files to read and write log */
	rc = init_files();
	if (rc)
		goto error_out;

	/* Setup a signal handler for SIGTERM */
	sigact.sa_handler = (void *)sigterm_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	if (sigaction(SIGTERM, &sigact, NULL)) {
		syslog(LOG_NOTICE, "Could not initialize signal handler for "
		       "termination signal (SIGTERM), %s\n", strerror(errno));
		goto error_out;
	}

	/* Read error/event log until we get termination signal */
	while (!terminate)
		read_elog_events();

error_out:
	syslog(LOG_NOTICE, "The opal_errd daemon is exiting.\n");

	/* Close all files once read is complete */
	close_files();
	closelog();

	return rc;
}
