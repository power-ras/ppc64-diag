/**
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef INDICATOR_RTAS_H
#define INDICATOR_RTAS_H

extern int get_rtas_indicator_mode(void);
extern int get_rtas_indicator_list(int , struct loc_code **);
extern int get_rtas_indicator_state(int , struct loc_code *, int *);
extern int set_rtas_indicator_state(int , struct loc_code *, int);

#endif	/* INDICATOR_RTAS_H */
