/*
 * Copyright (C) 2012, 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
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

#endif	/* _ENCL_LED_H */
