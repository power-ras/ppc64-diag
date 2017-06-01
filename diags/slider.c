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

#include <arpa/inet.h>
#include <errno.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag_encl.h"
#include "encl_common.h"
#include "encl_util.h"
#include "slider.h"

/* Global definition, as it is used by multiple caller */
static const char * const sas_connector_names[] = {
	"Upstream",
	"Downstream",
	"Uninstalled"
};
static const char * const input_power_names[] = {
	"Average Input Power",
	"Peak Input Power"
};
static const char * const left_right[] = {
	"Left",
	"Right"
};
static const char * const psu_temp_sensor_names[] = {
	"Ambient",
	"Hot-Spot",
	"Primary"
};
static const char * const esc_temp_sensor_names[] = {
	"Inlet",
	"Outlet"
};

char run_diag_encl[] = "  Run diag_encl for more detailed status,"
			" and refer to the system service documen"
			"tation for guidance.";
char ref_svc_doc[] = "  Refer to the system service documentation"
			" for guidance.";

/* Holds FRU location code */
static char location_code[LOCATION_LENGTH];

/* Common valid status for all elements */
static enum element_status_code valid_codes[] = {
	ES_UNSUPPORTED, ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_UNRECOVERABLE,
	ES_NOT_INSTALLED, ES_UNKNOWN, ES_NOT_AVAILABLE, ES_NO_ACCESS_ALLOWED,
	ES_EOL
};

enum slider_variant slider_variant_flag;

/* Slider variant page size/number of disk */
uint16_t slider_v_diag_page2_size;
uint16_t slider_v_ctrl_page2_size;
uint8_t slider_v_nr_disk;

/* Create a callout for power supply i (i = 0 or 1). */
static int slider_create_ps_callout(struct sl_callout **callouts,
				    char *location, unsigned int i, int fd)
{
	char fru_number[FRU_NUMBER_LEN + 1];
	char serial_number[SERIAL_NUMBER_LEN + 1];
	char *buffer;
	int rc;
	uint32_t size;
	struct slider_lff_element_descriptor_page *edpl;
	struct slider_sff_element_descriptor_page *edps;
	struct power_supply_descriptor *ps_vpd[SLIDER_NR_POWER_SUPPLY];

	if (fd < 0) {
		add_location_callout(callouts, location);
		return 0;
	}

	/* Check slider varient */
	if (slider_variant_flag == SLIDER_V_LFF)
		size = sizeof(struct slider_lff_element_descriptor_page);
	else if (slider_variant_flag == SLIDER_V_SFF)
		size = sizeof(struct slider_sff_element_descriptor_page);
	else
		return 1;

	buffer = calloc(1, size);
	if (!buffer) {
		fprintf(stderr, "%s: Failed to allocate memory\n", __func__);
		return 1;
	}

	rc = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 7, buffer, size);
	if (rc) {
		add_location_callout(callouts, location);
		free(buffer);
		return 0;
	}

	if (slider_variant_flag == SLIDER_V_LFF) {
		edpl = (struct slider_lff_element_descriptor_page *)buffer;
		ps_vpd[0] = &(edpl->ps0_vpd);
		ps_vpd[1] = &(edpl->ps1_vpd);
	} else if (slider_variant_flag == SLIDER_V_SFF) {
		edps = (struct slider_sff_element_descriptor_page *)buffer;
		ps_vpd[0] = &(edps->ps0_vpd);
		ps_vpd[1] = &(edps->ps1_vpd);
	}
	strzcpy(fru_number, ps_vpd[i]->fru_number, FRU_NUMBER_LEN);
	strzcpy(serial_number, ps_vpd[i]->serial_number, SERIAL_NUMBER_LEN);
	add_callout(callouts, 'M', 0, NULL,
		    location, fru_number, serial_number, NULL);

	return 0;
}

/* Used to get location code with location_suffix_code */
static void get_location_code(struct dev_vpd *vpd, char *loc_suffix_code)
{
	static char *loc_suffix = NULL;
	static int loc_suffix_size;

	if (location_code[0] == '\0') {
		strncpy(location_code, vpd->location, LOCATION_LENGTH - 1);
		location_code[LOCATION_LENGTH - 1] = '\0';
		loc_suffix_size = LOCATION_LENGTH - strlen(location_code);
		loc_suffix = location_code + strlen(location_code);
	}

	if (loc_suffix_code)
		snprintf(loc_suffix, loc_suffix_size, "%s", loc_suffix_code);
	else
		*loc_suffix = '\0';
}

/* Report slider disk fault to svclog */
static void report_slider_disk_fault_to_svclog(void *dp, void *prev_dp,
					       struct dev_vpd *vpd)
{
	char loc_suffix_code[LOCATION_LENGTH];
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	int i, sev;
	struct sl_callout *callouts;
	struct slider_disk_status *disk_status =
		(struct slider_disk_status *)(dp + element_offset(disk_status));

