#include "encl_common.h"
#include "slider.h"

/*
 * Note: Initializing struct members to zero is not strictly necessary,
 * but we explicitly initialize all members that can take on meaningful
 * values. All other members are unsupported by the SES implementation.
 */

/*
 * Slider Specification : V0.2
 * Date : 7/05/2016
 * Page reference : Table 5.16 Slider SAS Enclosure Status diagnostic page
 */
#define SLIDER_LFF_PAGE_LENGTH	(sizeof(struct slider_lff_diag_page2) - 4)
#define SLIDER_SFF_PAGE_LENGTH	(sizeof(struct slider_sff_diag_page2) - 4)

#define HEALTHY_STATUS_BYTE0 { .swap = 0, .status = 1 }

#define HEALTHY_DISK(n) { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.slot_address = n, \
	.app_client_bypassed_a = 0, \
	.app_client_bypassed_b = 0, \
	.enclosure_bypassed_a = 0, \
	.enclosure_bypassed_b = 0, \
	.device_bypassed_a = 0, \
	.device_bypassed_b = 0, \
	.bypassed_a = 0, \
	.bypassed_b = 0, \
	.ident = 0, \
	.fail = 0, \
}

#define HEALTHY_POWER_SUPPLY { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.dc_over_voltage = 0, \
	.dc_under_voltage = 0, \
	.dc_over_current = 0, \
	.fail = 0, \
	.ovrtmp_fail = 0, \
	.ac_fail = 0, \
	.dc_fail = 0, \
}

#define HEALTHY_ESC { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.fail = 0, \
	.reset_reason = 0, \
	.vpd_read_fail = 0, \
	.vpd_mismatch = 0, \
	.vpd_mirror_mismatch = 0, \
	.firm_img_unmatch = 0, \
	.ierr_asserted = 0, \
	.xcc_data_initialised = 0, \
	.report = 0, \
	.hot_swap = 0, \
}

#define HEALTHY_ENCLOSURE { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.fail = 0, \
}

#define FAN_SPEED_MSB	4
#define FAN_SPEED_LSB	123
#define HEALTHY_FAN(speed_msb, speed_lsb) { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.fail = 0, \
	.fan_speed_msb = speed_msb, \
	.fan_speed_lsb = speed_lsb \
}
#define HEALTHY_FAN_SET(speed_msb, speed_lsb) { \
	.fan_element = { \
		HEALTHY_FAN(speed_msb, speed_lsb), \
		HEALTHY_FAN(speed_msb, speed_lsb), \
		HEALTHY_FAN(speed_msb, speed_lsb), \
		HEALTHY_FAN(speed_msb, speed_lsb), \
		HEALTHY_FAN(speed_msb, speed_lsb), \
		HEALTHY_FAN(speed_msb, speed_lsb), \
		HEALTHY_FAN(speed_msb, speed_lsb), \
		HEALTHY_FAN(speed_msb, speed_lsb), \
	} \
}

#define ROOM_TEMPERATURE	20
#define TEMPERATURE_OFFSET	20
#define HEALTHY_TEMP_SENSOR(temp) { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.temperature = (temp + TEMPERATURE_OFFSET), \
	.ot_failure = 0, \
	.ot_warning = 0, \
	.ut_failure = 0, \
	.ut_warning = 0 \
}

#define ROOM_TEMP_SENSOR	HEALTHY_TEMP_SENSOR(ROOM_TEMPERATURE)
#define HEALTHY_TEMP_SENSOR_SET { \
	.temp_enclosure = ROOM_TEMP_SENSOR, \
	.temp_psu = { \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR, \
	}, \
	.temp_esc = { \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR, \
	} \
}

#define HEALTHY_VOLTAGE_3V3	350
#define HEALTHY_VOLTAGE_1V0	100
#define HEALTHY_VOLTAGE_1V8	180
#define HEALTHY_VOLTAGE_0V92	90
#define HEALTHY_VOLTAGE_SENSOR(volts) { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.warn_over = 0, \
	.warn_under = 0, \
	.crit_over = 0, \
	.crit_under = 0, \
	.voltage = volts \
}
#define HEALTHY_VOLTAGE_SENSOR_SET { \
	HEALTHY_VOLTAGE_SENSOR(HEALTHY_VOLTAGE_3V3), \
	HEALTHY_VOLTAGE_SENSOR(HEALTHY_VOLTAGE_1V0), \
	HEALTHY_VOLTAGE_SENSOR(HEALTHY_VOLTAGE_1V8), \
	HEALTHY_VOLTAGE_SENSOR(HEALTHY_VOLTAGE_0V92) \
}

#define HEALTHY_SAS_EXPANDER { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.fail = 0 \
}

