/**
 * @file diag_support.c
 *
 * Copyright (C) 2005, 2008 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <librtas.h>

#include "rtas_errd.h"

#define CMD_LSVPD "lsvpd"

char target_status[80];

void 
free_diag_vpd(struct event *event)
{
	if (event->diag_vpd.ds != NULL) {
		free(event->diag_vpd.ds);
		event->diag_vpd.ds = NULL; 
	}

	if (event->diag_vpd.yl != NULL) {
		free(event->diag_vpd.yl);
		event->diag_vpd.yl = NULL; 
	}

	if (event->diag_vpd.fn != NULL) {
		free(event->diag_vpd.fn);
		event->diag_vpd.fn = NULL; 
	}

	if (event->diag_vpd.sn != NULL) {
		free(event->diag_vpd.sn);
		event->diag_vpd.sn = NULL; 
	}

	if (event->diag_vpd.se != NULL) {
		free(event->diag_vpd.se);
		event->diag_vpd.se = NULL; 
	}

	if (event->diag_vpd.tm != NULL) {
		free(event->diag_vpd.tm);
		event->diag_vpd.tm = NULL; 
	}
}

/*
 * Execute the 'lsvpd' command and open a pipe to read the data
 */
static int 
lsvpd_init(FILE **fp)
{
	char	cmd[128];

	dbg("start lsvpd_init");

	sprintf(cmd, "%s 2>/dev/null", CMD_LSVPD);
	*fp = popen(cmd, "r");
	if (!*fp) {
		perror("popen");
		dbg("lsvpd_init failed popen (%s)", CMD_LSVPD);
                return -1;
        }

	dbg("end lsvpd_init");

        return 0;
}

/*
 * Close the open a pipe on 'lsvpd'.
 */
static int 
lsvpd_term(FILE *fp)
{
	int rc = 0;
	char line[512];

        /* If pipe was opened read all vpd   */
        /* entries until pipe is empty.      */
        if (fp) {
		while (fgets(line, sizeof(line), fp));
		rc = pclose(fp);
        }

        return rc;
}



/*
 * Read the next lsvpd keyword, value pair
 */
static int 
lsvpd_read(struct event *event, FILE *fp)
{
	int rc = 1;
	char line[512];

	dbg("start lsvpd_read");

	if (!fp)
		return rc;

	/* clear all residual data */
	free_diag_vpd(event);

	while (fgets(line, sizeof(line), fp)) {
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

		if (! strncmp(line, "*DS", 3)) {
			event->diag_vpd.ds = (char *)malloc(strlen (line) + 1);
			if (event->diag_vpd.ds == NULL)
				return rc;

			strcpy(event->diag_vpd.ds, &line[4]);		
			dbg("found DS: \"%s\"", event->diag_vpd.ds);
		}

		if (! strncmp(line, "*YL", 3)) { 
			event->diag_vpd.yl = (char *)malloc(strlen (line) + 1);
			if (event->diag_vpd.yl == NULL)
				return rc;

			strcpy(event->diag_vpd.yl, &line[4]);		
			dbg("found YL: \"%s\"", event->diag_vpd.yl); 
		}

		if (! strncmp(line, "*FN", 3)) { 
			event->diag_vpd.fn = (char *)malloc(strlen (line) + 1); 
			if (event->diag_vpd.fn == NULL)
				return rc;

			strcpy(event->diag_vpd.fn, &line[4]);
			dbg("found FN: \"%s\"", event->diag_vpd.fn); 
		}

		if (! strncmp(line, "*SN", 3)) { 
			event->diag_vpd.sn = (char *)malloc(strlen (line) + 1); 
			if (event->diag_vpd.sn == NULL)
				return rc;

			strcpy(event->diag_vpd.sn, &line[4]); 
			dbg("found SN: \"%s\"", event->diag_vpd.sn); 
		}

		if (! strncmp(line, "*SE", 3)) { 
			event->diag_vpd.se = (char *)malloc(strlen (line) + 1); 
			if (event->diag_vpd.se == NULL)
				return rc;

			strcpy(event->diag_vpd.se, &line[4]);
			dbg("found SE: \"%s\"", event->diag_vpd.se); 
		}

		if (! strncmp(line, "*TM", 3)) { 
			event->diag_vpd.tm = (char *)malloc(strlen (line) + 1); 
			if (event->diag_vpd.tm == NULL)
				return rc;

			strcpy(event->diag_vpd.tm, &line[4]);
			dbg("found TM: \"%s\"", event->diag_vpd.tm); 
		}

		if (! strncmp(line, "*FC", 3)) { 
			/* start of next record */
			dbg("found FC - start next record");
			return 0;
		}
	}

	dbg("end lsvpd_read");
	return 1;
}


int 
get_diag_vpd(struct event *event, char *phyloc)
{
	int rc = 0;
	FILE *fp = NULL;

	dbg("start get_diag_vpd");

	if (event->diag_vpd.yl != NULL)
		free_diag_vpd(event);

	/* sigchld_handler() messes up pclose(). */
	restore_sigchld_default();

	if (lsvpd_init(&fp) != 0) {
		setup_sigchld_handler();
		return 1;
	}

	while (event->diag_vpd.yl == NULL ||
	       strcmp(event->diag_vpd.yl, phyloc)) {
		if (lsvpd_read(event, fp)) {
			dbg("end get_diag_vpd, failure");
			rc = lsvpd_term(fp);
			setup_sigchld_handler();
			return 1;
		}
	}
	rc = lsvpd_term(fp);
	if (rc)
		dbg("end get_diag_vpd, pclose failure");
	else
		dbg("end get_diag_vpd, success");

	setup_sigchld_handler();

	return rc;
}

