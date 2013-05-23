/**
 * @file	indicator_ses.c
 * @brief	SES indicator manipulation routines
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include "lp_diag.h"
#include "lp_util.h"

/* SES LED command name */
#define SCSI_INDICATOR_CMD	"/usr/sbin/encl_led"

/* SES sys path */
#define SCSI_SES_PATH		"/sys/class/enclosure"


/*
 * Some versions of iprconfig/lscfg report the location code of the ESM/ERM
 * -- e.g., UEDR1.001.G12W34S-P1-C1.  For our purposes, we usually don't want
 * the -P1-C1.  (Don't trim location codes for disks and such.)
 *
 * TODO: This adjustment is appropriate for Bluehawks.  Need to understand
 * what, if anything, needs to be done for other (e.g. Pearl) enclosures.
 */
static void
trim_location_code(struct dev_vpd *vpd)
{
	char *hyphen;

	hyphen = strchr(vpd->location, '-');
	if (hyphen && (!strcmp(hyphen, "-P1-C1") || !strcmp(hyphen, "-P1-C2")))
		*hyphen = '\0';
}

/**
 * read_ses_vpd - Read SES device vpd data
 *
 * Returns :
 *	vpd structure on success, NULL on failure
 */
static struct dev_vpd *
read_ses_vpd(void)
{
	char	path[128];
	DIR	*dir;
	struct	dirent *dirent;
	struct	dev_vpd *vpd = NULL;
	struct	dev_vpd *v1;

	/* get enclosure vpd data */
	vpd = read_device_vpd(SCSI_SES_PATH);
	if (!vpd)
		return NULL;

	for (v1 = vpd; v1; v1 = v1->next) {

		/* remove ESM/ERM part of location code */
		trim_location_code(v1);

		/* read sg name */
		snprintf(path, 128, "%s/%s/device/scsi_generic",
						SCSI_SES_PATH, v1->dev);
		dir = opendir(path);
		if (!dir) {
			log_msg("Unable to open directory : %s", path);
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
	return vpd;
}

/**
 * Helper functions to parse encl_led command output SES LED's.
 *
 * Ex :
 * Listing the particular enclosure indicator list:
 * encl_led -v -l sgN [component] :
 *	fault	ident	location   description
 *       off	off	P1-E1	   left power supply
 *       off	off	P1-E2	   right power supply
 *
 * Modify fault/identify indicator.
 * encl_led -{i|f} {on|off} sgN [component]
 */

/**
 * get_relative_fru_location - split location code
 *
 * Get FRU location code relative to the enclosure location code.
 * Ex : If an enclosure has location code U5888.001.G123789, and one
 *      of its diks has a full location code U5888.001.G123789-P1-D1,
 *      then relatvie FRU location code would be P1-D1.
 *
 */
static char *
get_relative_fru_location(const char *loccode)
{
	char *fru_loc;

	fru_loc = strchr(loccode, '-');
	if (!fru_loc)
		return NULL;
	fru_loc++;	/* skip - */
	return fru_loc;
}

/**
 * get_ses_fru_desc - Parse SES FRU description
 *
 * Returns :
 *	FRU description on success, NULL on failure
 */
static char *
get_ses_fru_desc(char *buf)
{
	if (!buf)
		return NULL;
	while (*buf == ' ')
		buf++;

	return buf;
}

/**
 * get_ses_fru_location - Parse SES FRU location
 *
 * Returns :
 *	Relative FRU location code on success, NULL on failure
 */
static char *
get_ses_fru_location(char *buf)
{
	char	*fru;
	char	*space;

	fru = strchr(buf, 'P');
	if (!fru) {
		fru = strchr(buf, '-'); /* Enclosure location code */
		if (!fru)
			return NULL;
	}
	space = strchr(fru, ' ');
	if (!space)
		return NULL;
	*space = '\0';

	return fru;
}

/**
 * get_ses_fru_identify_state - Get FRU identify indicator state
 *
 * Returns :
 *	Indicator state on success, -1 on failure
 */
static int
get_ses_fru_identify_state(const char *buf)
{
	char	*fault;
	char	*ident;

	fault = strchr(buf, 'o');
	if (!fault)
		return -1;
	fault++;
	ident = strchr(fault, 'o');
	if (!ident)
		return -1;

	return !strncmp(ident, "on", 2);
}

/**
 * get_ses_fru_fault_state - Get FRU fault indicator state
 *
 * Returns :
 *	Indicator state on success, -1 on failure
 */
static int
get_ses_fru_fault_state(const char *buf)
{
	char	*fault;

	fault = strchr(buf, 'o');
	if (!fault)
		return -1;

	return !strncmp(fault, "on", 2);
}

/**
 * ses_indicator_list - Build SES indicator list for the given enclosure
 *
 * @list	loc_code structure
 * @vpd		dev_vpd structure
 *
 * Returns :
 *	0 on success, !0 on failure
 */
static int
ses_indicator_list(struct loc_code **list, struct dev_vpd *vpd)
{
	char	cmd[128];
	char	buf[128];
	char	*desc;
	char	*fru_loc;
	FILE	*fp;
	struct	loc_code *curr = *list;

	snprintf(cmd, 128, "%s -v -l %s 2> /dev/null", SCSI_INDICATOR_CMD,
		 vpd->dev);
	fp = popen(cmd, "r");
	if (fp == NULL) {
		log_msg("Unable to get enclosure indicator list. "
			"Ensure that encl_led command is installed.");
		return -1;
	}

	if (curr)
		while (curr->next)
			curr = curr->next;

	while (fgets_nonl(buf, 128, fp) != NULL) {
		fru_loc = get_ses_fru_location(buf);
		if (!fru_loc)
			continue;

		/* Advance buffer to parse description field */
		desc = get_ses_fru_desc(fru_loc + strlen(fru_loc) + 1);

		if (!curr) {
			curr = (struct loc_code *)malloc(sizeof
							 (struct loc_code));
			*list = curr;
		} else {
			curr->next = (struct loc_code *)malloc(sizeof
							  (struct loc_code));
			curr = curr->next;
		}
		if (!curr) {
			log_msg("Out of memory");
			/* Read until pipe becomes empty */
			while (fgets_nonl(buf, 128, fp) != NULL)
				;
			pclose(fp);
			return -1;
		}
		memset(curr, 0, sizeof(struct loc_code));

		/* fill loc_code structure */
		strncpy(curr->code, vpd->location, LOCATION_LENGTH);
		if (strcmp(fru_loc, "-")) {	/* Components */
			strncat(curr->code, "-",
				LOCATION_LENGTH - strlen(curr->code) - 1);
			strncat(curr->code, fru_loc,
				LOCATION_LENGTH - strlen(curr->code) - 1);
		}
		curr->length = strlen(curr->code) + 1;
		curr->type = TYPE_SES;

		/* We need to keep track of the sg device. */
		strncpy(curr->dev, vpd->dev, DEV_LENGTH);

		/* lsvpd does not provide vpd data for components like power
		 * supply inside enclosure. Lets keep the display name.
		 */
		if (desc)
			snprintf(curr->ds, VPD_LENGTH, "Enclosure %s : %s",
				 vpd->dev, desc);
	}
	pclose(fp);
	return 0;
}

/**
 * get_ses_indices - Get SES indicator list
 *
 * @loc		loc_code structure
 *
 * Returns :
 *	none
 */
void
get_ses_indices(int indicator, struct loc_code **list)
{
	int	vpd_matched;
	struct	dev_vpd *vpd;
	struct	dev_vpd *v1;
	struct	dev_vpd *v2;

	vpd = read_ses_vpd();
	if (!vpd)
		return;

	/* Some of the enclosures can be represented by multiple sg devices.
	 * Make sure we don't duplicate the location code for such devices.
	 * Ex: A Bluehawk can be represented by 4 different sg devices.
	 */
	for (v1 = vpd; v1; v1 = v1->next) {
		if (!enclosure_supported(v1->mtm))
			continue;
		vpd_matched = 0;
		for (v2 = v1->next; v2; v2 = v2->next) {
			if (!enclosure_supported(v2->mtm))
				continue;
			if (!strcmp(v1->location, v2->location)) {
				vpd_matched = 1;
				break;
			}
		}
		if (!vpd_matched && v1->location[0] != '\0')
			if (ses_indicator_list(list, v1)) {
				free_device_vpd(vpd);
				return;
			}
	}
	free_device_vpd(vpd);
	return;
}

/**
 * get_ses_indicator - get SES indicator state
 *
 * @loc		loc_code structure
 * @state	return ses indicator state of given loc
 *
 * Returns
 *	0 on success / -1 on failure
 */
int
get_ses_indicator(int indicator, struct loc_code *loc, int *state)
{
	int	fru_found = 0;
	char	cmd[128];
	char	buf[128];
	char	*fru_loc;
	char	*fru;
	FILE	*fp;

	fru_loc = get_relative_fru_location(loc->code);
	if (!fru_loc) /* Enclosure location code */
		fru_loc = "-";

	snprintf(cmd, 128, "%s -l %s %s 2> /dev/null", SCSI_INDICATOR_CMD,
		 loc->dev, fru_loc);
	fp = popen(cmd, "r");
	if (fp == NULL) {
		log_msg("Unable to get enclosure LED status. "
			"Ensure that encl_led command is installed.");
		return -1;
	}

	while (fgets_nonl(buf, 128, fp) != NULL) {
		if (fru_found)	/* read until pipe becomes empty*/
			continue;
		fru = get_ses_fru_location(buf);
		if (fru && !strcmp(fru_loc, fru)) {
			fru_found = 1;
			if (indicator == IDENT_INDICATOR)
				*state = get_ses_fru_identify_state(buf);
			else
				*state = get_ses_fru_fault_state(buf);
		}
	}
	pclose(fp);

	if (!fru_found)
		return -1;
	return 0;
}

/**
 * set_ses_indicator - Set SES indicators
 *
 * @loc		loc_code structure
 * @state	value to update indicator to
 *
 * Returns :
 *	ioctl call return code
 */
int
set_ses_indicator(int indicator, struct loc_code *loc, int new_value)
{
	char	cmd[128];
	char	*fru_loc;

	fru_loc = get_relative_fru_location(loc->code);
	if (!fru_loc) /* Enclosure location code */
		fru_loc = "-";

	if (loc->dev[0] == '\0')
		return -1;

	snprintf(cmd, 128, "%s %s %s %s %s 2>&1 > /dev/null",
		 SCSI_INDICATOR_CMD,
		 indicator == IDENT_INDICATOR ? "-i" : "-f",
		 new_value == INDICATOR_ON ? "on" : "off",
		 loc->dev,
		 fru_loc);
	return system(cmd);
}
