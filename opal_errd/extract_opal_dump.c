/**
 * @file	extract_opal_dump.c
 * @brief	Command to extract a platform dump on PowerNV platform
 *		and copy it to the filesystem
 *
 * Copyright (C) 2014 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <endian.h>
#include <syslog.h>

int opt_ack_dump = 1;
int opt_wait = 0;
char *opt_sysfs = "/sys";
char *opt_output_dir = "/var/log/dump";

static void help(const char* argv0)
{
	fprintf(stderr, "%s help:\n", argv0);
	fprintf(stderr, "\n");
	fprintf(stderr, "-A     - Don't acknowledge dump\n");
	fprintf(stderr, "-s dir - sysfs directory (default %s)\n",
		opt_sysfs);
	fprintf(stderr, "-o dir - directory to save dumps (default %s)\n",
		opt_output_dir);
	fprintf(stderr, "-w     - wait for a dump\n");
	fprintf(stderr, "-h     - help (this message)\n");
}

#define DUMP_HDR_PREFIX_OFFSET 0x16    /* Prefix size in dump header */
#define DUMP_HDR_FNAME_OFFSET  0x18    /* Suggested filename in dump header */
#define DUMP_MAX_FNAME_LEN     48      /* Including .PARTIAL */

static void dump_get_file_name(char *buf, int bsize,
			       char *dfile, uint16_t *prefix_size)
{
	if (bsize >= DUMP_HDR_PREFIX_OFFSET + sizeof(uint16_t))
		*prefix_size = be16toh(*(uint16_t *)(buf + DUMP_HDR_PREFIX_OFFSET));

	if (bsize >= DUMP_HDR_FNAME_OFFSET + DUMP_MAX_FNAME_LEN)
		strncpy(dfile, buf + DUMP_HDR_FNAME_OFFSET,
			DUMP_MAX_FNAME_LEN);
	else
		strcpy(dfile, "platform.dumpid.PARTIAL");

	dfile[DUMP_MAX_FNAME_LEN - 1] = '\0';
}

static void ack_dump(const char* dump_dir_path)
{
	char ack_file[PATH_MAX];
	int fd;
	int rc;

	snprintf(ack_file, sizeof(ack_file), "%s/acknowledge", dump_dir_path);

	fd = open(ack_file, O_WRONLY);

	if (fd == -1) {
		syslog(LOG_ERR, "Failed to acknowledge platform dump: %s"
		       " (%d:%s)\n",
		       ack_file, errno, strerror(errno));
		return;
	}

	rc = write(fd, "ack\n", 4);
	if (rc != 4) {
		syslog(LOG_ERR, "Failed to acknowledge platform dump: %s"
		       " (%d:%s)\n",
		       ack_file, errno, strerror(errno));
		return;
	}

	close(fd);
}


static int process_dump(const char* dump_dir_path, const char *output_dir)
{
	int in_fd = -1;
	int out_fd = -1;
	int dir_fd = -1;
	char dump_path[PATH_MAX];
	char final_dump_path[PATH_MAX];
	char *buf;
	size_t bufsz;
	struct stat sbuf;
	int ret = -1;
	ssize_t sz = 0;
	ssize_t readsz = 0;
	char outfname[DUMP_MAX_FNAME_LEN];
	uint16_t prefix_size;
	int rc;

	snprintf(dump_path, sizeof(dump_path), "%s/dump", dump_dir_path);

	if (stat(dump_path,&sbuf) == -1)
		return -1;

	bufsz = sbuf.st_size;
	buf = (char*)malloc(bufsz);

	in_fd = open(dump_path, O_RDONLY);

	if (in_fd == -1) {
		syslog(LOG_ERR, "Failed to open platform dump: %s (%d:%s)\n",
		       dump_path, errno, strerror(errno));
		goto err;
	}

	do {
		readsz = read(in_fd, buf+sz, bufsz-sz);
		if (readsz == -1) {
			syslog(LOG_ERR, "Failed to read platform dump: %s "
			       "(%d:%s)\n",
			       dump_path, errno, strerror(errno));
			goto err;
		}

		sz += readsz;
	} while(sz != bufsz);

	
	dump_get_file_name(buf, bufsz, outfname, &prefix_size);

	snprintf(dump_path, sizeof(dump_path), "%s/%s.tmp", output_dir, outfname);
	snprintf(final_dump_path, sizeof(dump_path), "%s/%s", output_dir, outfname);

	out_fd = open(dump_path, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IRGRP);

	if (out_fd == -1) {
		syslog(LOG_ERR, "Failed to write platform dump: %s (%d:%s)\n",
		       dump_path, errno, strerror(errno));
		goto err;
	}

	sz = write(out_fd, buf, bufsz);
	if (sz != bufsz) {
		syslog(LOG_ERR, "Failed to write platform dump: %s (%d:%s)\n",
		       dump_path, errno, strerror(errno));
		unlink(dump_path);
		goto err;
	}

	dir_fd = open(output_dir, O_RDONLY|O_DIRECTORY);

	rc = fsync(out_fd);
	if (rc == -1) {
		syslog(LOG_ERR, "Failed to sync platform dump: %s (%d:%s)\n",
		       dump_path, errno, strerror(errno));
		goto err;
	}

	rc = rename(dump_path, final_dump_path);

	if (rc == -1) {
		syslog(LOG_ERR, "Failed to rename platform dump %s to %s"
		       "(%d: %s)\n",
		       dump_path, final_dump_path, errno, strerror(errno));
		goto err;
	}

	rc = fsync(dir_fd);
	if (rc == -1) {
		syslog(LOG_ERR, "Failed to sync platform dump directory: %s"
		       " (%d:%s)\n", output_dir, errno, strerror(errno));
	}

	syslog(LOG_NOTICE, "New platform dump available. File: %s/%s\n",
	       output_dir, outfname);

	ret = 0;
err:
	if (in_fd != -1)
		close(in_fd);
	if (out_fd != -1)
		close(out_fd);
	if (dir_fd != -1)
		close(dir_fd);
	free(buf);
	return ret;
}

