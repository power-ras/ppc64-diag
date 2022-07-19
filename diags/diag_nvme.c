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

/* get_ibm_vpd_pcie - Get IBM VPD data from NVMe device PCIe
 * @controller_name - Name of the device to retrieve VPD data from, expected format is nvme[0-9]+
 * @vpd - Address of vpd struct to store the PCIe IBM VPD data
 *
 * Read vpd file in sysfs to retrieve IBM VPD data for the NVMe device
 *
 * Return - Return 0 on success, -1 on failure
 */
extern int get_ibm_vpd_pcie(char *controller_name, struct nvme_ibm_vpd *vpd) {
	int fd;
	char sysfs_vpd_path[PATH_MAX], keyword[3] = { '\0' };
	uint8_t vpd_data[256], length_data[2], length_keyword, tag;
	uint16_t length, length_parsed;

	snprintf(sysfs_vpd_path,sizeof(sysfs_vpd_path), "%s/%s/device/vpd", NVME_SYS_PATH, controller_name);
	fd = open(sysfs_vpd_path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s open failed: %s\n", sysfs_vpd_path, strerror(errno));
		return -1;
	}

	do {
		if (read(fd, &tag, 1) < 1)
			goto read_error;
		/* In case it is a large resource data type, length is stored in the next two bytes */
		if (tag & 0x80) {
			if (read(fd, length_data, 2) < 2)
				goto read_error;
			length = length_data[0] + (length_data[1] << 8);
		}
		/* It is a small resource data type, length is stored in the last three bits of tag byte */
		else {
			length = tag & 0x07;
		}

		switch (tag) {
		/* Identifier String */
		case 0x82:
			memset(vpd_data, '\0', sizeof(vpd_data));
			if (read(fd, vpd_data, length) < length)
				goto read_error;
			strncpy(vpd->description, (char *)vpd_data, sizeof(vpd->description));
			break;

		/* VPD-R */
		case 0x90:
		/* VPD-W */
		case 0x91:
			length_parsed = 0;
			while (length_parsed < length) {
				memset(vpd_data, '\0', sizeof(vpd_data));
				if (read(fd, keyword, 2) < 2)
					goto read_error;
				if (read(fd, &length_keyword, 1) < 1)
					goto read_error;
				if (read(fd, vpd_data, length_keyword) < length_keyword)
					goto read_error;
				set_vpd_pcie_field(keyword, (char *)vpd_data, vpd);
				length_parsed += length_keyword + 3;
			}
			break;

		/* End Tag */
		case 0x78:
			break;
		default:
			fprintf(stderr, "Unknown tag 0x%x, PCIe VPD data processing aborted\n", tag);
			close(fd);
			return -1;
		}
	} while (tag != 0x78);

	close(fd);
	return 0;

read_error:
	fprintf(stderr, "Failed to read expected amount of data, PCIe VPD might be corrupted. Abort further processing\n");
	close(fd);
	return -1;
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

/* set_vpd_pcie_field - Set the appropriate field with VPD information collected
 * @keyword - Address of keyword string extracted from PCIe VPD for a device
 * @vpd_data - Address of vpd data string extracted from PCIe VPD for a device
 * @vpd - Address of vpd struct to store the PCIe IBM VPD data
 *
 * Fill the appropriate field in the IBM VPD struct with the information collected from the PCIe VPD
 * sysfs file for a NVMe device. This is done by mapping the keyword to the corresponding field in
 * the struct.
 */
extern void set_vpd_pcie_field(const char *keyword, const char *vpd_data, struct nvme_ibm_vpd *vpd) {
	if (!strcmp(keyword, "PN"))
		strncpy(vpd->master_pn, vpd_data, sizeof(vpd->master_pn));
	else if (!strcmp(keyword, "EC"))
		strncpy(vpd->ec_level, vpd_data, sizeof(vpd->ec_level));
	else if (!strcmp(keyword, "FN"))
		strncpy(vpd->fru_pn, vpd_data, sizeof(vpd->fru_pn));
	else if (!strcmp(keyword, "AN"))
		strncpy(vpd->final_asm_pn, vpd_data, sizeof(vpd->final_asm_pn));
	else if (!strcmp(keyword, "FC"))
		strncpy(vpd->feature_code, vpd_data, sizeof(vpd->feature_code));
	else if (!strcmp(keyword, "CC"))
		strncpy(vpd->ccin, vpd_data, sizeof(vpd->ccin));
	else if (!strcmp(keyword, "SN"))
		strncpy(vpd->sn_11s, vpd_data, sizeof(vpd->sn_11s));
	else if (!strcmp(keyword, "Z0"))
		strncpy(vpd->pcie_ssid, vpd_data, sizeof(vpd->pcie_ssid));
	else if (!strcmp(keyword, "Z1"))
		strncpy(vpd->endurance, vpd_data, sizeof(vpd->endurance));
	else if (!strcmp(keyword, "Z2"))
		strncpy(vpd->capacity, vpd_data, sizeof(vpd->capacity));
	else if (!strcmp(keyword, "Z3"))
		strncpy(vpd->warranty, vpd_data, sizeof(vpd->warranty));
	else if (!strcmp(keyword, "Z4"))
		strncpy(vpd->encryption, vpd_data, sizeof(vpd->encryption));
	else if (!strcmp(keyword, "Z5"))
		strncpy(vpd->rctt, vpd_data, sizeof(vpd->rctt));
	else if (!strcmp(keyword, "Z6"))
		strncpy(vpd->load_id, vpd_data, sizeof(vpd->load_id));
	else if (!strcmp(keyword, "Z7"))
		strncpy(vpd->mfg_location, vpd_data, sizeof(vpd->mfg_location));
	else if (!strcmp(keyword, "Z8"))
		strncpy(vpd->ffc_led, vpd_data, sizeof(vpd->ffc_led));
	else if (!strcmp(keyword, "Z9"))
		strncpy(vpd->system_io_cmd_timeout, vpd_data, sizeof(vpd->system_io_cmd_timeout));
	else if (!strcmp(keyword, "ZA"))
		strncpy(vpd->format_cmd_timeout, vpd_data, sizeof(vpd->format_cmd_timeout));
	else if (!strcmp(keyword, "ZB"))
		strncpy(vpd->number_io_queues, vpd_data, sizeof(vpd->number_io_queues));
	else if (!strcmp(keyword, "ZC"))
		strncpy(vpd->flash_type, vpd_data, sizeof(vpd->flash_type));
	else if (!strcmp(keyword, "MN"))
		strncpy(vpd->manufacture_sn, vpd_data, sizeof(vpd->manufacture_sn));
	else if (!strcmp(keyword, "RM"))
		strncpy(vpd->firmware_level, vpd_data, sizeof(vpd->firmware_level));
}
