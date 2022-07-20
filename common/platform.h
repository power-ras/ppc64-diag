/**
 * Copyright (C) 2014 IBM Corporation
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

#ifndef PLATFORM_H
#define PLARFORM_H

#include <stdint.h>

#define PLATFORM_FILE	"/proc/cpuinfo"

/* Values extracted from linux kernel arch/powerpc/include/asm/reg.h file */
#define PVR_POWER4	0x0035
#define PVR_POWER4p	0x0038
#define PVR_POWER5	0x003A
#define PVR_POWER5p	0x003B
#define PVR_POWER6	0x003E
#define PVR_POWER7	0x003F
#define PVR_POWER7p	0x004A
#define PVR_POWER8E	0x004B
#define PVR_POWER8NVL	0x004C
#define PVR_POWER8	0x004D
#define PVR_POWER9	0x004E
#define PVR_POWER10	0x0080

enum {
	PLATFORM_UNKNOWN = 0,
	PLATFORM_POWERNV,
	PLATFORM_POWERKVM_GUEST,
	PLATFORM_PSERIES_LPAR,
	/* Add new platforms here */
	PLATFORM_MAX,
};

/* Keep this in ascending order of generation releases */
enum {
	PROCESSOR_UNKNOWN = 0,
	POWER4,
	POWER5,
	POWER6,
	POWER7,
	POWER8,
	POWER9,
	POWER10,
};

extern const char *__platform_name[];

extern int get_platform(void);

extern int get_processor(void);

extern uint16_t get_pvr_ver(void);

static inline const char * __power_platform_name(int platform)
{
	if (platform > PLATFORM_UNKNOWN && platform < PLATFORM_MAX)
		return __platform_name[platform];

	return __platform_name[PLATFORM_UNKNOWN];
}
#endif
