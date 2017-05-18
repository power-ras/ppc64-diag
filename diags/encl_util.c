/*
 * Copyright (C) 2012, 2015 IBM Corporation
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

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

#include "utils.h"
#include "encl_util.h"


/**
 * print_raw_data
 * @brief Dump a section of raw data
 *
 * @param ostream stream to which output should be written
 * @param data pointer to data to dump
 * @param data_len length of data to dump
 * @return number of bytes written
 */
int
print_raw_data(FILE *ostream, char *data, int data_len)
{
	char *h, *a;
	char *end = data + data_len;
	unsigned int offset = 0;
	int i, j;
	int len = 0;

	len += fprintf(ostream, "\n");

	h = a = data;

	while (h < end) {
		/* print offset */
		len += fprintf(ostream, "0x%04x:  ", offset);
		offset += 16;

		/* print hex */
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (h < end)
					len += fprintf(ostream, "%02x", *h++);
				else
					len += fprintf(ostream, "  ");
			}
			len += fprintf(ostream, " ");
		}

		/* print ascii */
		len += fprintf(ostream, "    [");
		for (i = 0; i < 16; i++) {
			if (a < end) {
				if ((*a >= ' ') && (*a <= '~'))
					len += fprintf(ostream, "%c", *a);
				else
					len += fprintf(ostream, ".");
				a++;
			} else
				len += fprintf(ostream, " ");
		}
		len += fprintf(ostream, "]\n");
	}

	return len;
}

/**
 * open_sg_device
 * @brief Open sg device for read/write operation
 *
 * @param encl sg device name
 */
int
open_sg_device(const char *encl)
{
	char dev_sg[PATH_MAX];
	int fd;

	snprintf(dev_sg, PATH_MAX, "/dev/%s", encl);
	fd = open(dev_sg, O_RDWR);
	if (fd < 0)
		perror(dev_sg);
	return fd;
}

/**
 * close_sg_device
 * @brief Close sg device
 *
 * @param opened encl sg device file descriptor
 */
int
close_sg_device(int fd)
{
	if (fd < 0)
		return 0;

	return close(fd);
}

/**
 * read_page2_from_file
 * @brief Read enclosure status diagnostics structure from file
 */
int
read_page2_from_file(const char *path, bool display_error_msg,
		     void *pg, int size)
{
	FILE *f;

	f = fopen(path, "r");
	if (!f) {
		if (display_error_msg || errno != ENOENT)
			perror(path);
		return -1;
	}
	if (fread(pg, size, 1, f) != 1) {
		if (display_error_msg)
			perror(path);
		fclose(f);
		return -2;
	}
	fclose(f);
	return 0;
}

/**
 * write_page2_to_file
 * @brief Write enclosure status diagnostics structure to file
 */
int
write_page2_to_file(const char *path, void *pg, int size)
{
	FILE *f;

	f = fopen(path, "w");
	if (!f) {
		perror(path);
		return -1;
	}
	if (fwrite(pg, size, 1, f) != 1) {
		perror(path);
		fclose(f);
		return -2;
	}
	fclose(f);
	return 0;
}

/*
 * enclosure_maint_mode
 * @brief Check the state of SCSI enclosure
 *
 * Returns:
 *	-1 on failure
 *	1 if sg is offline
 *	0 if sg is running
 */
int
enclosure_maint_mode(const char *sg)
{
	char devsg[PATH_MAX];
	char sgstate[128];
	FILE *fp;

	snprintf(devsg, PATH_MAX,
		 "/sys/class/scsi_generic/%s/device/state", sg);
	fp = fopen(devsg, "r");
	if (!fp) {
		perror(devsg);
		fprintf(stderr, "Unable to open enclosure "
				"state file : %s\n", devsg);
		return -1;
	}

	if (fgets_nonl(sgstate, 128, fp) == NULL) {
		fprintf(stderr, "Unable to read the state of "
				"enclosure device : %s\n", sg);
		fclose(fp);
		return -1;
	}

	/* Check device state */
	if (!strcmp(sgstate, "offline")) {
		fprintf(stderr, "Enclosure \"%s\" is offline.\n Cannot run"
				" diagnostics/LED operations.\n", sg);
		fclose(fp);
		return 1;
	}

	fclose(fp);
	return 0;
}

