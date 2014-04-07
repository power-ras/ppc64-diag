/**
 * @file	elog-daemon.c
 * @brief	Daemon to read/parse OPAL error/events
 *
 * Copyright (C) 2014 IBM Corporation
 */

/*
 * This file supports
 *   1. Reading OPAL platform logs from sysfs
 *   2. Writing OPAL platform logs to individual files under /var/log/opal-elog
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
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/inotify.h>
#include <assert.h>
#include <endian.h>
#include <inttypes.h>
#include <time.h>

#define DEFAULT_SYSFS_PATH		"/sys"
#define DEFAULT_OUTPUT_DIR              "/var/log/opal-elog"
#define DEFAULT_EXTRACT_DUMP_CMD	"/usr/sbin/extract_opal_dump"

char *opt_sysfs = DEFAULT_SYSFS_PATH;
char *opt_output = DEFAULT_OUTPUT_DIR;
int opt_daemon = 1;
int opt_watch = 1;

char *opt_extract_opal_dump_cmd = DEFAULT_EXTRACT_DUMP_CMD;

/*
 * As per PEL v6 (defined in PAPR spec) fixed offset for
 * error log information.
 */
#define OPAL_ERROR_LOG_MAX	16384
#define ELOG_ID_SIZE		4
#define ELOG_SRC_SIZE		8

#define ELOG_DATE_OFFSET	0x8
#define ELOG_TIME_OFFSET	0xc
#define ELOG_ID_OFFESET		0x2c
#define ELOG_SEVERITY_OFFSET	0x3a
#define ELOG_SUBSYSTEM_OFFSET	0x38
#define ELOG_ACTION_OFFSET	0x42
#define ELOG_SRC_OFFSET		0x78
#define ELOG_MIN_READ_OFFSET	ELOG_SRC_OFFSET + ELOG_SRC_SIZE

/* Severity of the log */
#define OPAL_INFORMATION_LOG	0x00
#define OPAL_RECOVERABLE_LOG	0x10
#define OPAL_PREDICTIVE_LOG	0x20
#define OPAL_UNRECOVERABLE_LOG	0x40
#define OPAL_CRITICAL_LOG	0x50
#define OPAL_DIAGNOSTICS_LOG	0x60
#define OPAL_SYMPTOM_LOG	0x70

#define ELOG_ACTION_FLAG	0xa800

volatile int terminate;

/* Aggregate severities into group */
static const char *get_severity_desc(uint8_t severity)
{
	if (severity >= OPAL_SYMPTOM_LOG)
		return "Symptom";
	if (severity >= OPAL_DIAGNOSTICS_LOG)
		return "Error on diag test";
	if (severity >= OPAL_CRITICAL_LOG)
		return "Critical Error";
	if (severity >= OPAL_UNRECOVERABLE_LOG)
		return "Unrecoverable Error";
	if (severity >= OPAL_PREDICTIVE_LOG)
		return "Predictive Error";
	if (severity >= OPAL_RECOVERABLE_LOG)
		return "Recoverable Error";
	if (severity >= OPAL_INFORMATION_LOG)
		return "Informational Event";

	return "UNKNOWN";
}

/* Parse required fields from error log */
static int parse_log(char *buffer, size_t bufsz)
{
	uint32_t logid;
	char src[ELOG_SRC_SIZE+1];
	uint8_t severity;
	uint8_t subsysid;
	uint16_t action;
	const char *parse;
	char *parse_action = "NONE";
	char *failingsubsys = "Not Applicable";

	if (bufsz < ELOG_MIN_READ_OFFSET) {
		syslog(LOG_NOTICE, "Insufficent data, cannot parse elog.\n");
		return -1;
	}

	logid = be32toh(*(uint32_t*)(buffer + ELOG_ID_OFFESET));

	memcpy(src, (buffer + ELOG_SRC_OFFSET), ELOG_SRC_SIZE);
	src[ELOG_SRC_SIZE] = '\0';

	subsysid = buffer[ELOG_SUBSYSTEM_OFFSET];
	severity = buffer[ELOG_SEVERITY_OFFSET];

	parse = get_severity_desc(severity);

	action = be16toh(*(uint16_t *)(buffer + ELOG_ACTION_OFFSET));
	if ((action & ELOG_ACTION_FLAG) == ELOG_ACTION_FLAG)
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
	       logid, src, failingsubsys, parse, parse_action);

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

	if (stat(opt_extract_opal_dump_cmd, &sbuf) < 0) {
		syslog(LOG_NOTICE, "The command \"%s\" does not exist.\n",
		       opt_extract_opal_dump_cmd);
		return;
	}

	rc = system(opt_extract_opal_dump_cmd);
	if (rc) {
		syslog(LOG_NOTICE, "Failed to execute platform dump "
		       "extractor (%s).\n", opt_extract_opal_dump_cmd);
		return;
	}
}

