/**
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef INDICATOR_SES_H
#define INDICATOR_SES_H

#include "lp_diag.h"

/* SES support */
extern void get_ses_indices(int, struct loc_code **);
extern int get_ses_indicator(int, struct loc_code *, int *);
extern int set_ses_indicator(int, struct loc_code *, int);

#endif	/* INDICATOR_SES_H */
