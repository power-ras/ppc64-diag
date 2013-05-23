/* Copyright (C) 2012 IBM Corporation */

#ifndef __BLUEHAWK_H__
#define __BLUEHAWK_H__

#include <stdint.h>

#define NR_DISKS_PER_BLUEHAWK 30

enum element_status_code {
	ES_UNSUPPORTED,		/* invalid status */
	ES_OK,
	ES_CRITICAL,		/* usually valid */
	ES_NONCRITICAL,		/* usually valid */
	ES_UNRECOVERABLE,	/* invalid status */
	ES_NOT_INSTALLED,
	ES_UNKNOWN,		/* usually invalid */
	ES_NOT_AVAILABLE,	/* usually invalid */
	ES_NO_ACCESS_ALLOWED,	/* invalid status */
	ES_EOL			/* end of list of status codes */
};

struct element_status_byte0 {
	uint8_t reserved1:1;	/* = 0 */
	uint8_t prdfail:1;	/* not implemented */
	uint8_t disabled:1;	/* not implemented */
	uint8_t swap:1;
	uint8_t status:4;	/* 0/1 or element_status_code */
};

struct overall_disk_status_byte1 {
	uint8_t device_environment:3;	/* = 010b */
	uint8_t slot_address:5;
};

struct disk_element_status_byte1 {
	uint8_t hot_swap:1;		/* = 1 */
	uint8_t slot_address:7;
};

struct disk_status {
	/* from 4.3.5 and 4.3.6 */
	struct element_status_byte0 byte0;
	/* Overall status = worst status of all disks. */

	union {
		struct overall_disk_status_byte1 overall_status;
		struct disk_element_status_byte1 element_status;
	} byte1;

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
};

struct enclosure_status {
	/* from 4.3.7 */
	struct element_status_byte0 byte0;
	/* status is always 1. */

	uint8_t ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3:6;
	uint8_t fail:1;		/* AKA failure_indication */
	uint8_t warning_indication:1;

	uint8_t reserved4:6;
	uint8_t failure_requested:1;
	uint8_t warning_requested:1;
};

struct esm_status {
	/* from 4.3.8, 4.3.9 */
	struct element_status_byte0 byte0;
	/* Overall status is TBD. */

	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t reserved2:6;

	uint8_t reserved3:7;
	uint8_t report:1;

	uint8_t hot_swap:1;
	uint8_t reserved4:7;
};

struct temperature_sensor_status {
	/* from 4.3.10, 4.3.11 */
	struct element_status_byte0 byte0;
	/* Overall status is TBD. */

	uint8_t ident:1;
	uint8_t reserved2:7;

	uint8_t temperature;

	uint8_t reserved3:4;
	uint8_t ot_failure:1;
	uint8_t ot_warning:1;
	uint8_t ut_failure:1;
	uint8_t ut_warning:1;
};

struct temperature_sensor_set {
	struct temperature_sensor_status croc;
	struct temperature_sensor_status ppc;
	struct temperature_sensor_status expander;
	struct temperature_sensor_status ambient[2];
	struct temperature_sensor_status power_supply[2];
};

struct fan_status {
	/* from 4.3.12, 4.3.13 */
	struct element_status_byte0 byte0;
	/* Overall status is 0 or 1??? */

	uint16_t ident:1;
	uint16_t reserved2:4;
	uint16_t fan_speed:11;

	uint8_t hot_swap:1;
	uint8_t fail:1;
	uint8_t rqsted_on:1;
	uint8_t off:1;
	uint8_t reserved3:1;
	uint8_t speed_code:3;
} __attribute__((packed));

struct fan_set {
	struct fan_status power_supply;
	struct fan_status fan_element[4];
};

struct power_supply_status {
	/* from 4.3.14, 4.3.15 */
	struct element_status_byte0 byte0;
	/* Overall status is TBD. */

	uint8_t ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3:4;
	uint8_t dc_over_voltage:1;
	uint8_t dc_under_voltage:1;
	uint8_t dc_over_current:1;
	uint8_t reserved4:1;

	uint8_t hot_swap:1;
	uint8_t fail:1;
	uint8_t rqsted_on:1;
	uint8_t off:1;
	uint8_t ovrtmp_fail:1;
	uint8_t temp_warn:1;
	uint8_t ac_fail:1;
	uint8_t dc_fail:1;
};

