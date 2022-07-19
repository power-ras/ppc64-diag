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

#include "diag_nvme.h"

/* get_ibm_vpd_log_page - Get IBM VPD data from NVMe device log page
 * @fd - File descriptor index of NVMe device
 * @vpd - Address of vpd struct to store the IBM VPD data
 *
 * Do an ioctl to retrieve VPD data for the specified NVMe device log page 0xF1
 *
 * Return - ioctl return code is returned, 0 on success
 */
extern int get_ibm_vpd_log_page(int fd, struct nvme_ibm_vpd *vpd) {
        struct nvme_admin_cmd admin_cmd = {
                .opcode = OPCODE_ADMIN_GET_LOG_PAGE,
                .addr = (uint64_t)(uintptr_t)vpd,
                .data_len = sizeof(*vpd),
                .cdw10 = 0x00FF00F1,
        };

	return ioctl(fd, NVME_IOCTL_ADMIN_CMD, &admin_cmd);
}

/* get_smart_log_page - Get SMART data for NVMe device
 * @fd - File descriptor index of NVMe device
 * @nsid - Namespace ID, use 0xFFFFFFFF or 0x0 to get controller data, former is preferred
 * @log - Address of log struct to store the SMART data
 *
 * Do an ioctl to retrieve SMART data for the specified NSID and NVMe device
 *
 * Return - ioctl return code is returned, 0 on success
 */
extern int get_smart_log_page(int fd, uint32_t nsid, struct nvme_smart_log_page *log) {
	struct nvme_admin_cmd admin_cmd = {
		.opcode = OPCODE_ADMIN_GET_LOG_PAGE,
		.nsid = nsid,
		.addr = (uint64_t)(uintptr_t)log,
		.data_len = sizeof(*log),
		.cdw10 = 0x007F0002,
	};

	return ioctl(fd, NVME_IOCTL_ADMIN_CMD, &admin_cmd);
}
