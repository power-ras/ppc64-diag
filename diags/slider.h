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

#ifndef __SLIDER_H__
#define __SLIDER_H__

#include <asm/byteorder.h>
#include <stdint.h>

#include "encl_common.h"

#define SLIDER_NR_LFF_DISK			12
#define SLIDER_NR_SFF_DISK			24
#define SLIDER_NR_ESC				2
#define SLIDER_NR_ENCLOSURE			1
#define SLIDER_NR_FAN_PER_POWER_SUPPLY		8
#define SLIDER_NR_POWER_SUPPLY			2
#define SLIDER_NR_PHY_PER_SAS_EXPANDER		36
#define SLIDER_NR_SAS_EXPANDER			2
#define SLIDER_NR_SAS_CONNECTOR_PER_EXPANDER	3
#define SLIDER_NR_PSU				2
#define SLIDER_NR_TEMP_SENSOR_PER_PSU		3
#define SLIDER_NR_INPUT_POWER_PER_PSU		2
#define SLIDER_NR_TEMP_SENSOR_PER_ESC		2
#define SLIDER_NR_SSB				2
#define SLIDER_NR_SAS_CONNECTOR			6
#define SLIDER_NR_INPUT_POWER			4
#define SLIDER_NR_PSU_TEMP_SENSOR		6
#define SLIDER_NR_ESC_TEMP_SENSOR		4
#define SLIDER_NR_TEMP_SENSOR			11
#define SLIDER_NR_VOLT_SENSOR_PER_ESM		4

#define SLIDER_FAN_SPEED_FACTOR			10

/*
 * Common structures across enclosure are defined in encl_common.h
 *
 * ******************Status Registers:********************************
 *	- slider_disk_status			: sec - 5.4.2,  table - 48
 *	- slider_temperature_sensor_status	: sec - 5.4.6,  table - 52
 *	- slider_enc_serv_ctrl_status		: sec - 5.4.7,  table - 53
 *	- slider_encl_status			: sec - 5.4.8,  table - 54
 *	- slider_voltage_sensor_status		: sec - 5.4.10, table - 56
 *	- slider_sas_expander_status		: sec - 5.4.11, table - 57
 *	- slider_sas_connector_status		: sec - 5.4.12, table - 58
 *	- slider_midplane_status		: sec - 5.4.13, table - 59
 *	- slider_phy_status			: sec - 5.4.15, table - 61
 *	- slider_statesave_buffer_status	: sec - 5.4.16, table - 62
 *	- slider_cpld_status			: sec - 5.4.17, table - 63
 *	- sllider_input_power_status		: sec - 5.4.18, table - 64
 *
 * ******************Ctrl Registers:********************************
 *	- slider_disk_ctrl		: sec - 5.3.2,  table - 26
 *	- slider_power_supply_ctrl	: sec - 5.3.4,  table - 28
 *	- slider_temperature_sensor_ctrl: sec - 5.3.6,  table - 30
 *	- slider_enc_service_ctrl_ctrl	: sec - 5.3.7,  table - 31
 *	- slider_encl_ctrl		: sec - 5.3.8,  table - 32
 *	- slider_voltage_sensor_ctrl	: sec - 5.3.10, table - 34
 *	- slider_sas_expander_ctrl	: sec - 5.3.11, table - 35
 *	- slider_sas_connector_ctrl	: sec - 5.3.12, table - 36
 *	- slider_midplane_ctrl		: sec - 5.3.13, table - 37
 *	- slider_phy_ctrl		: sec - 5.3.15, table - 39
 *	- slider_cpld_ctrl		: sec - 5.3.17, table - 42
 *	- slider_input_power_ctrl	: sec - 5.3.18, table - 43
 */

/* Slider disk status */
struct slider_disk_status {
	struct element_status_byte0 byte0;

	uint8_t slot_number;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t app_client_bypassed_a:1;
	uint8_t do_not_remove:1;
	uint8_t enclosure_bypassed_a:1;
	uint8_t enclosure_bypassed_b:1;
	uint8_t ready_to_insert:1;
	uint8_t rmv:1;
	uint8_t ident:1;
	uint8_t report:1;

