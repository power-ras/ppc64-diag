/*
 * Copyright (C) 2012 IBM Corporation
 */
#ifndef _ENCL_UTIL_H
#define _ENCL_UTIL_H

#include <stdint.h>

/* SES sys path */
#define SCSI_SES_PATH		"/sys/class/enclosure"
#define LSVPD_PATH		"/usr/sbin/lsvpd"
#define LSCFG_PATH		"/usr/sbin/lscfg"

#define	VPD_LENGTH		128
#define	LOCATION_LENGTH		80

/* device vpd */
struct dev_vpd {
	char mtm[VPD_LENGTH];
	char location[LOCATION_LENGTH];	/* like full_loc, but truncated at '-' */
	char full_loc[LOCATION_LENGTH];
	char sn[VPD_LENGTH];
	char fru[VPD_LENGTH];
	struct dev_vpd *next;
};

extern int print_raw_data(FILE *ostream, char *data, int data_len);

extern int do_ses_cmd(int fd, uint8_t cmd, uint8_t page_nr, uint8_t flags,
		      uint8_t cmd_len, int dxfer_direction, void *buf,
		      int buf_len);
extern int get_diagnostic_page(int fd, uint8_t cmd, uint8_t page_nr, void *buf,
			       int buf_len);

extern char *fgets_nonl(char *buf, int size, FILE *s);
extern int read_vpd_from_lscfg(struct dev_vpd *vpd, const char *sg);
extern void trim_location_code(struct dev_vpd *vpd);
extern int valid_enclosure_device(const char *sg);

#endif /* _ENCL_UTIL_H */
