/*
 * Copyright (C) 2009, 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef _DIAG_ENCL_H
#define _DIAG_ENCL_H

#include <servicelog-1/servicelog.h>

struct cmd_opts {
	int cmp_prev;	/* -c */
	int leds;	/* -l */
	int serv_event;	/* -s */
	int verbose;	/* -v */
	char *fake_path;	/* -f */
	char *prev_path;	/* for -c */
};
extern struct cmd_opts cmd_opts;

extern void add_callout(struct sl_callout **, char, uint32_t, char *, char *,
			char *, char *, char *);
extern uint32_t servevent(char *, int, char *, struct dev_vpd *, struct sl_callout *);

extern int diag_7031_D24_T24(int, struct dev_vpd *);
extern int diag_bluehawk(int, struct dev_vpd *);

#endif	/* _DIAG_ENCL_H */