	uint8_t app_client_bypassed_b:1;
	uint8_t fault_sensed:1;
	uint8_t fail:1;		/* AKA fault_reqstd */
	uint8_t device_off:1;
	uint8_t bypassed_a:1;
	uint8_t bypassed_b:1;
	uint8_t device_bypassed_a:1;
	uint8_t device_bypassed_b:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t report:1;
	uint8_t ident:1;
	uint8_t rmv:1;
	uint8_t ready_to_insert:1;
	uint8_t enclosure_bypassed_b:1;
	uint8_t enclosure_bypassed_a:1;
	uint8_t do_not_remove:1;
	uint8_t app_client_bypassed_a:1;

	uint8_t device_bypassed_b:1;
	uint8_t device_bypassed_a:1;
	uint8_t bypassed_b:1;
	uint8_t bypassed_a:1;
	uint8_t device_off:1;
	uint8_t fail:1;		/* AKA fault_reqstd */
	uint8_t fault_sensed:1;
	uint8_t app_client_bypassed_b:1;
#endif
};

/* Slider fan set - fan per power supply */
struct slider_fan_set {
	struct fan_status fan_element[SLIDER_NR_FAN_PER_POWER_SUPPLY];
};

/* Temperature sensor status */
struct slider_temperature_sensor_status {
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t reserved2:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:6;
	uint8_t fail:1;
	uint8_t ident:1;
#endif

	uint8_t temperature;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t reserved3:4;
	uint8_t ot_failure:1;
	uint8_t ot_warning:1;
	uint8_t ut_failure:1;
	uint8_t ut_warning:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t ut_warning:1;
	uint8_t ut_failure:1;
	uint8_t ot_warning:1;
	uint8_t ot_failure:1;
	uint8_t reserved3:4;
#endif
};

/* Temperature sensor set - per slider enclosure */
struct slider_temperature_sensor_set {
	struct slider_temperature_sensor_status temp_enclosure;
	struct slider_temperature_sensor_status
				temp_psu[SLIDER_NR_PSU_TEMP_SENSOR];
	struct slider_temperature_sensor_status
				temp_esc[SLIDER_NR_ESC_TEMP_SENSOR];
};

/* Enclosure service controller status */
struct slider_enc_service_ctrl_status {
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t comm_fail:1;
	uint8_t vpd_read_fail:1;
	uint8_t reset_reason:4;

	uint8_t vpd_mismatch:1;
	uint8_t vpd_mirror_mismatch:1;
	uint8_t reserved2:2;
	uint8_t firm_img_unmatch:1;
	uint8_t ierr_asserted:1;
	uint8_t xcc_data_initialised:1;
	uint8_t report:1;

	uint8_t hot_swap:1;
	uint8_t reserved3:7;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reset_reason:4;
	uint8_t vpd_read_fail:1;
	uint8_t comm_fail:1;
	uint8_t fail:1;
	uint8_t ident:1;

	uint8_t report:1;
	uint8_t xcc_data_initialised:1;
	uint8_t ierr_asserted:1;
	uint8_t firm_img_unmatch:1;
	uint8_t reserved2:2;
	uint8_t vpd_mirror_mismatch:1;
	uint8_t vpd_mismatch:1;

	uint8_t reserved3:7;
	uint8_t hot_swap:1;
#endif
};

/* Enclosure status */
struct slider_encl_status {
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t ident:1;
	uint8_t bmc_indication:1;
	uint8_t nebs:1;
	uint8_t reserved2:5;

	uint8_t time_until_power_cycle:6;
	uint8_t fail:1;
	uint8_t warning_indication:1;

	uint8_t req_power_off_duration:6;
	uint8_t failure_requested:1;
	uint8_t warning_requested:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:5;
	uint8_t nebs:1;
	uint8_t bmc_indication:1;
	uint8_t ident:1;

	uint8_t warning_indication:1;
	uint8_t fail:1;
	uint8_t time_until_power_cycle:6;

	uint8_t warning_requested:1;
	uint8_t failure_requested:1;
	uint8_t req_power_off_duration:6;
#endif
};

