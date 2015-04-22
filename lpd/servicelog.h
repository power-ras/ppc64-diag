/**
 * Copyright (C) 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef SERVICELOG_H
#define SERVICELOG_H

#include <servicelog-1/servicelog.h>

/* servicelog.c */
extern int get_service_event(int, struct sl_event **);
extern int get_all_open_service_event(struct sl_event **);
extern int get_repair_event(int, struct sl_event **);
extern int print_service_event(struct sl_event *);

#endif /* SERVICELOG_H */
