#include <stdlib.h>
#include <stdio.h>

#include "slider.h"
#include "encl_common.h"
#include "encl_util.h"
#include "test_utils.h"

extern struct slider_lff_diag_page2 healthy_page;

/* healthy pg2 for slider lff variant */
int
main(int argc, char **argv)
{
	int i;
	if (argc != 2) {
		fprintf(stderr, "usage: %s pathname\n", argv[0]);
		exit(1);
	}

	convert_htons(&healthy_page.page_length);
	convert_htons(&healthy_page.overall_input_power_status.input_power);
	for (i = 0; i < SLIDER_NR_INPUT_POWER; i++)
		convert_htons(&healthy_page.input_power_element[i].input_power);

	for (i = 0; i < SLIDER_NR_ESC; i++) {
		convert_htons
		(&healthy_page.voltage_sensor_sets[i].sensor_3V3.voltage);
		convert_htons
		(&healthy_page.voltage_sensor_sets[i].sensor_1V0.voltage);
		convert_htons
		(&healthy_page.voltage_sensor_sets[i].sensor_1V8.voltage);
		convert_htons
		(&healthy_page.voltage_sensor_sets[i].sensor_0V92.voltage);
	}

	if (write_page2_to_file(argv[1],
		 &healthy_page, sizeof(healthy_page)) != 0)
		exit(2);
	exit(0);
}
