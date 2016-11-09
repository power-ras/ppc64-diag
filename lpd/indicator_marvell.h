/**
 * Copyright (C) 2016 IBM Corporation
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

#ifndef INDICATOR_MARVELL_H
#define INDICATOR_MARVELL_H

/* Marvell HDD LEDs (indicators) support */
extern void get_mv_indices(int, struct loc_code **);
extern int get_mv_indicator(int, struct loc_code *, int *);
extern int set_mv_indicator(int, struct loc_code *, int);

#endif	/* INDICATOR_MARVELL_H */
