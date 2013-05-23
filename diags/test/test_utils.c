#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "bluehawk.h"

/*
 * Factor byte0->status into the composite status cur.  A missing element
 * (ES_NOT_INSTALLED) is ignored.  A non-critical status is less severe
 * than critical.  Otherwise assume that an increasing value of
 * element_status_code indicates and increasing severity.  Return the more
 * severe of byte0->status and cur.
 */
enum element_status_code
add_element_status(enum element_status_code cur,
				const struct element_status_byte0 *byte0)
{
	enum element_status_code s = (enum element_status_code) byte0->status;

	if (s == ES_OK || s == ES_NOT_INSTALLED)
		return cur;
	if ((cur == ES_OK || cur == ES_NONCRITICAL) && s > ES_OK)
		return s;
	return cur;
}

/*
 * Calculate the composite status for the nel elements starting at
 * address first_element.  We exploit the fact that every status element
 * is 4 bytes and starts with an element_status_byte0 struct.
 */
enum element_status_code
composite_status(const void* first_element, int nel)
{
	int i;
	const char *el = (const char*) first_element;
	enum element_status_code s = ES_OK;

	for (i = 0; i < nel; i++, el += 4)
		s = add_element_status(s,
			(const struct element_status_byte0*) el);
	return s;
}

enum element_status_code
roll_up_disk_status(const struct bluehawk_diag_page2 *pg)
{
	return composite_status(&pg->disk_status, NR_DISKS_PER_BLUEHAWK);
}

enum element_status_code
roll_up_esm_status(const struct bluehawk_diag_page2 *pg)
{
	return composite_status(&pg->esm_status, 2);
}

enum element_status_code
roll_up_temperature_sensor_status(const struct bluehawk_diag_page2 *pg)
{
	return composite_status(&pg->temp_sensor_sets, 2 * 7);
}

enum element_status_code
roll_up_fan_status(const struct bluehawk_diag_page2 *pg)
{
	return composite_status(&pg->fan_sets, 2 * 5);
}

enum element_status_code
roll_up_power_supply_status(const struct bluehawk_diag_page2 *pg)
{
	return composite_status(&pg->ps_status, 2);
}

enum element_status_code
roll_up_voltage_sensor_status(const struct bluehawk_diag_page2 *pg)
{
	return composite_status(&pg->voltage_sensor_sets, 2 * 2);
}

enum element_status_code
roll_up_sas_connector_status(const struct bluehawk_diag_page2 *pg)
{
	return composite_status(&pg->sas_connector_status, 4);
}

/* Is this valid? */
enum element_status_code
roll_up_scc_controller_status(const struct bluehawk_diag_page2 *pg)
{
	return composite_status(&pg->scc_controller_status, 2);
}

unsigned int
mean_temperature(const struct bluehawk_diag_page2 *pg)
{
	struct temperature_sensor_status *sensors =
				(struct temperature_sensor_status *)
				&pg->temp_sensor_sets;
	int sum = 0;
	int i;

	for (i = 0; i < 2*7; i++)
		sum += sensors[i].temperature;
	return sum / (2*7);
}

int
write_page2_to_file(const struct bluehawk_diag_page2 *pg, const char *path)
{
	FILE *f;

	f = fopen(path, "w");
	if (!f) {
		perror(path);
		return -1;
	}
	if (fwrite(pg, sizeof(*pg), 1, f) != 1) {
		perror(path);
		fclose(f);
		return -2;
	}
	fclose(f);
	return 0;
}
