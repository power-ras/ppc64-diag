/*
 * Copyright (C) 2009, 2015 IBM Corporation
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

#ifndef _DIAG_ENCL_H
#define _DIAG_ENCL_H

#include <stdint.h>

#include "encl_util.h"

struct cmd_opts {
	int cmp_prev;	/* -c */
	int leds;	/* -l */
	int serv_event;	/* -s */
	int verbose;	/* -v */
	char *fake_path;	/* -f */
	char *prev_path;	/* for -c */
};

extern struct cmd_opts cmd_opts;
extern int platform;

extern int diag_7031_D24_T24(int, struct dev_vpd *);
extern int diag_bluehawk(int, struct dev_vpd *);
extern int diag_homerun(int, struct dev_vpd *);

#endif	/* _DIAG_ENCL_H */
