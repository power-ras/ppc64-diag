#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bluehawk.h"
#include "test_utils.h"

#define TEMPERATURE_OFFSET 20

extern struct bluehawk_diag_page2 healthy_page;
static struct bluehawk_diag_page2 page;

int
main(int argc, char **argv)
{
	int i;
	struct power_supply_status *ps;
	struct voltage_sensor_status *vs;
	struct fan_status *fs;
	struct temperature_sensor_status *ts;

	if (argc != 2) {
		fprintf(stderr, "usage: %s pathname\n", argv[0]);
		exit(1);
	}
	memcpy(&page, &healthy_page, sizeof(page));

	page.non_crit = 1;

	ps = &page.ps_status[1];
	ps->byte0.status = ES_NONCRITICAL;
	ps->dc_over_voltage = 1;
	ps->fail = 1;
	roll_up_power_supply_status(&page);

	vs = &page.voltage_sensor_sets[1].sensor_12V;
	vs->byte0.status = ES_NONCRITICAL;
	vs->warn_over = 1;
	roll_up_voltage_sensor_status(&page);

	fs = &page.fan_sets[1].power_supply;
	fs->byte0.status = ES_NONCRITICAL;
	fs->fail = 1;
	fs->speed_code = 1;
	roll_up_fan_status(&page);

	for (i = 0; i < 2; i++) {
		ts = &page.temp_sensor_sets[1].power_supply[i];
		ts->byte0.status = ES_NONCRITICAL;
		ts->ot_warning = 1;
		ts->temperature = TEMPERATURE_OFFSET + 60; // 60 deg C
	}
	roll_up_temperature_sensor_status(&page);
	page.overall_temp_sensor_status.temperature = mean_temperature(&page);

	if (write_page2_to_file(&page, argv[1]) != 0)
		exit(2);
	exit(0);
}
