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

#ifndef _ENCL_UTIL_H
#define _ENCL_UTIL_H

#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>

/* SES sys path */
#define SCSI_SES_PATH		"/sys/class/enclosure"
#define LSVPD_PATH		"/usr/sbin/lsvpd"
#define LSCFG_PATH		"/usr/sbin/lscfg"

#define	VPD_LENGTH		128
#define	LOCATION_LENGTH		80

/* device vpd */
struct dev_vpd {
	char dev[PATH_MAX];
	char mtm[VPD_LENGTH];
	char location[LOCATION_LENGTH];	/* like full_loc, but truncated at '-' */
	char full_loc[LOCATION_LENGTH];
	char sn[VPD_LENGTH];
	char fru[VPD_LENGTH];
	struct dev_vpd *next;
};

extern int print_raw_data(FILE *ostream, char *data, int data_len);

extern int open_sg_device(const char *encl);
extern int read_page2_from_file(const char *path, bool display_error_msg,
				void *pg, int size);
extern int write_page2_to_file(const char *path, void *pg, int size);

extern int enclosure_maint_mode(const char *sg);

extern int do_ses_cmd(int fd, uint8_t cmd, uint8_t page_nr, uint8_t flags,
		      uint8_t cmd_len, int dxfer_direction, void *buf,
		      int buf_len);
extern int get_diagnostic_page(int fd, uint8_t cmd, uint8_t page_nr, void *buf,
			       int buf_len);

extern char *fgets_nonl(char *buf, int size, FILE *s);
extern int read_vpd_from_lscfg(struct dev_vpd *vpd, const char *sg);
extern void trim_location_code(struct dev_vpd *vpd);
extern char *strzcpy(char *dest, const char *src, size_t n);
extern int valid_enclosure_device(const char *sg);

#endif /* _ENCL_UTIL_H */
