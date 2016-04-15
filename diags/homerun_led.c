/*
 * Copyright (C) 2015, 2016 IBM Corporation
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include "encl_util.h"
#include "encl_led.h"
#include "homerun.h"

enum hr_component_type {
	HR_ENCLOSURE,
	HR_DISK,
	HR_POWER_SUPPLY,
	HR_ESM,
};


/* Retruns true if ESM has access to this element */
static inline bool
hr_element_access_allowed(enum element_status_code sc)
{
	if (sc == ES_NO_ACCESS_ALLOWED)
		return false;

	return true;
}

static void
hr_check_range(unsigned int n, unsigned int min, unsigned int max, const char *lc)
{
	if (n < min || n > max) {
		fprintf(stderr,
			"%s: number %u out of range in location code %s\n",
			progname, n, lc);
		exit(1);
	}
}

static int
hr_decode_component_loc(struct hr_diag_page2 *dp, const char *loc,
			enum hr_component_type *type, unsigned int *index)
{
	unsigned int n;
	char g;		/* catch trailing garbage */

	if (!loc || !strcmp(loc, "-")) { /* Enclosure */
		*type = HR_ENCLOSURE;
		*index = 0;
	} else if (sscanf(loc, "P1-D%u%c", &n, &g) == 1) { /* Disk */
		hr_check_range(n, 1, HR_NR_DISKS, loc);
		*type = HR_DISK;
		*index = n - 1;

		if (!hr_element_access_allowed(
				dp->disk_status[*index].byte0.status)) {
			fprintf(stderr,
				"Doesn't have access to element : %s\n\n", loc);
			return -1;
		}
	} else if (sscanf(loc, "P1-C%u%c", &n, &g) == 1) { /* ESM */
		hr_check_range(n, 1, HR_NR_ESM_CONTROLLERS, loc);
		*type = HR_ESM;
		*index = n-1;
	} else if (sscanf(loc, "P1-E%u%c", &n, &g) == 1) { /* Power supply */
		hr_check_range(n, 1, HR_NR_POWER_SUPPLY, loc);
		*type = HR_POWER_SUPPLY;
		*index = n-1;
	} else {
		fprintf(stderr, "%s: unrecognized location code: %s\n",
			progname, loc);
		return -1;
	}
	return 0;
}

static const char *on_off_string[] = { "off", "on" };

static void
hr_report_component(struct hr_diag_page2 *dp, int fault, int ident,
		    enum hr_component_type type, unsigned int i, int verbose)
{
	char loc_code[COMP_LOC_CODE];
	char desc[COMP_DESC_SIZE];
	static const char *left_right[] = { "left", "right" };

