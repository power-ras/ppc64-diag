/*
 * Copyright (C) 2016 IBM Corporation
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include "encl_led.h"
#include "encl_util.h"
#include "slider.h"

enum slider_component_type {
	SLIDER_DISK,
	SLIDER_ESC,
	SLIDER_PS,
	SLIDER_SAS_CONNECTOR,
	SLIDER_ENCLOSURE,
};

/* Returns true if ESC has access to this element */
static inline bool
slider_element_access_allowed(enum element_status_code sc)
{
	if (sc == ES_NO_ACCESS_ALLOWED)
		return false;

	return true;
}

static int
slider_decode_component_loc(struct slider_disk_status *disk_status,
	int nr_disk, const char *loc, enum slider_component_type *type,
	unsigned int *index)
{
	unsigned int n, n2;
	char g;		/* catch trailing garbage */

	if (!loc || !strcmp(loc, "-")) { /* Enclosure */
		*type = SLIDER_ENCLOSURE;
		*index = 0;
	} else if (sscanf(loc, "P1-D%u%c", &n, &g) == 1) { /* Disk */
		element_check_range(n, 1, nr_disk, loc);
		*type = SLIDER_DISK;
		*index = n - 1;

		if (!slider_element_access_allowed(
				disk_status[*index].byte0.status)) {
			fprintf(stderr,
				"Doesn't have access to element : %s\n\n", loc);
			return -1;
		}
	} else if (sscanf(loc, "P1-C%u%c", &n, &g) == 1) { /* ESC */
		element_check_range(n, 1, SLIDER_NR_ESC, loc);
		*type = SLIDER_ESC;
		*index = n-1;
	} else if (sscanf(loc, "P1-E%u%c", &n, &g) == 1) { /* PS */
		element_check_range(n, 1, SLIDER_NR_POWER_SUPPLY, loc);
		*type = SLIDER_PS;
		*index = n-1;
	} else if (sscanf(loc, "P1-C%u-T%u%c", &n, &n2, &g) == 2) { /* SAS connector */
		element_check_range(n, 1, SLIDER_NR_ESC, loc);
		element_check_range(n2, 1,
				    SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER, loc);
		*type = SLIDER_SAS_CONNECTOR;
		*index = (((n - 1) * SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER))
			+ (n2 - 1);
	} else {
		fprintf(stderr, "%s: unrecognized location code: %s\n",
			progname, loc);
		return -1;
	}
	return 0;
}

static const char * const on_off_string[] = { "off", "on" };