static int find_and_process_dumps(const char *opal_dump_dir,
				  const char *output_dir)
{
	int rc;
	int retval= 0;
	DIR *d;
	struct dirent *dirent;
	char dump_path[PATH_MAX];
	int is_dir= 0;
	struct stat sbuf;

	d = opendir(opal_dump_dir);
	if (d == NULL)
		return -1;

	while ((dirent = readdir(d))) {
		snprintf(dump_path, sizeof(dump_path), "%s/%s",
			 opal_dump_dir, dirent->d_name);

		is_dir = 0;

		if (dirent->d_name[0] == '.')
			continue;

		if (dirent->d_type == DT_DIR) {
			is_dir = 1;
		} else {
			/* Fall back to stat() */
			rc = stat(dump_path, &sbuf);
			if (S_ISDIR(sbuf.st_mode)) {
				is_dir = 1;
			}
		}

		if (is_dir) {
			rc = process_dump(dump_path, output_dir);
			if (rc != 0 && retval == 0)
				retval = -1;
			if (rc == 0 && retval >= 0)
				retval++;
			if (opt_ack_dump)
				ack_dump(dump_path);
		}
	}

	closedir(d);

	return retval;
}

int main(int argc, char *argv[])
{
	int opt;
	char sysfs_path[PATH_MAX];
	int rc;
	int fd;
	fd_set exceptfds;
	struct stat s;

	while ((opt = getopt(argc, argv, "As:o:wh")) != -1) {
		switch (opt) {
		case 'A':
			opt_ack_dump = 0;
			break;
		case 's':
			opt_sysfs = optarg;
			break;
		case 'o':
			opt_output_dir = optarg;
			break;
		case 'w':
			opt_wait = 1;
			break;
		case 'h':
			help(argv[0]);
			exit(EXIT_SUCCESS);
		default:
			help(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/firmware/opal/dump",
		 opt_sysfs);

	rc = stat(sysfs_path, &s);
	if (rc != 0) {
		fprintf(stderr, "Error accessing sysfs: %s (%d: %s)\n",
			sysfs_path, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	rc = stat(opt_output_dir, &s);
	if (rc != 0) {
		fprintf(stderr, "Error accessing output dir: %s (%d: %s)\n",
			opt_output_dir, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog("OPAL_DUMP", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

start:
	rc = find_and_process_dumps(sysfs_path, opt_output_dir);
	if (rc == 0 && opt_wait) {
		fd = open(sysfs_path, O_RDONLY|O_DIRECTORY);
		if (fd < 0)
			exit(EXIT_FAILURE);
		FD_ZERO(&exceptfds);
		FD_SET(fd, &exceptfds);
		rc = select(fd+1, NULL, NULL, &exceptfds, NULL);
		if (rc == -1)
			exit(EXIT_FAILURE);

		close(fd);

		goto start;
	}

	closelog();

	if (rc < 0)
		exit(EXIT_FAILURE);

	return 0;
}
