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

#define DEFAULT_SYSFS_PATH	"/sys"
#define DEFAULT_OUTPUT_DIR	"/var/log/dump"
#define DUMP_TYPE_LEN		7

/* Retention policy : default maximum dumps of each type */
#define DEFAULT_MAX_DUMP	4

int opt_ack_dump = 1;
int opt_wait = 0;
int opt_max_dump = DEFAULT_MAX_DUMP;

char *opt_sysfs = DEFAULT_SYSFS_PATH;
char *opt_output_dir = DEFAULT_OUTPUT_DIR;

static void help(const char* argv0)
{
	fprintf(stderr, "%s help:\n", argv0);
	fprintf(stderr, "\n");
	fprintf(stderr, "-A     - Don't acknowledge dump\n");
	fprintf(stderr, "-s dir - sysfs directory (default %s)\n",
		DEFAULT_SYSFS_PATH);
	fprintf(stderr, "-o dir - directory to save dumps (default %s)\n",
		DEFAULT_OUTPUT_DIR);
	fprintf(stderr, "-m max - maximum number of dumps of a specific type"
		" to be saved\n");
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
	}

	close(fd);
}

/**
 * Check for duplicate file
 */
static void check_dup_dump_file(char *dumpname)
{
	struct stat sbuf;
	char dump_path[PATH_MAX];
	int rc;

	rc = snprintf(dump_path, PATH_MAX, "%s/%s", opt_output_dir, dumpname);
	if (rc >= PATH_MAX) {
		syslog(LOG_NOTICE, "Path to dump file (%s) is too big",
		       dumpname);
		return;
	}

	if (stat(dump_path, &sbuf) == -1)
		return;

	if (unlink(dump_path) < 0)
		syslog(LOG_NOTICE, "Could not delete file \"%s\" "
		       "(%s) to make room for incoming platform dump."
		       " The new dump will be saved anyways.\n",
		       dump_path, strerror(errno));
}

static int timesort(const struct dirent **file1, const struct dirent **file2)
{
	struct stat sbuf1, sbuf2;
	char dump_path1[PATH_MAX];
	char dump_path2[PATH_MAX];
	int rc;

	snprintf(dump_path1, PATH_MAX, "%s/%s",
		 opt_output_dir, (*file1)->d_name);

	snprintf(dump_path2, PATH_MAX, "%s/%s",
		 opt_output_dir, (*file2)->d_name);

	rc = stat(dump_path1, &sbuf1);
	if (rc < 0)
		return rc;

	rc = stat(dump_path2, &sbuf2);
	if (rc < 0)
		return rc;

	return (sbuf2.st_mtime - sbuf1.st_mtime);
}

/**
 * remove_dump_files
 * @brief if needed, remove any old dump files
 *
 * Users can specify the number of old dumpfiles they wish to save
 * via command line option. This routine will search through and remove
 * any dump files of the specified type if the count exceeds the maximum value.
 *
 */
static void remove_dump_files(char *dumpname)
{
	struct dirent **namelist;
	struct dirent *dirent;
	char dump_path[PATH_MAX];
	int i;
	int n;
	int count = 0;

	check_dup_dump_file(dumpname);

	n = scandir(opt_output_dir, &namelist, NULL, timesort);
	if (n < 0)
		return;

	for (i = 0; i < n; i++) {
		dirent = namelist[i];

		/* Skip dump files of different type */
		if (dirent->d_name[0] == '.' ||
		    strncmp(dumpname, dirent->d_name, DUMP_TYPE_LEN)) {
			free(namelist[i]);
			continue;
		}

		count++;
		snprintf(dump_path, PATH_MAX, "%s/%s",
			 opt_output_dir, dirent->d_name);

		free(namelist[i]);

		if (count < opt_max_dump)
			continue;

		if (unlink(dump_path) < 0)
			syslog(LOG_NOTICE, "Could not delete file \"%s\" "
			"(%s) to make room for incoming platform dump."
			" The new dump will be saved anyways.\n",
			dump_path, strerror(errno));
	}

	free(namelist);
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
	buf = malloc(bufsz);
	if (!buf) {
		syslog(LOG_ERR, "Failed to allocate memory for dump\n");
		return -1;
	}

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

	remove_dump_files(outfname);

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
	struct dirent **namelist;
	struct dirent *dirent;
	char dump_path[PATH_MAX];
	int is_dir= 0;
	struct stat sbuf;
	int n;
	int i;

	n = scandir(opal_dump_dir, &namelist, NULL, alphasort);
	if (n < 0)
		return -1;

	for (i = 0; i < n; i++) {
		dirent = namelist[i];

		if (dirent->d_name[0] == '.') {
			free(namelist[i]);
			continue;
		}

		snprintf(dump_path, sizeof(dump_path), "%s/%s",
			 opal_dump_dir, dirent->d_name);

		is_dir = 0;

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
		free(namelist[i]);
	}

	free(namelist);

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

	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog("OPAL_DUMP", LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR,
		LOG_LOCAL1);

	while ((opt = getopt(argc, argv, "As:o:m:wh")) != -1) {
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
		case 'm':
			opt_max_dump = atoi(optarg);
			if (opt_max_dump <= 0) {
				syslog(LOG_ERR, "Invalid value specified for "
					"-m option (%d), using default value %d\n",
					opt_max_dump, DEFAULT_MAX_DUMP);
				opt_max_dump = DEFAULT_MAX_DUMP;
			}
			break;
		case 'w':
			opt_wait = 1;
			break;
		case 'h':
			help(argv[0]);
			closelog();
			exit(EXIT_SUCCESS);
		default:
			help(argv[0]);
			closelog();
			exit(EXIT_FAILURE);
		}
	}

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/firmware/opal/dump",
		 opt_sysfs);

	rc = stat(sysfs_path, &s);
	if (rc != 0) {
		syslog(LOG_ERR, "Error accessing sysfs: %s (%d: %s)\n",
		       sysfs_path, errno, strerror(errno));
		goto err_out;
	}

	rc = stat(opt_output_dir, &s);
	if (rc != 0) {
		syslog(LOG_ERR, "Error accessing output dir: %s (%d: %s)\n",
		       opt_output_dir, errno, strerror(errno));
		goto err_out;
	}

start:
	rc = find_and_process_dumps(sysfs_path, opt_output_dir);
	if (rc == 0 && opt_wait) {
		fd = open(sysfs_path, O_RDONLY|O_DIRECTORY);
		if (fd < 0) {
			rc = -1;
			goto err_out;
		}
		FD_ZERO(&exceptfds);
		FD_SET(fd, &exceptfds);
		rc = select(fd+1, NULL, NULL, &exceptfds, NULL);
		close(fd);

		if (rc == -1)
			goto err_out;

		goto start;
	}

err_out:
	closelog();

	if (rc < 0)
		exit(EXIT_FAILURE);

	return 0;
}
