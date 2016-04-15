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
#include <arpa/inet.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include "diag_encl.h"
#include "encl_util.h"
#include "homerun.h"


/* Retruns true if ESM has access to this component */
static inline bool
hr_element_access_allowed(enum element_status_code sc)
{
	if (sc == ES_NO_ACCESS_ALLOWED)
		return false;

	return true;
}

/** Helper functions to print various component status **/

static void
hr_print_drive_status(struct disk_status *s)
{
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL,
		ES_NOT_INSTALLED, ES_NO_ACCESS_ALLOWED, ES_EOL
	};

	return print_drive_status(s, valid_codes);
}

static void
hr_print_enclosure_status(struct enclosure_status *s)
{
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_EOL };

	return print_enclosure_status(s, valid_codes);
}

static void
hr_print_fan_status(struct fan_status *s)
{
	const char *speed[] = {
		"Fan at lowest speed",
		"Fan at lowest speed",
		"Fan at second lowest speed",
		"Fan at third lowest speed",
		"Fan at intermediate speed",
		"Fan at third highest speed",
		"Fan at second highest speed",
		"Fan at highest speed"
	};
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_NOT_INSTALLED,
		ES_UNKNOWN, ES_EOL
	};

	return print_fan_status(s, valid_codes, speed);
}


/*
 * The fru_label should be "P1-C1" or "P1-C2" (without the terminating null).
 * i is 0 or 1.
 */
static int
hr_esm_location_match(int i, const char *fru_label)
{
	return ('0'+i+1 == fru_label[4]);
}

/* Create a callout for power supply i (i = 0 or 1). */
static int
hr_create_ps_callout(struct sl_callout **callouts,
		     char *location, unsigned int i, int fd)
{
	char fru_number[FRU_NUMBER_LEN + 1];
	char serial_number[SERIAL_NUMBER_LEN + 1];
	int rc;
	struct hr_element_descriptor_page *edp;
	struct power_supply_descriptor *ps_vpd[HR_NR_POWER_SUPPLY];

	if (fd < 0) {
		add_location_callout(callouts, location);
		return 0;
	}

	edp = calloc(1, sizeof(struct hr_element_descriptor_page));
	if (!edp) {
		fprintf(stderr, "%s: Failed to allocate memory\n", __func__);
		return 1;
	}

	rc = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 7, edp,
				 sizeof(struct hr_element_descriptor_page));
	if (rc) {
		add_location_callout(callouts, location);
		goto out;
	}

	ps_vpd[0] = &(edp->ps0_vpd);
	ps_vpd[1] = &(edp->ps1_vpd);
	strzcpy(fru_number, ps_vpd[i]->fru_number, FRU_NUMBER_LEN);
	strzcpy(serial_number, ps_vpd[i]->serial_number, SERIAL_NUMBER_LEN);
	add_callout(callouts, 'M', 0, NULL,
		    location, fru_number, serial_number, NULL);

out:
	free(edp);
	return 0;
}

/**
 * Create a callout for ESM i (left=0, right=1). VPD page 1 contains
 * VPD for only one of the ESM. If it's the wrong one, just do without
 * the VPD.
 */
static void
hr_create_esm_callout(struct sl_callout **callouts,
		      char *location, unsigned int i, int fd)
{
	int rc = -1;
	struct vpd_page esm;

	/* Enclosure is opened for RW */
	if (fd >= 0)
		rc = get_diagnostic_page(fd, INQUIRY, 1, &esm, sizeof(esm));

	if (rc == 0 && hr_esm_location_match(i, esm.fru_label))
		add_callout_from_vpd_page(callouts, location, &esm);
	else
		add_location_callout(callouts, location);
}

static int
hr_report_faults_to_svclog(int fd, struct dev_vpd *vpd,
			   struct hr_diag_page2 *dp)
{
	const char *left_right[] = { "left", "right" };
	const char run_diag_encl[] = " Run diag_encl for more detailed status,"
		" and refer to the system service documentation for guidance.";
	const char ref_svc_doc[] =
		"  Refer to the system service documentation for guidance.";
	char srn[SRN_SIZE];
	char crit[ES_STATUS_STRING_MAXLEN];
	char description[EVENT_DESC_SIZE];
	char location[LOCATION_LENGTH], *loc_suffix;
	int i;
	int sev;
	int rc;
	int loc_suffix_size;
	struct sl_callout *callouts;
	struct hr_diag_page2 *prev_dp = NULL;     /* for -c */


