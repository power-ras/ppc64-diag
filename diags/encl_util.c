/*
 * Copyright (C) 2012 IBM Corporation
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>

#include "encl_util.h"

struct sense_data_t {
	uint8_t error_code;
	uint8_t segment_numb;
	uint8_t sense_key;
	uint8_t info[4];
	uint8_t add_sense_len;
	uint8_t cmd_spec_info[4];
	uint8_t add_sense_code;
	uint8_t add_sense_code_qual;
	uint8_t field_rep_unit_code;
	uint8_t sense_key_spec[3];
	uint8_t add_sense_bytes[0];
};

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
	
	strcpy(vpd->location, vpd->full_loc);
	hyphen = strchr(vpd->location, '-');
	if (hyphen && (!strcmp(hyphen, "-P1-C1") || !strcmp(hyphen, "-P1-C2")))
		*hyphen = '\0';
}

/*
 * Get enclosure type, etc. for sgN device. Caller has nulled out vpd
 * fields.
 */
int
read_vpd_from_lscfg(struct dev_vpd *vpd, const char *sg)
{
	char buf[128], *pos;
	FILE *fp;

	/* use lscfg to find the MTM and location for the specified device */
	snprintf(buf, 128, "lscfg -vl %s", sg);
	fp = popen(buf, "r");
	if (fp == NULL) {
		fprintf(stderr, "Unable to retrieve the MTM or location code. "
				"Ensure that lsvpd is installed.\n\n");
		return -1;
	}
	while (fgets_nonl(buf, 128, fp) != NULL) {
		if ((pos = strstr(buf, "Machine Type")) != NULL) {
			if (!(pos = skip_dots(pos)))
				continue;
			strncpy(vpd->mtm, pos, 128);
		}
		else if ((pos = strstr(buf, "Device Specific.(YL)")) != NULL) {
			/* Old-style label for YL */
			if (!(pos = skip_dots(pos)))
				continue;
			strncpy(vpd->full_loc, pos, 128);
		}
		else if ((pos = strstr(buf, "Location Code")) != NULL) {
			/* Newer label for YL */
			if (!(pos = skip_dots(pos)))
				continue;
			strncpy(vpd->full_loc, pos, 128);
		}
		else if ((pos = strstr(buf, "Serial Number")) != NULL) {
			if (!(pos = skip_dots(pos)))
				continue;
			strncpy(vpd->sn, pos, 128);
		}
		else if ((pos = strstr(buf, "..FN ")) != NULL) {
			pos += 5;
			while (*pos == ' ')
				pos++;
			strncpy(vpd->fru, pos, 128);
		}
	}
	trim_location_code(vpd);
	pclose(fp);

	return 0;
}
