/**
 * Copyright (C) 2015 IBM Corporation
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

#ifndef SERVICELOG_H
#define SERVICELOG_H

#include <servicelog-1/servicelog.h>

/* servicelog.c */
extern int get_service_event(int, struct sl_event **);
extern int get_all_open_service_event(struct sl_event **);
extern int get_repair_event(int, struct sl_event **);
extern int print_service_event(struct sl_event *);

#endif /* SERVICELOG_H */
