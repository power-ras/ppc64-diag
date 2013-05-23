#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include "bluehawk.h"

extern enum element_status_code add_element_status(enum element_status_code cur,
				const struct element_status_byte0 *byte0);
extern enum element_status_code composite_status(const void* first_element,
								int nel);
extern enum element_status_code roll_up_disk_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code roll_up_esm_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code roll_up_temperature_sensor_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code roll_up_fan_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code roll_up_power_supply_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code roll_up_voltage_sensor_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code roll_up_sas_connector_status(
					const struct bluehawk_diag_page2 *pg);
/* Is this valid? */
extern enum element_status_code roll_up_scc_controller_status(
					const struct bluehawk_diag_page2 *pg);
extern unsigned int mean_temperature(const struct bluehawk_diag_page2 *pg);
extern int write_page2_to_file(const struct bluehawk_diag_page2 *pg,
							const char *path);

#endif	/* __TEST_UTILS_H__ */
