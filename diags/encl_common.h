/*
 * Copyright (C) 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef _ENCL_COMMON_H
#define _ENCL_COMMON_H

#include <servicelog-1/servicelog.h>

#include "encl_util.h"

extern void add_callout(struct sl_callout **, char, uint32_t,
			char *, char *, char *, char *, char *);
extern uint32_t servevent(const char *, int, const char *,
			  struct dev_vpd *, struct sl_callout *);

#endif	/* _ENCL_COMMON_H */
