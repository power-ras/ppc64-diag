/*
 * Copyright (C) 2012, 2015, 2016 IBM Corporation
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

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include "encl_common.h"
#include "encl_util.h"
#include "diag_encl.h"
#include "bluehawk.h"


static void
bh_print_drive_status(struct disk_status *s)
{
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_NOT_INSTALLED, ES_EOL
	};

	return print_drive_status(s, valid_codes);
}

static void
bh_print_enclosure_status(struct enclosure_status *s)
{
	/* Note: Deviation from spec V0.7
	 *	 Spec author says below are valid state
	 */
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_EOL
	};

	return print_enclosure_status(s, valid_codes);
}

static void
bh_print_fan_status(struct fan_status *s)
{
	const char *speed[] = {
		"Fan at lowest speed",
		"Fan at 1-16% of highest speed",
		"Fan at 17-33% of highest speed",
		"Fan at 34-49% of highest speed",
		"Fan at 50-66% of highest speed",
		"Fan at 67-83% of highest speed",
		"Fan at 84-99% of highest speed",
		"Fan at highest speed"
	};

	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_NOT_INSTALLED,
		ES_UNKNOWN, ES_EOL
	};

	return print_fan_status(s, valid_codes, speed);
}

static void
bh_print_sas_connector_status(struct sas_connector_status *s)
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_NONCRITICAL, ES_NOT_INSTALLED, ES_EOL
	};

	printf("%s", status_string(sc, valid_codes));

	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	printf("\n");
}

static void
bh_print_scc_controller_status(struct scc_controller_element_status *s)
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NOT_INSTALLED, ES_NOT_AVAILABLE, ES_EOL
	};

	printf("%s", status_string(sc, valid_codes));

	if (s->report)
		printf(" | REPORT");
	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	printf("\n");
}

static void
bh_print_midplane_status(struct midplane_status *s)
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_EOL
	};

	printf("%s", status_string(sc, valid_codes));

	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	printf("\n");
}


/* Create a callout for power supply i (i = 0 or 1). */
static int
bh_create_ps_callout(struct sl_callout **callouts, char *location,
						unsigned int i, int fd)
{
	char fru_number[FRU_NUMBER_LEN + 1];
	char serial_number[SERIAL_NUMBER_LEN + 1];
	int rc;
	struct power_supply_descriptor *ps_vpd[2];
	struct element_descriptor_page *edp;

	if (fd < 0) {
		add_location_callout(callouts, location);
		return 0;
	}

	edp = calloc(1, sizeof(struct element_descriptor_page));
	if (!edp) {
		fprintf(stderr, "Failed to allocate memory to "
			"hold page containing VPD for PS (pg 07h).\n");
		return 1;
	}

	rc = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 7, edp,
				 sizeof(struct element_descriptor_page));
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

static int
bh_report_faults_to_svclog(struct dev_vpd *vpd,
			struct bluehawk_diag_page2 *dp, int fd)
{
	char location[LOCATION_LENGTH], *loc_suffix;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	char srn[SRN_SIZE];
	unsigned int i;
	int sev, loc_suffix_size;
	char run_diag_encl[] = "  Run diag_encl for more detailed status,"
		" and refer to the system service documentation for guidance.";
	char ref_svc_doc[] =
		"  Refer to the system service documentation for guidance.";
	struct sl_callout *callouts;
	const char *left_right[] = { "left", "right" };
	struct bluehawk_diag_page2 *prev_dp = NULL;	/* for -c */

	strncpy(location, vpd->location, LOCATION_LENGTH - 1);
	location[LOCATION_LENGTH - 1] = '\0';
	loc_suffix_size = LOCATION_LENGTH - strlen(location);
	loc_suffix = location + strlen(location);