	for (i = 0; i < slider_v_nr_disk; i++) {
		if (disk_status[i].byte0.status == ES_NO_ACCESS_ALLOWED)
			continue;
		sev = svclog_element_status(&(disk_status[i].byte0),
				(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;

		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in enclosure disk %u.%s",
			 crit, i + 1, run_diag_encl);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1-D%u", i+1);
		get_location_code(vpd, loc_suffix_code);
		callouts = NULL;
		/* VPD for disk drives is not available from the SES. */
		add_location_callout(&callouts, location_code);
		servevent("none", sev, description, vpd, callouts);
	}
}

/* Report slider power supply fault to svclog */
static void report_slider_power_supply_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd, int fd)
{
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	char srn[SRN_SIZE];
	char loc_suffix_code[LOCATION_LENGTH];
	int i, sev, rc;
	struct sl_callout *callouts;
	struct slider_power_supply_status *ps_status =
		(struct slider_power_supply_status *)(dp + element_offset(ps_status));

	for (i = 0; i < SLIDER_NR_POWER_SUPPLY; i++) {
		sev = svclog_element_status(&(ps_status[i].byte0),
				(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;
		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in %s power supply in enclosure.%s",
			 crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1-E%u", i+1);
		get_location_code(vpd, loc_suffix_code);
		build_srn(srn, SRN_RC_CRIT_PS);
		callouts = NULL;
		rc = slider_create_ps_callout(&callouts, location_code, i, fd);
		if (rc == 0)
			servevent(srn, sev, description, vpd, callouts);
	}
}

/* Report slider cooling element fault to svclog */
static void report_slider_cooling_element_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd, int fd)
{
	int i, j, sev, rc;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char srn[SRN_SIZE];
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_fan_set *fan_sets =
		(struct slider_fan_set *)(dp + element_offset(fan_sets));

	for (i = 0; i < SLIDER_NR_POWER_SUPPLY; i++) {
		for (j = 0; j < SLIDER_NR_FAN_PER_POWER_SUPPLY; j++) {
			sev = svclog_element_status
				(&(fan_sets[i].fan_element[j].byte0),
				 (char *) dp, (char *) prev_dp, crit);
			if (sev == 0)
				continue;
			snprintf(description, EVENT_DESC_SIZE,
				 "%s fault in fan element %d of %s power"
				 " supply in enclosure.%s", crit, j + 1,
				 left_right[i], run_diag_encl);
			snprintf(loc_suffix_code,
				 LOCATION_LENGTH, "-P1-E%u", i+1);
			get_location_code(vpd, loc_suffix_code);
			build_srn(srn, SRN_RC_CRIT_FAN);
			callouts = NULL;
			rc = slider_create_ps_callout(&callouts,
						      location_code, i, fd);
			if (rc ==  0)
				servevent(srn, sev, description, vpd, callouts);
		}
	}
}

/* Report slider temperture sensor fault to svclog */
static void report_slider_temp_sensor_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd, int fd)
{
	int i, j, sev, rc;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char srn[SRN_SIZE];
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_temperature_sensor_set *temp_sensor_sets =
		(struct slider_temperature_sensor_set *)
		(dp + element_offset(temp_sensor_sets));

	/* Temperature sensor for enclosure */
	for (i = 0; i < SLIDER_NR_ENCLOSURE; i++) {
		sev = svclog_element_status
			(&(temp_sensor_sets->temp_enclosure.byte0),
			 (char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;

		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in Ambient air temperature sensor of"
			 " enclosure.%s", crit, run_diag_encl);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1");
		get_location_code(vpd, loc_suffix_code);
		build_srn(srn, SRN_RC_TEMP_THRESHOLD);
		callouts = NULL;
		create_esm_callout(&callouts, location_code, i, fd);
		servevent(srn, sev, description, vpd, callouts);
	}

	/* Temperature sensor for PSU */
	for (i = 0; i < SLIDER_NR_PSU; i++) {
		for (j = 0; j < SLIDER_NR_TEMP_SENSOR_PER_PSU; j++) {
			sev = svclog_element_status
				(&(temp_sensor_sets->temp_psu
				   [(i * SLIDER_NR_TEMP_SENSOR_PER_PSU) + j]
				   .byte0),
				(char *) dp, (char *) prev_dp, crit);
			if (sev == 0)
				continue;
			snprintf(description, EVENT_DESC_SIZE,
				 "%s fault in %s temp sensor of %s power"
				 " supply unit in enclosure.%s",
				 crit, psu_temp_sensor_names[j],
				 left_right[i], run_diag_encl);
			snprintf(loc_suffix_code,
				 LOCATION_LENGTH, "-P1-E%u", i+1);
			get_location_code(vpd, loc_suffix_code);
			build_srn(srn, SRN_RC_PS_TEMP_THRESHOLD);
			callouts = NULL;
			rc = slider_create_ps_callout(&callouts,
				location_code, i, fd);
			if (rc ==  0)
				servevent(srn, sev, description, vpd, callouts);
		}
	}

	/* Temperature sensor for ESC */
	for (i = 0; i < SLIDER_NR_ESC; i++) {
		for (j = 0; j < SLIDER_NR_TEMP_SENSOR_PER_ESC; j++) {
			sev = svclog_element_status
				(&(temp_sensor_sets->temp_psu
				   [(i * SLIDER_NR_TEMP_SENSOR_PER_ESC) + j]
				   .byte0),
				 (char *) dp, (char *) prev_dp, crit);
			if (sev == 0)
				continue;
			snprintf(description, EVENT_DESC_SIZE,
				 "%s fault in %s temp sensor of %s enclosure "
				 "service controller in enclosure.%s",
				 crit, esc_temp_sensor_names[j],
				 left_right[i], run_diag_encl);
			snprintf(loc_suffix_code,
				 LOCATION_LENGTH, "-P1-C%u", i+1);
			get_location_code(vpd, loc_suffix_code);
			build_srn(srn, SRN_RC_TEMP_THRESHOLD);
			callouts = NULL;
			create_esm_callout(&callouts, location_code, i, fd);
			servevent(srn, sev, description, vpd, callouts);
		}
	}
}

/* Report slider enclosure service controller fault to svclog */
static void report_slider_esc_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd, int fd)
{
	int i, sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char srn[SRN_SIZE];
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_enc_service_ctrl_status *enc_service_ctrl_element =
		(struct slider_enc_service_ctrl_status *)
		(dp + element_offset(enc_service_ctrl_element));

	for (i = 0; i < SLIDER_NR_ESC; i++) {
		sev = svclog_element_status(&(enc_service_ctrl_element[i]
				.byte0), (char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;

		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in %s enclosure service controller.%s",
			 crit, left_right[i], ref_svc_doc);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1-C%u", i+1);
		get_location_code(vpd, loc_suffix_code);
		build_srn(srn, SRN_RC_CRIT_ESM);
		callouts = NULL;
		create_esm_callout(&callouts, location_code, i, fd);
		servevent(srn, sev, description, vpd, callouts);
	}
}

/* Report slider enclosure fault to svclog */
static void report_slider_enclosure_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd)
{
	int i, sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char loc_suffix_code[LOCATION_LENGTH];
	struct enclosure_status *encl_element =
		(struct enclosure_status *)
		(dp + element_offset(encl_element));

	for (i = 0; i < SLIDER_NR_ENCLOSURE; i++) {
		sev = svclog_element_status(&(encl_element->byte0),(char *) dp,
					    (char *) prev_dp, crit);
		if (sev == 0)
			continue;

		snprintf(description, EVENT_DESC_SIZE,
			 "%s fault in enclosure.%s", crit, run_diag_encl);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1");
		get_location_code(vpd, loc_suffix_code);
		callouts = NULL;
		add_location_callout(&callouts, location_code);
		servevent("none", sev, description, vpd, callouts);
	}
}

