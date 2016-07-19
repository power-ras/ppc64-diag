/* Copyright (C) 2016 IBM Corporation */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "encl_common.h"
#include "slider.h"

#define szp(x) printf(#x " %zu\n", sizeof(struct x))
#define ofsl(m) printf(#m " %lu\n", (unsigned long) &((struct slider_lff_diag_page2*)0)->m)
#define ofcl(m) printf(#m " %lu\n", (unsigned long) &((struct slider_lff_ctrl_page2*)0)->m)
#define ofss(m) printf(#m " %lu\n", (unsigned long) &((struct slider_sff_diag_page2*)0)->m)
#define ofcs(m) printf(#m " %lu\n", (unsigned long) &((struct slider_sff_ctrl_page2*)0)->m)

/* dump slider element structure details */
int main()
{
	printf("\n Slider LFF/SFF status structure size :\n\n");

	szp(element_status_byte0);
	szp(slider_disk_status);
	szp(power_supply_status);
	szp(fan_status);
	szp(slider_fan_set);
	szp(slider_temperature_sensor_status);
	szp(slider_enc_service_ctrl_status);
	szp(slider_encl_status);
	szp(slider_voltage_sensor_status);
	szp(slider_voltage_sensor_set);
	szp(slider_sas_expander_status);
	szp(slider_sas_connector_status);
	szp(slider_midplane_status);
	szp(slider_phy_status);
	szp(slider_phy_set);
	szp(slider_statesave_buffer_status);
	szp(slider_cpld_status);
	szp(slider_input_power_status);
	szp(slider_lff_diag_page2);
	szp(slider_sff_diag_page2);

	printf("\n");
	printf("\n Slider LFF/SFF cntrl structure size :\n");

	szp(common_ctrl);
	szp(slider_disk_ctrl);
	szp(slider_power_supply_ctrl);
	szp(fan_ctrl);
	szp(slider_fan_ctrl_set);
	szp(slider_temperature_sensor_ctrl);
	szp(slider_temperature_sensor_ctrl_set);
	szp(slider_enc_service_ctrl_ctrl);
	szp(slider_encl_ctrl);
	szp(slider_voltage_sensor_ctrl);
	szp(slider_voltage_sensor_ctrl_set);
	szp(slider_sas_expander_ctrl);
	szp(slider_sas_connector_ctrl);
	szp(slider_midplane_ctrl);
	szp(slider_phy_ctrl);
	szp(slider_phy_ctrl_set);
	szp(slider_statesave_buffer_ctrl);
	szp(slider_cpld_ctrl);
	szp(slider_input_power_ctrl);
	szp(slider_lff_ctrl_page2);
	szp(slider_sff_ctrl_page2);

	printf("\n");
	printf("Slider LFF status element offset :\n");

	ofsl(overall_disk_status);
	ofsl(disk_status);
	ofsl(overall_power_status);
	ofsl(ps_status);
	ofsl(overall_fan_status);
	ofsl(fan_sets);
	ofsl(overall_temp_sensor_status);
	ofsl(temp_sensor_sets);
	ofsl(overall_enc_service_ctrl_status);
	ofsl(enc_service_ctrl_element);
	ofsl(overall_encl_status);
	ofsl(encl_element);
	ofsl(overall_voltage_status);
	ofsl(voltage_sensor_sets);
	ofsl(overall_sas_expander_status);
	ofsl(sas_expander_element);
	ofsl(overall_sas_connector_status);
	ofsl(sas_connector_status);
	ofsl(overall_midplane_status);
	ofsl(midplane_element_status);
	ofsl(overall_phy_status);
	ofsl(phy_sets);
	ofsl(overall_ssb_status);
	ofsl(ssb_element);
	ofsl(overall_cpld_status);
	ofsl(cpld_element);
	ofsl(overall_input_power_status);
	ofsl(input_power_element);

	printf("\n");
	printf("Slider SFF status element offset :\n");

	ofss(overall_disk_status);
	ofss(disk_status);
	ofss(overall_power_status);
	ofss(ps_status);
	ofss(overall_fan_status);
	ofss(fan_sets);
	ofss(overall_temp_sensor_status);
	ofss(temp_sensor_sets);
	ofss(overall_enc_service_ctrl_status);
	ofss(enc_service_ctrl_element);
	ofss(overall_encl_status);
	ofss(encl_element);
	ofss(overall_voltage_status);
	ofss(voltage_sensor_sets);
	ofss(overall_sas_expander_status);
	ofss(sas_expander_element);
	ofss(overall_sas_connector_status);
	ofss(sas_connector_status);
	ofss(overall_midplane_status);
	ofss(midplane_element_status);
	ofss(overall_phy_status);
	ofss(phy_sets);
	ofss(overall_ssb_status);
	ofss(ssb_element);
	ofss(overall_cpld_status);
	ofss(cpld_element);
	ofss(overall_input_power_status);
	ofss(input_power_element);

	printf("\n");
	printf("Slider LFF cntrl element offset :\n");

	ofcl(overall_disk_ctrl);
	ofcl(disk_ctrl);
	ofcl(overall_power_ctrl);
	ofcl(ps_ctrl);
	ofcl(overall_fan_ctrl);
	ofcl(fan_sets);
	ofcl(overall_temp_sensor_ctrl);
	ofcl(temp_sensor_sets);
	ofcl(overall_enc_service_ctrl_ctrl);
	ofcl(enc_service_ctrl_element);
	ofcl(overall_encl_ctrl);
	ofcl(encl_element);
	ofcl(overall_voltage_ctrl);
	ofcl(voltage_sensor_sets);
	ofcl(overall_sas_expander_ctrl);
	ofcl(sas_expander_element);
	ofcl(overall_sas_connector_ctrl);
	ofcl(sas_connector_ctrl);
	ofcl(overall_midplane_ctrl);
	ofcl(midplane_element_ctrl);
	ofcl(overall_phy_ctrl);
	ofcl(phy_sets);
	ofcl(overall_ssb_ctrl);
	ofcl(ssb_elemnet);
	ofcl(overall_cpld_ctrl);
	ofcl(cpld_ctrl_element);
	ofcl(overall_input_power_ctrl);
	ofcl(input_power_ctrl_element);

	printf("\n");
	printf("Slider SFF cntrl element offset :\n");

	ofcs(overall_disk_ctrl);
	ofcs(disk_ctrl);
	ofcs(overall_power_ctrl);
	ofcs(ps_ctrl);
	ofcs(overall_fan_ctrl);
	ofcs(fan_sets);
	ofcs(overall_temp_sensor_ctrl);
	ofcs(temp_sensor_sets);
	ofcs(overall_enc_service_ctrl_ctrl);
	ofcs(enc_service_ctrl_element);
	ofcs(overall_encl_ctrl);
	ofcs(encl_element);
	ofcs(overall_voltage_ctrl);
	ofcs(voltage_sensor_sets);
	ofcs(overall_sas_expander_ctrl);
	ofcs(sas_expander_element);
	ofcs(overall_sas_connector_ctrl);
	ofcs(sas_connector_ctrl);
	ofcs(overall_midplane_ctrl);
	ofcs(midplane_element_ctrl);
	ofcs(overall_phy_ctrl);
	ofcs(phy_sets);
	ofcs(overall_ssb_ctrl);
	ofcs(ssb_elemnet);
	ofcs(overall_cpld_ctrl);
	ofcs(cpld_ctrl_element);
	ofcs(overall_input_power_ctrl);
	ofcs(input_power_ctrl_element);

	exit(0);
}