/* Voltage sensor status */
struct slider_voltage_sensor_status {
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t reserved2:2;
	uint8_t warn_over:1;
	uint8_t warn_under:1;
	uint8_t crit_over:1;
	uint8_t crit_under:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t crit_under:1;
	uint8_t crit_over:1;
	uint8_t warn_under:1;
	uint8_t warn_over:1;
	uint8_t reserved2:2;
	uint8_t fail:1;
	uint8_t ident:1;
#endif

	uint16_t voltage;
};

/* Slider voltage sensor set - per esc */
struct slider_voltage_sensor_set {
	struct slider_voltage_sensor_status sensor_3V3;
	struct slider_voltage_sensor_status sensor_1V0;
	struct slider_voltage_sensor_status sensor_1V8;
	struct slider_voltage_sensor_status sensor_0V92;
};

/* Sas expander status */
struct slider_sas_expander_status {
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t reserved2:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:6;
	uint8_t fail:1;
	uint8_t ident:1;
#endif

	uint16_t reserved3;
};

/* Slider sas connector status */
struct slider_sas_connector_status {
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t ident:1;
	uint8_t connector_type:7;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t connector_type:7;
	uint8_t ident:1;
#endif

	uint8_t connector_physical_link;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t client_bypassed:1;
	uint8_t fail:1;
	uint8_t enclosure_bypassed:1;
	uint8_t reserved2:5;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:5;
	uint8_t enclosure_bypassed:1;
	uint8_t fail:1;
	uint8_t client_bypassed:1;
#endif
};

/* Midplane status */
struct slider_midplane_status {
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t vpd1_read_fail1:1;
	uint8_t vpd1_read_fail2:1;
	uint8_t vpd2_read_fail1:1;
	uint8_t vpd2_read_fail2:1;
	uint8_t vpd_mismatch1:1;
	uint8_t vpd_mismatch2:1;
	uint8_t vpd_mirror_mismatch:1;
	uint8_t reserved2:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:1;
	uint8_t vpd_mirror_mismatch:1;
	uint8_t vpd_mismatch2:1;
	uint8_t vpd_mismatch1:1;
	uint8_t vpd2_read_fail2:1;
	uint8_t vpd2_read_fail1:1;
	uint8_t vpd1_read_fail2:1;
	uint8_t vpd1_read_fail1:1;
#endif

	uint16_t reserved3;
};

/* Phy status */
struct slider_phy_status {
	struct element_status_byte0 byte0;

	uint8_t phy_identifier;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t enable:1;
	uint8_t connect:1;
	uint8_t reserved2:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:6;
	uint8_t connect:1;
	uint8_t enable:1;
#endif

	uint8_t phy_change_count;
};

/* Phy set */
struct slider_phy_set {
	struct slider_phy_status phy_element[SLIDER_NR_PHY_PER_SAS_EXPANDER];
};

/* Statesave buffer */
struct slider_statesave_buffer_status {
	struct element_status_byte0 byte0;

	uint8_t buffer_status;

	uint16_t reserved2;
} __attribute__((packed));

/* CPLD status */
struct slider_cpld_status {
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t fail:1;
	uint8_t reserved2:7;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:7;
	uint8_t fail:1;
#endif

	uint16_t reserved3;
} __attribute__((packed));

/* Input power status */
struct slider_input_power_status {
	struct element_status_byte0 byte0;

	uint16_t input_power;

	uint8_t reserved2;
} __attribute__((packed));


/*
 * This data structure matches the layout described in Slider
 * specification, Table 45 and the element counts in sec 2.1.2.
 */
struct slider_lff_diag_page2 {
	uint8_t page_code;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t reserved1:3;
	uint8_t invop:1;
	uint8_t info:1;
	uint8_t non_crit:1;
	uint8_t crit:1;
	uint8_t unrecov:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t unrecov:1;
	uint8_t crit:1;
	uint8_t non_crit:1;
	uint8_t info:1;
	uint8_t invop:1;
	uint8_t reserved1:3;
#endif

	/* Page length : n-3 */
	uint16_t page_length;

	uint32_t generation_code;

	/* Disk */
	struct slider_disk_status overall_disk_status;
	struct slider_disk_status disk_status[SLIDER_NR_LFF_DISK];