/* Report slider voltage sensor fault to svclog */
static void report_slider_volt_sensor_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd, int fd)
{
	int i, sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char srn[SRN_SIZE];
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_voltage_sensor_set *voltage_sensor_sets =
		(struct slider_voltage_sensor_set *)(dp
			+ element_offset(voltage_sensor_sets));

	for (i = 0; i < SLIDER_NR_ESC; i++) {
		sev = svclog_composite_status(&(voltage_sensor_sets[i]),
				(char *) dp, (char *) prev_dp,
				SLIDER_NR_VOLT_SENSOR_PER_ESM, crit);
		if (sev == 0)
			continue;

		snprintf(description, EVENT_DESC_SIZE,
			"%s fault associated with %s enclosure service"
			" controller in enclosure: voltage sensor(s)"
			" reporting voltage(s) out of range.%s",
			crit, left_right[i], run_diag_encl);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1-C%u", i+1);
		get_location_code(vpd, loc_suffix_code);
		build_srn(srn, SRN_RC_VOLTAGE_THRESHOLD);
		callouts = NULL;
		create_esm_callout(&callouts, location_code, i, fd);
		servevent(srn, sev, description, vpd, callouts);
	}
}

/* Report slider sas expander fault to svclog */
static void report_slider_sas_expander_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd)
{
	int i, sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char srn[SRN_SIZE];
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_sas_expander_status *sas_expander_element =
		(struct slider_sas_expander_status *)(dp
			+ element_offset(sas_expander_element));

	/* SAS Expander */
	for (i = 0; i < SLIDER_NR_SAS_EXPANDER; i++) {
		sev = svclog_element_status(&(sas_expander_element[i].byte0),
				(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;

		snprintf(description, EVENT_DESC_SIZE,
			"%s fault in SAS expander of %s enclosure service"
			" controller.%s", crit, left_right[i], ref_svc_doc);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1-C%u", i+1);
		get_location_code(vpd, loc_suffix_code);
		build_srn(srn, SRN_RC_CRIT_ESM);
		callouts = NULL;
		add_location_callout(&callouts, location_code);
		servevent(srn, sev, description, vpd, callouts);
	}
}

/* Report slider sas connector fault to svclog */
static void report_slider_sas_connector_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd)
{
	int i, j, sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_sas_connector_status *sas_connector_status =
		(struct slider_sas_connector_status *)(dp
			+ element_offset(sas_connector_status));

	for (i = 0; i < SLIDER_NR_SAS_EXPANDER; i++) {
		for (j = 0; j < SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER ; j++) {
			sev = svclog_element_status(&(sas_connector_status
				[(i * SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER)
					+ j].byte0), (char *) dp,
					(char *) prev_dp, crit);
			if (sev == 0)
				continue;

			snprintf(description, EVENT_DESC_SIZE,
				"%s fault in %s SAS connector of %s sas"
				" expander of enclosure module.%s", crit,
				sas_connector_names[j],
				left_right[i], ref_svc_doc);
			snprintf(loc_suffix_code,
				 LOCATION_LENGTH, "-C%u-T%u", i+1, j+1);
			get_location_code(vpd, loc_suffix_code);
			callouts = NULL;
			/* No VPD for SAS connectors in the SES. */
			add_location_callout(&callouts, location_code);
			servevent("none", sev, description, vpd, callouts);
		}
	}
}

/* Report slider midplane fault to svclog */
static void report_slider_midplane_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd, int fd)
{
	int sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_midplane_status *midplane_element_status =
		(struct slider_midplane_status *)
		(dp + element_offset(midplane_element_status));

	sev = svclog_element_status(&(midplane_element_status->byte0),
				    (char *) dp, (char *) prev_dp, crit);
	if (sev != 0) {
		snprintf(description, EVENT_DESC_SIZE,
			"%s fault in midplane of enclosure.%s",
			crit, ref_svc_doc);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1");
		get_location_code(vpd, loc_suffix_code);
		callouts = NULL;
		create_midplane_callout(&callouts, location_code, fd);
		servevent("none", sev, description, vpd, callouts);
	}
}

/* Report slider phy fault to svclog */
static void report_slider_phy_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd)
{
	int i, j, sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_phy_set *phy_sets =
		(struct slider_phy_set *)(dp + element_offset(phy_sets));

	for (i = 0; i < SLIDER_NR_SAS_EXPANDER; i++) {
		for (j = 0; j < SLIDER_NR_PHY_PER_SAS_EXPANDER; j++) {
			sev = svclog_element_status
				(&(phy_sets[i].phy_element[j].byte0),
				 (char *) dp, (char *) prev_dp, crit);
			if (sev == 0)
				continue;
			snprintf(description, EVENT_DESC_SIZE,
				"%s fault in physical link[%d] of %s SAS"
				" expander.%s",	crit, j + 1, left_right[i],
				ref_svc_doc);
			snprintf(loc_suffix_code,
				 LOCATION_LENGTH, "-P1-C%u", i+1);
			get_location_code(vpd, loc_suffix_code);
			callouts = NULL;
			add_location_callout(&callouts, location_code);
			servevent("none", sev, description, vpd, callouts);
		}
	}
}