struct voltage_sensor_status {
	/* from 4.3.16, 4.3.17 */
	struct element_status_byte0 byte0;
	/* Overall status is TBD. */

	uint8_t ident:1;
	uint8_t reserved2:3;
	uint8_t warn_over:1;
	uint8_t warn_under:1;
	uint8_t crit_over:1;
	uint8_t crit_under:1;

	int16_t voltage;
};

struct voltage_sensor_set {
	struct voltage_sensor_status sensor_12V;
	struct voltage_sensor_status sensor_3_3VA;
};

struct sas_connector_status {
	/* from 4.3.18, 4.3.19 */
	struct element_status_byte0 byte0;

	uint8_t ident:1;
	uint8_t connector_type:7;	/* = 5 */

	uint8_t connector_physical_link;	/* = 0xff */

	uint8_t reserved2:1;
	uint8_t fail:1;
	uint8_t reserved3:6;
};

struct scc_controller_overall_status {
	/* from 4.3.20 */
	struct element_status_byte0 byte0;

	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t reserved2:6;

	uint8_t reserved3;

	uint8_t reserved4;
};

struct scc_controller_element_status {
	/* from 4.3.21 */
	struct element_status_byte0 byte0;

	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t reserved2:6;

	uint8_t reserved3:7;
	uint8_t report:1;

	uint8_t reserved4;
};

struct midplane_status {
	/* from 4.3.22, 4.3.23 */
	struct element_status_byte0 byte0;

	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t reserved2:6;

	uint8_t reserved3;

	uint8_t reserved4;
};

/*
 * Note: This data structure matches the layout described in v0.7 of
 * the Bluehawk SAS Expander Specification, Table 4.16, ... Status
 * Diagnostic Page, and the element counts in Table 4.15, Configuration
 * Diagnostic Page.
 */
struct bluehawk_diag_page2 {
	uint8_t page_code;

	uint8_t reserved1:3;
	uint8_t invop:1;
	uint8_t info:1;
	uint8_t non_crit:1;
	uint8_t crit:1;
	uint8_t unrecov:1;

	uint16_t page_length;

	uint32_t generation_code;

	struct disk_status overall_disk_status;
	struct disk_status disk_status[NR_DISKS_PER_BLUEHAWK];

	struct enclosure_status overall_enclosure_status;
	struct enclosure_status enclosure_element_status;

	struct esm_status overall_esm_status;
	struct esm_status esm_status[2];	/* L and R, AKA A and B */

	struct temperature_sensor_status overall_temp_sensor_status;
	struct temperature_sensor_set temp_sensor_sets[2];	/* L and R */

	struct fan_status overall_fan_status;
	struct fan_set fan_sets[2];	/* L and R */

	struct power_supply_status overall_power_status;
	struct power_supply_status ps_status[2];	/* PS0=L, PS1=R */

	struct voltage_sensor_status overall_voltage_status;
	struct voltage_sensor_set voltage_sensor_sets[2];	/* PS0, PS1 */

	struct sas_connector_status overall_sas_connector_status;
	struct sas_connector_status sas_connector_status[4];	/* 1L,2L,1R,2R*/

	struct scc_controller_overall_status overall_scc_controller_status;
	struct scc_controller_element_status scc_controller_status[2]; /* L, R*/

	struct midplane_status overall_midplane_status;
	struct midplane_status midplane_element_status;
} __attribute__((packed));

/* Diagnostic page 2 layout for SEND DIAGNOSTIC command */

struct common_ctrl {
	uint8_t select:1;
	uint8_t prdfail:1;
	uint8_t disable:1;
	uint8_t rst_swap:1;
	uint8_t reserved1:4;
};

struct disk_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t reserved2;

	uint8_t reserved3:1;
	uint8_t do_not_remove:1;
	uint8_t reserved4:1;
	uint8_t rqst_missing:1;
	uint8_t rqst_insert:1;
	uint8_t rqst_remove:1;
	uint8_t rqst_ident:1;
	uint8_t reserved5:1;

	uint8_t reserved6:2;
	uint8_t rqst_fail:1;	/* AKA rqst_fault */
	uint8_t device_off:1;
	uint8_t enable_byp_a:1;
	uint8_t enable_byp_b:1;
	uint8_t reserved7:2;
};

