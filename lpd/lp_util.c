/**
 * @file	lp_util.c
 * @brief	Light Path utility
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>

#include "lp_diag.h"
#include "utils.h"

#define LSVPD_PATH	"/usr/sbin/lsvpd"


/* Note:
 *	At present, we do not support all SES enclosures/devices. Consider
 *	removing device_supported() and enclosure_supported() once we
 *	have Light Path support for all enclosures/devices.
 */
/* List of all Light Path supported enclosures */
static struct {
	char *model;
} supported_ses_enclosure[] = {
	{"5888"},	/* Bluehawk */
	{"EDR1"},	/* Bluehawk */
	{NULL}
};

/* List of all Light Path supported devices */
static struct {
	char	*subsystem;
	char	*driver;
} os_supported_device[] = {
	{"net", "cxgb3"},	/* Chelsio network card*/
	{"net", "e1000e"},	/* Intel network card*/
	{NULL, NULL}
};

/**
 * device_supported - Check Light Path support for device
 *
 * @subsystem	subsystem
 * @driver	kernel driver
 *
 * Returns :
 *	1 if device is supported / 0 if device is not supported
 */
int
device_supported(const char *subsystem, const char *driver)
{
	int i;

	if (!subsystem || !driver)
		return 0;

	for (i = 0; os_supported_device[i].driver; i++)
		if (!strcmp(os_supported_device[i].subsystem, subsystem) &&
		    !strcmp(os_supported_device[i].driver, driver))
			return 1;
	return 0;
}

/**
 * enclosure_supported - Check Light Path support for enclosure
 *
 * @model	enclosure model
 *
 * Returns :
 *	1 if enclosure is supported / 0 if enclosure is not supported
 */
int
enclosure_supported(const char *model)
{
	int i;

	if (!model)
		return 0;

	for (i = 0; supported_ses_enclosure[i].model; i++)
		if (!strcmp(supported_ses_enclosure[i].model, model))
			return 1;
	return 0;
}


/**
 * fgets_nonl - Read a line and strip the newline.
 */
char *
fgets_nonl(char *buf, int size, FILE *s)
{
	char *nl;

	if (!fgets(buf, size, s))
		return NULL;
	nl = strchr(buf, '\n');
	if (nl == NULL)
		return NULL;
	*nl = '\0';
	return buf;
}

/**
 * skip_space -
 */
static char *
skip_space(const char *pos)
{
	pos = strchr(pos, ' ');
	if (!pos)
		return NULL;
	while (*pos == ' ')
		pos++;
	return (char *)pos;
}

/**
 * read_vpd_from_lsvpd - Retrieve vpd data
 *
 * Returns :
 *	0 on success, !0 on failure
 */
int
read_vpd_from_lsvpd(struct dev_vpd *vpd, const char *device)
{
	char	buf[128];
	char	*pos;
	char	*args[] = {LSVPD_PATH, "-l", (char *const)device, NULL};
	pid_t	cpid;
	int	rc;
	FILE	*fp;

	/* use lsvpd to find the device vpd */
	fp = spopen(args, &cpid);
	if (fp == NULL) {
		log_msg("Unable to retrieve the vpd for \"%s\". "
			"Ensure that lsvpd package is installed.", device);
		return -1;
	}

	while (fgets_nonl(buf, 128, fp) != NULL) {
		if ((pos = strstr(buf, "*TM")) != NULL) {
			if (!(pos = skip_space(pos)))
				continue;
			strncpy(vpd->mtm, pos, VPD_LENGTH - 1);
			vpd->mtm[VPD_LENGTH - 1] = '\0';
		} else if ((pos = strstr(buf, "*YL")) != NULL) {
			if (!(pos = skip_space(pos)))
				continue;
			strncpy(vpd->location, pos, LOCATION_LENGTH - 1);
			vpd->location[LOCATION_LENGTH - 1] = '\0';
		} else if ((pos = strstr(buf, "*DS")) != NULL) {
			if (!(pos = skip_space(pos)))
				continue;
			strncpy(vpd->ds, pos, VPD_LENGTH - 1);
			vpd->ds[VPD_LENGTH - 1] = '\0';
		} else if ((pos = strstr(buf, "*SN")) != NULL) {
			if (!(pos = skip_space(pos)))
				continue;
			strncpy(vpd->sn, pos, VPD_LENGTH - 1);
			vpd->sn[VPD_LENGTH - 1] = '\0';
		} else if ((pos = strstr(buf, "PN")) != NULL) {
			if (!(pos = skip_space(pos)))
				continue;
			strncpy(vpd->pn, pos, VPD_LENGTH - 1);
			vpd->pn[VPD_LENGTH - 1] = '\0';
		} else if ((pos = strstr(buf, "*FN")) != NULL) {
			if (!(pos = skip_space(pos)))
				continue;
			strncpy(vpd->fru, pos, VPD_LENGTH - 1);
			vpd->fru[VPD_LENGTH - 1] = '\0';
		}
	}

	rc = spclose(fp, cpid);
	if (rc) {
		log_msg("spclose() failed [rc=%d]\n", rc);
		return -1;
	}

	return 0;
}

