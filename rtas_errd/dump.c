/**
 * @file dump.c
 * @brief Routines to handle platform dump and scanlog dump RTAS events
 *
 * Copyright (C) 2004 IBM Corporation
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <librtas.h>
#include <librtasevent.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include "utils.h"
#include "rtas_errd.h"

#define DUMP_MAX_FNAME_LEN	40
#define DUMP_BUF_SZ		4096
#define EXTRACT_PLATDUMP_CMD	"/usr/sbin/extract_platdump"
#define SCANLOG_DUMP_FILE	"/proc/ppc64/scan-log-dump"
#define SCANLOG_DUMP_EXISTS	"/proc/device-tree/chosen/ibm,scan-log-data"
#define SYSID_FILE		"/proc/device-tree/system-id"
#define SYSID_VENDOR_FILE	"/proc/device-tree/ibm,vendor-system-id"
#define SCANLOG_MODULE		"scanlog"
#define MODPROBE_PROGRAM	"/sbin/modprobe"

/**
 * get_machine_serial
 * @brief Retrieve a machines serial number
 *
 * Return a string containing the machine's serial number, obtained
 * from procfs (the file named SYSID_FILE).  This routine mallocs a
 * new string; it is the caller's responsibility to ensure that the
 * string is freed.
 *
 * @return pointer to (allocated) string, NULL on failure
 */
static char *
get_machine_serial()
{
	FILE *fp;
	char buf[20] = {0,}, *ret = NULL;
	char *filename = SYSID_VENDOR_FILE;

	/*
	 * Odds of SYSID_FILE, open failing is almost none.
	 * But, better to catch the odds.
	 */
	fp = fopen(filename, "r");
	if (!fp) {
		filename = SYSID_FILE;
		fp = fopen(filename, "r");
		if (!fp) {
			log_msg(NULL, "%s: Failed to open %s, %s", __func__,
					filename, strerror(errno));
			return ret;
		}
	}

	if (fgets(buf, sizeof(buf), fp) == NULL) {
		log_msg(NULL, "%s: Reading file %s failed, %s",
				__func__, filename, strerror(errno));
	} else {
		ret = strdup(buf + 4);
		if (!ret) {
			log_msg(NULL, "%s: Memory allocation failed, %s",
				__func__, strerror(errno));
		} /* strdup */
	}

	fclose(fp);
	return ret;
}

/**
 * load_scanlog_module
 * @brief Load/Unload scanlog module
 *
 * Loads or unloads the scanlog module.  The load parameter should 
 * be 1 to load the module, or 0 to unload it.
 *
 * @param load directive to load/unload module
 * @return 0 on success, !0 on failure.
 */
static int
load_scanlog_module(int load)
{
	int rc, i = 0;
	pid_t cpid;			/* child pid			*/
	char *system_args[5] = {NULL,};	/* **argv passed to execv	*/
	int status;			/* exit status of child		*/

	/* If SCANLOG_DUMP_FILE exists, then the module is either already
	 * loaded, or the procfs interface is built into the kernel rather
	 * than as a module. 
	 */
	if (load && !access(SCANLOG_DUMP_FILE, F_OK))
		return 0;

	system_args[i++] = MODPROBE_PROGRAM;
	system_args[i++] = "-q";
	if (!load)
		system_args[i++] = "-r";
	system_args[i++] = SCANLOG_MODULE;

	cpid = fork();
	if (cpid == -1) {
		log_msg(NULL, "Fork failed, while probing for scanlog\n");
		return 1;
	} /* fork */

	if (cpid == 0 ) { /* child */
		rc = execv(system_args[0], system_args);
		if (errno != ENOENT) {
			log_msg(NULL, "%s module could not be %s, could not run "
				"%s; system call returned %d", SCANLOG_MODULE,
				(load ? "loaded" : "unloaded"), MODPROBE_PROGRAM, rc);
		} else {
			log_msg(NULL, "module %s not found", SCANLOG_MODULE);
		}

		exit(-2);
	} else { /* parent */
		rc = waitpid(cpid, &status, 0);
		if (rc == -1) {
			log_msg(NULL, "wait failed, while probing for "
				"scanlog\n");
			return 1;
		}

		if ((signed char)WEXITSTATUS(status) == -2) /* exit on failure */
			return 1;
	}

	return 0;
}