	if (cmd_opts.cmp_prev) {
		prev_dp = calloc(1, sizeof(struct bluehawk_diag_page2));
		if (!prev_dp) {
			fprintf(stderr, "Failed to allocate memory to hold "
				"prev. status diagnostics page 02 results.\n");
			return 1;
		}

		if (read_page2_from_file(cmd_opts.prev_path, false, prev_dp,
					 sizeof(struct bluehawk_diag_page2)) != 0) {
			free(prev_dp);
			prev_dp = NULL;
		}
	}

	/* disk drives */
	for (i = 0; i < NR_DISKS_PER_BLUEHAWK; i++) {
		sev = svclog_element_status(&(dp->disk_status[i].byte0),
					(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in RAID enclosure disk %u.%s",
			 crit, i + 1, run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-D%u", i+1);
		callouts = NULL;
		/* VPD for disk drives is not available from the SES. */
		add_location_callout(&callouts, location);
		servevent("none", sev, description, vpd, callouts);
	}

	/* power supplies */
	for (i = 0; i < 2; i++) {
		sev = svclog_element_status(&(dp->ps_status[i].byte0),
					(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in %s power supply in RAID enclosure.%s",
			 crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-E%u", i+1);
		build_srn(srn, SRN_RC_CRIT_PS);
		callouts = NULL;
		if (bh_create_ps_callout(&callouts, location, i, fd))
			goto err_out;
		servevent(srn, sev, description, vpd, callouts);
	}

	/* voltage sensors */
	for (i = 0; i < 2; i++) {
		sev = svclog_composite_status(&(dp->voltage_sensor_sets[i]),
			(char *) dp, (char *) prev_dp, 2, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault associated with %s power supply in RAID "
			 "enclosure: voltage sensor(s) reporting voltage(s) "
			 "out of range.%s", crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-E%u", i+1);
		build_srn(srn, SRN_RC_VOLTAGE_THRESHOLD);
		callouts = NULL;
		if (bh_create_ps_callout(&callouts, location, i, fd))
			goto err_out;
		servevent(srn, sev, description, vpd, callouts);
	}

	/* power-supply fans -- lump with power supplies, not fan assemblies */
	for (i = 0; i < 2; i++) {
		sev = svclog_element_status(
			&(dp->fan_sets[i].power_supply.byte0),
			(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in fan for %s power supply in RAID "
			 "enclosure.%s", crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-E%u", i+1);
		build_srn(srn, SRN_RC_CRIT_PS);
		callouts = NULL;
		if (bh_create_ps_callout(&callouts, location, i, fd))
			goto err_out;
		servevent(srn, sev, description, vpd, callouts);
	}

	/* fan assemblies */
	for (i = 0; i < 2; i++) {
		/* 4 fans for each fan assembly */
		sev = svclog_composite_status(&(dp->fan_sets[i].fan_element),
				(char *) dp, (char *) prev_dp, 4, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in %s fan assembly in RAID enclosure.%s",
			 crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-C%u-A1", i+1);
		build_srn(srn, SRN_RC_CRIT_FAN);
		callouts = NULL;
		/* VPD for fan assemblies is not available from the SES. */
		add_location_callout(&callouts, location);
		servevent(srn, sev, description, vpd, callouts);
	}

	/* power-supply temperature sensors -- lump with power supplies */
	for (i = 0; i < 2; i++) {
		/* 2 sensors for each power supply */
		sev = svclog_composite_status(
			&(dp->temp_sensor_sets[i].power_supply),
			(char *) dp, (char *) prev_dp, 2, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault associated with %s power supply in RAID "
			 "enclosure: temperature sensor(s) reporting "
			 "temperature(s) out of range.%s",
			 crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix, loc_suffix_size, "-P1-E%u", i+1);
		build_srn(srn, SRN_RC_PS_TEMP_THRESHOLD);
		callouts = NULL;
		if (bh_create_ps_callout(&callouts, location, i, fd))
			goto err_out;
		servevent(srn, sev, description, vpd, callouts);
	}

	/* temp sensors, except for those associated with power supplies */
	for (i = 0; i < 2; i++) {
		/* 5 sensors: croc, ppc, expander, 2*ambient */
		sev = svclog_composite_status(&(dp->temp_sensor_sets[i]),
			(char *) dp, (char *) prev_dp, 5, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault associated with %s side of RAID "
			 "enclosure: temperature sensor(s) reporting "
			 "temperature(s) out of range.%s",
			 crit, left_right[i], run_diag_encl);
		/* Not the power supply, so assume the warhawk. */
		snprintf(loc_suffix, loc_suffix_size, "-P1-C%u", i+1);
		build_srn(srn, SRN_RC_TEMP_THRESHOLD);
		callouts = NULL;
		create_esm_callout(&callouts, location, i, fd);
		servevent(srn, sev, description, vpd, callouts);
	}

	/* ERM/ESM electronics */
	for (i = 0; i < 2; i++) {
		sev = svclog_element_status(&(dp->esm_status[i].byte0),
					(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s electronics fault in %s Enclosure RAID Module.%s",
			 crit, left_right[i], ref_svc_doc);
		snprintf(loc_suffix, loc_suffix_size, "-P1-C%u", i+1);
		build_srn(srn, SRN_RC_CRIT_ESM);
		callouts = NULL;
		create_esm_callout(&callouts, location, i, fd);
		servevent(srn, sev, description, vpd, callouts);
	}

	/* SAS connectors */
	for (i = 0; i < 4; i++) {
		unsigned int t = i%2 + 1, lr = i/2;
		sev = svclog_element_status(
				&(dp->sas_connector_status[i].byte0),
				(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in SAS connector T%u of %s RAID Enclosure"
			 " Module.%s", crit, t, left_right[lr], ref_svc_doc);
		snprintf(loc_suffix, loc_suffix_size, "-P1-C%u-T%u", lr+1, t);
		callouts = NULL;
		/* No VPD for SAS connectors in the SES. */
		add_location_callout(&callouts, location);
		servevent("none", sev, description, vpd, callouts);
	}

	/* PCIe controllers */
	for (i = 0; i < 2; i++) {
		sev = svclog_element_status(
			&(dp->scc_controller_status[i].byte0),
			(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in PCIe controller for %s RAID Enclosure "
			 "Module.%s", crit, left_right[i], ref_svc_doc);
		snprintf(loc_suffix, loc_suffix_size, "-P1-C%u-T3", i+1);
		callouts = NULL;
		/* No VPD for PCIe controllers in the SES. */
		add_location_callout(&callouts, location);
		servevent("none", sev, description, vpd, callouts);
	}

	/* midplane */
	sev = svclog_element_status(&(dp->midplane_element_status.byte0),
				(char *) dp, (char *) prev_dp, crit);
	if (sev != 0) {
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in midplane of RAID enclosure.%s",
			 crit, ref_svc_doc);
		strncpy(loc_suffix, "-P1", loc_suffix_size - 1);
		loc_suffix[loc_suffix_size - 1] = '\0';
		callouts = NULL;
		create_midplane_callout(&callouts, location, fd);
		servevent("none", sev, description, vpd, callouts);
	}

	if (prev_dp)
		free(prev_dp);

	return write_page2_to_file(cmd_opts.prev_path, dp,
				   sizeof(struct bluehawk_diag_page2));

err_out:
	if (prev_dp)
		free(prev_dp);
	return 1;
}


static int
bh_turn_on_fault_leds(struct bluehawk_diag_page2 *dp, int fd)
{
	int i;
	int poked_leds = 0;
	struct bluehawk_ctrl_page2 *ctrl_page;

	ctrl_page = calloc(1, sizeof(struct bluehawk_ctrl_page2));
	if (!ctrl_page) {
		fprintf(stderr, "Failed to allocate memory to hold "
				"control diagnostics page 02.\n");
		return 1;
	}

	/* disk drives */
	for (i = 0; i < NR_DISKS_PER_BLUEHAWK; i++)
		FAULT_LED(poked_leds, dp, ctrl_page,
			  disk_ctrl[i], disk_status[i]);

	/* power supplies */
	for (i = 0; i < 2; i++)
		FAULT_LED(poked_leds, dp, ctrl_page, ps_ctrl[i], ps_status[i]);

	/* No LEDs for voltage sensors */

	/* fan assemblies */
	for (i = 0; i < 2; i++) {
		enum element_status_code sc =
				composite_status(&(dp->fan_sets[i]), 5);
		if (sc != ES_OK && sc != ES_NOT_INSTALLED)
			FAULT_LED(poked_leds, dp, ctrl_page,
				  fan_sets[i].fan_element[0],
				  fan_sets[i].fan_element[0]);
	}

	/* No LEDs for temperature sensors */

	/* ERM/ESM electronics */
	for (i = 0; i < 2; i++)
		FAULT_LED(poked_leds, dp, ctrl_page,
			  esm_ctrl[i], esm_status[i]);

	/* SAS connectors */
	for (i = 0; i < 4; i++)
		FAULT_LED(poked_leds, dp, ctrl_page,
			  sas_connector_ctrl[i], sas_connector_status[i]);

	/* PCIe controllers */
	for (i = 0; i < 2; i++)
		FAULT_LED(poked_leds, dp, ctrl_page,
			  scc_controller_ctrl[i], scc_controller_status[i]);

	/* midplane */
	FAULT_LED(poked_leds, dp, ctrl_page,
		  midplane_element_ctrl, midplane_element_status);

	if (poked_leds) {
		int result;

		ctrl_page->page_code = 2;
		ctrl_page->page_length = sizeof(struct bluehawk_ctrl_page2) - 4;
		/* Convert host byte order to network byte order */
		ctrl_page->page_length = htons(ctrl_page->page_length);
		ctrl_page->generation_code = 0;
		result = do_ses_cmd(fd, SEND_DIAGNOSTIC, 0, 0x10, 6,
				SG_DXFER_TO_DEV, ctrl_page,
				sizeof(struct bluehawk_ctrl_page2));
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
diag_bluehawk(int fd, struct dev_vpd *vpd)
{
	int i;
	static const char *power_supply_names[] = {
		"PS0 (Left)",
		"PS1 (Right)"
	};
	static const char *fan_set_names[] = {
		"Left Fan Assembly",
		"Right Fan Assembly",
	};
	static const char *temp_sensor_set_names[] = { "Left", "Right" };
	static const char *esm_names[] = { "Left", "Right" };
	static const char *sas_connector_names[] = {
		"Left - T1",
		"Left - T2",
		"Right - T1",
		"Right - T2"
	};
	static const char *scc_controller_names[] = { "Left", "Right" };

	int rc;
	struct bluehawk_diag_page2 *dp;

	dp = calloc(1, sizeof(struct bluehawk_diag_page2));
	if (!dp) {
		fprintf(stderr, "Failed to allocate memory to hold "
			"current status diagnostics page 02 results.\n");
		return 1;
	}

	if (cmd_opts.fake_path) {
		rc = read_page2_from_file(cmd_opts.fake_path, true,
					  dp, sizeof(struct bluehawk_diag_page2));
		fd = -1;
	} else
		rc = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 2,
			(void *)dp, (int) sizeof(struct bluehawk_diag_page2));
	if (rc != 0) {
		fprintf(stderr, "Failed to read SES diagnostic page; "
				"cannot report status.\n");
		goto err_out;
	}

	printf("  Overall Status:    ");
	if (dp->crit) {
		printf("CRITICAL_FAULT");
		if (dp->non_crit)
			printf(" | NON_CRITICAL_FAULT");
	} else if (dp->non_crit)
		printf("NON_CRITICAL_FAULT");
	else
		printf("ok");

	printf("\n\n  Drive Status\n");
	for (i = 0; i < NR_DISKS_PER_BLUEHAWK; i++) {
		struct disk_status *ds = &(dp->disk_status[i]);
		printf("    Disk %02d (Slot %02d): ", i+1,
				ds->byte1.element_status.slot_address);
		bh_print_drive_status(ds);
	}

	printf("\n  Power Supply Status\n");
	for (i = 0; i < 2; i++) {
		printf("    %s:  ", power_supply_names[i]);
		print_power_supply_status(&(dp->ps_status[i]));
		printf("      12V:  ");
		print_voltage_sensor_status(
				&(dp->voltage_sensor_sets[i].sensor_12V));
		printf("      3.3VA:  ");
		print_voltage_sensor_status(
				&(dp->voltage_sensor_sets[i].sensor_3_3VA));
	}

	printf("\n  Fan Status\n");
	for (i = 0; i < 2; i++) {
		int j;
		printf("    %s:\n", fan_set_names[i]);
		printf("      Power Supply:  ");
		bh_print_fan_status(&(dp->fan_sets[i].power_supply));
		for (j = 0; j < 4; j++) {
			printf("      Fan Element %d:  ", j);
			bh_print_fan_status(&(dp->fan_sets[i].fan_element[j]));
		}
	}

	printf("\n  Temperature Sensors\n");
	for (i = 0; i < 2; i++) {
		int j;
		struct temperature_sensor_set *tss = &(dp->temp_sensor_sets[i]);
		printf("    %s:\n", temp_sensor_set_names[i]);
		printf("      CRoC:  ");
		print_temp_sensor_status(&tss->croc);
		printf("      PPC:  ");
		print_temp_sensor_status(&tss->ppc);
		printf("      Expander:  ");
		print_temp_sensor_status(&tss->expander);
		for (j = 0; j < 2; j++) {
			printf("      Ambient %d:  ", j);
			print_temp_sensor_status(&tss->ambient[j]);
		}
		for (j = 0; j < 2; j++) {
			printf("      Power Supply %d:  ", j);
			print_temp_sensor_status(&tss->power_supply[j]);
		}
	}

	printf("\n  Enclosure Status:  ");
	bh_print_enclosure_status(&(dp->enclosure_element_status));

	printf("\n  ERM Electronics Status\n");
	for (i = 0; i < 2; i++) {
		printf("    %s:  ", esm_names[i]);
		print_esm_status(&(dp->esm_status[i]));
	}

	printf("\n  SAS Connector Status\n");
	for (i = 0; i < 4; i++) {
		printf("    %s:  ", sas_connector_names[i]);
		bh_print_sas_connector_status(&(dp->sas_connector_status[i]));
	}

	printf("\n  PCIe Controller Status\n");
	for (i = 0; i < 2; i++) {
		printf("    %s:  ", scc_controller_names[i]);
		bh_print_scc_controller_status(&(dp->scc_controller_status[i]));
	}

	printf("\n  Midplane Status:  ");
	bh_print_midplane_status(&(dp->midplane_element_status));

	if (cmd_opts.verbose) {
		printf("\n\nRaw diagnostic page:\n");
		print_raw_data(stdout, (char *) dp,
				sizeof(struct bluehawk_diag_page2));
	}

	printf("\n\n");

	/*
	 * Report faults to servicelog, and turn on LEDs as appropriate.
	 * LED status reported previously may not be accurate after we
	 * do this, but the alternative is to report faults first and then
	 * read the diagnostic page a second time.  And unfortunately, the
	 * changes to LED settings don't always show up immediately in
	 * the next query of the SES.
	 */
	if (cmd_opts.serv_event) {
		rc = bh_report_faults_to_svclog(vpd, dp, fd);
		if (rc != 0)
			goto err_out;
	}

	/* -l is not supported for fake path */
	if (fd != -1 && cmd_opts.leds)
		rc = bh_turn_on_fault_leds(dp, fd);

err_out:
	free(dp);
	return (rc != 0);
}