/**
 * do_ses_cmd
 * @brief Make the necessary sg ioctl() to do the indicated SES command
 *
 * @param fd a file descriptor to the appropriate /dev/sg<x> file
 * @param cmd the command code -- e.g., SEND_DIAGNOSTIC
 * @param page_nr the SES page number to be read or written -- 0 for write
 * @param flags flags for the second byte of the SCSI command buffer
 * @param cmd_len length in bytes of relevant data in SCSI command buffer
 * @paran dxfer_direction SG_DXFER_FROM_DEV or SG_DXFER_TO_DEV
 * @param buf the contents of the page
 * @param buf_len the length of the previous parameter
 * @return 0 on success, -EIO on invalid I/O status, or CHECK_CONDITION
 */
int
do_ses_cmd(int fd, uint8_t cmd, uint8_t page_nr, uint8_t flags,
	   uint8_t cmd_len, int dxfer_direction, void *buf, int buf_len)
{
	unsigned char scsi_cmd_buf[16] = {
		cmd,
		flags,
		page_nr,
		(buf_len >> 8) & 0xff,	/* most significant byte */
		buf_len & 0xff,		/* least significant byte */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	struct sense_data_t sense_data;
	sg_io_hdr_t hdr;
	int i, rc;

	for (i = 0; i < 3; i++) {
		memset(&hdr, 0, sizeof(hdr));
		memset(&sense_data, 0, sizeof(struct sense_data_t));

		hdr.interface_id = 'S';
		hdr.dxfer_direction = dxfer_direction;
		hdr.cmd_len = cmd_len;
		hdr.mx_sb_len = sizeof(sense_data);
		hdr.iovec_count = 0;	/* scatter gather not necessary */
		hdr.dxfer_len = buf_len;
		hdr.dxferp = buf;
		hdr.cmdp = scsi_cmd_buf;
		hdr.sbp = (unsigned char *)&sense_data;
		hdr.timeout = 120 * 1000;	/* set timeout to 2 minutes */
		hdr.flags = 0;
		hdr.pack_id = 0;
		hdr.usr_ptr = 0;

		rc = ioctl(fd, SG_IO, &hdr);

		if ((rc == 0) && (hdr.masked_status == CHECK_CONDITION))
			rc = CHECK_CONDITION;
		else if ((rc == 0) && (hdr.host_status || hdr.driver_status))
			rc = -EIO;

		if (rc == 0 || hdr.host_status == 1)
			break;
	}

	return rc;
}

int
get_diagnostic_page(int fd, uint8_t cmd, uint8_t page_nr, void *buf,
		    int buf_len)
{
	return do_ses_cmd(fd, cmd, page_nr, 0x1, 16, SG_DXFER_FROM_DEV,
								buf, buf_len);
}

/* Read a line and strip the newline. */
char *
fgets_nonl(char *buf, int size, FILE *s)
{
	char *nl;

	if (!fgets(buf, size, s))
		return NULL;
	nl = strchr(buf, '\n');
	if (nl == NULL)
		return NULL;
	*nl = '\0';
	return buf;
}

/*
 * Find at least 2 consecutive periods after pos, skip to the end of that
 * set of periods, and return a pointer to the next character.
 *
 * For example, if pos points at the 'S' in
 *	Serial Number.............24099050
 * return a pointer to the '2'.
 *
 * Return NULL if there aren't 2 consecutive periods.
 */
static char *
skip_dots(const char *pos)
{
	pos = strstr(pos, "..");
	if (!pos)
		return NULL;
	while (*pos == '.')
		pos++;
	return (char *) pos;
}

/*
 * Some versions of iprconfig/lscfg report the location code of the ESM/ERM
 * -- e.g., UEDR1.001.G12W34S-P1-C1.  For our purposes, we usually don't want
 * the -P1-C1.  (Don't trim location codes for disks and such.)
 *
 * TODO: This adjustment is appropriate for Bluehawks.  Need to understand
 * what, if anything, needs to be done for (e.g.) Pearl enclosures.
 */
void
trim_location_code(struct dev_vpd *vpd)
{
	char *hyphen;

	strncpy(vpd->location, vpd->full_loc, LOCATION_LENGTH - 1);
	vpd->location[LOCATION_LENGTH - 1] = '\0';
	hyphen = strchr(vpd->location, '-');
	if (hyphen && (!strcmp(hyphen, "-P1-C1") || !strcmp(hyphen, "-P1-C2")))
		*hyphen = '\0';
}

/**
 * strzcpy
 * @brief Copy src to dest and append NULL char
 */
char *
strzcpy(char *dest, const char *src, size_t n)
{
	memcpy(dest, src, n);
	dest[n] = '\0';
	return dest;
}

/*
 * Get enclosure type, etc. for sgN device. Caller has nulled out vpd
 * fields.
 */
int
read_vpd_from_lscfg(struct dev_vpd *vpd, const char *sg)
{
	int status = 0;
	char buf[128], *pos;
	FILE *fp;
	pid_t cpid;
	char *args[] = {LSCFG_PATH, "-vl", NULL, NULL};

	/* use lscfg to find the MTM and location for the specified device */

	/* Command exists and has exec permissions ? */
	if (access(LSCFG_PATH, F_OK|X_OK) == -1) {
		fprintf(stderr, "Unable to retrieve the MTM or location code.\n"
				"Check that %s is installed and has execute "
				"permissions.", LSCFG_PATH);
			return -1;
	}

	args[2] = (char *const) sg;
	fp = spopen(args, &cpid);
	if (fp == NULL) {
		fprintf(stderr, "Unable to retrieve the MTM or location code.\n"
				"Failed to execute \"%s -vl %s\" command.\n",
				LSCFG_PATH, sg);
		return -1;
	}

	while (fgets_nonl(buf, 128, fp) != NULL) {
		if ((pos = strstr(buf, "Machine Type")) != NULL) {
			if (!(pos = skip_dots(pos)))
				continue;
			strncpy(vpd->mtm, pos, VPD_LENGTH - 1);
		}
		else if ((pos = strstr(buf, "Device Specific.(YL)")) != NULL) {
			/* Old-style label for YL */
			if (!(pos = skip_dots(pos)))
				continue;
			strncpy(vpd->full_loc, pos, LOCATION_LENGTH - 1);
		}
		else if ((pos = strstr(buf, "Location Code")) != NULL) {
			/* Newer label for YL */
			if (!(pos = skip_dots(pos)))
				continue;
			strncpy(vpd->full_loc, pos, LOCATION_LENGTH - 1);
		}
		else if ((pos = strstr(buf, "Serial Number")) != NULL) {
			if (!(pos = skip_dots(pos)))
				continue;
			strncpy(vpd->sn, pos, VPD_LENGTH - 1);
		}
		else if ((pos = strstr(buf, "..FN ")) != NULL) {
			pos += 5;
			while (*pos == ' ')
				pos++;
			strncpy(vpd->fru, pos, VPD_LENGTH - 1);
		}
	}
	trim_location_code(vpd);

	/* Add sg device name */
	strncpy(vpd->dev, sg, PATH_MAX - 1);

	status = spclose(fp, cpid);
	/* spclose failed */
	if (status == -1) {
		fprintf(stderr, "%s : %d -- Failed in spclose(), "
				"error : %s\n", __func__, __LINE__,
				strerror(errno));
		return -1;
	}
	/* spclose() succeeded, but command failed */
	if (status != 0) {
		fprintf(stdout, "%s : %d -- spclose() exited with status : "
				"%d\n", __func__, __LINE__, status);
		return -1;
	}

	return 0;
}

/*
 * Validate sg device.
 *
 * Note:
 *  /sys/class/enclosure/<ID>/device/scsi_generic/ dir
 *  will have the 'scsi' generic name of the device.
 *
 * Returns: 0 on valid enclosure device, -1 on invalid enclosure device.
 */
int
valid_enclosure_device(const char *sg)
{
	struct dirent *edirent, *sdirent;
	DIR *edir, *sdir;
	char path[PATH_MAX];

	edir = opendir(SCSI_SES_PATH);
	if (!edir) {
		fprintf(stderr, "System does not have SCSI enclosure(s).\n");
		return -1;
	}

	/* loop over all enclosures */
	while ((edirent = readdir(edir)) != NULL) {
		if (!strcmp(edirent->d_name, ".") ||
		    !strcmp(edirent->d_name, ".."))
			continue;

		snprintf(path, PATH_MAX, "%s/%s/device/scsi_generic",
			 SCSI_SES_PATH, edirent->d_name);

		sdir = opendir(path);
		if (!sdir)
			continue;

		while ((sdirent = readdir(sdir)) != NULL) {
			if (!strcmp(sdirent->d_name, ".") ||
			    !strcmp(sdirent->d_name, ".."))
				continue;

			/* found sg device */
			if (!strcmp(sdirent->d_name, sg))
				goto out;
		}
		closedir(sdir);
	}

	closedir(edir);
	fprintf(stderr, "%s is not a valid enclosure device\n", sg);
	return -1;

out:
	closedir(sdir);
	closedir(edir);
	return 0;
}

void element_check_range(unsigned int n, unsigned int min,
			 unsigned int max, const char *lc)
{
	if (n < min || n > max) {
		fprintf(stderr,
			"Number %u out of range in location code %s\n", n, lc);
		exit(1);
	}
}
