/**
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef INDICATOR_RTAS_H
#define INDICATOR_RTAS_H

#include "lp_diag.h"

extern int get_rtas_indices(int, struct loc_code **);
extern int get_rtas_sensor(int, struct loc_code *, int *);
extern int set_rtas_indicator(int, struct loc_code *, int);

#endif	/* INDICATOR_RTAS_H */