	/* Power supply */
	struct power_supply_status overall_power_status;
	struct power_supply_status ps_status[SLIDER_NR_POWER_SUPPLY];

	/* Cooling element */
	struct fan_status overall_fan_status;
	struct slider_fan_set fan_sets[SLIDER_NR_POWER_SUPPLY];

	/* Temperature sensor */
	struct slider_temperature_sensor_status overall_temp_sensor_status;
	struct slider_temperature_sensor_set temp_sensor_sets;

	/* Enclosure service controller */
	struct slider_enc_service_ctrl_status overall_enc_service_ctrl_status;
	struct slider_enc_service_ctrl_status
		enc_service_ctrl_element[SLIDER_NR_ESC];

	/* Enclosure */
	struct slider_encl_status overall_encl_status;
	struct slider_encl_status encl_element;

	/* Voltage sensor */
	struct slider_voltage_sensor_status overall_voltage_status;
	struct slider_voltage_sensor_set voltage_sensor_sets[SLIDER_NR_ESC];

	/* Sas expander */
	struct slider_sas_expander_status overall_sas_expander_status;
	struct slider_sas_expander_status sas_expander_element[SLIDER_NR_ESC];

	/* Sas connector */
	struct slider_sas_connector_status overall_sas_connector_status;
	struct slider_sas_connector_status
		sas_connector_status[SLIDER_NR_SAS_CONNECTOR];

	/* Disk midplane */
	struct slider_midplane_status overall_midplane_status;
	struct slider_midplane_status midplane_element_status;

	/* Phy */
	struct slider_phy_status overall_phy_status;
	struct slider_phy_set phy_sets[SLIDER_NR_SAS_EXPANDER];

	/* Statesave Buffer */
	struct slider_statesave_buffer_status overall_ssb_status;
	struct slider_statesave_buffer_status ssb_element[SLIDER_NR_SSB];

	/* CPLD */
	struct slider_cpld_status overall_cpld_status;
	struct slider_cpld_status cpld_element[SLIDER_NR_ESC];

	/* Input power */
	struct slider_input_power_status overall_input_power_status;
	struct slider_input_power_status
		input_power_element[SLIDER_NR_INPUT_POWER];

} __attribute__((packed));

/*
 * This data structure matches the layout described in Slider
 * specification, Table 45 and the element counts in sec 2.1.2.
 */
struct slider_sff_diag_page2 {
	uint8_t page_code;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t reserved1:3;
	uint8_t invop:1;
	uint8_t info:1;
	uint8_t non_crit:1;
	uint8_t crit:1;
	uint8_t unrecov:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t unrecov:1;
	uint8_t crit:1;
	uint8_t non_crit:1;
	uint8_t info:1;
	uint8_t invop:1;
	uint8_t reserved1:3;
#endif

	/* Page length : n-3 */
	uint16_t page_length;

	uint32_t generation_code;

	/* Disk */
	struct slider_disk_status overall_disk_status;
	struct slider_disk_status disk_status[SLIDER_NR_SFF_DISK];

	/* Power supply */
	struct power_supply_status overall_power_status;
	struct power_supply_status ps_status[SLIDER_NR_POWER_SUPPLY];

	/* Cooling element */
	struct fan_status overall_fan_status;
	struct slider_fan_set fan_sets[SLIDER_NR_POWER_SUPPLY];

	/* Temperature sensor */
	struct slider_temperature_sensor_status overall_temp_sensor_status;
	struct slider_temperature_sensor_set temp_sensor_sets;

	/* Enclosure service controller */
	struct slider_enc_service_ctrl_status overall_enc_service_ctrl_status;
	struct slider_enc_service_ctrl_status
		enc_service_ctrl_element[SLIDER_NR_ESC];

	/* Enclosure */
	struct slider_encl_status overall_encl_status;
	struct slider_encl_status encl_element;

	/* Voltage sensor */
	struct slider_voltage_sensor_status overall_voltage_status;
	struct slider_voltage_sensor_set voltage_sensor_sets[SLIDER_NR_ESC];

	/* Sas expander */
	struct slider_sas_expander_status overall_sas_expander_status;
	struct slider_sas_expander_status sas_expander_element[SLIDER_NR_ESC];