/* Report slider statesave buffer fault to svclog */
static void report_slider_statesave_buffer_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd)
{
	int i, sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_statesave_buffer_status *ssb_element =
		(struct slider_statesave_buffer_status *)
		(dp + element_offset(ssb_element));

	for (i = 0; i < SLIDER_NR_SSB; i++) {
		sev = svclog_element_status(&(ssb_element[i].byte0),
				(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;

		snprintf(description, EVENT_DESC_SIZE,
			"%s fault in statesave of %s enclosure service"
			" controller.%s", crit, left_right[i], ref_svc_doc);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1-C%u", i+1);
		get_location_code(vpd, loc_suffix_code);
		callouts = NULL;
		add_location_callout(&callouts, location_code);
		servevent("none", sev, description, vpd, callouts);
	}
}

/* Report slider cpld fault to svclog */
static void report_slider_cpld_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd)
{
	int i, sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_cpld_status *cpld_element =
		(struct slider_cpld_status *)
		(dp + element_offset(cpld_element));

	for (i = 0; i < SLIDER_NR_ESC; i++) {
		sev = svclog_element_status(&(cpld_element[i].byte0),
				(char *) dp, (char *) prev_dp, crit);
		if (sev == 0)
			continue;

		snprintf(description, EVENT_DESC_SIZE,
			"%s fault in CPLD of %s enclosure.%s",
			crit, left_right[i], ref_svc_doc);
		snprintf(loc_suffix_code, LOCATION_LENGTH, "-P1-C%u", i+1);
		get_location_code(vpd, loc_suffix_code);
		callouts = NULL;
		add_location_callout(&callouts, location_code);
		servevent("none", sev, description, vpd, callouts);
	}
}

/* Report slider input power fault to svclog */
static void report_slider_input_power_fault_to_svclog(
	void *dp, void *prev_dp, struct dev_vpd *vpd)
{
	int i, j, sev;
	char description[EVENT_DESC_SIZE], crit[ES_STATUS_STRING_MAXLEN];
	struct sl_callout *callouts;
	char srn[SRN_SIZE];
	char loc_suffix_code[LOCATION_LENGTH];
	struct slider_input_power_status *input_power_element =
		(struct slider_input_power_status *)
		(dp + element_offset(input_power_element));

	for (i = 0; i < SLIDER_NR_PSU; i++) {
		for (j = 0; j < SLIDER_NR_INPUT_POWER_PER_PSU; j++) {
			sev = svclog_element_status
				(&(input_power_element
				   [(i * SLIDER_NR_INPUT_POWER_PER_PSU) + j]
				   .byte0), (char *) dp,
				 (char *) prev_dp, crit);
			if (sev == 0)
				continue;

			snprintf(description, EVENT_DESC_SIZE,
				 "%s fault in input power(%s) of %s power"
				 " supply unit in enclosure.%s", crit,
				 input_power_names[j], left_right[i],
				 run_diag_encl);
			snprintf(loc_suffix_code,
				 LOCATION_LENGTH, "-P1-E%u", i+1);
			get_location_code(vpd, loc_suffix_code);
			build_srn(srn, SRN_RC_CRIT_PS);
			callouts = NULL;
			add_location_callout(&callouts, location_code);
			servevent(srn, sev, description, vpd, callouts);
		}
	}
}

static int slider_read_previous_page2(void **buffer)
{
	int rc;

	if (!cmd_opts.cmp_prev)
		return 0;

	*buffer = calloc(1, slider_v_diag_page2_size);
	if (!(*buffer)) {
		fprintf(stderr, "%s : Failed to allocate memory\n", __func__);
		return 1;
	}

	rc = read_page2_from_file(cmd_opts.prev_path, false,
				  *buffer, slider_v_diag_page2_size);
	if (rc != 0) {
		free(*buffer);
		*buffer = NULL;
	}

	return 0;
}

/*
 * Report slider fault to sericelog for both variant of slider by calculating
 * offset of different element
 */
static int report_slider_fault_to_svclog(void *dp, struct dev_vpd *vpd, int fd)
{
	int rc;
	void *prev_dp = NULL;

	rc = slider_read_previous_page2((void *)&prev_dp);
	if (rc)
		return 1;

	/* report disk fault */
	report_slider_disk_fault_to_svclog(dp, prev_dp, vpd);

	/* report power supply fault */
	report_slider_power_supply_fault_to_svclog(dp, prev_dp, vpd, fd);

	/* report cooling element fault */
	report_slider_cooling_element_fault_to_svclog(dp, prev_dp, vpd, fd);

	/* report temp sensor fault */
	report_slider_temp_sensor_fault_to_svclog(dp, prev_dp, vpd, fd);

	/* report esc fault */
	report_slider_esc_fault_to_svclog(dp, prev_dp, vpd, fd);

	/* report enclosure fault */
	report_slider_enclosure_fault_to_svclog(dp, prev_dp, vpd);

	/* report voltage sensor fault */
	report_slider_volt_sensor_fault_to_svclog(dp, prev_dp, vpd, fd);

	/* report sas expander fault */
	report_slider_sas_expander_fault_to_svclog(dp, prev_dp, vpd);

	/* report sas connector fault */
	report_slider_sas_connector_fault_to_svclog(dp, prev_dp, vpd);

	/* report midplane fault */
	report_slider_midplane_fault_to_svclog(dp, prev_dp, vpd, fd);

	/* report phy fault */
	report_slider_phy_fault_to_svclog(dp, prev_dp, vpd);

	/* report statesave buffer fault */
	report_slider_statesave_buffer_fault_to_svclog(dp, prev_dp, vpd);

	/* report cpld fault */
	report_slider_cpld_fault_to_svclog(dp, prev_dp, vpd);

	/* report input power fault */
	report_slider_input_power_fault_to_svclog(dp, prev_dp, vpd);

	if (prev_dp)
		free(prev_dp);

	return write_page2_to_file(cmd_opts.prev_path, dp,
				   slider_v_diag_page2_size);
}

#define SLIDER_FAULT_LED(poked_leds, dp, ctrl_page, ctrl_element, \
						status_element) \
	do { \
		if (slider_variant_flag == SLIDER_V_LFF) { \
			struct slider_lff_diag_page2 *d \
			= (struct slider_lff_diag_page2 *)dp; \
			struct slider_lff_ctrl_page2 *c \
			= (struct slider_lff_ctrl_page2 *)ctrl_page; \
			FAULT_LED(poked_leds, d, c, ctrl_element, \
					status_element);\
		} else { \
			struct slider_sff_diag_page2 *d \
			= (struct slider_sff_diag_page2 *)dp; \
			struct slider_sff_ctrl_page2 *c \
			= (struct slider_sff_ctrl_page2 *)ctrl_page; \
			FAULT_LED(poked_leds, d, c, ctrl_element, \
					status_element); \
		} \
	} while (0)

