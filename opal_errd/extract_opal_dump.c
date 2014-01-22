/**
 * @file	extract_opal_dump.c
 * @brief	Command to extract a platform dump on PowerNV platform
 *		and copy it to the filesystem
 *
 * Copyright (C) 2014 IBM Corporation
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Return codes */
#define E_SUCCESS	0
#define E_PERM		1
#define E_MOUNT_FS	2
#define E_FILE_OP	3
#define E_NOMEM		4
#define E_PARTIAL	5

#define DUMP_HDR_PREFIX_OFFSET	0x16	/* Prefix size in dump header */
#define DUMP_HDR_FNAME_OFFSET	0x18	/* Suggested filename in dump header */
#define DUMP_MAX_FNAME_LEN	48	/* Including .PARTIAL */

/* Dump Source files */
#define DUMP_FILE_AVAIL		"fsp/dump_avail"
#define DUMP_FILE_DATA		"fsp/dump"
#define DUMP_FILE_ACK		"fsp/dump_control"

#define DUMP_BUF_SZ		4096
#define PATH_LENGTH		64

/* Default destination dir to store dump */
char *dest_dir = "/var/log/dump";

/* Default debugfs mount point */
char *debugfs_path = "/sys/kernel/debug";


/**
 * Helper routines to open/close file
 */
static inline FILE *dump_file_open(char *path, char *file, char *mode)
{
	char	filename[PATH_LENGTH];
	FILE	*fp;

	snprintf(filename, PATH_LENGTH, "%s/%s", path, file);
	fp = fopen(filename, mode);
	return fp;
}

static inline void dump_file_close(FILE *fp)
{
	fclose(fp);
}

static inline int dump_read_data_file(FILE *fp, char *buf, int bsize)
{
	return fread(buf, 1, bsize, fp);
}

/**
 * Check debugfs mounted or not
 *
 * Note:
 *   At present we check /sys/kernel/debug path and if not mounted
 *   we log a message to syslog. But its possible that debugfs can
 *   be mounted on to different path.
 *
 * FIXME:
 *   If required, add support to get debugfs mount path dynamically.
 */
static int check_debugfs(void)
{
	int	rc;
	char	pathname[PATH_LENGTH];

	snprintf(pathname, PATH_LENGTH,
		 "%s/%s", debugfs_path, DUMP_FILE_AVAIL);
	rc = access(pathname, F_OK);
	if (rc) {
		syslog(LOG_NOTICE, "debugfs is not mounted or mounted "
		       "at different path. Please mount debugfs at %s "
		       "to extract platform dump(s).\n", debugfs_path);
		return E_MOUNT_FS;
	}
	return E_SUCCESS;
}

/**
 * Check to see if a new dump available, and if so,
 * copy it to the filesystem.
 */
static int dump_check_avail(char *avail)
{
	int	rc;
	FILE	*fp;

	fp = dump_file_open(debugfs_path, DUMP_FILE_AVAIL, "r");
	if (!fp) {
		syslog(LOG_NOTICE, "Dump extraction failed, couldn't open "
		       "%s/%s file.\n", debugfs_path, DUMP_FILE_AVAIL);
		return E_FILE_OP;
	}

	rc = fread(avail, sizeof(*avail), 1, fp);
	dump_file_close(fp);

	if (rc != 1)	/* read failed */
		return E_FILE_OP;

	return E_SUCCESS;
}

/**
 * Acknowledge dump
 */
static int dump_ack(void)
{
	char	ack = '1';
	int	rc;
	FILE	*fp;

	fp = dump_file_open(debugfs_path, DUMP_FILE_ACK, "w");
	if (!fp) {
		syslog(LOG_NOTICE, "Dump ACK failed, couldn't open "
		       "%s/%s file.\n", debugfs_path, DUMP_FILE_ACK);
		return E_FILE_OP;
	}

	rc = fwrite(&ack, sizeof(ack), 1, fp);
	dump_file_close(fp);

	if (rc != 1)
		return E_FILE_OP;

	return E_SUCCESS;
}

/**
 * Retreive the prefix size and suggested filename for the dump
 * from the dump header
 */
static void dump_get_file_name(char *buf, int bsize,
			       char *dfile, uint16_t *prefix_size)
{
	if (bsize >= DUMP_HDR_PREFIX_OFFSET + sizeof(uint16_t))
		*prefix_size = *(uint16_t *)(buf + DUMP_HDR_PREFIX_OFFSET);

	if (bsize >= DUMP_HDR_FNAME_OFFSET + DUMP_MAX_FNAME_LEN)
		strncpy(dfile, buf + DUMP_HDR_FNAME_OFFSET,
			DUMP_MAX_FNAME_LEN);
	else
		strcpy(dfile, "platform.dumpid.PARTIAL");

	dfile[DUMP_MAX_FNAME_LEN - 1] = '\0';
}