static void
slider_lff_report_component(struct slider_lff_diag_page2 *dp, int fault,
	int ident, enum slider_component_type type, unsigned int i, int verbose)
{
	char loc_code[COMP_LOC_CODE];
	char desc[COMP_DESC_SIZE];
	static const char * const left_right[] = { "left", "right" };

	switch (type) {
	case SLIDER_ENCLOSURE:
		REPORT_COMPONENT(dp, encl_element,
				 fault, ident, "-", "enclosure", verbose);
		break;
	case SLIDER_DISK:
		snprintf(loc_code, COMP_LOC_CODE, "P1-D%u", i+1);
		snprintf(desc, COMP_DESC_SIZE, "disk %u", i+1);
		REPORT_COMPONENT(dp, disk_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case SLIDER_ESC:
		snprintf(loc_code, COMP_LOC_CODE, "P1-C%u", i+1);
		snprintf(desc, COMP_DESC_SIZE,
			 "%s Enclosure service controller", left_right[i]);
		REPORT_COMPONENT(dp, enc_service_ctrl_element[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case SLIDER_PS:
		snprintf(loc_code, COMP_LOC_CODE, "P1-E%u", i+1);
		snprintf(desc, COMP_DESC_SIZE,
			 "%s Power supply", left_right[i]);
		REPORT_COMPONENT(dp, ps_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case SLIDER_SAS_CONNECTOR:
		snprintf(loc_code, COMP_LOC_CODE, "P1-C%u-T%u",
			i/SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER + 1,
			i - ((i/SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER)
				* SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER) + 1);
		snprintf(desc, COMP_DESC_SIZE,
			"SAS CONNECTOR on %s ESC ",
			left_right[i/SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER]);
		REPORT_COMPONENT(dp, sas_connector_status[i],
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
slider_sff_report_component(struct slider_sff_diag_page2 *dp, int fault,
	int ident, enum slider_component_type type, unsigned int i, int verbose)
{
	char loc_code[COMP_LOC_CODE];
	char desc[COMP_DESC_SIZE];
	static const char * const left_right[] = { "left", "right" };

	switch (type) {
	case SLIDER_ENCLOSURE:
		REPORT_COMPONENT(dp, encl_element,
				 fault, ident, "-", "enclosure", verbose);
		break;
	case SLIDER_DISK:
		snprintf(loc_code, COMP_LOC_CODE, "P1-D%u", i+1);
		snprintf(desc, COMP_DESC_SIZE, "disk %u", i+1);
		REPORT_COMPONENT(dp, disk_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case SLIDER_ESC:
		snprintf(loc_code, COMP_LOC_CODE, "P1-C%u", i+1);
		snprintf(desc, COMP_DESC_SIZE,
			 "%s Enclosure RAID Module", left_right[i]);
		REPORT_COMPONENT(dp, enc_service_ctrl_element[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case SLIDER_PS:
		snprintf(loc_code, COMP_LOC_CODE, "P1-E%u", i+1);
		snprintf(desc, COMP_DESC_SIZE,
			 "%s Power supply", left_right[i]);
		REPORT_COMPONENT(dp, ps_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case SLIDER_SAS_CONNECTOR:
		snprintf(loc_code, COMP_LOC_CODE, "P1-C%u-T%u",
			i/SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER + 1,
			i - ((i/SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER)
				* SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER) + 1);
		snprintf(desc, COMP_DESC_SIZE,
			"SAS CONNECTOR on %s ESC ",
			left_right[i/SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER]);
		REPORT_COMPONENT(dp, sas_connector_status[i],
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
slider_lff_report_component_from_ses(struct slider_lff_diag_page2 *dp,
	enum slider_component_type type, unsigned int i, int verbose)
{
	slider_lff_report_component(dp, LED_SAME, LED_SAME, type, i, verbose);
}

static void
slider_sff_report_component_from_ses(struct slider_sff_diag_page2 *dp,
	enum slider_component_type type, unsigned int i, int verbose)
{
	slider_sff_report_component(dp, LED_SAME, LED_SAME, type, i, verbose);
}

/* Read diagnostics page */
static int slider_read_diag_page2(const char *enclosure,
				  int *fdp, char *dp_p, int size)
{
	int rc = -1;

	*fdp = open_sg_device(enclosure);
	if (*fdp < 0)
		return rc;

	memset(dp_p, 0, size);

	rc = get_diagnostic_page(*fdp, RECEIVE_DIAGNOSTIC, 2, dp_p, size);
	if (rc != 0) {
		fprintf(stderr, "%s: cannot read diagnostic page"
				" from SES for %s\n", progname, enclosure);
		close(*fdp);
	}

	return rc;
}

int
slider_lff_list_leds(const char *enclosure, const char *component, int verbose)
{
	enum slider_component_type ctype;
	int fd;
	int rc;
	unsigned int i;
	unsigned int cindex;
	struct slider_lff_diag_page2 dp;

	rc = slider_read_diag_page2(enclosure, &fd, (char *)&dp, sizeof(dp));
	if (rc)
		return rc;

	if (component) {
		rc = slider_decode_component_loc(dp.disk_status,
			SLIDER_NR_LFF_DISK, component, &ctype, &cindex);

		if (rc != 0) {
			close(fd);
			return -1;
		}

		printf("fault ident location  description\n");
		slider_lff_report_component_from_ses(&dp, ctype, cindex,
						     verbose);
	} else {
		printf("fault ident location  description\n");

		/* Enclosure LED */
		slider_lff_report_component_from_ses(&dp, SLIDER_ENCLOSURE, 0,
						     verbose);

		/* Disk LED */
		for (i = 0; i < SLIDER_NR_LFF_DISK; i++) {
			if (!slider_element_access_allowed(
					dp.disk_status[i].byte0.status))
				continue;

			slider_lff_report_component_from_ses(&dp, SLIDER_DISK,
				i, verbose);
		}

		/* ESC LED */
		for (i = 0; i < SLIDER_NR_ESC; i++)
			slider_lff_report_component_from_ses(&dp, SLIDER_ESC,
				i, verbose);

		/* PS LED */
		for (i = 0; i < SLIDER_NR_POWER_SUPPLY; i++)
			slider_lff_report_component_from_ses(&dp, SLIDER_PS,
							     i, verbose);

		/* SAS connector LED */
		for (i = 0; i < SLIDER_NR_SAS_CONNECTOR; i++)
			slider_lff_report_component_from_ses
				(&dp, SLIDER_SAS_CONNECTOR, i, verbose);
	}

	close(fd);
	return 0;
}

int
slider_sff_list_leds(const char *enclosure, const char *component, int verbose)
{
	enum slider_component_type ctype;
	int fd;
	int rc;
	unsigned int i;
	unsigned int cindex;
	struct slider_sff_diag_page2 dp;

	rc = slider_read_diag_page2(enclosure, &fd, (char *)&dp, sizeof(dp));
	if (rc)
		return -1;

	if (component) {
		rc = slider_decode_component_loc(dp.disk_status,
			SLIDER_NR_SFF_DISK, component, &ctype, &cindex);
		if (rc != 0) {
			close(fd);
			return -1;
		}

		printf("fault ident location  description\n");
		slider_sff_report_component_from_ses(&dp, ctype, cindex,
						     verbose);
	} else {
		printf("fault ident location  description\n");

		/* Enclosure LED */
		slider_sff_report_component_from_ses(&dp, SLIDER_ENCLOSURE, 0,
						     verbose);

		/* Disk LED */
		for (i = 0; i < SLIDER_NR_SFF_DISK; i++) {
			if (!slider_element_access_allowed(
					dp.disk_status[i].byte0.status))
				continue;

			slider_sff_report_component_from_ses(&dp, SLIDER_DISK,
							     i, verbose);
		}

		/* ESC LED */
		for (i = 0; i < SLIDER_NR_ESC; i++)
			slider_sff_report_component_from_ses(&dp, SLIDER_ESC,
				i, verbose);

		/* PS LED */
		for (i = 0; i < SLIDER_NR_POWER_SUPPLY; i++)
			slider_lff_report_component_from_ses(&dp, SLIDER_PS,
							     i, verbose);

		/* SAS connector LED */
		for (i = 0; i < SLIDER_NR_SAS_CONNECTOR; i++)
			slider_sff_report_component_from_ses
				(&dp, SLIDER_SAS_CONNECTOR, i, verbose);
	}

	close(fd);
	return 0;
}

int
slider_lff_set_led(const char *enclosure,
		   const char *component, int fault, int ident, int verbose)
{
	enum slider_component_type type;
	int fd, rc;
	unsigned int index;
	struct slider_lff_diag_page2 dp;
	struct slider_lff_ctrl_page2 cp;

	rc = slider_read_diag_page2(enclosure, &fd, (char *)&dp, sizeof(dp));
	if (rc)
		return -1;

	rc = slider_decode_component_loc(dp.disk_status, SLIDER_NR_LFF_DISK,
					 component, &type, &index);
	if (rc != 0) {
		close(fd);
		return -1;
	}

	memset(&cp, 0, sizeof(cp));

	switch (type) {
	case SLIDER_ENCLOSURE:
		if (fault == LED_ON) {
			fprintf(stderr, "%s: Cannot directly enable enclosure"
					" fault indicator\n", enclosure);
			close(fd);
			return -1;
		}
		SET_LED(&cp, &dp, fault, ident, encl_element, encl_element);
		break;
	case SLIDER_DISK:
		SET_LED(&cp, &dp, fault, ident,
			disk_ctrl[index], disk_status[index]);
		break;
	case SLIDER_ESC:
		SET_LED(&cp, &dp, fault, ident,
			enc_service_ctrl_element[index],
			enc_service_ctrl_element[index]);
		break;
	case SLIDER_PS:
		if (fault == LED_ON) {
			fprintf(stderr, "%s: Power supply fault indicator is not "
				"supported.\n", enclosure);
			close(fd);
			return -1;
		}
		SET_LED(&cp, &dp, fault, ident,
			ps_ctrl[index], ps_status[index]);
		break;
	case SLIDER_SAS_CONNECTOR:
		SET_LED(&cp, &dp, fault, ident,
			sas_connector_ctrl[index],
			sas_connector_status[index]);
		break;
	default:
		fprintf(stderr,
			"%s internal error: unexpected component type %u\n",
			progname, type);
		close(fd);
		exit(3);
	}

	cp.page_code = 2;
	/* Convert host byte order to network byte order */
	cp.page_length = htons(sizeof(cp) - 4);
	cp.generation_code = 0;

	rc = do_ses_cmd(fd, SEND_DIAGNOSTIC,
			0, 0x10,  6, SG_DXFER_TO_DEV, &cp, sizeof(cp));
	if (rc != 0) {
		fprintf(stderr, "%s: failed to set LED(s) via SES for %s.\n",
			progname, enclosure);
		close(fd);
		exit(2);
	}

	if (verbose)
		slider_lff_report_component(&dp, fault, ident, type, index,
			verbose);

	close(fd);
	return 0;
}

int
slider_sff_set_led(const char *enclosure,
		   const char *component, int fault, int ident, int verbose)
{
	enum slider_component_type type;
	int fd, rc;
	unsigned int index;
	struct slider_sff_diag_page2 dp;
	struct slider_sff_ctrl_page2 cp;

	rc = slider_read_diag_page2(enclosure, &fd, (char *)&dp, sizeof(dp));
	if (rc)
		return -1;

	rc = slider_decode_component_loc(dp.disk_status, SLIDER_NR_SFF_DISK,
					 component, &type, &index);
	if (rc != 0) {
		close(fd);
		return -1;
	}

	memset(&cp, 0, sizeof(cp));

	switch (type) {
	case SLIDER_ENCLOSURE:
		if (fault == LED_ON) {
			fprintf(stderr, "%s: Cannot directly enable enclosure"
					" fault indicator\n", enclosure);
			close(fd);
			return -1;
		}
		SET_LED(&cp, &dp, fault, ident, encl_element, encl_element);
		break;
	case SLIDER_DISK:
		SET_LED(&cp, &dp, fault, ident,
			disk_ctrl[index], disk_status[index]);
		break;
	case SLIDER_ESC:
		SET_LED(&cp, &dp, fault, ident,
			enc_service_ctrl_element[index],
			enc_service_ctrl_element[index]);
		break;
	case SLIDER_PS:
		if (fault == LED_ON) {
			fprintf(stderr, "%s: Power supply fault indicator is not "
				"supported.\n", enclosure);
			close(fd);
			return -1;
		}
		SET_LED(&cp, &dp, fault, ident,
			ps_ctrl[index], ps_status[index]);
		break;
	case SLIDER_SAS_CONNECTOR:
		SET_LED(&cp, &dp, fault, ident,
			sas_connector_ctrl[index],
			sas_connector_status[index]);
		break;
	default:
		fprintf(stderr,
			"%s internal error: unexpected component type %u\n",
			progname, type);
		close(fd);
		exit(3);
	}

	cp.page_code = 2;
	/* Convert host byte order to network byte order */
	cp.page_length = htons(sizeof(cp) - 4);
	cp.generation_code = 0;

	rc = do_ses_cmd(fd, SEND_DIAGNOSTIC,
			0, 0x10,  6, SG_DXFER_TO_DEV, &cp, sizeof(cp));
	if (rc != 0) {
		fprintf(stderr, "%s: failed to set LED(s) via SES for %s.\n",
			progname, enclosure);
		close(fd);
		exit(2);
	}

	if (verbose)
		slider_sff_report_component(&dp, fault, ident,
					    type, index, verbose);

	close(fd);
	return 0;
}