struct enclosure_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3;

	uint8_t reserved4:6;
	uint8_t rqst_fail:1;
	uint8_t rqst_warn:1;
};

struct esm_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t rqst_fail:1;
	uint8_t reserved2:6;

	uint8_t reserved3:7;
	uint8_t select_element:1;

	uint8_t reserved4;
};

struct fan_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3;

	uint8_t reserved4:1;
	uint8_t rqst_fail:1;
	uint8_t rqst_on:1;
	uint8_t reserved5:2;
	uint8_t requested_speed_code:3;
};

struct fan_ctrl_set {
	struct fan_ctrl power_supply;
	struct fan_ctrl fan_element[4];
};

struct power_supply_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3;

	uint8_t reserved4:1;
	uint8_t rqst_fail:1;
	uint8_t reserved5:6;
};

/* Same format as power_supply_ctrl */
struct sas_connector_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3;

	uint8_t reserved4:1;
	uint8_t rqst_fail:1;
	uint8_t reserved5:6;
};

struct scc_controller_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t rqst_fail:1;
	uint8_t reserved2:6;

	uint16_t reserved3;
};

/* Same format as scc_controller_ctrl */
struct midplane_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t rqst_fail:1;
	uint8_t reserved2:6;

	uint16_t reserved3;
};

struct bluehawk_ctrl_page2 {
	uint8_t page_code;

	uint8_t reserved1:4;
	uint8_t info:1;
	uint8_t non_crit:1;
	uint8_t crit:1;
	uint8_t unrecov:1;

	uint16_t page_length;

	uint32_t generation_code;

	struct disk_ctrl overall_disk_ctrl;
	struct disk_ctrl disk_ctrl[NR_DISKS_PER_BLUEHAWK];

	struct enclosure_ctrl overall_enclosure_ctrl;
	struct enclosure_ctrl enclosure_element_ctrl;

	struct esm_ctrl overall_esm_ctrl;
	struct esm_ctrl esm_ctrl[2];	/* L and R, AKA A and B */

	unsigned char temperature_sensor_ctrl[60]; /* per spec 56, all zeroes */

	struct fan_ctrl overall_fan_ctrl;
	struct fan_ctrl_set fan_sets[2];	/* L and R */

	struct power_supply_ctrl overall_power_ctrl;
	struct power_supply_ctrl ps_ctrl[2];	/* PS0=L, PS1=R */

	unsigned char voltage_sensor_ctrl[20];	/* all zeroes */

	struct sas_connector_ctrl overall_sas_connector_ctrl;
	struct sas_connector_ctrl sas_connector_ctrl[4]; /* 1L,1R,2L,2R */

	struct scc_controller_ctrl overall_scc_controller_ctrl;
	struct scc_controller_ctrl scc_controller_ctrl[2]; /* L, R */

	struct midplane_ctrl overall_midplane_ctrl;
	struct midplane_ctrl midplane_element_ctrl;
} __attribute__((packed));

/* Obtains VPD for esm (page 1), midplane (page 5) via INQUIRY command */
struct vpd_page {
	uint8_t peripheral_qualifier;
	uint8_t	page_code;
	uint8_t reserved1;
	uint8_t page_length;
	uint8_t ascii_length;
	char fn[3];		/* "FN " */
	char fru_number[8];
	uint8_t reserved2;
	char sn[3];		/* "SN " */
	char serial_number[12];
	uint8_t reserved3;
	char cc[3];		/* "CC " */
	char model_number[4];	/* CCIN */
	uint8_t reserved4;
	char fl[3];		/* "FL " */
	char fru_label[5];
	uint8_t reserved5;
};

struct power_supply_descriptor {
	uint16_t descriptor_length;
	char fn[3];		/* "FN " */
	char fru_number[8];
	char sn[3];		/* "SN " */
	char serial_number[12];
	char vendor[12];
	char date_of_manufacture[12];
	char fl[3];		/* "FL " */
	char fru_label[5];
} __attribute__((packed));

/* Obtains VPD for power supplies (page 7) via RECEIVE_DIAGNOSTIC command */
struct element_descriptor_page {
	char ignored1[1074];
	struct power_supply_descriptor ps0_vpd;
	uint16_t reserved;
	struct power_supply_descriptor ps1_vpd;
	char ignored2[137];
} __attribute__((packed));

#endif /* __BLUEHAWK_H__ */