#define HEALTHY_SAS_CONNECTOR { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.connector_physical_link = 0xff, \
	.fail = 0 \
}

#define HEALTHY_MIDPLANE(stat) { \
	.byte0 = { .swap = 0, .status = stat }, \
	.vpd1_read_fail1 = 0, \
	.vpd1_read_fail2 = 0, \
	.vpd2_read_fail1 = 0, \
	.vpd2_read_fail2 = 0, \
	.vpd_mirror_mismatch = 0, \
}

#define HEALTHY_PHY { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.enable = 1,\
	.connect = 1,\
}

#define HEALTHY_PHY_SET { \
	.phy_element = { \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
		HEALTHY_PHY, \
	} \
}

#define HEALTHY_SSB { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.buffer_status = 0 \
}

#define HEALTHY_CPLD { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.fail = 0 \
}


#define HEALTHY_INPUT_POWER(value) { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.input_power = value \
}

struct slider_lff_diag_page2 healthy_page = {
	.page_code = 2,
	.non_crit = 0,
	.crit = 0,
	.page_length = SLIDER_LFF_PAGE_LENGTH,
	.generation_code = 0,

	.overall_disk_status = {
		.byte0 = HEALTHY_STATUS_BYTE0
	},
	.disk_status = {
		[0] = HEALTHY_DISK(0),
		[1] = HEALTHY_DISK(1),
		[2] = HEALTHY_DISK(2),
		[3] = HEALTHY_DISK(3),
		[4] = HEALTHY_DISK(4),
		[5] = HEALTHY_DISK(5),
		[6] = HEALTHY_DISK(6),
		[7] = HEALTHY_DISK(7),
		[8] = HEALTHY_DISK(8),
		[9] = HEALTHY_DISK(9),
		[10] = HEALTHY_DISK(10),
		[11] = HEALTHY_DISK(11)
	},
	.overall_power_status = HEALTHY_POWER_SUPPLY,
	.ps_status = {
		HEALTHY_POWER_SUPPLY,
		HEALTHY_POWER_SUPPLY
	},
	.overall_fan_status = HEALTHY_FAN(FAN_SPEED_MSB, FAN_SPEED_LSB),
	.fan_sets = {
		HEALTHY_FAN_SET(FAN_SPEED_MSB, FAN_SPEED_LSB),
		HEALTHY_FAN_SET(FAN_SPEED_MSB, FAN_SPEED_LSB),
	},
	.overall_temp_sensor_status = ROOM_TEMP_SENSOR,
	.temp_sensor_sets = HEALTHY_TEMP_SENSOR_SET,
	.overall_enc_service_ctrl_status = HEALTHY_ESC,
	.enc_service_ctrl_element = {
		HEALTHY_ESC,
		HEALTHY_ESC
	},
	.overall_encl_status = HEALTHY_ENCLOSURE,
	.encl_element = HEALTHY_ENCLOSURE,
	.overall_voltage_status = HEALTHY_VOLTAGE_SENSOR(0),
	.voltage_sensor_sets = {
		HEALTHY_VOLTAGE_SENSOR_SET,
		HEALTHY_VOLTAGE_SENSOR_SET
	},
	.overall_sas_expander_status = HEALTHY_SAS_EXPANDER,
	.sas_expander_element = {
		HEALTHY_SAS_EXPANDER,
		HEALTHY_SAS_EXPANDER,
	},
	.overall_sas_connector_status = HEALTHY_SAS_CONNECTOR,
	.sas_connector_status = {
		HEALTHY_SAS_CONNECTOR,
		HEALTHY_SAS_CONNECTOR,
		HEALTHY_SAS_CONNECTOR,
		HEALTHY_SAS_CONNECTOR,
		HEALTHY_SAS_CONNECTOR,
		HEALTHY_SAS_CONNECTOR
	},
	.overall_midplane_status = HEALTHY_MIDPLANE(0),
	.midplane_element_status = HEALTHY_MIDPLANE(1),

	.overall_phy_status = HEALTHY_PHY,
	.phy_sets = {
		HEALTHY_PHY_SET,
		HEALTHY_PHY_SET,
	},
	.overall_ssb_status = HEALTHY_SSB,
	.ssb_element = {
		HEALTHY_SSB,
		HEALTHY_SSB,
	},
	.overall_cpld_status = HEALTHY_CPLD,
	.cpld_element = {
		HEALTHY_CPLD,
		HEALTHY_CPLD,
	},
	.overall_input_power_status = HEALTHY_INPUT_POWER(5),
	.input_power_element = {
		HEALTHY_INPUT_POWER(5),
		HEALTHY_INPUT_POWER(5),
		HEALTHY_INPUT_POWER(5),
		HEALTHY_INPUT_POWER(5),
	},
};