	/* Sas connector */
	struct slider_sas_connector_status overall_sas_connector_status;
	struct slider_sas_connector_status
		sas_connector_status[SLIDER_NR_SAS_CONNECTOR];

	/* Disk midplane */
	struct slider_midplane_status overall_midplane_status;
	struct slider_midplane_status midplane_element_status;

	/* Phy */
	struct slider_phy_status overall_phy_status;
	struct slider_phy_set phy_sets[SLIDER_NR_SAS_EXPANDER];

	/* Statesave Buffer */
	struct slider_statesave_buffer_status overall_ssb_status;
	struct slider_statesave_buffer_status ssb_element[SLIDER_NR_SSB];

	/* CPLD */
	struct slider_cpld_status overall_cpld_status;
	struct slider_cpld_status cpld_element[SLIDER_NR_ESC];

	/* Input power */
	struct slider_input_power_status overall_input_power_status;
	struct slider_input_power_status
		input_power_element[SLIDER_NR_INPUT_POWER];

} __attribute__((packed));


/* Disk control register */
struct slider_disk_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t reserved2;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t rqst_active:1;
	uint8_t do_not_remove:1;
	uint8_t reserved3:1;
	uint8_t rqst_missing:1;
	uint8_t rqst_insert:1;
	uint8_t rqst_remove:1;
	uint8_t rqst_ident:1;
	uint8_t reserved4:1;

	uint8_t reserved5:2;
	uint8_t rqst_fail:1;
	uint8_t device_off:1;
	uint8_t enable_byp_a:1;
	uint8_t enable_byp_b:1;
	uint8_t reserved6:2;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved4:1;
	uint8_t rqst_ident:1;
	uint8_t rqst_remove:1;
	uint8_t rqst_insert:1;
	uint8_t rqst_missing:1;
	uint8_t reserved3:1;
	uint8_t do_not_remove:1;
	uint8_t rqst_active:1;

	uint8_t reserved6:2;
	uint8_t enable_byp_b:1;
	uint8_t enable_byp_a:1;
	uint8_t device_off:1;
	uint8_t rqst_fail:1;
	uint8_t reserved5:2;
#endif
};

/* Power supply ctrl sec - 5.3.4, table - 28 */
struct slider_power_supply_ctrl {
	struct common_ctrl common_ctrl;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t rqst_ident:1;
	uint8_t reserved2:7;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:7;
	uint8_t rqst_ident:1;
#endif

	uint8_t reserved3;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t reserved4:1;
	uint8_t rqst_fail:1;
	uint8_t rqst_on:1;
	uint8_t reserved5:5;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved5:5;
	uint8_t rqst_on:1;
	uint8_t rqst_fail:1;
	uint8_t reserved4:1;
#endif
};

/* Temp sensor cntrl sec - 5.3.6, table - 30 */
struct slider_temperature_sensor_ctrl {
	struct common_ctrl common_ctrl;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t rqst_ident:1;
	uint8_t rqst_fail:1;
	uint8_t reserved2:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:6;
	uint8_t rqst_fail:1;
	uint8_t rqst_ident:1;
#endif

	uint16_t reserved3;
};

/* Temperature sensor set - per slider enclosure */
struct slider_temperature_sensor_ctrl_set {
	struct slider_temperature_sensor_ctrl temp_enclosure;
	struct slider_temperature_sensor_ctrl
		temp_psu[SLIDER_NR_PSU_TEMP_SENSOR];
	struct slider_temperature_sensor_ctrl
		temp_esc[SLIDER_NR_ESC_TEMP_SENSOR];
};

/* Enclosure service controller ctrl register sec - 5.3.7, table - 31 */
struct slider_enc_service_ctrl_ctrl {
	struct common_ctrl common_ctrl;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t rqst_ident:1;
	uint8_t rqst_fail:1;
	uint8_t reserved2:6;

	uint8_t reserved3:7;
	uint8_t select_element:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:6;
	uint8_t rqst_fail:1;
	uint8_t rqst_ident:1;

	uint8_t select_element:1;
	uint8_t reserved3:7;
#endif

	uint8_t reserved4;
};