/**
 * free_device_vpd - free vpd structure
 */
void
free_device_vpd(struct dev_vpd *vpd)
{
	struct	dev_vpd *tmp;

	while (vpd) {
		tmp = vpd;
		vpd = vpd->next;
		free(tmp);
	}
}

/**
 * read_device_vpd -
 *
 * @path	/sys path
 *
 * Returns :
 *	vpd structure on success / NULL on failure
 */
struct dev_vpd *
read_device_vpd(const char *path)
{
	DIR	*dir;
	struct	dirent *dirent;
	struct	dev_vpd *vpd = NULL;
	struct	dev_vpd *curr = NULL;

	dir = opendir(path);
	if (!dir) {
		if (errno != ENOENT)
			log_msg("Unable to open directory : %s", path);
		return NULL;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, ".."))
			continue;

		if (!curr) {
			curr = calloc(1, sizeof(struct dev_vpd));
			vpd = curr;
		} else {
			curr->next = calloc(1, sizeof(struct dev_vpd));
			curr = curr->next;
		}
		if (!curr) {
			log_msg("Out of memory");
			free_device_vpd(vpd);
			closedir(dir);
			return NULL;
		}

		/* device name */
		strncpy(curr->dev, dirent->d_name, DEV_LENGTH - 1);
		curr->dev[DEV_LENGTH - 1] = '\0';

		if (read_vpd_from_lsvpd(curr, dirent->d_name)) {
			free_device_vpd(vpd);
			closedir(dir);
			return NULL;
		}
	}
	closedir(dir);
	return vpd;
}

/**
 * fill_indicators_vpd - Fill indicators vpd data
 */
void
fill_indicators_vpd(struct loc_code *list)
{
	struct	dev_vpd vpd;
	struct	loc_code *curr;

	for (curr = list; curr; curr = curr->next) {
		/* zero out the vpd structure */
		memset(&vpd, 0, sizeof(struct dev_vpd));

		if (read_vpd_from_lsvpd(&vpd, curr->code))
			return;

		strncpy(curr->devname, vpd.dev, DEV_LENGTH);
		strncpy(curr->mtm, vpd.mtm, VPD_LENGTH);
		strncpy(curr->sn, vpd.sn, VPD_LENGTH);
		strncpy(curr->pn, vpd.pn, VPD_LENGTH);
		strncpy(curr->fru, vpd.fru, VPD_LENGTH);
		/* retain existing DS */
		if (curr->ds[0] == '\0')
			strncpy(curr->ds, vpd.ds, VPD_LENGTH);
	}
}

/**
 * get_loc_code_for_dev - Get location code for the given device
 *
 * @device	device name
 * @location	output location code
 * @locsize	location code size
 *
 * Returns :
 *	0 on success / -1 on failure
 */
int
get_loc_code_for_dev(const char *device, char *location, int locsize)
{
	struct	dev_vpd vpd;

	memset(&vpd, 0, sizeof(struct dev_vpd));
	if (device && !read_vpd_from_lsvpd(&vpd, device)) {
		if (location && vpd.location[0] != '\0') {
			strncpy(location, vpd.location, locsize);
			location[locsize - 1] = '\0';
			return 0;
		}
	}
	return -1;
}
