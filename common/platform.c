/**
 * @file        platform.c
 *
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
 *
 * @author      Aruna Balakrishnaiah <aruna@linux.vnet.ibm.com>
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "platform.h"

#define LENGTH		512

const char *__platform_name[] = {
        "Unknown",
        "PowerNV",
        "PowerKVM pSeries Guest",
        "PowerVM pSeries LPAR",
	/* Add new platforms name here */
};

int
get_platform(void)
{
	int rc = PLATFORM_UNKNOWN;
	FILE *fp;
	char line[LENGTH];

	if((fp = fopen(PLATFORM_FILE, "r")) == NULL)
		return rc;

	while (fgets(line, LENGTH, fp)) {
		if (strstr(line, "PowerNV")) {
			rc = PLATFORM_POWERNV;
			break;
		} else if (strstr(line, "pSeries (emulated by qemu)")) {
			rc = PLATFORM_POWERKVM_GUEST;
			break;
		} else if (strstr(line, "pSeries")) {
			rc = PLATFORM_PSERIES_LPAR;
			/* catch model for PowerNV guest */
			continue;
		}
	}

	fclose(fp);
	return rc;
}

int
get_processor(void)
{
	int rc;

	switch(get_pvr_ver()) {
	case PVR_POWER4:
	case PVR_POWER4p:
		rc = POWER4;
		break;
	case PVR_POWER5:
	case PVR_POWER5p:
		rc = POWER5;
		break;
	case PVR_POWER6:
		rc = POWER6;
		break;
	case PVR_POWER7:
	case PVR_POWER7p:
		rc = POWER7;
		break;
	case PVR_POWER8E:
	case PVR_POWER8NVL:
	case PVR_POWER8:
		rc = POWER8;
		break;
	case PVR_POWER9:
		rc = POWER9;
		break;
	case PVR_POWER10:
		rc = POWER10;
		break;
	default:
		rc = PROCESSOR_UNKNOWN;
		break;
	}

	return rc;
}

uint16_t
get_pvr_ver(void)
{
	uint16_t pvr_ver = 0xFFFF;
	FILE *fp;
	char line[LENGTH], *p;

	if((fp = fopen(PLATFORM_FILE, "r")) == NULL)
		return pvr_ver;

	while (fgets(line, LENGTH, fp)) {
		if ((p = strstr(line, "(pvr")) != NULL) {
			if (sscanf(p, "(pvr%" SCNx16, &pvr_ver) == 1)
				break;
		}
	}

	fclose(fp);
	return pvr_ver;
}