	switch (type) {
	case HR_ENCLOSURE:
		REPORT_COMPONENT(dp, enclosure_element_status,
				 fault, ident, "-", "enclosure", verbose);
		break;
	case HR_DISK:
		snprintf(loc_code, COMP_LOC_CODE, "P1-D%u", i+1);
		snprintf(desc, COMP_DESC_SIZE, "disk %u", i+1);
		REPORT_COMPONENT(dp, disk_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case HR_POWER_SUPPLY:
		snprintf(loc_code, COMP_LOC_CODE, "P1-E%u", i+1);
		snprintf(desc, COMP_DESC_SIZE,
			 "%s power supply", left_right[i]);
		REPORT_COMPONENT(dp, ps_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case HR_ESM:
		snprintf(loc_code, COMP_LOC_CODE, "P1-C%u", i+1);
		snprintf(desc, COMP_DESC_SIZE,
			 "%s Enclosure RAID Module", left_right[i]);
		REPORT_COMPONENT(dp, esm_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	default:
		fprintf(stderr,
			"%s internal error: unexpected component type %u\n",
			progname, type);
		exit(3);
	}
}

static void
hr_report_component_from_ses(struct hr_diag_page2 *dp,
			     enum hr_component_type type, unsigned int i,
			     int verbose)
{
	hr_report_component(dp, LED_SAME, LED_SAME, type, i, verbose);
}

int
homerun_list_leds(const char *enclosure, const char *component, int verbose)
{
	enum hr_component_type ctype;
	int fd;
	int rc;
	unsigned int i;
	unsigned int cindex;
	struct hr_diag_page2 dp;

	fd = open_sg_device(enclosure);
	if (fd < 0)
		return -1;

	rc = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 2, &dp, sizeof(dp));
	if (rc != 0) {
		fprintf(stderr,
			"%s: cannot read diagnostic page from SES for %s\n",
			progname, enclosure);
		close(fd);
		return -1;
	}


	if (component) {
		rc = hr_decode_component_loc(&dp, component, &ctype, &cindex);
		if (rc != 0) {
			close(fd);
			return -1;
		}

		printf("fault ident location  description\n");
		hr_report_component_from_ses(&dp, ctype, cindex, verbose);
	} else {
		printf("fault ident location  description\n");

		/* Enclosure LED */
		hr_report_component_from_ses(&dp, HR_ENCLOSURE, 0, verbose);

		/* Disk LED */
		for (i = 0; i < HR_NR_DISKS; i++) {
			if (!hr_element_access_allowed(
					dp.disk_status[i].byte0.status))
				continue;

			hr_report_component_from_ses(&dp, HR_DISK, i, verbose);
		}

		/* Power Supply LED */
		for (i = 0; i < HR_NR_POWER_SUPPLY; i++)
			hr_report_component_from_ses(&dp,
						  HR_POWER_SUPPLY, i, verbose);

		/* ESM LED */
		for (i = 0; i < HR_NR_ESM_CONTROLLERS; i++)
			hr_report_component_from_ses(&dp, HR_ESM, i, verbose);
	}

	close(fd);
	return 0;
}

int
homerun_set_led(const char *enclosure,
		const char *component, int fault, int ident, int verbose)
{
	enum hr_component_type type;
	int fd, rc;
	unsigned int index;
	struct hr_diag_page2 dp;
	struct hr_ctrl_page2 cp;

	fd = open_sg_device(enclosure);
	if (fd < 0)
		return -1;

	memset(&dp, 0, sizeof(dp));
	rc = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 2, &dp, sizeof(dp));
	if (rc != 0) {
		fprintf(stderr, "%s: cannot read diagnostic page"
			" from SES for %s\n", progname, enclosure);
		return -1;
	}

	rc = hr_decode_component_loc(&dp, component, &type, &index);
	if (rc != 0)
		return -1;

	memset(&cp, 0, sizeof(cp));

	switch (type) {
	case HR_ENCLOSURE:
		if (fault == LED_ON) {
			fprintf(stderr, "%s: Cannot directly enable enclosure"
					" fault indicator\n", enclosure);
			return -1;
		}
		SET_LED(&cp, &dp, fault, ident,
			enclosure_element_ctrl, enclosure_element_status);
		break;
	case HR_DISK:
		SET_LED(&cp, &dp, fault, ident,
			disk_ctrl[index], disk_status[index]);
		break;
	case HR_POWER_SUPPLY:
		SET_LED(&cp, &dp, fault, ident,
			ps_ctrl[index], ps_status[index]);
		break;
	case HR_ESM:
		SET_LED(&cp, &dp, fault, ident,
			esm_ctrl[index], esm_status[index]);
		break;
	default:
		fprintf(stderr,
			"%s internal error: unexpected component type %u\n",
			progname, type);
		exit(3);
	}

	cp.page_code = 2;
	/* Convert host byte order to network byte order */
	cp.page_length = htons(sizeof(cp) - 4);
	cp.generation_code = 0;

	rc = do_ses_cmd(fd, SEND_DIAGNOSTIC,
			0, 0x10,  6, SG_DXFER_TO_DEV, &cp, sizeof(cp));
	if (rc != 0) {
		fprintf(stderr,"%s: failed to set LED(s) via SES for %s.\n",
			progname, enclosure);
		exit(2);
	}

	if (verbose)
		hr_report_component(&dp, fault, ident, type, index, verbose);
	return 0;
}
