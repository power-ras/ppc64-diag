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

#define _GNU_SOURCE

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

enum slider_variant slider_variant_flag;

/* Slider variant page size/number of disk */
static uint16_t slider_v_diag_page2_size;
static uint16_t slider_v_ctrl_page2_size;
static uint8_t slider_v_nr_disk;
static const char * const on_off_string[] = { "off", "on" };


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
			    const char *loc, enum slider_component_type *type,
			    unsigned int *index)
{
	unsigned int n, n2;
	char g;		/* catch trailing garbage */
	int nr_disk;

	nr_disk = ((slider_variant_flag == SLIDER_V_LFF) ?
			 SLIDER_NR_LFF_DISK: SLIDER_NR_SFF_DISK);

	/* NB: COMP_LOC_CODE is max 16 char wide, and index is formatted as
	 *     unsigned char when the total size is suspected to go beyond 16.
	 *     So, the value cant go above 255. Reconsider increasing the
	 *     width when max value of the range goes higher. */

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

#define SLIDER_REPORT_COMPONENT(dp, status_element, fault, ident, \
					loc_code, desc, verbose) \
	do { \
		if (slider_variant_flag == SLIDER_V_LFF) { \
			struct slider_lff_diag_page2 *d \
			= (struct slider_lff_diag_page2 *)dp; \
			REPORT_COMPONENT(d, status_element, \
				fault, ident, loc_code, desc, verbose); \
		} else { \
			struct slider_sff_diag_page2 *d \
			= (struct slider_sff_diag_page2 *)dp; \
			REPORT_COMPONENT(d, status_element, \
				fault, ident, loc_code, desc, verbose); \
		} \
	} while (0)

