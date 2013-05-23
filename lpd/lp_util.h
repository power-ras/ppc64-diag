/**
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef LP_UTIL_H
#define LP_UTIL_H

#include "lp_diag.h"

extern char *fgets_nonl(char *, int, FILE *);
extern int read_vpd_from_lsvpd(struct dev_vpd *, const char *);
extern struct dev_vpd *read_device_vpd(const char *);
extern void free_device_vpd(struct dev_vpd *);

#endif	/* LP_UTIL_H */