/* Turn on led in case of failure of any element of enclosure */
static int slider_turn_on_fault_leds(void *dp, int fd)
{
	int i, j, result;
	void *ctrl_page;
	int poked_leds = 0;

	ctrl_page = calloc(1, slider_v_ctrl_page2_size);
	if (!ctrl_page) {
		fprintf(stderr, "Failed to allocate memory to hold "
				"control diagnostics page 02.\n");
		return 1;
	}

	/* Disk */
	for (i = 0; i < slider_v_nr_disk; i++) {
		SLIDER_FAULT_LED(poked_leds, dp, ctrl_page,
				 disk_ctrl[i], disk_status[i]);
	}

	/* Power supply */
	for (i = 0; i < SLIDER_NR_POWER_SUPPLY; i++) {
		SLIDER_FAULT_LED(poked_leds, dp, ctrl_page,
				 ps_ctrl[i], ps_status[i]);
	}

	/* Cooling element */
	for (i = 0; i < SLIDER_NR_POWER_SUPPLY; i++) {
		for (j = 0; j < SLIDER_NR_FAN_PER_POWER_SUPPLY; j++) {
			SLIDER_FAULT_LED(poked_leds, dp, ctrl_page,
			fan_sets[i].fan_element[j], fan_sets[i].fan_element[j]);
		}
	}

	/* ERM/ESM electronics */
	for (i = 0; i < SLIDER_NR_ESC; i++) {
		SLIDER_FAULT_LED(poked_leds, dp, ctrl_page,
				 enc_service_ctrl_element[i],
				 enc_service_ctrl_element[i]);
	}

	/* SAS Connector*/
	for (i = 0; i < SLIDER_NR_SAS_CONNECTOR; i++) {
		SLIDER_FAULT_LED(poked_leds, dp, ctrl_page,
			sas_connector_ctrl[i], sas_connector_status[i]);
	}

	/* Enclosure */
	SLIDER_FAULT_LED(poked_leds, dp, ctrl_page, encl_element, encl_element);

	if (poked_leds) {
		SLIDER_ASSIGN_CTRL_PAGE(ctrl_page);
		result = do_ses_cmd(fd, SEND_DIAGNOSTIC, 0, 0x10, 6,
				SG_DXFER_TO_DEV, ctrl_page,
				slider_v_ctrl_page2_size);
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

/* Print slider temperature sensor status */
static void print_slider_temp_sensor_status(
	struct temperature_sensor_status *s)
{
	enum element_status_code sc =
		(enum element_status_code) s->byte0.status;

	printf("%s", status_string(sc, valid_codes));

	if (s->ot_failure)
		printf(" | OVER_TEMPERATURE_FAILURE");
	if (s->ot_warning)
		printf(" | OVER_TEMPERATURE_WARNING");
	if (s->ut_failure)
		printf(" | UNDER_TEMPERATURE_FAILURE");
	if (s->ut_warning)
		printf(" | UNDER_TEMPERATURE_WARNING");
	if (cmd_opts.verbose)
		/* between -19 and +235 degrees Celsius */
		printf(" | TEMPERATURE = %dC", s->temperature - 20);
	printf("\n");
}

/* Print slider voltage sensor status */
static void print_slider_voltage_sensor_status(
	struct voltage_sensor_status *s)
{
	enum element_status_code sc =
		(enum element_status_code) s->byte0.status;

	printf("%s", status_string(sc, valid_codes));

	if (s->warn_over)
		printf(" | NON_CRITICAL_OVER_VOLTAGE");
	if (s->warn_under)
		printf(" | NON_CRITICAL_UNDER_VOLTAGE");
	if (s->crit_over)
		printf(" | CRITICAL_OVER_VOLTAGE");
	if (s->crit_under)
		printf(" | CRITICAL_UNDER_VOLTAGE");
	if (cmd_opts.verbose)
		/* between +327.67 to -327.67 */
		printf(" | VOLTAGE = %.2f volts", ntohs(s->voltage) / 100.0);
	printf("\n");
}

static void print_slider_drive_status(struct slider_disk_status *s)
{
	enum element_status_code sc =
		(enum element_status_code) s->byte0.status;

	printf("%-19s", status_string(sc, valid_codes));
	if (sc == ES_NOT_INSTALLED) {
		printf("\n");
		return;
	}

	if (s->hot_swap)
		printf(" | Hotswap enabled");
	if (s->app_client_bypassed_a)
		printf(" | APP_CLIENT_BYPASSED_A");
	if (s->app_client_bypassed_b)
		printf(" | APP_CLIENT_BYPASSED_B");
	if (s->enclosure_bypassed_a)
		printf(" | ENCLOSURE_BYPASSED_A");
	if (s->enclosure_bypassed_b)
		printf(" | ENCLOSURE_BYPASSED_B");
	if (s->device_bypassed_a)
		printf(" | DEVICE_BYPASSED_A");
	if (s->device_bypassed_b)
		printf(" | DEVICE_BYPASSED_B");
	if (s->bypassed_a)
		printf(" | BYPASSED_A");
	if (s->bypassed_b)
		printf(" | BYPASSED_B");

	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	printf("\n");
}

/* Print slider disk status */
static void print_slider_disk_status(struct slider_disk_status *disk_status)
{
	int i;
	struct slider_disk_status *ds;

	printf("\n\n  Drive Status\n");

	for (i = 0; i < slider_v_nr_disk; i++) {
		ds = &(disk_status[i]);
		/* Print only if disk element access bit is set */
		if (ds->byte0.status == ES_NO_ACCESS_ALLOWED)
			continue;

		printf("    Disk %02d (Slot %02d): ", i+1, ds->slot_address);
		print_slider_drive_status(ds);
	}
}

static void slider_print_power_supply_status(struct slider_power_supply_status *s)
{
	enum element_status_code sc =
		(enum element_status_code) s->byte0.status;

	printf("%s", status_string(sc, valid_codes));

	if (s->cycled)
		printf(" | CYCLED");
	if (s->dc_fail)
		printf(" | DC_FAIL");
	if (s->dc_over_voltage)
		printf(" | DC_OVER_VOLTAGE");
	if (s->dc_under_voltage)
		printf(" | DC_UNDER_VOLTAGE");
	if (s->dc_over_current)
		printf(" | DC_OVER_CURRENT");
	if (s->ac_fail)
		printf(" | AC_FAIL");
	if (s->temp_warn)
		printf(" | TEMPERATURE_WARNING");
	if (s->ovrtmp_fail)
		printf(" | OVER_TEMPERATURE_FAILURE");

	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	printf("\n");
}

/* Print slider power supply status */
static void print_slider_power_supply_status(
	struct slider_power_supply_status *ps_status,
	struct slider_voltage_sensor_set *voltage_sensor_sets)
{
	int i;

	printf("\n  Power Supply Status\n");
	for (i = 0; i < SLIDER_NR_POWER_SUPPLY; i++) {
		printf("    PS%d (%-5s):  ", i, left_right[i]);
		slider_print_power_supply_status(&(ps_status[i]));
		printf("      3.3V:  ");
		print_slider_voltage_sensor_status(
				&(voltage_sensor_sets[i].sensor_3V3));
		printf("      1V0 :  ");
		print_slider_voltage_sensor_status(
				&(voltage_sensor_sets[i].sensor_1V0));
		printf("      1V8 :  ");
		print_slider_voltage_sensor_status(
				&(voltage_sensor_sets[i].sensor_1V8));
		printf("      0V92:  ");
		print_slider_voltage_sensor_status(
				&(voltage_sensor_sets[i].sensor_0V92));
	}
}

/* Print slider enclosure status */
static void print_slider_enclosure_status(struct enclosure_status *s)
{
	printf("\n  Enclosure Status:  ");
	return print_enclosure_status(s, valid_codes);
}

/* Print slider midplane status */
static void print_slider_midplane_status(struct slider_midplane_status *s)
{
	enum element_status_code sc =
		(enum element_status_code) s->byte0.status;

	printf("\n  Midplane Status:  ");
	printf("%s", status_string(sc, valid_codes));

	if (s->vpd1_read_fail1)
		printf(" | Last read on VPD1 via ESC1 was failed");
	if (s->vpd1_read_fail2)
		printf(" | Last read on VPD1 via ESC2 failed");
	if (s->vpd2_read_fail1)
		printf(" | Last read on VPD2 via ESC1 was failed");
	if (s->vpd2_read_fail2)
		printf(" | Last read on VPD2 via ESC2 failed");
	if (s->vpd_mirror_mismatch)
		printf(" | Mismatch between copies of VPD");

	printf("\n");
}

/* Print slider fan status */
static void print_slider_fan_status(struct slider_fan_set *fan_sets)
{
	int i, j;
	struct fan_status *s;
	enum element_status_code sc;

	printf("\n  Fan Status\n");

	for (i = 0; i < SLIDER_NR_POWER_SUPPLY; i++) {
		printf("    %s %s\n", left_right[i], "power supply fans");
		for (j = 0; j < SLIDER_NR_FAN_PER_POWER_SUPPLY; j++) {
			printf("      Fan Element %d:  ", j + 1);
			s = &(fan_sets[i].fan_element[j]);
			sc = (enum element_status_code) s->byte0.status;
			printf("%s", status_string(sc, valid_codes));
			CHK_IDENT_LED(s);
			CHK_FAULT_LED(s);

			if (cmd_opts.verbose)
				printf(" | speed :%d rpm",
				       (((s->fan_speed_msb) << 8) |
					(s->fan_speed_lsb)) *
				       SLIDER_FAN_SPEED_FACTOR);
			printf("\n");
		}
	}
}

/* Print slider temperature sensor status */
static void print_slider_temp_sensor_sets(
	struct slider_temperature_sensor_set *tss)
{
	int i, j;

	printf("\n  Temperature Sensors\n");

	printf("    Enclosure Ambient air:  ");
	print_slider_temp_sensor_status(&tss->temp_enclosure);

	for (i = 0; i < SLIDER_NR_PSU; i++) {
		printf("    PSU%d:\n", i + 1);
		for (j = 0; j < SLIDER_NR_TEMP_SENSOR_PER_PSU; j++) {
			printf("      Temp sensor element %d (%-8s):  ",
			       j + 1, psu_temp_sensor_names[j]);
			print_slider_temp_sensor_status(&tss->temp_psu
				[(i * SLIDER_NR_TEMP_SENSOR_PER_PSU) + j]);
		}
	}

	for (i = 0; i < SLIDER_NR_ESC; i++) {
		printf("    ENCL Service Cntrl %d:\n", i + 1);
		for (j = 0; j < SLIDER_NR_TEMP_SENSOR_PER_ESC; j++) {
			printf("      Temp sensor Element %d (%-8s):  ",
			       j + 1, esc_temp_sensor_names[j]);
			print_slider_temp_sensor_status(&tss->temp_esc
				[(i * SLIDER_NR_TEMP_SENSOR_PER_ESC) + j]);
		}
	}
}

/* Print slider ESC reset reason */
static void slider_print_esc_reset_reason(uint8_t reset_reason)
{
	switch (reset_reason) {
	case 0:
		printf(" | First power on");
		break;
	case 1:
		printf(" | Application (host) requested");
		break;
	case 2:
		printf(" | Self initiated");
		break;
	case 3:
		printf(" | Firmware download");
		break;
	case 4:
		printf(" | Exception");
		break;
	case 5:
		printf(" | Error");
		break;
	case 6:
		printf(" | Unknown");
		break;
	case 7:
		printf(" | Partner request");
		break;
	case 8:
		printf(" | Watchdog");
		break;
	case 9:
		printf(" | Address load exception");
		break;
	case 10:
		printf(" | Address store exception");
		break;
	case 11:
		printf(" | Instruction fetch exception");
		break;
	case 12:
		printf(" | Data load or store exception");
		break;
	case 13:
		printf(" | Illegal instruction exception");
		break;
	case 14:
		printf(" | System reset");
		break;
	}
}

/* Print slider enclosure service controller status */
static void print_esc_status(
	struct slider_enc_service_ctrl_status *enc_service_ctrl_element)
{
	int i;
	struct slider_enc_service_ctrl_status *s;
	enum element_status_code sc;

	printf("\n  Enclosure service controller status\n");

	for (i = 0; i < SLIDER_NR_ESC; i++) {
		printf("    %-5s:  ", left_right[i]);
		s = &(enc_service_ctrl_element[i]);
		sc = (enum element_status_code) s->byte0.status;

		printf("%s", status_string(sc, valid_codes));

		if (s->vpd_read_fail)
			printf(" | VPD read fail");
		slider_print_esc_reset_reason
			(enc_service_ctrl_element->reset_reason);
		if (s->vpd_mismatch)
			printf(" | VPD mismatch");
		if (s->vpd_mirror_mismatch)
			printf(" | VPD mirror mismatch");
		if (s->firm_img_unmatch)
			printf(" | Fw image unmatch");
		if (s->ierr_asserted)
			printf(" | IERR asserted");
		if (s->xcc_data_initialised)
			printf(" | Cross card data initialized");
		if (s->report)
			printf(" | Report active");
		if (s->hot_swap)
			printf(" | Hotswap enabled");

		CHK_IDENT_LED(s);
		CHK_FAULT_LED(s);
		printf("\n");
	}
}

/* Print slider sas expander status */
static void print_slider_sas_expander_status(
	struct slider_sas_expander_status *sas_expander_element)
{
	int i;
	struct slider_sas_expander_status *s;
	enum element_status_code sc;

	printf("\n  SAS Expander Status\n");
	for (i = 0; i < SLIDER_NR_ESC; i++) {
		printf("    %-5s:  ", left_right[i]);
		s = &(sas_expander_element[i]);
		sc = (enum element_status_code) s->byte0.status;
		printf("%s", status_string(sc, valid_codes));

		printf("\n");
	}
}

/* Print slider sas connector status */
static void print_slider_sas_connector_status(
	struct slider_sas_connector_status *sas_connector_status)
{
	int i, j;
	struct slider_sas_connector_status *s;
	enum element_status_code sc;

	printf("\n  SAS Connector Status\n");
	for (i = 0; i < SLIDER_NR_SAS_EXPANDER; i++) {
		printf("    %s:\n", left_right[i]);
		for (j = 0; j < SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER; j++) {
			printf("      %-11s:  ", sas_connector_names[j]);
			s = &(sas_connector_status
				[(i * SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER)
						+ j]);
			sc = (enum element_status_code) s->byte0.status;
			printf("%s", status_string(sc, valid_codes));

			CHK_IDENT_LED(s);
			CHK_FAULT_LED(s);
			printf("\n");
		}
	}
}

/* Print slider phy status */
static void print_slider_phy_status(struct slider_phy_set *phy_sets)
{
	int i, j;
	struct slider_phy_status *s;
	enum element_status_code sc;

	printf("\n  Phy Status\n");
	for (i = 0; i < SLIDER_NR_SAS_EXPANDER; i++) {
		printf("    %s:\n", left_right[i]);
		for (j = 0; j < SLIDER_NR_PHY_PER_SAS_EXPANDER; j++) {
			s = &(phy_sets[i].phy_element[j]);
			if (s->enable) {
				printf("      Physical link %2d: ", j + 1);
				sc = (enum element_status_code)s->byte0.status;
				printf("%s", status_string(sc, valid_codes));
			}
			printf("\n");
		}
	}
}

/* Print slider statesave buffer status */
static void print_slider_ssb_status(
	struct slider_statesave_buffer_status *ssb_element)
{
	int i;
	struct slider_statesave_buffer_status *s;
	enum element_status_code sc;

	printf("\n  Statesave Buffer Status:\n");

	for (i = 0; i < SLIDER_NR_SSB; i++) {
		printf("    Element %d (%-5s): ", i + 1, left_right[i]);
		s = &(ssb_element[i]);
		sc = (enum element_status_code) s->byte0.status;

		printf("%s", status_string(sc, valid_codes));
		if (cmd_opts.verbose) {
			switch (s->buffer_status) {
			case 0:
				printf(" | No statesave available for "
							"retrieval");
				break;
			case 1:
				printf(" | At least one statesave available"
					" for retrieval");
				break;
			case 2:
				printf(" | Statesave collection in progress"
					" and no prior statesave available"
					" for retrieval");
				break;
			case 0xFF:
				printf(" | Unknown buffer status");
				break;
			}
			if (s->buffer_type == 0x00)
				printf(" | A statesave");
			else if (s->buffer_type == 0x01)
				printf(" | Host");
			else if (s->buffer_type == 0x02)
				printf(" | Self informational");
			else if ((s->buffer_type >= 0x03) &&
					(s->buffer_type <= 0x7F))
				printf(" | Future host type");
			else if (s->buffer_type == 0x80)
				printf(" | Self");
			else if ((s->buffer_type >= 0x81) &&
					(s->buffer_type <= 0xFF))
				printf(" | Future self type");
		}
		printf("\n");
	}
}

/* Print slider cpld status */
static void print_slider_cpld_status(struct slider_cpld_status *cpld_element)
{
	int i;
	struct slider_cpld_status *s;
	enum element_status_code sc;

	printf("\n  CPLD Status:\n");
	for (i = 0; i < SLIDER_NR_ESC; i++) {
		printf("    %-5s: ", left_right[i]);
		s = &(cpld_element[i]);
		sc = (enum element_status_code) s->byte0.status;

		printf("%s", status_string(sc, valid_codes));
		if (s->fail)
			printf(" | Unrecoverable Issue found");
		printf("\n");
	}
}

/* Print slider input power status */
static void print_slider_input_power_status(
	struct slider_input_power_status *input_power_element)
{
	int i, j;
	struct slider_input_power_status *s;
	enum element_status_code sc;

	printf("\n  Input Power Status:\n");
	for (i = 0; i < SLIDER_NR_PSU; i++) {
		printf("    PSU%d:\n", i + 1);
		for (j = 0; j < SLIDER_NR_INPUT_POWER_PER_PSU; j++) {
			printf("      %-19s: ", input_power_names[j]);
			s = &(input_power_element
				[(i * SLIDER_NR_INPUT_POWER_PER_PSU) + j]);
			sc = (enum element_status_code) s->byte0.status;

			printf("%-28s", status_string(sc, valid_codes));
			if (cmd_opts.verbose) {
				printf(" | Input power : %d",
				       ntohs(s->input_power));
			}
			printf("\n");
		}
	}
}

/* @return 0 for success, !0 for failure */
static int slider_read_diag_page2(char **page, int *fdp)
{
	char *buffer;
	int rc;

	/* Allocating status diagnostics page */
	buffer = calloc(1, slider_v_diag_page2_size);
	if (!buffer) {
		fprintf(stderr, "Failed to allocate memory to hold "
			"current status diagnostics page 02 results.\n");
		return 1;
	}

	if (cmd_opts.fake_path) {
		rc = read_page2_from_file(cmd_opts.fake_path, true,
					  buffer, slider_v_diag_page2_size);
		*fdp = -1;
	} else {
		rc = get_diagnostic_page(*fdp, RECEIVE_DIAGNOSTIC, 2,
			(void *)buffer, (int)slider_v_diag_page2_size);
	}

	if (rc != 0) {
		fprintf(stderr, "Failed to read SES diagnostic page; "
				"cannot report status.\n");
		free(buffer);
	}

	*page = buffer;
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

/* Prints slider element status */
static void diag_print_slider_status(void *dp)
{
	print_slider_disk_status
		((struct slider_disk_status *)
		(dp + element_offset(disk_status)));

	print_slider_power_supply_status
		((struct slider_power_supply_status *)
		(dp + element_offset(ps_status)),
		(struct slider_voltage_sensor_set *)
		(dp + element_offset(voltage_sensor_sets)));

	print_slider_fan_status
		((struct slider_fan_set *)
		(dp + element_offset(fan_sets)));

	print_slider_temp_sensor_sets
		((struct slider_temperature_sensor_set *)
		(dp + element_offset(temp_sensor_sets)));

	print_esc_status
		((struct slider_enc_service_ctrl_status *)
		(dp + element_offset(enc_service_ctrl_element)));

	print_slider_enclosure_status
		((struct enclosure_status *)
		(dp + element_offset(encl_element)));

	print_slider_sas_expander_status
		((struct slider_sas_expander_status *)
		(dp + element_offset(sas_expander_element)));

	print_slider_sas_connector_status
		((struct slider_sas_connector_status *)
		(dp + element_offset(sas_connector_status)));

	print_slider_midplane_status
		((struct slider_midplane_status *)
		(dp + element_offset(midplane_element_status)));

	print_slider_phy_status((struct slider_phy_set *)
		(dp + element_offset(phy_sets)));

	print_slider_ssb_status
		((struct slider_statesave_buffer_status *)
		(dp + element_offset(ssb_element)));

	print_slider_cpld_status
		((struct slider_cpld_status *)
		(dp + element_offset(cpld_element)));

	print_slider_input_power_status
		((struct slider_input_power_status *)
		(dp + element_offset(input_power_element)));
}

static void diag_print_slider_overall_lff_status(void *buffer)
{
	struct slider_lff_diag_page2 *dp
		= (struct slider_lff_diag_page2 *)buffer;

	printf("  Overall Status:    ");
	if (dp->crit) {
		printf("CRITICAL_FAULT");
		if (dp->non_crit)
			printf(" | NON_CRITICAL_FAULT");
	} else if (dp->non_crit)
		printf("NON_CRITICAL_FAULT");
	else if (dp->unrecov)
		printf("UNRECOVERABLE_FAULT");
	else
		printf("ok");
}

static void diag_print_slider_overall_sff_status(void *buffer)
{
	struct slider_sff_diag_page2 *dp
		= (struct slider_sff_diag_page2 *)buffer;

	printf("  Overall Status:    ");
	if (dp->crit) {
		printf("CRITICAL_FAULT");
		if (dp->non_crit)
			printf(" | NON_CRITICAL_FAULT");
	} else if (dp->non_crit)
		printf("NON_CRITICAL_FAULT");
	else if (dp->unrecov)
		printf("UNRECOVERABLE_FAULT");
	else
		printf("ok");
}

static inline int diag_print_slider_overall_status(void *buffer)
{
	if (slider_variant_flag == SLIDER_V_LFF) {
		diag_print_slider_overall_lff_status(buffer);
	} else if (slider_variant_flag == SLIDER_V_SFF) {
		diag_print_slider_overall_sff_status(buffer);
	} else {
		return 1;
	}

	return 0;
}

/*
 * Slider diagnostics
 * Returns 0 in success and 1 in failure
 */
static int diag_slider(int slider_type, int fd, struct dev_vpd *vpd)
{
	char *buffer;
	int rc;

	/* Slider variant flag */
	slider_variant_flag = slider_type;

	/* Fill slider variant specific details */
	if (fill_slider_v_specific_details())
		return 1;

	/* Read diag page */
	if (slider_read_diag_page2(&buffer, &fd))
		return 1;

	/* Print enclosure overall status */
	rc = diag_print_slider_overall_status((void *)buffer);
	if (rc)
		goto err_out;

	/* Print slider element status */
	diag_print_slider_status(buffer);

	/* Print raw data */
	if (cmd_opts.verbose) {
		printf("\n\nRaw diagnostic page:\n");
		print_raw_data(stdout, buffer, slider_v_diag_page2_size);
	}
	printf("\n\n");

	/*
	 * Report faults to syslog(in powerNV)/servicelog(powerVM), and turn
	 * on LEDs as appropriate. LED status reported previously may not be
	 * accurate after we do this, but the alternative is to report faults
	 * first and then read the diagnostic page a second time.
	 * And unfortunately, the changes to LED settings don't always show up
	 * immediately in the next query of the SES.
	 */
	if (cmd_opts.serv_event) {
		rc = report_slider_fault_to_svclog(buffer, vpd, fd);
		if (rc)
			goto err_out;
	}

	/* Set led state (option -l is not supported for fake path) */
	if (fd != -1 && cmd_opts.leds)
		rc = slider_turn_on_fault_leds(buffer, fd);

err_out:
	free(buffer);
	return rc;
}

/*
 * callback function for lff variant of slider
 * @return 0 for success, 1 for failure
 */
int diag_slider_lff(int fd, struct dev_vpd *vpd)
{
	if (diag_slider(SLIDER_V_LFF, fd, vpd))
		return 1;

	return 0;
}

/*
 * callback function for sff variant of slider
 * @return 0 for success, 1 for failure
 */
int diag_slider_sff(int fd, struct dev_vpd *vpd)
{
	if (diag_slider(SLIDER_V_SFF, fd, vpd))
		return 1;

	return 0;
}
