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

#ifndef _ENCL_LED_H
#define _ENCL_LED_H

#define COMP_DESC_SIZE	64
#define COMP_LOC_CODE	16

#define LED_SAME	-1
#define LED_OFF		0
#define LED_ON		1

/*
 * It'd be nicer to do all this with functions, but different components
 * have their fail, ident, rqst_fail, and rqst_ident  bits in different
 * locations.
 */
#define SET_LED(cp, dp, fault, idnt, ctrl_element, status_element) \
do { \
	(cp)->ctrl_element.common_ctrl.select = 1; \
	(cp)->ctrl_element.rqst_fail = (fault == LED_SAME ? \
					(dp)->status_element.fail : fault); \
	(cp)->ctrl_element.rqst_ident = (idnt == LED_SAME ? \
					(dp)->status_element.ident : idnt); \
} while (0)


#define REPORT_COMPONENT(dp, element, fault, idnt, loc_code, desc, verbose) \
do { \
	printf("%-5s %-5s %-9s", \
		on_off_string[fault == LED_SAME ? dp->element.fail : fault], \
		on_off_string[idnt == LED_SAME ? dp->element.ident : idnt], \
		loc_code); \
	if (verbose) \
		printf("  %s", desc); \
	printf("\n"); \
} while (0)

extern const char *progname;

extern int bluehawk_list_leds(const char *, const char *, int);
extern int bluehawk_set_led(const char *, const char *, int, int, int);

extern int homerun_list_leds(const char *, const char *, int);
extern int homerun_set_led(const char *, const char *, int, int, int);

extern int slider_lff_list_leds(const char *, const char *, int);
extern int slider_lff_set_led(const char *, const char *, int, int, int);

extern int slider_sff_list_leds(const char *, const char *, int);
extern int slider_sff_set_led(const char *, const char *, int, int, int);

#endif	/* _ENCL_LED_H */