char *
get_dt_status(char *dev)
{
	FILE *fp1, *fp2;
	char loc_file[80];
	char target[80];
	char *ptr;
	char command[]="/usr/bin/find /proc/device-tree -name status -print > /tmp/get_dt_files";

	if (system(command) != 0) {
		fprintf(stderr, "get_dt_status find command failed\n");
		return NULL;
	}

	/* results of the find command */
	fp1 = fopen("/tmp/get_dt_files", "r");
	if (fp1 == 0) {
		fprintf(stderr, "open failed on /tmp/get_dt_files\n");
		return NULL;
	}

	while (fscanf (fp1, "%s", loc_file) != EOF) {
		dbg("read from /tmp/get_dt_files, \"%s\"", loc_file);

		/* read the status in case this is the one */
		fp2 = fopen(loc_file, "r");
		if (fp2 == 0) {
			fprintf(stderr, "open failed on %s\n", loc_file);
			return NULL;
		}
		if (fscanf(fp2, "%s", target_status)) {
			dbg("target_status = \"%s\", loc_file = \"%s\"",
			    target_status, loc_file);
		} 
		else {
			fprintf(stderr, "read failed on %s\n", loc_file);
			return NULL;
		}

		fclose(fp2);

		/* read the loc-code file to determine if found dev */
		ptr = strstr(loc_file, "status");
		strcpy(ptr, "ibm,loc-code");
		fp2 = fopen(loc_file, "r");
		if (fp2 == 0) {
			fprintf(stderr, "open failed on %s\n", loc_file);
			return NULL;
		}

		if (fscanf(fp2, "%s", target)) {
			dbg("target = \"%s\", loc_file = \"%s\"",
			    target, loc_file);
			if (strcmp(dev, target) == 0) {
				dbg("status = \"%s\"", target_status);
				return target_status; 
			} 

			fclose (fp2);
		} 
		else {
			fprintf(stderr, "read failed on %s\n", loc_file);
			return NULL;
		}
	}

	fclose(fp1);
	fprintf(stderr, "error: status NOT FOUND\n");
	return NULL;
}

/**
 * is_integrated 
 * @brief This function determines if the device is an integrated device or not.
 *        
 * RETURNS: 
 *	 0 - Not integrated
 *	 1 - Is integrated
 *	-1 - System error obtaining data
 *
 */
int 
is_integrated(char *phy_loc)
{
	int	rc;
	int	index = -1;

	dbg("phy_loc = \"%s\"", phy_loc);
	rc = 0;
	index = strlen(phy_loc);

	/* Start at the end of the location code looking for */
	/* a slash or a dash. If the beginning of the        */
	/* location code is reached without finding either,  */
	/* then the resource is not integrated.              */
	while (index > 0) {
		if (phy_loc[index] == '/') {
			dbg("found slash, resource may be integrated");
			index--;

			/* Now find planar. Only allowable */
			/* characters between here and 'P' */
			/* are numerics and '.'	      */
			while (index >= 0) {
				if (phy_loc[index] == 'P') {
					rc = 1;
					break;
				} 
				else if (phy_loc[index] == '.')
					index--;
				else if (phy_loc[index] >= '0' &&
					 phy_loc[index] <= '9')
					index--;
				else {
					rc = 0;
					break;
				}
			}
			break;
		}

		if (phy_loc[index] == '-') {
 		    	dbg("found dash, so resource is not integrated");
			rc = 0;
			break;
		}

		index--;
	} /* while index > 0 */

	return rc;
}

/**
 * get_base_loc
 * 
 * FUNCTION: Reduce the given location code to the base physical FRU, thus
 *	     removing the extended functional location information. Currently
 *	     this works for CHRP physical location codes, but will need to 
 *	     updated for converged location codes.
 */
void 
get_base_loc(char *phyloc, char *base)
{
	/* Char by char copy until reaching a slash ('/) or the end */
	while (*phyloc != 0 && *phyloc != '/') {
		*base = *phyloc;
		base++;
		phyloc++;
	}
	/* Now the terminating null */ 
	*base = 0;
}


/**
 * diag_fru_pn_by_ploc
 *
 * Returns the FRU part number from VPD, as defined by the "FN" vpd keyword,
 * for the FRU given by the physical location code
 *
 * RETURNS: 
 *      0  if not found information. 
 *      1  if found information. 
 *      -1 if error found during search 
 */
char *
diag_get_fru_pn(struct event *event, char *phyloc)
{
	char baseloc[80];

	if (is_integrated(phyloc)) {
		/* Use the base location code */
		get_base_loc(phyloc, baseloc);
		if (get_diag_vpd(event, baseloc))
			return NULL;
		if (event->diag_vpd.fn == NULL) 
			/* not found for base loc. code */
			return NULL;
	} 
	else {
		/* not integrated */
		if (get_diag_vpd(event, phyloc))
			return NULL;
		if (event->diag_vpd.fn == NULL) 
			/* not found */
	    		return NULL;
	}

	/* found for given location code */
	return event->diag_vpd.fn;
}