	strncpy(location, vpd->location, LOCATION_LENGTH - 1);
	location[LOCATION_LENGTH - 1] = '\0';
	loc_suffix_size = LOCATION_LENGTH - strlen(location);
	loc_suffix = location + strlen(location);

	if (cmd_opts.cmp_prev) {
		prev_dp = calloc(1, sizeof(struct hr_diag_page2));
		if (!prev_dp) {
			fprintf(stderr, "%s: Failed to allocate memory\n",
				__func__);
			return 1;
		}

		rc = read_page2_from_file(cmd_opts.prev_path, false, prev_dp,
					  sizeof(struct hr_diag_page2));
		if (rc) {
			free(prev_dp);
			prev_dp = NULL;
		}
	}

	/* disk drives */
	for (i = 0; i < HR_NR_DISKS; i++) {
		if (!hr_element_access_allowed(dp->disk_status[i].byte0.status))
			continue;

		sev = svclog_element_status(&(dp->disk_status[i].byte0),
					    (char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in enclosure disk %u.%s",
			 crit, i + 1, run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-D%u", i+1);
		callouts = NULL;
		/* VPD for disk drives is not available from the SES. */
		add_location_callout(&callouts, location);
		servevent("none", sev, description, vpd, callouts);
	}

	/* power supplies */
	for (i = 0; i < HR_NR_POWER_SUPPLY; i++) {
		sev = svclog_element_status(&(dp->ps_status[i].byte0),
					    (char *) dp, (char *) prev_dp,
					    crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in %s power supply in enclosure.%s",
			 crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-E%u", i+1);
		build_srn(srn, SRN_RC_CRIT_PS);
		callouts = NULL;
		rc = hr_create_ps_callout(&callouts, location, i, fd);
		if (rc == 0)
			servevent(srn, sev, description, vpd, callouts);
	}

	/* voltage sensors */
	for (i = 0; i < HR_NR_VOLTAGE_SENSOR_SET; i++) {
		sev = svclog_composite_status(&(dp->voltage_sensor_sets[i]),
					      (char *) dp, (char *) prev_dp,
					      3, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			"%s fault associated with %s power supply in "
			"enclosure: voltage sensor(s) reporting voltage(s) "
			"out of range.%s", crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-E%u", i+1);
		build_srn(srn, SRN_RC_VOLTAGE_THRESHOLD);
		callouts = NULL;
		rc = hr_create_ps_callout(&callouts, location, i, fd);
		if (rc == 0)
			servevent(srn, sev, description, vpd, callouts);
	}

	/* Cooling elements (8 per element) */
	for (i = 0; i < HR_NR_FAN_SET; i++) {
		sev = svclog_composite_status(&(dp->fan_sets[i].fan_element),
					      (char *) dp, (char *) prev_dp,
					      HR_NR_FAN_ELEMENT_PER_SET, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			"%s fault in cooling element for %s power supply in"
			" enclosure.%s", crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-E%u", i+1);
		build_srn(srn, SRN_RC_CRIT_FAN);
		callouts = NULL;
		rc = hr_create_ps_callout(&callouts, location, i, fd);
		if (rc == 0)
			servevent(srn, sev, description, vpd, callouts);
	}

	/* Temperature Sensors (3 each per PS) */
	for (i = 0; i < HR_NR_TEMP_SENSOR_SET; i++) {
		sev = svclog_composite_status(
				&(dp->temp_sensor_sets[i].power_supply),
				(char *) dp, (char *) prev_dp,
				HR_NR_TEMP_SENSORS_PER_SET, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			"%s fault in temperature sensor(s) for %s power"
			" supply in enclosure.%s", crit,
			left_right[i], run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-E%u", i+1);
		build_srn(srn, SRN_RC_PS_TEMP_THRESHOLD);
		callouts = NULL;
		rc = hr_create_ps_callout(&callouts, location, i, fd);
		if (rc == 0)
			servevent(srn, sev, description, vpd, callouts);
	}

	/* Temperature Sensors (1 per ESM) */
	for (i = 0; i < HR_NR_ESM_CONTROLLERS; i++) {
		sev = svclog_element_status(
				&(dp->temp_sensor_sets[i].controller.byte0),
				(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			"%s fault associated with ESM-%c side of the "
			"enclosure: temperature sensor(s) reporting "
			"temperature(s) out of range.%s",
			crit, 'A' + i, run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-C%u", i+1);
		build_srn(srn, SRN_RC_TEMP_THRESHOLD);
		callouts = NULL;
		hr_create_esm_callout(&callouts, location, i, fd);
		if (rc == 0)
			servevent(srn, sev, description, vpd, callouts);
	}

	/* ESM electronics */
	for (i = 0; i < HR_NR_ESM_CONTROLLERS; i++) {
		sev = svclog_element_status(&(dp->esm_status[i].byte0),
					    (char *) dp, (char *) prev_dp,
					    crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			"%s electronics fault in %s ESM Module.%s",
			crit, left_right[i], ref_svc_doc);
		snprintf(loc_suffix, loc_suffix_size, "-P1-C%u", i+1);
		build_srn(srn, SRN_RC_CRIT_ESM);
		callouts = NULL;
		hr_create_esm_callout(&callouts, location, i, fd);
		servevent(srn, sev, description, vpd, callouts);
	}

	return write_page2_to_file(cmd_opts.prev_path, dp,
				   sizeof(struct hr_diag_page2));
}

static int
hr_turn_on_fault_leds(struct hr_diag_page2 *dp, int fd)
{
	int i;
	int poked_leds = 0;
	struct hr_ctrl_page2 *ctrl_page;

	ctrl_page = calloc(1, sizeof(struct hr_ctrl_page2));
	if (!ctrl_page) {
		fprintf(stderr, "%s: Failed to allocate memory\n",
			__func__);
		return 1;
	}

	/* disk drives */
	for (i = 0; i < HR_NR_DISKS; i++) {
		if (!hr_element_access_allowed(dp->disk_status[i].byte0.status))
			continue;

		FAULT_LED(poked_leds, dp, ctrl_page,
			  disk_ctrl[i], disk_status[i]);
	}

	/* ERM/ESM electronics */
	for (i = 0; i < HR_NR_ESM_CONTROLLERS; i++)
		FAULT_LED(poked_leds, dp, ctrl_page,
			  esm_ctrl[i], esm_status[i]);

	/* No LEDs for temperature sensors */

	/* fan assemblies */
	for (i = 0; i < HR_NR_FAN_SET; i++) {
		enum element_status_code sc =
				composite_status(&(dp->fan_sets[i]),
						 HR_NR_FAN_ELEMENT_PER_SET);
		if (sc != ES_OK && sc != ES_NOT_INSTALLED)
			FAULT_LED(poked_leds, dp, ctrl_page,
				  fan_sets[i].fan_element[0],
				  fan_sets[i].fan_element[0]);
	}

	/* power supplies */
	for (i = 0; i < HR_NR_POWER_SUPPLY; i++)
		FAULT_LED(poked_leds, dp, ctrl_page, ps_ctrl[i], ps_status[i]);

	/* No LEDs for voltage sensors */

	if (poked_leds) {
		int result;

		ctrl_page->page_code = 2;
		ctrl_page->page_length = sizeof(struct hr_ctrl_page2) - 4;
		/* Convert host byte order to network byte order */
		ctrl_page->page_length = htons(ctrl_page->page_length);
		ctrl_page->generation_code = 0;
		result = do_ses_cmd(fd, SEND_DIAGNOSTIC, 0, 0x10, 6,
				    SG_DXFER_TO_DEV, ctrl_page,
				    sizeof(struct hr_ctrl_page2));
		if (result != 0) {
			perror("ioctl - SEND_DIAGNOSTIC");
			fprintf(stderr, "result = %d\n", result);
			fprintf(stderr, "failed to set LED(s) via SES\n");
			free(ctrl_page);
			return -1;
		}
	}

	free(ctrl_page);
	return 0;
}

/* @return 0 for success, 1 for failure */
int
diag_homerun(int fd, struct dev_vpd *vpd)
{
	static const char *power_supply_names[] = {
		"PS0 (Left)",
		"PS1 (Right)"
	};
	static const char *fan_set_names[] = {
		"Left Cooling Assembly",
		"Right Cooling Assembly",
	};
	static const char *temp_sensor_set_names[] = {
		"Left Temperature Sensor Controller",
		"Right Temperature Sensor Controller"
	};
	static const char *esm_names[] = {
		"Left",
		"Right"
	};
	int i, j, rc;
	struct hr_diag_page2 *dp;

	dp = calloc(1, sizeof(struct hr_diag_page2));
	if (!dp) {
		fprintf(stderr, "%s: Failed to allocate memory\n", __func__);
		return 1;
	}

	if (cmd_opts.fake_path) {
		rc = read_page2_from_file(cmd_opts.fake_path, true,
					  dp, sizeof(struct hr_diag_page2));
		fd = -1;
	} else {
		rc = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 2, (void *)dp,
					 sizeof(struct hr_diag_page2));
	}

	if (rc != 0) {
		fprintf(stderr, "Failed to read SES diagnostic page; "
				"cannot report status.\n");
		goto err_out;
	}

	printf("\n");
	printf("  Overall Status:    ");
	if (dp->crit) {
		printf("CRITICAL_FAULT");
		if (dp->non_crit)
			printf(" | NON_CRITICAL_FAULT");
	} else if (dp->non_crit) {
		printf("NON_CRITICAL_FAULT");
	} else {
		printf("ok");
	}

	printf("\n\n  Drive Status\n");
	for (i = 0; i < HR_NR_DISKS; i++) {
		if (!hr_element_access_allowed(dp->disk_status[i].byte0.status))
			continue;

		struct disk_status *ds = &(dp->disk_status[i]);
		printf("    Disk %02d (Slot %02d): ", i+1,
		       ds->byte1.element_status.slot_address);
		hr_print_drive_status(ds);
	}

	printf("\n  Power Supply Status\n");
	for (i = 0; i < HR_NR_POWER_SUPPLY; i++) {
		printf("    %s:  ", power_supply_names[i]);
		print_power_supply_status(&(dp->ps_status[i]));
		printf("      12V:  ");
		print_voltage_sensor_status(
			    &(dp->voltage_sensor_sets[i].sensor_12V));
		printf("      5V:   ");
		print_voltage_sensor_status(
			    &(dp->voltage_sensor_sets[i].sensor_5V));
		printf("      5VA:  ");
		print_voltage_sensor_status(
			    &(dp->voltage_sensor_sets[i].sensor_5VA));
	}

	printf("\n  Fan Status\n");
	for (i = 0; i < HR_NR_FAN_SET; i++) {
		printf("    %s:\n", fan_set_names[i]);
		for (j = 0; j < HR_NR_FAN_ELEMENT_PER_SET; j++) {
			printf("      Fan Element %d:  ", j);
			hr_print_fan_status(&(dp->fan_sets[i].fan_element[j]));
		}
	}

	printf("\n  Temperature Sensors\n");
	for (i = 0; i < HR_NR_TEMP_SENSOR_SET; i++) {
		struct hr_temperature_sensor_set *tss =
					&(dp->temp_sensor_sets[i]);
		printf("      %s:  ", temp_sensor_set_names[i]);
		print_temp_sensor_status(&tss->controller);
		for (j = 0; j < HR_NR_TEMP_SENSORS_PER_SET; j++) {
			printf("      Temperature Sensor Power Supply %d%c:  ",
			       j, i == 0?'L':'R');
			print_temp_sensor_status(&tss->power_supply[j]);
		}
	}

	printf("\n  Enclosure Status:  ");
	hr_print_enclosure_status(&(dp->enclosure_element_status));

	printf("\n  ESM Electronics Status:  \n");
	for (i = 0; i < HR_NR_ESM_CONTROLLERS; i++) {
		printf("      %s:  ", esm_names[i]);
		print_esm_status(&(dp->esm_status[i]));
	}

	if (cmd_opts.verbose) {
		printf("\n\nRaw diagnostic page:\n");
		print_raw_data(stdout, (char *) dp,
			       sizeof(struct hr_diag_page2));
	}

	printf("\n\n");

	/*
	 * Log fault event, and turn on LEDs as appropriate.
	 * LED status reported previously may not be accurate after we
	 * do this, but the alternative is to report faults first and then
	 * read the diagnostic page a second time.  And unfortunately, the
	 * changes to LED settings don't always show up immediately in
	 * the next query of the SES.
	 */
	if (cmd_opts.serv_event) {
		rc = hr_report_faults_to_svclog(fd, vpd, dp);
		if (rc != 0)
			goto err_out;
	}

	/* -l is not supported for fake path */
	if (fd != -1 && cmd_opts.leds)
		rc = hr_turn_on_fault_leds(dp, fd);

err_out:
	free(dp);
	return (rc != 0);
}
