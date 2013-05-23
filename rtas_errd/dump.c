/**
 * @file dump.c
 * @brief Routines to handle platform dump and scanlog dump RTAS events
 *
 * Copyright (C) 2004 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <librtas.h>
#include <librtasevent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "rtas_errd.h"

#define DUMP_MAX_FNAME_LEN	40
#define DUMP_BUF_SZ		4096
#define EXTRACT_PLATDUMP_CMD	"/usr/sbin/extract_platdump"
#define SCANLOG_DUMP_FILE	"/proc/ppc64/scan-log-dump"
#define SCANLOG_DUMP_EXISTS	"/proc/device-tree/chosen/ibm,scan-log-data"
#define SYSID_FILE		"/proc/device-tree/system-id"
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
	char buf[20], *ret;

	fp = fopen(SYSID_FILE, "r");
	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	ret = (char *)malloc(20);
	if (ret) {
		/* Discard first 4 characters ("IBM,") */
		strncpy(ret, buf+4, 20);
		ret[19] = '\0';
		return ret;
	}

	return NULL;
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
	int rc;
	char system_arg[80];

	/* If SCANLOG_DUMP_FILE exists, then the module is either already
	 * loaded, or the procfs interface is built into the kernel rather
	 * than as a module. 
	 */
	if (load && !access(SCANLOG_DUMP_FILE, F_OK))
		return 0;

	if ((strlen(MODPROBE_PROGRAM) + strlen(SCANLOG_MODULE)
			+ (load ? 2 : 4)) > 79) {
		log_msg(NULL, "%s module could not be %s, buffer is not long "
			"enough to run %s", SCANLOG_MODULE, 
			(load ? "loaded" : "unloaded"), MODPROBE_PROGRAM);

		return 1;
	}

	sprintf(system_arg, "%s %s %s 2>/dev/null", MODPROBE_PROGRAM,
		(load ? "" : "-r"), SCANLOG_MODULE); 
	rc = system(system_arg);
	if (rc) {
		if (errno == ENOENT)
			return 1;
		log_msg(NULL, "%s module could not be %s, could not run "
			"%s; system call returned %d", SCANLOG_MODULE,
			(load ? "loaded" : "unloaded"), MODPROBE_PROGRAM, rc);

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
	char *temp;
	char scanlog_filename[80], scanlog_filename_bak[85];
	char *dump_buf = NULL;

	rc = load_scanlog_module(1);
	if (rc || access(SCANLOG_DUMP_EXISTS, F_OK))
		goto scanlog_out;

	/* A dump exists;  copy it off to the filesystem */
	if ((temp = get_machine_serial()) != 0) {
		sprintf(scanlog_filename, "%sscanoutlog.%s",
			d_cfg.scanlog_dump_path, temp);
		free(temp);
	} 
	else {
		sprintf(scanlog_filename, "%sscanoutlog.NOSERIAL",
			d_cfg.scanlog_dump_path);
	}

	if (!access(scanlog_filename, F_OK)) {
		/* A scanlog dump already exists on the filesystem;
		 * rename it with a .bak extension. 
		 */
		sprintf(scanlog_filename_bak, "%s.bak", scanlog_filename);
		rc = rename(scanlog_filename, scanlog_filename_bak);
		if (rc) {
			log_msg(NULL, "Could not rename %s, an existing "
				"scanlog dump will be copied over by a new "
				"dump, %s", scanlog_filename, strerror(errno));
		}
	}

	in = open(SCANLOG_DUMP_FILE, O_RDONLY);
	if (in <= 0) {
		log_msg(NULL, "Could not open %s for reading, %s", 
			SCANLOG_DUMP_FILE, strerror(errno));
		goto scanlog_error;
	}

	out = open(scanlog_filename, O_WRONLY | O_CREAT | O_TRUNC);
	if (out <= 0) {
		log_msg(NULL, "Could not open %s for writing, %s", 
			scanlog_filename, strerror(errno));
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
				scanlog_filename, strerror(errno));
			goto scanlog_error;
		}
	}

	if (bytes < 0)
		log_msg(NULL, "Could not read from %s, %s", SCANLOG_DUMP_FILE,
			strerror(errno));

	chmod(scanlog_filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	temp = scanlog_filename + strlen(d_cfg.scanlog_dump_path);
	bytes = strlen(temp);
	if ((scanlog = malloc(bytes + 1)) != 0)
		strncpy(scanlog, temp, bytes + 1);
	else {
		log_msg(NULL, "Could not allocate space for scanlog filename, "
			"%s. A scanlog dump could not be copied to the "
			"filesystem", strerror(errno)); 
		scanlog = NULL;
	}

scanlog_out:
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
	uint64_t dump_tag;
	char	filename[DUMP_MAX_FNAME_LEN + 20], *pos;
	char	pathname[DUMP_MAX_FNAME_LEN + 40];
	char	cmd_buf[60];
	FILE	*f;
	int	rc, bytes;

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

	/* Retrieve the dump */
	dump_tag = dump_scn->id;
	dump_tag |= ((uint64_t)dump_scn->v6hdr.subtype << 32);
	dbg("Dump ID: 0x%016LX", dump_tag);

	snprintf(cmd_buf, 60, "%s 0x%016LX", EXTRACT_PLATDUMP_CMD, 
		(long long unsigned int)dump_tag);

	/* sigchld_handler() messes up pclose(). */
	restore_sigchld_default();

	f = popen(cmd_buf, "r");
	if (f == NULL) {
		log_msg(event, "Failed to open pipe to %s.",
			EXTRACT_PLATDUMP_CMD);
		setup_sigchld_handler();
		return;
	}
	fgets(filename, DUMP_MAX_FNAME_LEN + 20, f);
	rc = pclose(f);

	setup_sigchld_handler();

	if ((rc == -1) || (WEXITSTATUS(rc) != 0)) {
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

	snprintf(pathname, DUMP_MAX_FNAME_LEN + 40, "%s/%s",
		 d_cfg.platform_dump_path, filename);
	platform_log_write("Platform Dump Notification\n");
	platform_log_write("    Dump Location: %s\n", pathname);

	return;
}
