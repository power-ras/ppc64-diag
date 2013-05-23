/*
 * Copyright (C) 2012 IBM Corporation
 */
#ifndef _ENCL_UTIL_H
#define _ENCL_UTIL_H

#include <stdint.h>

/* device vpd */
struct dev_vpd {
	char mtm[128];
	char location[128];	/* like full_loc, but truncated at '-' */
	char full_loc[128];
	char sn[128];
	char fru[128];
	struct dev_vpd *next;
};

extern int do_ses_cmd(int fd, uint8_t cmd, uint8_t page_nr, uint8_t flags,
		      uint8_t cmd_len, int dxfer_direction, void *buf,
		      int buf_len);
extern int get_diagnostic_page(int fd, uint8_t cmd, uint8_t page_nr, void *buf,
			       int buf_len);
extern char *fgets_nonl(char *buf, int size, FILE *s);
extern int read_vpd_from_lscfg(struct dev_vpd *vpd, const char *sg);
extern void trim_location_code(struct dev_vpd *vpd);

#endif /* _ENCL_UTIL_H */
