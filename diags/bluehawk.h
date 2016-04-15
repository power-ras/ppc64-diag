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

#ifndef __BLUEHAWK_H__
#define __BLUEHAWK_H__

#include <stdint.h>
#include <asm/byteorder.h>

#include "encl_common.h"

#define NR_DISKS_PER_BLUEHAWK 30

/*
 * Common structures across enclosure are defined in encl_common.h
 * Reference : Bluehawk SAS Expander Specification v0.7
 *   - disk_status - table 4.3.5 and 4.3.6
 *   - enclosure_status - table 4.3.7
 *   - esm_status - table 4.3.8, 4.3.9
 *   - temperature_sensor_status - table 4.3.10, 4.3.11
 *   - fan_status - table 4.3.12, 4.3.13
 *   - power_supply_status - table 4.3.14, 4.3.15
 *   - voltage_sensor_status - table 4.3.16, 4.3.17
 */


struct temperature_sensor_set {
	struct temperature_sensor_status croc;
	struct temperature_sensor_status ppc;
	struct temperature_sensor_status expander;
	struct temperature_sensor_status ambient[2];
	struct temperature_sensor_status power_supply[2];
};

struct fan_set {
	struct fan_status power_supply;
	struct fan_status fan_element[4];
};

struct voltage_sensor_set {
	struct voltage_sensor_status sensor_12V;
	struct voltage_sensor_status sensor_3_3VA;
};

struct sas_connector_status {
	/* from 4.3.18, 4.3.19 */
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t ident:1;
	uint8_t connector_type:7;	/* = 5 */

	uint8_t connector_physical_link;	/* = 0xff */

	uint8_t reserved2:1;
	uint8_t fail:1;
	uint8_t reserved3:6;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t connector_type:7;	/* = 5 */
	uint8_t ident:1;

	uint8_t connector_physical_link;	/* = 0xff */

	uint8_t reserved3:6;
	uint8_t fail:1;
	uint8_t reserved2:1;
#endif
};

struct scc_controller_overall_status {
	/* from 4.3.20 */
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

	uint8_t reserved3;

	uint8_t reserved4;
};

struct scc_controller_element_status {
	/* from 4.3.21 */
	struct element_status_byte0 byte0;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t reserved2:6;

	uint8_t reserved3:7;
	uint8_t report:1;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:6;
	uint8_t fail:1;
	uint8_t ident:1;

	uint8_t report:1;
	uint8_t reserved3:7;
#endif

	uint8_t reserved4;
};

struct midplane_status {
	/* from 4.3.22, 4.3.23 */
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

struct disk_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t reserved2;

#if defined (__BIG_ENDIAN_BITFIELD)
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

#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved5:1;
	uint8_t rqst_ident:1;
	uint8_t rqst_remove:1;
	uint8_t rqst_insert:1;
	uint8_t rqst_missing:1;
	uint8_t reserved4:1;
	uint8_t do_not_remove:1;
	uint8_t reserved3:1;

	uint8_t reserved7:2;
	uint8_t enable_byp_b:1;
	uint8_t enable_byp_a:1;
	uint8_t device_off:1;
	uint8_t rqst_fail:1;	/* AKA rqst_fault */
	uint8_t reserved6:2;
#endif
};

struct fan_ctrl_set {
	struct fan_ctrl power_supply;
	struct fan_ctrl fan_element[4];
};

/* Same format as power_supply_ctrl */
struct sas_connector_ctrl {
	struct common_ctrl common_ctrl;

#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t rqst_ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3;

	uint8_t reserved4:1;
	uint8_t rqst_fail:1;
	uint8_t reserved5:6;

#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t reserved2:7;
	uint8_t rqst_ident:1;

	uint8_t reserved3;

	uint8_t reserved5:6;
	uint8_t rqst_fail:1;
	uint8_t reserved4:1;
#endif
};

struct scc_controller_ctrl {
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

/* Same format as scc_controller_ctrl */
struct midplane_ctrl {
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

struct bluehawk_ctrl_page2 {
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

/* Obtains VPD for power supplies (page 7) via RECEIVE_DIAGNOSTIC command */
struct element_descriptor_page {
	char ignored1[1074];
	struct power_supply_descriptor ps0_vpd;
	uint16_t reserved;
	struct power_supply_descriptor ps1_vpd;
	char ignored2[137];
} __attribute__((packed));

#endif /* __BLUEHAWK_H__ */