/**
 * check_scanlog_dump
 * @brief Check for new scanlog dumps
 * 
 * This routine checks to see if a new scanlog dump is available, and 
 * if so, copies it to the filesystem.  The return value is the filename 
 * of the new scanlog dump, or NULL if one is not copied.  This routine 
 * will malloc space for the returned string;  it is up to the caller to 
 * free it.
 *		
 * This routine should be invoked once when the daemon is started.
 */
void
check_scanlog_dump(void)
{
	int rc, in = -1, out = -1, bytes;
	char *temp = NULL;
	char *scanlog_file = NULL, *scanlog_file_bak = NULL;
	char *dump_buf = NULL;

	rc = load_scanlog_module(1);
	if (rc || access(SCANLOG_DUMP_EXISTS, F_OK))
		goto scanlog_out;

	/* A dump exists;  copy it off to the filesystem */
	if ((temp = get_machine_serial()) != 0) {
		if (asprintf(&scanlog_file, "%sscanoutlog.%s",
			d_cfg.scanlog_dump_path, temp) < 0) {
			free(temp);
			goto scanlog_out;
		}
		free(temp);
	}
	else {
		if (asprintf(&scanlog_file, "%sscanoutlog.NOSERIAL",
			     d_cfg.scanlog_dump_path) < 0)
			goto scanlog_out;
	}

	if (!access(scanlog_file, F_OK)) {
		/* A scanlog dump already exists on the filesystem;
		 * rename it with a .bak extension. 
		 */
		if (asprintf(&scanlog_file_bak, "%s.bak", scanlog_file) < 0)
			goto scanlog_out;
		rc = rename(scanlog_file, scanlog_file_bak);
		if (rc) {
			log_msg(NULL, "Could not rename %s, an existing "
				"scanlog dump will be copied over by a new "
				"dump, %s", scanlog_file, strerror(errno));
		}
	}
	free(scanlog_file_bak);

	in = open(SCANLOG_DUMP_FILE, O_RDONLY);
	if (in <= 0) {
		log_msg(NULL, "Could not open %s for reading, %s",
			SCANLOG_DUMP_FILE, strerror(errno));
		goto scanlog_error;
	}

	out = open(scanlog_file, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (out <= 0) {
		log_msg(NULL, "Could not open %s for writing, %s",
			scanlog_file, strerror(errno));
		goto scanlog_error;
	}

	dump_buf = malloc(DUMP_BUF_SZ);
	if (dump_buf == NULL) {
		log_msg(NULL, "Could not allocate buffer to read in "
			"scanlog dump, %s", strerror(errno));
		goto scanlog_error;
	}

	while ((bytes = read(in, dump_buf, DUMP_BUF_SZ)) > 0) {
		rc = write(out, dump_buf, bytes);
		if (rc < bytes) {
			log_msg(NULL, "Could not write to %s, %s",
				scanlog_file, strerror(errno));
			goto scanlog_error;
		}
	}

	if (bytes < 0)
		log_msg(NULL, "Could not read from %s, %s", SCANLOG_DUMP_FILE,
			strerror(errno));

	temp = scanlog_file + strlen(d_cfg.scanlog_dump_path);
	scanlog = strdup(temp);
	if (scanlog == NULL) {
		log_msg(NULL, "Could not allocate space for scanlog filename, "
			"%s. A scanlog dump could not be copied to the "
			"filesystem", strerror(errno));
	} /* strdup(scanlog) */

scanlog_out:
	free(scanlog_file);
	if (in != -1)
		close(in);
	if (out != -1)
		close(out);
	if (dump_buf != NULL)
		free(dump_buf);

	load_scanlog_module(0);

	return;

scanlog_error:
	log_msg(NULL, "Due to previous error a scanlog dump could not be"
		"copied to the filesystem");
	goto scanlog_out;
}

/**
 * check_platform_dump
 * @brief Check RTAS event for a platform dump
 *
 * Parses error information to determine if it indicates the availability 
 * of a platform dump.  The platform dump is copied to the filesystem, 
 * and the error log is updated to indicate the path to the dump.
 *		
 * This should be invoked _before_ the error information is written to 
 * LOG_FILE, because the error may need to be updated with the path to 
 * the dump.
 *
 * @param event pointer to struct event
 */
void
check_platform_dump(struct event *event)
{
	struct rtas_dump_scn *dump_scn;
	struct statvfs vfs;
	uint64_t dump_tag;
	uint64_t dump_size;
	char	filename[DUMP_MAX_FNAME_LEN + 20], *pos;
	char	*pathname = NULL;
	FILE	*f;
	int	rc, bytes;
	char    *system_args[3] = {NULL, };     /* execv arguments      */
	char 	tmp_sys_arg[60];		/* tmp sys_args		*/
	pid_t	cpid;                           /* child pid            */

	dump_scn = rtas_get_dump_scn(event->rtas_event);
	if (dump_scn == NULL)
		return;

	dbg("platform dump found");
	snprintf(event->addl_text, ADDL_TEXT_MAX, "Platform Dump "
		"(could not be retrieved to filesystem)");

	if (dump_scn->v6hdr.subtype == 0) {
		dbg("ignoring platform dump with subtype 0");
		return;
	}

	/* Retrieve the dump tag */
	dump_tag = dump_scn->id;
	dump_tag |= ((uint64_t)dump_scn->v6hdr.subtype << 32);
	dbg("Dump ID: 0x%016LX", dump_tag);

	if (statvfs(d_cfg.platform_dump_path, &vfs) == -1) {
		log_msg(event, "statvfs() failed on %s: %s",
				d_cfg.platform_dump_path, strerror(errno));
		return;
	}

	/* Retrieve the size of the platform dump */
	dump_size = dump_scn->size_hi;
	dump_size <<= 32;
	dump_size |= dump_scn->size_lo;

	/* Check if there is sufficient space in the file system to store the dump */
	if (vfs.f_bavail * vfs.f_frsize < dump_size) {
		syslog(LOG_ERR, "Insufficient space in %s to store platform dump for dump ID: "
				"0x%016lX (required: %lu bytes, available: %lu bytes)",
				d_cfg.platform_dump_path, dump_tag, dump_size,
				(vfs.f_bavail * vfs.f_frsize));
		syslog(LOG_ERR, "After clearing space, run 'extract_platdump "
				"0x%016lX'.\n", dump_tag);
		return;
	}

	/* Retrieve the dump */
	snprintf(tmp_sys_arg, 60, "0x%016LX", (long long unsigned int)dump_tag);
	system_args[0] = EXTRACT_PLATDUMP_CMD;
	system_args[1] = tmp_sys_arg;

	/* sigchld_handler() messes up pclose(). */
	restore_sigchld_default();

	f = spopen(system_args, &cpid);
	if (f == NULL) {
		log_msg(event, "Failed to open pipe to %s.",
			EXTRACT_PLATDUMP_CMD);
		setup_sigchld_handler();
		return;
	}
	if (!fgets(filename, DUMP_MAX_FNAME_LEN + 20, f)) {
		dbg("Failed to collect filename info");
		spclose(f, cpid);
		setup_sigchld_handler();
		return;
	}
	rc = spclose(f, cpid);

	setup_sigchld_handler();

	if (rc) {
		dbg("%s failed to extract the dump", EXTRACT_PLATDUMP_CMD);
		return;
	}

	if ((pos = strchr(filename, '\n')) != NULL)
		*pos = '\0';

	dbg("Dump Filename: %s", filename);
	event->flags |= RE_PLATDUMP_AVAIL;

	/* Update the raw and parsed events with the dump path and length */
	update_os_id_scn(event->rtas_event, filename);
	memcpy(dump_scn->os_id, filename, DUMP_MAX_FNAME_LEN);
	bytes = strlen(filename);
	if ((bytes % 4) > 0)
		bytes += (4 - (bytes % 4));
	dump_scn->id_len = bytes;

	if (asprintf(&pathname, "%s/%s",
		 d_cfg.platform_dump_path, filename) < 0) {
		dbg("%s: Failed to allocate memory", __func__);
		return;
	}
	platform_log_write("Platform Dump Notification\n");
	platform_log_write("    Dump Location: %s\n", pathname);
	free(pathname);

	return;
}
