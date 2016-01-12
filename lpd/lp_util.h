/**
 * Copyright (C) 2012 IBM Corporation
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
