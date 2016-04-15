#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <arpa/inet.h>

#include "bluehawk.h"
#include "encl_common.h"
#include "homerun.h"

/* Convert host byte order to network byte order */
static inline void convert_htons(uint16_t *offset)
{
	*offset = htons(*offset);
}

extern enum element_status_code add_element_status(enum element_status_code cur,
				const struct element_status_byte0 *byte0);
extern enum element_status_code composite_status(const void* first_element,
								int nel);

/* bluehawk specific call */
extern enum element_status_code bh_roll_up_disk_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code bh_roll_up_esm_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code bh_roll_up_temperature_sensor_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code bh_roll_up_fan_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code bh_roll_up_power_supply_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code bh_roll_up_voltage_sensor_status(
					const struct bluehawk_diag_page2 *pg);
extern enum element_status_code bh_roll_up_sas_connector_status(
					const struct bluehawk_diag_page2 *pg);
/* Is this valid? */
extern enum element_status_code bh_roll_up_scc_controller_status(
					const struct bluehawk_diag_page2 *pg);
extern unsigned int bh_mean_temperature(const struct bluehawk_diag_page2 *pg);

/* homerun specific call */
extern enum element_status_code hr_roll_up_disk_status(
					const struct hr_diag_page2 *pg);
extern enum element_status_code hr_roll_up_esm_status(
					const struct hr_diag_page2 *pg);
extern enum element_status_code hr_roll_up_temperature_sensor_status(
					const struct hr_diag_page2 *pg);
extern enum element_status_code hr_roll_up_fan_status(
					const struct hr_diag_page2 *pg);
extern enum element_status_code hr_roll_up_power_supply_status(
					const struct hr_diag_page2 *pg);
extern enum element_status_code hr_roll_up_voltage_sensor_status(
					const struct hr_diag_page2 *pg);
extern unsigned int hr_mean_temperature(const struct hr_diag_page2 *pg);

#endif	/* __TEST_UTILS_H__ */