/* Enclosure ctrl register sec - 5.3.8, table - 32 */
struct slider_encl_ctrl {
	struct common_ctrl common_ctrl;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t rqst_ident:1;
	uint8_t reserved2:1;
	uint8_t nebs:1;
	uint8_t reserved3:5;

	uint8_t power_cycle_request:2;
	uint8_t power_cycle_delay:6;

	uint8_t power_off_duration:6;
	uint8_t rqst_fail:1;
	uint8_t request_warning:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved3:5;
	uint8_t nebs:1;
	uint8_t reserved2:1;
	uint8_t rqst_ident:1;

	uint8_t power_cycle_delay:6;
	uint8_t power_cycle_request:2;

	uint8_t request_warning:1;
	uint8_t rqst_fail:1;
	uint8_t power_off_duration:6;
#endif
};

/* Voltage sensor ctrl register sec - 5.3.10, table - 34 */
struct slider_voltage_sensor_ctrl {
	struct common_ctrl common_ctrl;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t rqst_ident:1;
	uint8_t rqst_fail:1;
	uint8_t reserved2:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:6;
	uint8_t rqst_fail:1;
	uint8_t rqst_ident:1;
#endif

	uint16_t reserved3;
};

/* Voltage_sensor_set - per esc */
struct slider_voltage_sensor_ctrl_set {
	struct slider_voltage_sensor_ctrl sensor_3V3;
	struct slider_voltage_sensor_ctrl sensor_1V0;
	struct slider_voltage_sensor_ctrl sensor_1V8;
	struct slider_voltage_sensor_ctrl sensor_0V92;
};

/* Sas expander enclosure ctrl register sec - 5.3.11, table - 35 */
struct slider_sas_expander_ctrl {
	struct common_ctrl common_ctrl;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t rqst_ident:1;
	uint8_t rqst_fail:1;
	uint8_t reserved2:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:6;
	uint8_t rqst_fail:1;
	uint8_t rqst_ident:1;
#endif

	uint16_t reserved3;
};

/* Sas expander enclosure ctrl register sec - 5.3.12, table - 36 */
struct slider_sas_connector_ctrl {
	struct common_ctrl common_ctrl;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t rqst_ident:1;
	uint8_t rqst_bypass:1;
	uint8_t reserved2:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:6;
	uint8_t rqst_bypass:1;
	uint8_t rqst_ident:1;
#endif

	uint8_t reserved3;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t reserved4:1;
	uint8_t rqst_fail:1;
	uint8_t reserved5:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved5:6;
	uint8_t rqst_fail:1;
	uint8_t reserved4:1;
#endif
};

/* Midplane ctrl register sec - 5.3.13, table - 37 */
struct slider_midplane_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t reserved2;

	uint16_t reserved3;
} __attribute__((packed));

/* Phy ctrl register sec - 5.3.15, table - 39 */
struct slider_phy_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t reserved2;

	uint8_t phy_operation;

	uint8_t reserved3;
};

/* Phy set */
struct slider_phy_ctrl_set {
	struct slider_phy_ctrl phy_element[SLIDER_NR_PHY_PER_SAS_EXPANDER];
};

/* Statesave buffer ctrl register sec - 5.3.16, table - 40 */
struct slider_statesave_buffer_ctrl {
	struct common_ctrl common_ctrl;
	uint8_t mode;
	uint16_t reserved2;
} __attribute__((packed));

/* CPLD ctrl register sec - 5.3.17, table - 42 */
struct slider_cpld_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t reserved2;

	uint16_t reserved3;
} __attribute__((packed));

/* Input power ctrl register sec - 5.3.18, table - 43 */
struct slider_input_power_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t reserved2;

	uint16_t reserved3;
} __attribute__((packed));

/* Fan ctrl register */
struct slider_fan_ctrl_set {
	struct fan_ctrl fan_element[SLIDER_NR_FAN_PER_POWER_SUPPLY];
};

/* Slider LFF control page layout */
struct slider_lff_ctrl_page2 {
	uint8_t page_code;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t reserved1:4;
	uint8_t info:1;
	uint8_t non_crit:1;
	uint8_t crit:1;
	uint8_t unrecov:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t unrecov:1;
	uint8_t crit:1;
	uint8_t non_crit:1;
	uint8_t info:1;
	uint8_t reserved1:4;
#endif

