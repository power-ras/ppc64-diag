/**
 * @file        platform.c
 *
 * Copyright (C) 2014 IBM Corporation
 * See 'COPYRIGHT' for License of this code
 *
 * @author      Aruna Balakrishnaiah <aruna@linux.vnet.ibm.com>
 */

#include <stdio.h>
#include <string.h>

#include "platform.h"

#define LENGTH		512

const char *__platform_name[] = {
        "Unknown",
        "PowerKVM Host",
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
			rc = PLATFORM_POWERKVM;
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
