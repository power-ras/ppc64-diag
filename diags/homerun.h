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

#ifndef __HOMERUN_H__
#define __HOMERUN_H__

#include <stdint.h>
#include <asm/byteorder.h>

#include "encl_common.h"

/*
 * Home Run contains:
 *   - 2 ESM
 *   - 24 disks
 *   - 2 power supply
 *   - 1 fan element/power supply containing 8 fan elements
 *   - 2 voltage sensors
 *   - 2 temperature sensor containing 3 elements
 */
#define HR_NR_ESM_CONTROLLERS		2
#define	HR_NR_DISKS			24
#define	HR_NR_POWER_SUPPLY		2
#define HR_NR_FAN_SET			2
#define HR_NR_FAN_ELEMENT_PER_SET	8
#define HR_NR_VOLTAGE_SENSOR_SET	2
#define HR_NR_TEMP_SENSOR_SET		2
#define HR_NR_TEMP_SENSORS_PER_SET	3


/* 8 cooling element */
struct hr_fan_set {
	struct fan_status fan_element[HR_NR_FAN_ELEMENT_PER_SET];
};

/* 2 temperature controller and 3 temperature sensor/controller */
struct hr_temperature_sensor_set {
	struct temperature_sensor_status controller;
	struct temperature_sensor_status power_supply[HR_NR_TEMP_SENSORS_PER_SET];
};

/* Voltage sensors status. */
struct hr_voltage_sensor_set {
	struct voltage_sensor_status sensor_12V;
	struct voltage_sensor_status sensor_5V;
	struct voltage_sensor_status sensor_5VA;
};

/*
 * Status diagnostics page
 *
 * Note: This data structure matches the layout described in v1.1
 *       of the Home Run Drive Enclosure Specification, Table 5.16.
 */
struct hr_diag_page2 {
	/* byte 0 */
	uint8_t page_code;	/* 0x02 */

	/* byte 1 */
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

	/* byte 2-3 */
	uint16_t page_length;	/* 0x10C */

	/* byte 4-7 */
	uint32_t generation_code;	/* 0x00000000 */

	/* Disk */
	struct disk_status overall_disk_status;
	struct disk_status disk_status[HR_NR_DISKS];

	/* Enclosure */
	struct enclosure_status overall_enclosure_status;
	struct enclosure_status enclosure_element_status;

	/* ESM Electronics */
	struct esm_status overall_esm_status;
	struct esm_status esm_status[HR_NR_ESM_CONTROLLERS];

	/* Temperature Sensor */
	struct temperature_sensor_status temp_sensor_overall_status;
	/* A and B */
	struct hr_temperature_sensor_set temp_sensor_sets[HR_NR_TEMP_SENSOR_SET];

	/* Cooling element */
	struct fan_status cooling_element_overall_status;
	/* L & R cooling element */
	struct hr_fan_set fan_sets[HR_NR_FAN_SET];

	/* Power Supply */
	struct power_supply_status power_supply_overall_status;
	/* PS0(L) and PS1(R) */
	struct power_supply_status ps_status[HR_NR_POWER_SUPPLY];

	/* Voltage Sensor */
	struct voltage_sensor_status voltage_sensor_overall_status;
	/* PS0, PS1 */
	struct hr_voltage_sensor_set voltage_sensor_sets[HR_NR_VOLTAGE_SENSOR_SET];
} ;


/*
 * Holds VPD for Power Supplies (page 7) got via RECEIVE DIAGNOSTICS command
 * with a PCV bit set to one and a PAGE CODE field set to 07h.
 *
 * This data structure holds a list of variable-length ASCII strings, one
 * for each element in the Configuration diagnostic page.
 *
 * The Element Descriptor diagnostic page is read by the RECEIVE DIAGNOSTIC
 * RESULTS command with a PCV bit set to one and a PAGE CODE field set to 07h.
 * Table 5.37 defines the Element Descriptor diagnostic page.
 */
struct hr_element_descriptor_page {
	/**
	 * Note: Deviation from spec V0.7. As per spec, Power supply
	 * descriptor starts at offset 638, but actual offset is 642.
	 */
	char ignored1[642];
	struct power_supply_descriptor ps0_vpd;
	uint16_t reserved;	/* bytes 698-699 */
	struct power_supply_descriptor ps1_vpd;
	char ignored2[28];
} ;


/* --- Diagnostic page 2 layout for SEND DIAGNOSTIC command --- */

/*
 * Device element manages a SCSI device.
 * Please see Table 5.48 from v1.1 of the Home Run Drive
 * Enclosure Specification.
 */
struct hr_disk_ctrl {
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
	uint8_t enable_bypa:1;
	uint8_t enable_bypb:1;
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
	uint8_t enable_bypb:1;
	uint8_t enable_bypa:1;
	uint8_t device_off:1;
	uint8_t rqst_fail:1;
	uint8_t reserved5:2;
#endif
};

struct hr_fan_ctrl_set {
	struct fan_ctrl fan_element[HR_NR_FAN_ELEMENT_PER_SET];
};

/*
 * This data structure implements the Home Run SAS Enclosure Control
 * diagnostic page. Please check Table 5.46 from v1.1 of the Home Run
 * Drive Enclosure Specification.
 */
struct hr_ctrl_page2 {
	/* byte 1 */
	uint8_t page_code;	/* 0x02 */

	/* byte 2 */
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

	/* byte 3 */
	uint16_t page_length;	/* 0x10c */

	/* byte 4-7 */
	uint32_t generation_code;	/* 0x0000 */

	/* Disk element status - byte 8-11 */
	struct hr_disk_ctrl overall_disk_ctrl;
	/* byte 12-107 */
	struct hr_disk_ctrl disk_ctrl[HR_NR_DISKS];

	/* Enclosure status - byte 108-111 */
	struct enclosure_ctrl overall_enclosure_ctrl;
	/* byte 112-115 */
	struct enclosure_ctrl enclosure_element_ctrl;

	/* ESM electronics status - byte 116-119 */
	struct esm_ctrl overall_esm_ctrl;
	/* for A and B */
	struct esm_ctrl esm_ctrl[HR_NR_ESM_CONTROLLERS];

	/* Temperature sensor */
	unsigned char temperature_sensor_ctrl[36]; /* all zeros */

	/* bytes 168-231 */
	struct fan_ctrl overall_fan_ctrl;
	/* L & R */
	struct hr_fan_ctrl_set fan_sets[HR_NR_FAN_SET];

	/* Power supply */
	struct power_supply_ctrl overall_power_supply_ctrl;
	/* 0L & 1R */
	struct power_supply_ctrl ps_ctrl[HR_NR_POWER_SUPPLY];

	unsigned char voltage_sensor_ctrl[28];	/* all zeroes */
} ;

#endif /* __HOMERUN_H__ */