static int ack_elog(const char *elog_path)
{
	char ack_file[PATH_MAX];
	int fd;
	int rc;

	snprintf(ack_file, sizeof(ack_file), "%s/acknowledge", elog_path);

	fd = open(ack_file, O_WRONLY);

	if (fd == -1) {
		syslog(LOG_ERR, "Failed to acknowledge elog: %s"
		       " (%d:%s)\n",
		       ack_file, errno, strerror(errno));
		return -1;
	}

	rc = write(fd, "ack\n", 4);
	if (rc != 4) {
		syslog(LOG_ERR, "Failed to acknowledge elog: %s"
		       " (%d:%s)\n",
		       ack_file, errno, strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

static int process_elog(const char *elog_path)
{
	int in_fd = -1;
	int out_fd = -1;
	int dir_fd = -1;
	char elog_raw_path[PATH_MAX];
	char *buf;
        char *name;
	size_t bufsz;
	struct stat sbuf;
	int ret = -1;
	ssize_t sz = 0;
	ssize_t readsz = 0;
	int rc;
	char *opt_output_dir = strdup(opt_output);
        char opt_output_file[PATH_MAX];

	snprintf(elog_raw_path, sizeof(elog_raw_path), "%s/raw", elog_path);

	if (stat(elog_raw_path, &sbuf) == -1)
		return -1;

	bufsz = sbuf.st_size;
	buf = (char*)malloc(bufsz);
	if (!buf) {
		syslog(LOG_ERR, "Failed to allocate memory\n");
		return -1;
	}

	in_fd = open(elog_raw_path, O_RDONLY);
	if (in_fd == -1) {
		syslog(LOG_ERR, "Failed to open elog: %s (%d:%s)\n",
		       elog_raw_path, errno, strerror(errno));
		goto err;
	}

	do {
		readsz = read(in_fd, buf+sz, bufsz-sz);
		if (readsz == -1) {
			syslog(LOG_ERR, "Failed to read elog: %s (%d:%s)\n",
			       elog_raw_path, errno, strerror(errno));
			goto err;
		}

		sz += readsz;
	} while(sz != bufsz);


        /* Parse elog filename */
        name = basename(dirname(elog_raw_path));
        snprintf(opt_output_file, sizeof(opt_output_file), "%s/%d-%s",
                 opt_output,(int)time(NULL),name);

        out_fd = open(opt_output_file, O_WRONLY  | O_CREAT,
                      S_IRUSR | S_IWUSR | S_IRGRP);

	if (out_fd == -1) {
		syslog(LOG_ERR, "Failed to create elog output file: %s (%d:%s)\n",
		       opt_output_file, errno, strerror(errno));
		goto err;
	}

	assert(bufsz <= OPAL_ERROR_LOG_MAX);

	sz = write(out_fd, buf, bufsz);
	if (sz != bufsz) {
		syslog(LOG_ERR, "Failed to write elog output file: %s (%d:%s)\n",
		       opt_output_file, errno, strerror(errno));
		goto err;
	}

	rc = fsync(out_fd);
	if (rc == -1) {
		syslog(LOG_ERR, "Failed to sync elog output file: %s (%d:%s)\n",
		       opt_output_file, errno, strerror(errno));
		goto err;
	}

	dir_fd = open(dirname(opt_output_dir), O_RDONLY|O_DIRECTORY);
	rc = fsync(dir_fd);
	if (rc == -1) {
		syslog(LOG_ERR, "Failed to sync platform elog directory: %s"
		       " (%d:%s)\n", opt_output_dir, errno, strerror(errno));
	}

	check_platform_dump();

	parse_log(buf, bufsz);

	ret = 0;
err:
	if (in_fd != -1)
		close(in_fd);
	if (out_fd != -1)
		close(out_fd);
	if (dir_fd != -1)
		close(dir_fd);
	free(opt_output_dir);
	free(buf);
	return ret;
}

/* Read logs from opal sysfs interface */
static int find_and_read_elog_events(const char *elog_dir)
{
	int rc = 0;
	struct dirent **namelist;
	struct dirent *dirent;
	char elog_path[PATH_MAX];
	int is_dir = 0;
	struct stat sbuf;
	int retval = 0;
	int n;
	int i;

	n = scandir(elog_dir, &namelist, NULL, alphasort);
	if (n < 0)
		return -1;

	for (i = 0; i < n; i++) {
		dirent = namelist[i];

		if (dirent->d_name[0] == '.') {
			free(namelist[i]);
			continue;
		}

		snprintf(elog_path, sizeof(elog_path), "%s/%s",
			 elog_dir, dirent->d_name);

		is_dir = 0;

		if (dirent->d_type == DT_DIR) {
			is_dir = 1;
		} else {
			/* Fall back to stat() */
			rc = stat(elog_path, &sbuf);
			if (S_ISDIR(sbuf.st_mode)) {
				is_dir = 1;
			}
		}

		if (is_dir) {
			rc = process_elog(elog_path);
			if (rc != 0 && retval == 0)
				retval = -1;
			if (rc == 0 && retval >= 0)
				retval++;
			ack_elog(elog_path);
		}

		free(namelist[i]);
	}

	free(namelist);

	return retval;
}

static void help(const char* argv0)
{
	fprintf(stderr, "%s help:\n\n", argv0);
	fprintf(stderr, "-e cmd  - path to extract_opal_dump (default %s)\n",
		DEFAULT_EXTRACT_DUMP_CMD);
	fprintf(stderr, "-o dir  - output log entries to directory (default %s)\n",
		DEFAULT_OUTPUT_DIR);
	fprintf(stderr, "-s dir  - path to sysfs (default %s)\n",
		DEFAULT_SYSFS_PATH);
	fprintf(stderr, "-D      - don't daemonize, just run once.\n");
	fprintf(stderr, "-w      - watch for new events (default when daemon)\n");
	fprintf(stderr, "-h      - help (this message)\n");
}

int main(int argc, char *argv[])
{
	int rc = 0;
	int opt;
	char sysfs_path[PATH_MAX];
	struct stat s;
	int inotifyfd;
	char inotifybuf[sizeof(struct inotify_event) + NAME_MAX + 1];
	fd_set fds;
	struct timeval tv;
	int r;
	int log_options;

	while ((opt = getopt(argc, argv, "De:ho:s:w")) != -1) {
		switch (opt) {
		case 'D':
			opt_daemon = 0;
			opt_watch = 0;
			break;
		case 'w':
			opt_daemon = 0;
			opt_watch = 1;
			break;
		case 'o':
			opt_output = optarg;
			break;
		case 'e':
			opt_extract_opal_dump_cmd = optarg;
			break;
		case 's':
			opt_sysfs = optarg;
			break;
		case 'h':
			help(argv[0]);
			exit(EXIT_SUCCESS);
		default:
			help(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	/* syslog initialization */
	setlogmask(LOG_UPTO(LOG_NOTICE));
	log_options = LOG_CONS | LOG_PID | LOG_NDELAY;
	if (!opt_daemon)
		log_options |= LOG_PERROR;
	openlog("ELOG", log_options, LOG_LOCAL1);


	snprintf(sysfs_path, sizeof(sysfs_path), "%s/firmware/opal/elog",
		 opt_sysfs);

	rc = stat(sysfs_path, &s);
	if (rc != 0) {
		syslog(LOG_ERR, "Error accessing sysfs: %s (%d: %s)\n",
		       sysfs_path, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

        rc = stat(opt_output, &s);
        if (rc != 0) {
		if (errno == ENOENT) {
			rc = mkdir(opt_output,
				   S_IRGRP | S_IRUSR | S_IWGRP | S_IWUSR | S_IXUSR);
			if (rc != 0) {
				syslog(LOG_ERR, "Error creating output directory:"
				       " %s (%d: %s)\n",
				       opt_output, errno, strerror(errno));
				exit(EXIT_FAILURE);
			}
		} else {
			syslog(LOG_ERR, "Error accessing directory: %s (%d: %s)\n",
			       opt_output, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	inotifyfd = inotify_init();
	if (inotifyfd == -1) {
		syslog(LOG_ERR, "Error setting up inotify (%d:%s)\n",
		       errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	rc = inotify_add_watch(inotifyfd, sysfs_path, IN_CREATE);
	if (rc == -1) {
		syslog(LOG_ERR, "Error adding inotify watch for %s (%d: %s)\n",
		       sysfs_path, errno, strerror(errno));
		close(inotifyfd);
		closelog();
		exit(EXIT_FAILURE);
	}

	/* Convert the opal_errd process to a daemon. */
	if (opt_daemon) {
		rc = daemon(0, 0);
		if (rc) {
			syslog(LOG_NOTICE, "Cannot daemonize opal_errd, "
			       "opal_errd cannot continue.\n");
			closelog();
			close(inotifyfd);
			return rc;
		}
	}

	/* Read error/event log until we get termination signal */
	while (!terminate) {
		find_and_read_elog_events(sysfs_path);
		if (!opt_watch) {
			terminate = 1;
		} else {
			/* We don't care about the content of the inotify
			 * event, we'll just scan the directory anyway
			 */
			FD_ZERO(&fds);
			FD_SET(inotifyfd, &fds);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			r = select(inotifyfd+1, &fds, NULL, NULL, &tv);
			if (r > 0 && FD_ISSET(inotifyfd, &fds))
				read(inotifyfd, inotifybuf, sizeof(inotifybuf));
		}
	}

	close(inotifyfd);
	closelog();

	return rc;
}
