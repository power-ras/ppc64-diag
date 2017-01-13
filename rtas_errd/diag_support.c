/**
 * @file diag_support.c
 *
 * Copyright (C) 2005, 2008 IBM Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <librtas.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "utils.h"
#include "rtas_errd.h"

#define CMD_LSVPD "/usr/sbin/lsvpd"

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
lsvpd_init(FILE **fp, pid_t *cpid)
{
	char *system_args[2] = {CMD_LSVPD, NULL,}; /* execv arguments      */

	dbg("start lsvpd_init");

	*fp = spopen(system_args, cpid);
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
lsvpd_term(FILE *fp, pid_t *cpid)
{
	int rc = 0;
	char line[512];

        /* If pipe was opened read all vpd   */
        /* entries until pipe is empty.      */
        if (fp) {
		while (fgets(line, sizeof(line), fp));
		rc = spclose(fp, *cpid);
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
			event->diag_vpd.ds = malloc(strlen (line) + 1);
			if (event->diag_vpd.ds == NULL)
				return rc;

			strcpy(event->diag_vpd.ds, &line[4]);
			dbg("found DS: \"%s\"", event->diag_vpd.ds);
		}

		if (! strncmp(line, "*YL", 3)) {
			event->diag_vpd.yl = malloc(strlen (line) + 1);
			if (event->diag_vpd.yl == NULL)
				return rc;

			strcpy(event->diag_vpd.yl, &line[4]);
			dbg("found YL: \"%s\"", event->diag_vpd.yl);
		}

		if (! strncmp(line, "*FN", 3)) {
			event->diag_vpd.fn = malloc(strlen (line) + 1);
			if (event->diag_vpd.fn == NULL)
				return rc;

			strcpy(event->diag_vpd.fn, &line[4]);
			dbg("found FN: \"%s\"", event->diag_vpd.fn);
		}

		if (! strncmp(line, "*SN", 3)) {
			event->diag_vpd.sn = malloc(strlen (line) + 1);
			if (event->diag_vpd.sn == NULL)
				return rc;

			strcpy(event->diag_vpd.sn, &line[4]);
			dbg("found SN: \"%s\"", event->diag_vpd.sn);
		}

		if (! strncmp(line, "*SE", 3)) {
			event->diag_vpd.se = malloc(strlen (line) + 1);
			if (event->diag_vpd.se == NULL)
				return rc;

			strcpy(event->diag_vpd.se, &line[4]);
			dbg("found SE: \"%s\"", event->diag_vpd.se);
		}

		if (! strncmp(line, "*TM", 3)) {
			event->diag_vpd.tm = malloc(strlen (line) + 1);
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
	pid_t cpid;                       /* child pid */

	dbg("start get_diag_vpd");

	if (event->diag_vpd.yl != NULL)
		free_diag_vpd(event);

	/* sigchld_handler() messes up pclose(). */
	restore_sigchld_default();

	if (lsvpd_init(&fp, &cpid) != 0) {
		setup_sigchld_handler();
		return 1;
	}

	while (event->diag_vpd.yl == NULL ||
	       strcmp(event->diag_vpd.yl, phyloc)) {
		if (lsvpd_read(event, fp)) {
			dbg("end get_diag_vpd, failure");
			rc = lsvpd_term(fp, &cpid);
			setup_sigchld_handler();
			return 1;
		}
	}
	rc = lsvpd_term(fp, &cpid);
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
	FILE *fp1 = NULL, *fp2 = NULL;
	char loc_file[80];
	char target[80];
	char *ptr;
	char *system_args[6] = {NULL, };		/* execv args		*/
	char tmp_file[] = "/tmp/get_dt_files-XXXXXX";
	int fd;
	pid_t cpid;					/* child pid		*/
	int rc;						/* return value		*/
	int status;					/* child exit status	*/

	system_args[0] = "/usr/bin/find";
	system_args[1] = "/proc/device-tree";
	system_args[2] = "-name";
	system_args[3] = "status";
	system_args[4] = "-print";

	fd = mkstemp(tmp_file);
	if (fd == -1) {
		log_msg(NULL, "tmp file creation failed, at "
			"get_dt_status\n");
		exit (-2);
	} /* open */

	cpid = fork();
	if (cpid == -1) {
		close(fd);
		log_msg(NULL, "Fork failed, at get_dt_status\n");
		return NULL;
	} /* fork */

	if (cpid == 0) { /* child */
		int fd = open(tmp_file, O_CREAT | O_WRONLY | O_TRUNC,
					S_IRUSR | S_IWUSR);
		if (fd == -1) {
			log_msg(NULL, "tmp file creation failed, at "
				"get_dt_status\n");
			exit (-2);
		} /* open */

		rc = dup2(fd, STDOUT_FILENO);
		if (rc == -1) {
			log_msg(NULL, "STDOUT redirection failed, at "
				"get_dt_status\n");
			close(fd);
			exit (-2);
		}

		rc = execv(system_args[0], system_args);
		log_msg(NULL, "get_dt_status find command failed\n");
		close(fd);
		exit (-2);
	} else { /* parent */
		close(fd);
		rc = waitpid(cpid, &status, 0);
		if (rc == -1) {
			log_msg(NULL, "wait on child failed at, "
				"get_dt_status\n");
			return NULL;
		} /* waitpid */

		/* Return on EXIT_FAILURE */
		if ((signed char)WEXITSTATUS(status) == -2)
			return NULL;
	}

	/* results of the find command */
	fp1 = fopen(tmp_file, "r");
	if (fp1 == 0) {
		fprintf(stderr, "open failed on %s\n", tmp_file);
		return NULL;
	}

	while (fscanf (fp1, "%s", loc_file) != EOF) {
		dbg("read from /%s, \"%s\"", tmp_file, loc_file);

		/* read the status in case this is the one */
		fp2 = fopen(loc_file, "r");
		if (fp2 == 0) {
			fprintf(stderr, "open failed on %s\n", loc_file);
			goto out;
		}

		if (fscanf(fp2, "%s", target_status) != EOF) {
			dbg("target_status = \"%s\", loc_file = \"%s\"",
			    target_status, loc_file);
		} else {
			fprintf(stderr, "read failed on %s\n", loc_file);
			goto out;
		}

		fclose(fp2);
		fp2 = NULL;

		/* read the loc-code file to determine if found dev */
		ptr = strstr(loc_file, "status");
		if (!ptr)
			continue;

		strcpy(ptr, "ibm,loc-code");
		fp2 = fopen(loc_file, "r");
		if (fp2 == 0) {
			fprintf(stderr, "open failed on %s\n", loc_file);
			goto out;
		}

		if (fscanf(fp2, "%s", target) != EOF) {
			dbg("target = \"%s\", loc_file = \"%s\"",
			    target, loc_file);
			if (strcmp(dev, target) == 0) {
				dbg("status = \"%s\"", target_status);
					fclose (fp2);
					fclose (fp1);
					return target_status;
			}
		} else {
			fprintf(stderr, "read failed on %s\n", loc_file);
			goto out;
		}

		fclose(fp2);
		fp2 = NULL;
	}

	fprintf(stderr, "error: status NOT FOUND\n");

out:
	if (fp2)
		fclose(fp2);

	if (fp1)
		fclose(fp1);

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
