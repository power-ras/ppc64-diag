#include "bluehawk.h"

/*
 * Note: Initializing struct members to zero is not strictly necessary,
 * but we explicitly initialize all members that can take on meaningful
 * values.  All other members are unsupported by the SES implementation.
 */

#define HEALTHY_STATUS_BYTE0 { .swap = 0, .status = 1 }

#define HEALTHY_DISK(n) { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.byte1.element_status = { .hot_swap = 1, .slot_address = n }, \
	.app_client_bypassed_a = 0, \
	.ready_to_insert = 0, \
	.rmv = 0, \
	.ident = 0, \
	.app_client_bypassed_b = 0, \
	.fail = 0, \
	.bypassed_a = 0, \
	.bypassed_b = 0, \
}

#define HEALTHY_ENCLOSURE { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.fail = 0, \
	.failure_requested = 0 \
}

#define HEALTHY_ESM { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.fail = 0, \
	.hot_swap = 1, \
}

#define ROOM_TEMPERATURE 20
#define TEMPERATURE_OFFSET 20
#define HEALTHY_TEMP_SENSOR(temp) { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.temperature = (temp + TEMPERATURE_OFFSET), \
	.ot_failure = 0, \
	.ot_warning = 0 \
}
#define ROOM_TEMP_SENSOR HEALTHY_TEMP_SENSOR(ROOM_TEMPERATURE)
#define HEALTHY_TEMP_SENSOR_SET { \
	.croc = ROOM_TEMP_SENSOR, \
	.ppc = ROOM_TEMP_SENSOR, \
	.expander = ROOM_TEMP_SENSOR, \
	.ambient = { \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR \
	}, \
	.power_supply = { \
		ROOM_TEMP_SENSOR, \
		ROOM_TEMP_SENSOR \
	} \
}

#define HEALTHY_FAN_SPEED_CODE 0x3	/* Just a guess */
#define HEALTHY_FAN(spdcd) { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.hot_swap = 1, \
	.fail = 0, \
	.speed_code = spdcd \
}
#define HEALTHY_FAN_SET(spdcd) { \
	.power_supply = HEALTHY_FAN(spdcd), \
	.fan_element = { \
		HEALTHY_FAN(spdcd), \
		HEALTHY_FAN(spdcd), \
		HEALTHY_FAN(spdcd), \
		HEALTHY_FAN(spdcd) \
	} \
}

#define HEALTHY_POWER_SUPPLY { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.dc_over_voltage = 0, \
	.dc_under_voltage = 0, \
	.dc_over_current = 0, \
	.hot_swap = 1, \
	.fail = 0, \
	.ovrtmp_fail = 0, \
	.ac_fail = 0, \
	.dc_fail = 0, \
}

#define HEALTHY_VOLTAGE_12V 1200
#define HEALTHY_VOLTAGE_3_3V 350
#define HEALTHY_VOLTAGE_SENSOR(volts) { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.warn_over = 0, \
	.warn_under = 0, \
	.crit_over = 0, \
	.crit_under = 0, \
	.voltage = volts \
}
#define HEALTHY_VOLTAGE_SENSOR_SET { \
	HEALTHY_VOLTAGE_SENSOR(HEALTHY_VOLTAGE_12V), \
	HEALTHY_VOLTAGE_SENSOR(HEALTHY_VOLTAGE_3_3V) \
}

#define HEALTHY_SAS_CONNECTOR { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.connector_type = 5, \
	.connector_physical_link = 0xff, \
	.fail = 0 \
}

#define HEALTHY_SCC_CONTROLLER_OVERALL { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.fail = 0 \
}
#define HEALTHY_SCC_CONTROLLER { \
	.byte0 = HEALTHY_STATUS_BYTE0, \
	.ident = 0, \
	.fail = 0, \
	.report = 0 \
}

#define HEALTHY_MIDPLANE(stat) { \
	.byte0 = { .swap = 0, .status = stat }, \
	.ident = 0, \
	.fail = 0 \
}

struct bluehawk_diag_page2 healthy_page = {
	.page_code = 2,
	.non_crit = 0,
	.crit = 0,
	.page_length = 0x144,
	.generation_code = 0,

	.overall_disk_status = {
		.byte0 = HEALTHY_STATUS_BYTE0,
		.byte1.overall_status = {
			.device_environment = 2,
			.slot_address = 0
		},
		.ready_to_insert = 0,
		.rmv = 0,
		.ident = 0,
		.report = 0,
		.fail = 0
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
		[11] = HEALTHY_DISK(11),
		[12] = HEALTHY_DISK(12),
		[13] = HEALTHY_DISK(13),
		[14] = HEALTHY_DISK(14),
		[15] = HEALTHY_DISK(15),
		[16] = HEALTHY_DISK(16),
		[17] = HEALTHY_DISK(17),
		[18] = HEALTHY_DISK(18),
		[19] = HEALTHY_DISK(19),
		[20] = HEALTHY_DISK(20),
		[21] = HEALTHY_DISK(21),
		[22] = HEALTHY_DISK(22),
		[23] = HEALTHY_DISK(23),
		[24] = HEALTHY_DISK(24),
		[25] = HEALTHY_DISK(25),
		[26] = HEALTHY_DISK(26),
		[27] = HEALTHY_DISK(27),
		[28] = HEALTHY_DISK(28),
		[29] = HEALTHY_DISK(29)
	},
	.overall_enclosure_status = HEALTHY_ENCLOSURE,
	.enclosure_element_status = HEALTHY_ENCLOSURE,
	.overall_esm_status = HEALTHY_ESM,
	.esm_status = {
		HEALTHY_ESM,
		HEALTHY_ESM
	},
	.overall_temp_sensor_status = ROOM_TEMP_SENSOR,
	.temp_sensor_sets = {
		HEALTHY_TEMP_SENSOR_SET,
		HEALTHY_TEMP_SENSOR_SET
	},
	.overall_fan_status = HEALTHY_FAN(0),	// speed code undefined here
	.fan_sets = {
		HEALTHY_FAN_SET(HEALTHY_FAN_SPEED_CODE),
		HEALTHY_FAN_SET(HEALTHY_FAN_SPEED_CODE)
	},
	.overall_power_status = HEALTHY_POWER_SUPPLY,
	.ps_status = {
		HEALTHY_POWER_SUPPLY,
		HEALTHY_POWER_SUPPLY
	},
	.overall_voltage_status = HEALTHY_VOLTAGE_SENSOR(0),
	.voltage_sensor_sets = {
		HEALTHY_VOLTAGE_SENSOR_SET,
		HEALTHY_VOLTAGE_SENSOR_SET
	},
	.overall_sas_connector_status = HEALTHY_SAS_CONNECTOR,
	.sas_connector_status = {
		HEALTHY_SAS_CONNECTOR,
		HEALTHY_SAS_CONNECTOR,
		HEALTHY_SAS_CONNECTOR,
		HEALTHY_SAS_CONNECTOR
	},
	.overall_scc_controller_status = HEALTHY_SCC_CONTROLLER_OVERALL,
	.scc_controller_status = {
		HEALTHY_SCC_CONTROLLER,
		HEALTHY_SCC_CONTROLLER
	},
	.overall_midplane_status = HEALTHY_MIDPLANE(0),
	.midplane_element_status = HEALTHY_MIDPLANE(1)
};
