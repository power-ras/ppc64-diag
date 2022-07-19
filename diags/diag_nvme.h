/*
 * Copyright (C) 2022 IBM Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifndef _DIAG_NVME_H
#define _DIAG_NVME_H

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <linux/nvme_ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define NVME_SYS_PATH			"/sys/class/nvme"
#define OPCODE_ADMIN_GET_LOG_PAGE	0x02

/* Struct representing IBM VPD information retrieved from Log Page 0xF1.
 * This struct is also reutilized for PCIe VPD information in case Log Page 0xF1 is not supported.
 * PCIe VPD according to internal specification is a subset of VPD from Log Page 0xF1, meaning that
 * PCIe VPD has fewer fields and their maximum lengths are lower than those from Log Page 0xF1.
 * So reutilizing this structure shouldn't lead to any loss of information.
 */
struct nvme_ibm_vpd {
	char version[4];
	char description[40];
	char master_pn[12];
	char ec_level[10];
	char fru_pn[12];
	char final_asm_pn[12];
	char feature_code[4];
	char ccin[4];
	char sn_11s[8];
	char pcie_ssid[8];
	char endurance[4];
	char capacity[10];
	char warranty[12];
	char encryption[1];
	char rctt[2];
	char load_id[8];
	char mfg_location[3];
	char ffc_led[5];
	char system_io_cmd_timeout[2];
	char format_cmd_timeout[4];
	char number_io_queues[4];
	char flash_type[2];
	char manufacture_sn[20];
	char firmware_level[8];
	uint8_t async_firmware_commit;
	uint8_t query_lba_map_status;
	uint8_t query_storage_usage;
	uint8_t async_format_nvm;
	uint16_t endurance_value;
	uint16_t sanitize_cmd_timeout;
	uint8_t dsm_io_hints_supported;
	uint8_t reserved[816];
} __attribute__((packed));

/* Struct representing the SMART / health information log page as defined in NVMe base specification */
struct nvme_smart_log_page {
	uint8_t critical_warning;
#define CRIT_WARN_SPARE		0x01
#define CRIT_WARN_TEMP		0x02
#define CRIT_WARN_DEGRADED	0x04
#define CRIT_WARN_RO		0x08
#define CRIT_WARN_VOLATILE_MEM	0x10
#define CRIT_WARN_PMR_RO	0x20
	uint16_t composite_temp;
	uint8_t avail_spare;
	uint8_t avail_spare_threshold;
	uint8_t percentage_used;
	uint8_t endurance_group_crit_warn_summary;
#define EGCWS_SPARE		0x01
#define EGCWS_DEGRADED		0x04
#define EGCWS_RO		0x08
	uint8_t reserved7[25];
	uint8_t data_units_read[16];
	uint8_t data_units_written[16];
	uint8_t host_reads_cmd[16];
	uint8_t host_writes_cmd[16];
	uint8_t ctrl_busy_time[16];
	uint8_t power_cycles[16];
	uint8_t power_on_hours[16];
	uint8_t unsafe_shutdowns[16];
	uint8_t media_data_integrity_err[16];
	uint8_t num_err_info_log_entries[16];
	uint32_t warn_composite_temp_time;
	uint32_t crit_composite_temp_time;
	uint16_t temp_sensor1;
	uint16_t temp_sensor2;
	uint16_t temp_sensor3;
	uint16_t temp_sensor4;
	uint16_t temp_sensor5;
	uint16_t temp_sensor6;
	uint16_t temp_sensor7;
	uint16_t temp_sensor8;
	uint32_t thrm_mgmt_temp1_trans_count;
	uint32_t thrm_mgmt_temp2_trans_count;
	uint32_t total_time_thm_mgmt_temp1;
	uint32_t total_time_thm_mgmt_temp2;
	uint8_t reserved232[280];
} __attribute__((packed));

extern int get_ibm_vpd_log_page(int fd, struct nvme_ibm_vpd *vpd);
extern int get_ibm_vpd_pcie(char *controller_name, struct nvme_ibm_vpd *vpd);
extern int get_smart_log_page(int fd, uint32_t nsid, struct nvme_smart_log_page *log);
extern void set_vpd_pcie_field(const char *keyword, const char *vpd_data, struct nvme_ibm_vpd *vpd);

#endif /* _DIAG_NVME_H */