	/* Page length : n-3 */
	uint16_t page_length;

	uint32_t generation_code;

	/* Disk */
	struct slider_disk_ctrl overall_disk_ctrl;
	struct slider_disk_ctrl disk_ctrl[SLIDER_NR_LFF_DISK];

	/* Power supply */
	struct slider_power_supply_ctrl overall_power_ctrl;
	struct slider_power_supply_ctrl ps_ctrl[SLIDER_NR_POWER_SUPPLY];

	/* Cooling element */
	struct fan_ctrl overall_fan_ctrl;
	struct slider_fan_ctrl_set fan_sets[SLIDER_NR_POWER_SUPPLY];

	/* Temperature sensor */
	struct slider_temperature_sensor_ctrl overall_temp_sensor_ctrl;
	struct slider_temperature_sensor_ctrl_set temp_sensor_sets;

	/* Enclosure service controller */
	struct slider_enc_service_ctrl_ctrl overall_enc_service_ctrl_ctrl;
	struct slider_enc_service_ctrl_ctrl
		enc_service_ctrl_element[SLIDER_NR_ESC];

	/* Enclosure */
	struct slider_encl_ctrl overall_encl_ctrl;
	struct slider_encl_ctrl encl_element;

	/* Voltage sensor */
	struct slider_voltage_sensor_ctrl overall_voltage_ctrl;
	struct slider_voltage_sensor_ctrl_set
		voltage_sensor_sets[SLIDER_NR_ESC];

	/* Sas expander */
	struct slider_sas_expander_ctrl overall_sas_expander_ctrl;
	struct slider_sas_expander_ctrl sas_expander_element[SLIDER_NR_ESC];

	/* Sas connector */
	struct slider_sas_connector_ctrl overall_sas_connector_ctrl;
	struct slider_sas_connector_ctrl
		sas_connector_ctrl[SLIDER_NR_SAS_CONNECTOR];

	/* Disk midplane */
	struct slider_midplane_ctrl overall_midplane_ctrl;
	struct slider_midplane_ctrl midplane_element_ctrl;

	/* Phy */
	struct slider_phy_ctrl overall_phy_ctrl;
	struct slider_phy_ctrl_set phy_sets[SLIDER_NR_SAS_EXPANDER];

	/* Statesave buffer */
	struct slider_statesave_buffer_ctrl overall_ssb_ctrl;
	struct slider_statesave_buffer_ctrl  ssb_elemnet[SLIDER_NR_SSB];

	/* CPLD */
	struct slider_cpld_ctrl overall_cpld_ctrl;
	struct slider_cpld_ctrl cpld_ctrl_element[SLIDER_NR_ESC];

	/* Input power */
	struct slider_input_power_ctrl overall_input_power_ctrl;
	struct slider_input_power_ctrl
		input_power_ctrl_element[SLIDER_NR_INPUT_POWER];

} __attribute__((packed));

/* Slider SFF control page layout */
struct slider_sff_ctrl_page2 {
	uint8_t page_code;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t reserved1:4;
	uint8_t info:1;
	uint8_t non_crit:1;
	uint8_t crit:1;
	uint8_t unrecov:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t unrecov:1;
	uint8_t crit:1;
	uint8_t non_crit:1;
	uint8_t info:1;
	uint8_t reserved1:4;
#endif

	/* Page length : n-3 */
	uint16_t page_length;

	uint32_t generation_code;

	/* Disk */
	struct slider_disk_ctrl overall_disk_ctrl;
	struct slider_disk_ctrl disk_ctrl[SLIDER_NR_SFF_DISK];

	/* Power supply */
	struct slider_power_supply_ctrl overall_power_ctrl;
	struct slider_power_supply_ctrl ps_ctrl[SLIDER_NR_POWER_SUPPLY];

	/* Cooling element */
	struct fan_ctrl overall_fan_ctrl;
	struct slider_fan_ctrl_set fan_sets[SLIDER_NR_POWER_SUPPLY];

	/* Temperature sensor */
	struct slider_temperature_sensor_ctrl overall_temp_sensor_ctrl;
	struct slider_temperature_sensor_ctrl_set temp_sensor_sets;