static int
slider_report_component(void *dp, int fault, int ident,
			enum slider_component_type type,
			unsigned int i, int verbose)
{
	char loc_code[COMP_LOC_CODE];
	char desc[COMP_DESC_SIZE];
	static const char * const left_right[] = { "left", "right" };

	switch (type) {
	case SLIDER_ENCLOSURE:
		SLIDER_REPORT_COMPONENT(dp, encl_element,
				 fault, ident, "-", "enclosure", verbose);
		break;
	case SLIDER_DISK:
		snprintf(loc_code, COMP_LOC_CODE, "P1-D%u", i+1);
		snprintf(desc, COMP_DESC_SIZE, "disk %u", i+1);
		SLIDER_REPORT_COMPONENT(dp, disk_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case SLIDER_ESC:
		snprintf(loc_code, COMP_LOC_CODE, "P1-C%u", i+1);
		snprintf(desc, COMP_DESC_SIZE,
			 "%s Enclosure RAID Module", left_right[i]);
		SLIDER_REPORT_COMPONENT(dp, enc_service_ctrl_element[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case SLIDER_PS:
		snprintf(loc_code, COMP_LOC_CODE, "P1-E%u", i+1);
		snprintf(desc, COMP_DESC_SIZE,
			 "%s power supply", left_right[i]);
		SLIDER_REPORT_COMPONENT(dp, ps_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	case SLIDER_SAS_CONNECTOR:
		snprintf(loc_code, COMP_LOC_CODE, "P1-C%u-T%u",
			(unsigned char)(i/SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER + 1),
			(unsigned char)(i - ((i/SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER)
				* SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER) + 1));
		snprintf(desc, COMP_DESC_SIZE,
			"SAS connector on %s ESM ",
			left_right[i/SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER]);
		SLIDER_REPORT_COMPONENT(dp, sas_connector_status[i],
				 fault, ident, loc_code, desc, verbose);
		break;
	default:
		fprintf(stderr, "%s internal error: unexpected component "
			"type %u\n", progname, type);
		return -1;
	}

	return 0;
}

/* @return 0 for success, !0 for failure */
static int slider_read_diag_page2(char **page, int *fdp, const char *enclosure)
{
	char *buffer;
	int rc = -1;

	*fdp = open_sg_device(enclosure);
	if (*fdp < 0)
		return rc;

	/* Allocating status diagnostics page */
	buffer = calloc(1, slider_v_diag_page2_size);
	if (!buffer) {
		fprintf(stderr, "Failed to allocate memory to hold "
				"current status diagnostics page 02 results.\n");
		goto close_fd;
	}

	rc = get_diagnostic_page(*fdp, RECEIVE_DIAGNOSTIC, 2,
			(void *)buffer, (int)slider_v_diag_page2_size);
	if (rc != 0) {
		fprintf(stderr, "Failed to read SES diagnostic page; "
				"cannot report status.\n");
		free(buffer);
		goto close_fd;
	}

	*page = buffer;
	return rc;

close_fd:
	close(*fdp);
	return rc;
}

/* Slider variant details */
static int fill_slider_v_specific_details(void)
{
	if (slider_variant_flag == SLIDER_V_LFF) {
		slider_v_diag_page2_size = sizeof(struct slider_lff_diag_page2);
		slider_v_ctrl_page2_size = sizeof(struct slider_lff_ctrl_page2);
		slider_v_nr_disk = SLIDER_NR_LFF_DISK;
	} else if (slider_variant_flag == SLIDER_V_SFF) {
		slider_v_diag_page2_size = sizeof(struct slider_sff_diag_page2);
		slider_v_ctrl_page2_size = sizeof(struct slider_sff_ctrl_page2);
		slider_v_nr_disk = SLIDER_NR_SFF_DISK;
	} else {
		return -1;
	}

	return 0;
}

int slider_list_leds(int slider_type, const char *enclosure,
		     const char *component, int verbose)
{
	char *status_page;
	int fd;
	int rc = -1;
	enum slider_component_type ctype;
	unsigned int cindex, i;

	/* Slider variant flag */
	slider_variant_flag = slider_type;

	/* Fill slider variant specific details */
	if (fill_slider_v_specific_details())
		return rc;

	/* Read diag page */
	if (slider_read_diag_page2(&status_page, &fd, enclosure))
		return rc;

	if (component) {
		rc = slider_decode_component_loc
			((struct slider_disk_status *)
			(status_page + element_offset(disk_status)),
			component, &ctype, &cindex);

		if (rc != 0)
			goto comp_decode_fail;

		printf("fault ident location  description\n");
		rc = slider_report_component(status_page, LED_SAME, LED_SAME,
						ctype, cindex, verbose);
	} else {
		printf("fault ident location  description\n");

		/* Enclosure LED */
		rc = slider_report_component(status_page, LED_SAME, LED_SAME,
					SLIDER_ENCLOSURE, 0, verbose);

		/* Disk LED */
		for (i = 0; i < slider_v_nr_disk; i++) {
			struct slider_disk_status *disk_status
				= (struct slider_disk_status *)
				(status_page + element_offset(disk_status));
			if (!slider_element_access_allowed(disk_status[i].byte0.status))
				continue;

			rc = slider_report_component(status_page, LED_SAME,
				LED_SAME, SLIDER_DISK, i, verbose);
		}

		/* ESC LED */
		for (i = 0; i < SLIDER_NR_ESC; i++)
			rc = slider_report_component(status_page, LED_SAME,
				LED_SAME, SLIDER_ESC, i, verbose);

		/* PS LED */
		for (i = 0; i < SLIDER_NR_POWER_SUPPLY; i++)
			rc = slider_report_component(status_page, LED_SAME,
				LED_SAME, SLIDER_PS, i, verbose);

		/* SAS connector LED */
		for (i = 0; i < SLIDER_NR_SAS_CONNECTOR; i++)
			rc = slider_report_component(status_page, LED_SAME,
				LED_SAME, SLIDER_SAS_CONNECTOR, i, verbose);
	}

comp_decode_fail:
	free(status_page);
	close(fd);
	return rc;
}

int
slider_sff_list_leds(const char *enclosure, const char *component, int verbose)
{
	return slider_list_leds(SLIDER_V_SFF, enclosure, component, verbose);
}

int
slider_lff_list_leds(const char *enclosure, const char *component, int verbose)
{
	return slider_list_leds(SLIDER_V_LFF, enclosure, component, verbose);
}

#define SLIDER_SET_LED(ctrl_page, dp, fault, ident, ctrl_element, \
						status_element) \
	do { \
		if (slider_variant_flag == SLIDER_V_LFF) { \
			struct slider_lff_diag_page2 *d \
			= (struct slider_lff_diag_page2 *)dp; \
			struct slider_lff_ctrl_page2 *c \
			= (struct slider_lff_ctrl_page2 *)ctrl_page; \
			SET_LED(c, d, fault, ident, ctrl_element, \
						status_element); \
		} else { \
			struct slider_sff_diag_page2 *d \
			= (struct slider_sff_diag_page2 *)dp; \
			struct slider_sff_ctrl_page2 *c \
			= (struct slider_sff_ctrl_page2 *)ctrl_page; \
			SET_LED(c, d, fault, ident, ctrl_element, \
						status_element); \
		} \
	} while (0)

static int slider_set_led(int slider_type, const char *enclosure,
			const char *component, int fault, int ident,
							int verbose)
{
	int rc = -1;
	char *ctrl_page;
	char *status_page;
	int fd;
	enum slider_component_type type;
	unsigned int index;

	/* Slider variant flag */
	slider_variant_flag = slider_type;

	/* Fill slider variant specific details */
	if (fill_slider_v_specific_details())
		return rc;

	/* Read diag page */
	if (slider_read_diag_page2(&status_page, &fd, enclosure))
		return rc;

	rc = slider_decode_component_loc((struct slider_disk_status *)
					 (status_page + element_offset(disk_status)),
					 component, &type, &index);
	if (rc != 0)
		goto free_alloc_dp;

	/* Allocate ctrl page buffer */
	ctrl_page = calloc(1, slider_v_ctrl_page2_size);
	if (!ctrl_page) {
		fprintf(stderr, "Failed to allocate memory to hold "
				"current ctrl diagnostics page 02 results.\n");
		goto free_alloc_dp;
	}

	switch (type) {
	case SLIDER_ENCLOSURE:
		if (fault == LED_ON) {
			fprintf(stderr, "%s: Cannot directly enable enclosure"
					" fault indicator\n", enclosure);
			goto free_alloc_cp;
		}
		SLIDER_SET_LED(ctrl_page, status_page, fault, ident,
					encl_element, encl_element);
		break;
	case SLIDER_DISK:
		SLIDER_SET_LED(ctrl_page, status_page, fault, ident,
			disk_ctrl[index], disk_status[index]);
		break;
	case SLIDER_ESC:
		SLIDER_SET_LED(ctrl_page, status_page, fault, ident,
			enc_service_ctrl_element[index],
			enc_service_ctrl_element[index]);
		break;
	case SLIDER_PS:
		SLIDER_SET_LED(ctrl_page, status_page, fault, ident,
			ps_ctrl[index], ps_status[index]);
		break;
	case SLIDER_SAS_CONNECTOR:
		SLIDER_SET_LED(ctrl_page, status_page, fault, ident,
			sas_connector_ctrl[index],
			sas_connector_status[index]);
		break;
	default:
		fprintf(stderr,
			"%s internal error: unexpected component type %u\n",
			progname, type);
		goto free_alloc_cp;
	}

	SLIDER_ASSIGN_CTRL_PAGE(ctrl_page);
	rc = do_ses_cmd(fd, SEND_DIAGNOSTIC, 0, 0x10, 6, SG_DXFER_TO_DEV,
			ctrl_page, slider_v_ctrl_page2_size);
	if (rc != 0) {
		fprintf(stderr, "%s: failed to set LED(s) via SES for %s.\n",
			progname, enclosure);
		goto free_alloc_cp;
	}

	if (verbose)
		rc = slider_report_component(status_page, fault, ident,
					     type, index, verbose);

free_alloc_cp:
	free(ctrl_page);

free_alloc_dp:
	free(status_page);
	close(fd);

	return rc;
}

int
slider_sff_set_led(const char *enclosure, const char *component,
		   int fault, int ident, int verbose)
{
	return slider_set_led(SLIDER_V_SFF, enclosure,
			      component, fault, ident, verbose);
}

int
slider_lff_set_led(const char *enclosure, const char *component, int fault,
						int ident, int verbose)
{
	return slider_set_led(SLIDER_V_LFF, enclosure,
			      component, fault, ident, verbose);
}
