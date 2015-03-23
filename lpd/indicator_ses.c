/**
 * @file	indicator_ses.c
 * @brief	SES indicator manipulation routines
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utils.h"
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
	char	path[PATH_MAX];
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
		snprintf(path, PATH_MAX, "%s/%s/device/scsi_generic",
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
			strncpy(v1->dev, dirent->d_name, DEV_LENGTH - 1);
			v1->dev[DEV_LENGTH - 1] = '\0';
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
	char	buf[128];
	char	*desc;
	char	*fru_loc;
	char	*args[] = {SCSI_INDICATOR_CMD, "-v", "-l",
				(char *const)vpd->dev, NULL};
	int	rc;
	pid_t	cpid;
	FILE	*fp;
	struct	loc_code *curr = *list;

	fp = spopen(args, &cpid);
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
			curr = malloc(sizeof(struct loc_code));
			*list = curr;
		} else {
			curr->next = malloc(sizeof(struct loc_code));
			curr = curr->next;
		}
		if (!curr) {
			log_msg("Out of memory");
			/* Read until pipe becomes empty */
			while (fgets_nonl(buf, 128, fp) != NULL)
				;

			rc = spclose(fp, cpid);
			if (rc)
				log_msg("spclose failed [rc=%d]\n", rc);

			return -1;
		}
		memset(curr, 0, sizeof(struct loc_code));

		/* fill loc_code structure */
		strncpy(curr->code, vpd->location, LOCATION_LENGTH -1);
		if (strcmp(fru_loc, "-")) {	/* Components */
			strncat(curr->code, "-",
				LOCATION_LENGTH - strlen(curr->code) - 1);
			strncat(curr->code, fru_loc,
				LOCATION_LENGTH - strlen(curr->code) - 1);
		}
		curr->length = strlen(curr->code) + 1;
		curr->type = TYPE_SES;

		/* We need to keep track of the sg device. */
		strncpy(curr->dev, vpd->dev, DEV_LENGTH - 1);

		/* lsvpd does not provide vpd data for components like power
		 * supply inside enclosure. Lets keep the display name.
		 */
		if (desc)
			snprintf(curr->ds, VPD_LENGTH, "Enclosure %s : %s",
				 vpd->dev, desc);
	}

	rc = spclose(fp, cpid);
	if (rc) {
		log_msg("spclose failed [rc=%d]\n", rc);
		return -1;
	}
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
	char	buf[128];
	char	*fru_loc;
	char	*fru;
	char	*args[] = {SCSI_INDICATOR_CMD,
		           "-l",
			   (char *const)loc->dev,
			   /* FRU loc */ NULL,
			   NULL};
	int	rc;
	pid_t	cpid;
	FILE	*fp;

	fru_loc = get_relative_fru_location(loc->code);
	if (!fru_loc) /* Enclosure location code */
		fru_loc = "-";

	args[3] = (char *const)fru_loc;
	fp = spopen(args, &cpid);
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

	rc = spclose(fp, cpid);
	if (rc) {
		log_msg("spclose failed [rc=%d]\n", rc);
		return -1;
	}

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
	char	*fru_loc;
	pid_t	fork_pid;
	int	status;

	fru_loc = get_relative_fru_location(loc->code);
	if (!fru_loc) /* Enclosure location code */
		fru_loc = "-";

	if (loc->dev[0] == '\0')
		return -1;

	if (access(SCSI_INDICATOR_CMD, X_OK) != 0) {
		log_msg("The command \"%s\" is not executable.\n",
			SCSI_INDICATOR_CMD);
		return -1;
	}

	fork_pid = fork();
	if (fork_pid == -1) {
		log_msg("Couldn't fork() (%d:%s)\n", errno, strerror(errno));
		return -1;
	} else if (fork_pid == 0) {
		/* Child */
		char *args[6];

		args[0] = (char *)SCSI_INDICATOR_CMD;
		args[1] = indicator == IDENT_INDICATOR ? "-i" : "-f";
		args[2] = new_value == INDICATOR_ON ? "on" : "off";
		args[3] = loc->dev;
		args[4] = fru_loc;
		args[5] = NULL;

		execv(SCSI_INDICATOR_CMD, args);
		log_msg("Couldn't execv() into: %s (%d:%s)\n",
			SCSI_INDICATOR_CMD, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Wait for set SES indicator command to complete */
	if (waitpid(fork_pid, &status, 0) == -1) {
		log_msg("Wait failed, while running set SES indicator "
			"command\n");
		return -1;
	}

	status = (int8_t)WEXITSTATUS(status);
	if (status) {
		log_msg("%s command execution failed\n", SCSI_INDICATOR_CMD);
		return -1;
	}

	return 0;
}
