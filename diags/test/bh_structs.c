/* Copyright (C) 2009, 2012 IBM Corporation */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "bluehawk.h"

#define szp(x) printf(#x " %zu\n", sizeof(struct x))
#define ofs(m) printf(#m " %lu\n", (unsigned long) &((struct bluehawk_diag_page2*)0)->m)
#define ofc(m) printf(#m " %lu\n", (unsigned long) &((struct bluehawk_ctrl_page2*)0)->m)

/* dump bhluehawk element structure details */
int
main()
{
	szp(element_status_byte0);
	szp(overall_disk_status_byte1);
	szp(disk_element_status_byte1);
	szp(disk_status);
	szp(enclosure_status);
	szp(esm_status);
	szp(temperature_sensor_status);
	szp(temperature_sensor_set);
	szp(fan_status);
	szp(fan_set);
	szp(power_supply_status);
	szp(voltage_sensor_status);
	szp(voltage_sensor_set);
	szp(sas_connector_status);
	szp(scc_controller_overall_status);
	szp(scc_controller_element_status);
	szp(midplane_status);
	szp(bluehawk_diag_page2);

	printf("\n");

	ofs(overall_disk_status);
	ofs(disk_status);
	ofs(overall_enclosure_status);
	ofs(enclosure_element_status);
	ofs(overall_esm_status);
	ofs(esm_status);
	ofs(overall_temp_sensor_status);
	ofs(temp_sensor_sets);
	ofs(overall_fan_status);
	ofs(fan_sets);
	ofs(overall_power_status);
	ofs(ps_status);
	ofs(overall_voltage_status);
	ofs(voltage_sensor_sets);
	ofs(overall_sas_connector_status);
	ofs(sas_connector_status);
	ofs(overall_scc_controller_status);
	ofs(scc_controller_status);
	ofs(overall_midplane_status);
	ofs(midplane_element_status);

	printf("\n");

	szp(common_ctrl);
	szp(disk_ctrl);
	szp(enclosure_ctrl);
	szp(esm_ctrl);
	szp(fan_ctrl);
	szp(fan_ctrl_set);
	szp(power_supply_ctrl);
	szp(sas_connector_ctrl);
	szp(scc_controller_ctrl);
	szp(midplane_ctrl);
	szp(bluehawk_ctrl_page2);

	printf("\n");

	ofc(overall_disk_ctrl);
	ofc(disk_ctrl);
	ofc(overall_enclosure_ctrl);
	ofc(enclosure_element_ctrl);
	ofc(overall_esm_ctrl);
	ofc(esm_ctrl);
	ofc(temperature_sensor_ctrl);
	ofc(overall_fan_ctrl);
	ofc(fan_sets);
	ofc(overall_power_ctrl);
	ofc(ps_ctrl);
	ofc(voltage_sensor_ctrl);
	ofc(overall_sas_connector_ctrl);
	ofc(sas_connector_ctrl);
	ofc(overall_scc_controller_ctrl);
	ofc(scc_controller_ctrl);
	ofc(overall_midplane_ctrl);
	ofc(midplane_element_ctrl);

	exit(0);
}