	/* Enclosure service controller */
	struct slider_enc_service_ctrl_ctrl overall_enc_service_ctrl_ctrl;
	struct slider_enc_service_ctrl_ctrl
		enc_service_ctrl_element[SLIDER_NR_ESC];

	/* Enclosure */
	struct slider_encl_ctrl overall_encl_ctrl;
	struct slider_encl_ctrl encl_element;

	/* Voltage sensor */
	struct slider_voltage_sensor_ctrl overall_voltage_ctrl;
	struct slider_voltage_sensor_ctrl_set
		voltage_sensor_sets[SLIDER_NR_ESC];

	/* Sas expander */
	struct slider_sas_expander_ctrl overall_sas_expander_ctrl;
	struct slider_sas_expander_ctrl sas_expander_element[SLIDER_NR_ESC];

	/* Sas connector */
	struct slider_sas_connector_ctrl overall_sas_connector_ctrl;
	struct slider_sas_connector_ctrl
		sas_connector_ctrl[SLIDER_NR_SAS_CONNECTOR];

	/* Disk midplane */
	struct slider_midplane_ctrl overall_midplane_ctrl;
	struct slider_midplane_ctrl midplane_element_ctrl;

	/* Phy */
	struct slider_phy_ctrl overall_phy_ctrl;
	struct slider_phy_ctrl_set phy_sets[SLIDER_NR_SAS_EXPANDER];

	/* Statesave buffer */
	struct slider_statesave_buffer_ctrl overall_ssb_ctrl;
	struct slider_statesave_buffer_ctrl  ssb_elemnet[SLIDER_NR_SSB];

	/* CPLD */
	struct slider_cpld_ctrl overall_cpld_ctrl;
	struct slider_cpld_ctrl cpld_ctrl_element[SLIDER_NR_ESC];

	/* Input power */
	struct slider_input_power_ctrl overall_input_power_ctrl;
	struct slider_input_power_ctrl
		input_power_ctrl_element[SLIDER_NR_INPUT_POWER];

} __attribute__((packed));

/* Element descriptor page */
#define SLIDER_NR_LFF_ELEMENT_DES_PAGE_SIZE		952
#define SLIDER_NR_SFF_ELEMENT_DES_PAGE_SIZE		1192
#define SLIDER_NR_PS_ELEMENT_DES_PAGE_SIZE		122

/* Power supply start offset in element descriptor page */
#define SLIDER_NR_LFF_PS_ELEMENT_DES_PAGE_START_OFFSET	259
#define SLIDER_NR_SFF_PS_ELEMENT_DES_PAGE_START_OFFSET	499

/*
 * Slider LFF element descriptor page layout
 * Used only for power supply, so ignored other elements
 */
struct slider_lff_element_descriptor_page {
	char ignored1[SLIDER_NR_LFF_PS_ELEMENT_DES_PAGE_START_OFFSET - 1];
	struct power_supply_descriptor ps0_vpd;
	uint16_t reserved;
	struct power_supply_descriptor ps1_vpd;
	char ignored2[SLIDER_NR_LFF_ELEMENT_DES_PAGE_SIZE -
			SLIDER_NR_LFF_PS_ELEMENT_DES_PAGE_START_OFFSET -
			SLIDER_NR_PS_ELEMENT_DES_PAGE_SIZE]; /* 811 */
} __attribute__((packed));

/*
 * Slider SFF element descriptor page layout
 * Used only for power supply, so ignored other elements
 */
struct slider_sff_element_descriptor_page {
	char ignored1[SLIDER_NR_SFF_PS_ELEMENT_DES_PAGE_START_OFFSET - 1];
	struct power_supply_descriptor ps0_vpd;
	uint16_t reserved;
	struct power_supply_descriptor ps1_vpd;
	char ignored2[SLIDER_NR_SFF_ELEMENT_DES_PAGE_SIZE -
			SLIDER_NR_SFF_PS_ELEMENT_DES_PAGE_START_OFFSET -
			SLIDER_NR_PS_ELEMENT_DES_PAGE_SIZE]; /* 811 */
} __attribute__((packed));

#endif /* __SLIDER_H__ */
