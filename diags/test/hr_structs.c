/* Copyright (C) 2009, 2016 IBM Corporation */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "encl_common.h"
#include "homerun.h"

#define szp(x) printf(#x " %zu\n", sizeof(struct x))
#define ofs(m) printf(#m " %lu\n", (unsigned long) &((struct hr_diag_page2 *)0)->m)
#define ofc(m) printf(#m " %lu\n", (unsigned long) &((struct hr_ctrl_page2 *)0)->m)

/* dump homerun element structure details */
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
	szp(hr_temperature_sensor_set);
	szp(fan_status);
	szp(hr_fan_set);
	szp(power_supply_status);
	szp(voltage_sensor_status);
	szp(hr_voltage_sensor_set);
	szp(hr_diag_page2);

	printf("\n");

	ofs(overall_disk_status);
	ofs(disk_status);
	ofs(overall_enclosure_status);
	ofs(enclosure_element_status);
	ofs(overall_esm_status);
	ofs(esm_status);
	ofs(temp_sensor_overall_status);
	ofs(temp_sensor_sets);
	ofs(cooling_element_overall_status);
	ofs(fan_sets);
	ofs(power_supply_overall_status);
	ofs(ps_status);
	ofs(voltage_sensor_overall_status);
	ofs(voltage_sensor_sets);

	printf("\n");

	szp(common_ctrl);
	szp(hr_disk_ctrl);
	szp(enclosure_ctrl);
	szp(esm_ctrl);
	szp(fan_ctrl);
	szp(hr_fan_ctrl_set);
	szp(power_supply_ctrl);
	szp(hr_ctrl_page2);

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
	ofc(overall_power_supply_ctrl);
	ofc(ps_ctrl);
	ofc(voltage_sensor_ctrl);

	exit(0);
}
