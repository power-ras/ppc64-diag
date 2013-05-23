/**
 * @file	sesdevices.c
 * @brief	List all Light Path Daignostics supported SES enclosures
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author      Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include "lp_diag.h"
#include "lp_util.h"

/* SES sys path */
#define SCSI_SES_PATH	"/sys/class/enclosure"

/* List of all Light Path supported enclosures */
static struct {
        char *mtm;
} supported_enclosure[] = {
        {"5888"},	/* Bluehawk */
        {"EDR1"},	/* Bluehawk */
        {NULL}
};

/* trim ESM/ERM part */
static void
trim_location_code(struct dev_vpd *vpd)
{
	char *hyphen;

	hyphen = strchr(vpd->location, '-');
	if (hyphen && (!strcmp(hyphen, "-P1-C1") || !strcmp(hyphen, "-P1-C2")))
		*hyphen = '\0';
}


int main()
{
	int	i;
	int	count = 0;
	char	path[128];
	DIR	*dir;
	struct	dirent *dirent;
	struct	dev_vpd *vpd = NULL;
	struct	dev_vpd *v1;

	vpd = read_device_vpd(SCSI_SES_PATH);
	if (!vpd)
		return 0;

	for (v1 = vpd; v1; v1 = v1->next) {

		/* remove ESM/ERM part of location code */
		trim_location_code(v1);

		/* read sg name */
		snprintf(path, 128, "%s/%s/device/scsi_generic",
			 SCSI_SES_PATH, v1->dev);

		dir = opendir(path);
		if (!dir) {
			fprintf(stderr,
				"Unable to open directory : %s\n", path);
			continue;
		}

		/* fill sg device name */
		while ((dirent = readdir(dir)) != NULL) {
			if (!strcmp(dirent->d_name, ".") ||
			    !strcmp(dirent->d_name, ".."))
				continue;
			strncpy(v1->dev, dirent->d_name, DEV_LENGTH);
		}
		closedir(dir);
	}

	for (v1 = vpd; v1; v1 = v1->next)
		for (i = 0; supported_enclosure[i].mtm; i++)
			if (!strcmp(v1->mtm, supported_enclosure[i].mtm)) {
				count++;
				fprintf(stdout, "\n");
				fprintf(stdout, "SCSI Device   : %s\n", v1->dev);
				fprintf(stdout, "Model         : %s\n", v1->mtm);
				fprintf(stdout, "Location Code : %s\n", v1->location);
				fprintf(stdout, "Serial Number : %s\n", v1->sn);
			}

	if (count == 0)
		fprintf(stderr, "System does not have Light Path supported "
				"SES enclosure(s).\n");

	free_device_vpd(vpd);

	return 0;
}