/* Create the platform dump directory if necessary */
static int dump_dest_create_dir(void)
{
	int rc;

	rc = mkdir(dest_dir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if (rc && errno != EEXIST) {
		syslog(LOG_NOTICE, "Could not create %s : %s\nThe platform "
		       "dump could not be copied to the platform dump "
		       "directory.\n", dest_dir, strerror(errno));
		return E_FILE_OP;
	}
	return E_SUCCESS;
}

/* Create destination file */
static FILE *dump_dest_create_file(char *filename)
{
	FILE *fp;

	fp = dump_file_open(dest_dir, filename, "wb+");
	if(!fp) {
		syslog(LOG_NOTICE, "Could not open %s/%s for writing (%s).\n"
		       "The platform dump could not be retrieved.\n",
		       dest_dir, filename, strerror(errno));
		return NULL;
	}
	return fp;
}

static void dump_dest_rename_file(char *old)
{
	char new[DUMP_MAX_FNAME_LEN];

	strncpy(new, old, DUMP_MAX_FNAME_LEN);
	strcat(new, ".PARTIAL");
	if (rename(old, new) != 0)
		syslog(LOG_NOTICE, "Could not rename %s to %s\n", old, new);
}

/**
 * If needed, remove any old dump files
 *
 * FIXME:
 *   Presently we are deleting old files based on prefix size. Need to
 *   introduce PlatformDumpMax config option.
 */
static void dump_remove_old_files(char *dump_dir,
				  char *dumpname, int prefix_size)
{
	struct dirent	*entry;
	DIR		*dir;
	char		pathname[PATH_LENGTH + DUMP_MAX_FNAME_LEN];

	dir = opendir(dump_dir);
	if (!dir) {
		if (errno == ENOENT)
			syslog(LOG_NOTICE, "Directory does not exists : %s\n",
			       dump_dir);
		else
			syslog(LOG_NOTICE, "Unable to open directory : %s\n",
			       dump_dir);
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		if ((strcmp(entry->d_name, ".") == 0) ||
		    (strcmp(entry->d_name, "..") == 0))
			continue;

		if (strncmp(dumpname, entry->d_name, prefix_size) != 0)
			continue;

		snprintf(pathname, PATH_LENGTH + DUMP_MAX_FNAME_LEN,
			 "%s/%s", dump_dir, entry->d_name);

		if (unlink(pathname) < 0) {
			syslog(LOG_NOTICE, "Could not delete file \"%s\" to "
			       "make room for incoming platform dump \"%s\" "
			       "(%s). The new dump will be saved anyways.\n",
			       pathname, dumpname, strerror(errno));
		}
	}
	closedir(dir);
}

/**
 * Extract a platform dump and write to the filesystem
 */
static int extract_opal_dump(void)
{
	bool	 partial = false;
	char	 *buf = NULL;
	char	 avail = '0';
	char	 dfile[DUMP_MAX_FNAME_LEN];
	uint16_t prefix_size = 7;
	int	 bsize;
	int	 len;
	int	 rc;
	FILE	 *fp;
	FILE	 *fout;

	/* Check dump availability */
	rc = dump_check_avail(&avail);
	if (rc != E_SUCCESS)
		return rc;

	if (avail != '1')	/* No dump available */
		return E_SUCCESS;

	/* Open dump data file */
	fp = dump_file_open(debugfs_path, DUMP_FILE_DATA, "r");
	if (!fp) {
		syslog(LOG_NOTICE, "Dump extraction failed, couldn't open "
		       "%s/%s file.\n", debugfs_path, DUMP_FILE_DATA);
		return E_FILE_OP;
	}

	buf = malloc(DUMP_BUF_SZ);
	if (!buf) {
		syslog(LOG_NOTICE, "Could not allocate buffer to retrieve "
		       "dump (%s).\n", strerror(errno));
		rc = E_NOMEM;
		goto out;
	}

	/* Extract dump filename */
	bsize = dump_read_data_file(fp, buf, DUMP_BUF_SZ);
	if (bsize < DUMP_BUF_SZ)
		goto out_mem;
	dump_get_file_name(buf, bsize, dfile, &prefix_size);

	/* Create output dir */
	rc = dump_dest_create_dir();
	if (rc != E_SUCCESS)
		goto out_mem;

	/*
	 * Before writing the new dump out, we need to see if any older
	 * dumps need to be removed first
	 */
	dump_remove_old_files(dest_dir, dfile, prefix_size);

	/* Create output file */
	fout = dump_dest_create_file(dfile);
	if (!fout) {
		rc = E_FILE_OP;
		goto out_mem;
	}

	/* Write dump to filesystem */
	len = fwrite(buf, 1, bsize, fout);
	if (len < bsize) {
		syslog(LOG_NOTICE, "Partially read platform dump (Could "
		       "not write to %s/%s).\n", dest_dir, dfile);
		partial = true;
		goto out_partial;
	}

	while (!feof(fp)) {
		bsize = dump_read_data_file(fp, buf, DUMP_BUF_SZ);
		if (bsize <= 0) {
			partial = true;
			syslog(LOG_NOTICE, "Partially read platform "
			       "dump (Could not read from %s/%s).\n",
			       debugfs_path, DUMP_FILE_DATA);
			break;
		}
		len = fwrite(buf, 1, bsize, fout);
		if (len < bsize) {
			partial = true;
			syslog(LOG_NOTICE, "Partially read platform "
			       "dump (Could not write to %s/%s).\n",
			       dest_dir, dfile);
			break;
		}
	}

	syslog(LOG_NOTICE, "New platform dump available. File : %s/%s\n",
	       dest_dir, dfile);

out_partial:
	/* ACK dump */
	rc = dump_ack();

	/* Dump partially read? */
	if (partial) {
		dump_dest_rename_file(dfile);
		rc = E_PARTIAL;
	}

	/* close output file */
	dump_file_close(fout);

out_mem:
	free(buf);
out:
	dump_file_close(fp);
	return rc;
}

int main(int argc, char *argv[])
{
	int rc = E_SUCCESS;

	/* syslog initialization */
	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog("OPAL_DUMP", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	if (geteuid() != 0) {
		syslog(LOG_NOTICE,
		       "%s: Requires superuser privileges.\n", argv[0]);
		return E_PERM;
	}

	/* FIXME: Read config file */

	rc = check_debugfs();
	if (rc != E_SUCCESS)
		goto out;

	rc = extract_opal_dump();

out:
	closelog();
	return rc;
}
