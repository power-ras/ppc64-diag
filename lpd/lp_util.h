/**
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef LP_UTIL_H
#define LP_UTIL_H

extern int device_supported(const char *, const char *);
extern int enclosure_supported(const char *);
extern char *fgets_nonl(char *, int, FILE *);
extern int read_vpd_from_lsvpd(struct dev_vpd *, const char *);
extern void free_device_vpd(struct dev_vpd *);
extern struct dev_vpd *read_device_vpd(const char *);
extern void fill_indicators_vpd(struct loc_code *);
extern int get_loc_code_for_dev(const char *, char *, int);

#endif	/* LP_UTIL_H */
