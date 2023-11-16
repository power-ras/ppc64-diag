/**
 * @file	disk_health.c
 * @brief	Collect disk health information and store it in xml file
 *
 * Copyright (C) 2017 IBM Corporation
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * @author	Ankit Kumar <ankit@linux.vnet.ibm.com>
 */

#include <asm/byteorder.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "encl_util.h"

#define DIAG_OUTPUT_PATH		"/var/log/ppc64-diag/"
#define DISK_OUTPUT_PATH		DIAG_OUTPUT_PATH"diag_disk"
#define SYSFS_SG_PATH			"/sys/class/scsi_generic"
#define DEVICE_TREE			"/proc/device-tree/"
#define DEVICE_TREE_SYSTEM_ID		DEVICE_TREE"system-id"
#define DEVICE_TREE_VENDOR_SYSTEM_ID	DEVICE_TREE"ibm,vendor-system-id"
#define DEVICE_TREE_MODEL		DEVICE_TREE"model"
#define DEVICE_TREE_VENDOR_MODEL	DEVICE_TREE"ibm,vendor-model"

#define BUFFER_LENGTH			16
#define SERIAL_NUM_LEN			8
#define MACHINE_MODEL_LEN		8

/* Disk firmware, part number length */
#define FIRM_VER_LEN			16
#define PART_NUM_LEN			12

/* SCSI command length */
#define CCB_CDB_LEN			16

/* Smart data page for disk */
#define SMART_DATA_PAGE			0x34
#define LOG_SENSE_PROBE_ALLOC_LEN	4

#define DISK_HEALTH_VERSION		"1.0.0"

/* xml output file descriptor */
static FILE *result_file;

/*
 * Length of cdb (command descriptor block) for different scsi opcode.
 * For more details, go through scsi command opcode details.
 */
static const int cdb_size[] = {6, 10, 10, 0, 16, 12, 16, 16};

struct page_c4 {
#if defined (__BIG_ENDIAN_BITFIELD)
	uint8_t peri_qual:3;
	uint8_t peri_dev_type:5;
#elif defined (__LITTLE_ENDIAN_BITFIELD)
	uint8_t peri_dev_type:5;
	uint8_t peri_qual:3;
#endif
	uint8_t page_code;
	uint8_t reserved1;
	uint8_t page_length;
	uint8_t endurance;
	uint8_t endurance_val;
	uint8_t reserved2[5];
	uint8_t revision;
	uint8_t serial_num[SERIAL_NUM_LEN];
	uint8_t supp_serial_num[SERIAL_NUM_LEN];
	uint8_t master_drive_part_num[PART_NUM_LEN];
} __attribute__((packed));

struct page_3 {
	uint8_t peri_qual_dev_type;
	uint8_t page_code;
	uint8_t reserved1;
	uint8_t page_length;
	uint8_t ascii_len;
	uint8_t reserved2[3];
	uint8_t load_id[4];
	uint8_t release_level[4];
	uint8_t ptf_num[4];
	uint8_t patch_num[4];
} __attribute__((packed));


static int get_page_34_data(int device_fd)
{
	uint8_t *buff;
	int rc, i, buff_len;
	char first_4_bytes[4] = { 0 };

	/* Read first four byte to know actual length of smart data page */
	rc = do_ses_cmd(device_fd, LOG_SENSE, SMART_DATA_PAGE, 0x01,
			cdb_size[(LOG_SENSE >> 5) & 0x7], SG_DXFER_FROM_DEV,
			first_4_bytes, sizeof(first_4_bytes));
	if (rc)
		return rc;

	/* Second (MSB) and third byte contains data length field */
	buff_len = (first_4_bytes[2] << 8) + first_4_bytes[3] + 4;
	buff = (uint8_t *)calloc(1, buff_len);
	if (!buff)
		return -1;

	rc = do_ses_cmd(device_fd, LOG_SENSE, SMART_DATA_PAGE, 0x01,
			cdb_size[(LOG_SENSE >> 5) & 0x7], SG_DXFER_FROM_DEV,
			buff, buff_len);
	if (rc) {
		free(buff);
		return rc;
	}

	fprintf(result_file, "<LogPage34>\n");
	for (i = 0; i < buff_len; i++)
		fprintf(result_file, "%02x", buff[i]);

	free(buff);
	fprintf(result_file, "\n");
	fprintf(result_file, "</LogPage34>\n");

	return 0;
}

static inline void dir_sync(char * path)
{
	int dir_fd;

	dir_fd = open(path, O_RDONLY|O_DIRECTORY);
	if (dir_fd >= 0) {
		fsync(dir_fd);
		close(dir_fd);
	}
}

static int open_output_xml_file(const char *xml_filename)
{
	char filename[PATH_MAX];
	int rc;

	rc = access(DISK_OUTPUT_PATH, W_OK);
	if (rc) {
		/* Return if it fails with error code other than ENOENT */
		if (errno != ENOENT)
			return -1;

		/* Check for the existence of parent directory */
		rc = access(DIAG_OUTPUT_PATH, W_OK);
		if (rc) {
			if (errno != ENOENT)
				return -1;

			rc = mkdir(DIAG_OUTPUT_PATH,
				S_IRGRP | S_IRUSR | S_IWGRP | S_IWUSR | S_IXUSR);
			if (rc)
				return -1;

			dir_sync(DIAG_OUTPUT_PATH);
		}

		rc = mkdir(DISK_OUTPUT_PATH,
			   S_IRGRP | S_IRUSR | S_IWGRP | S_IWUSR | S_IXUSR);
		if (rc)
			return -1;

		dir_sync(DISK_OUTPUT_PATH);
	}


	rc = snprintf(filename, sizeof(filename) - 1, "%s/%s",
			DISK_OUTPUT_PATH, xml_filename);
	if (rc < 0 || (rc >= sizeof(filename) - 1)) {
		fprintf(stderr, "%s:%d - Unable to format %s\n",
				__func__, __LINE__, xml_filename);
		return -1;
	}

	result_file = fopen(filename, "w");
	if (!result_file)
		return -1;

	return 0;
}

static inline void close_output_xml_file(void)
{
	fclose(result_file);
}

/* In UTC format */
static inline int get_current_time(struct tm *tmptr)
{
	time_t t = time(NULL);

	*tmptr = *gmtime(&t);
	if (tmptr == NULL)
		return -1;

	return 0;
}

static void populate_xml_header(char *result_file_name, char *time_stamp,
		char *machine_type, char *machine_model, char *machine_serial)
{
	fprintf(result_file, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	fprintf(result_file, "<DiskDrives version=\"%s\">\n",
		DISK_HEALTH_VERSION);
	fprintf(result_file, "<Document id=\"%s\" timestamp=\"%s\"/>\n",
		result_file_name, time_stamp);
	fprintf(result_file,
		"<Machine type=\"%s\" model=\"%s\" serial=\"%s\"/>\n",
		machine_type, machine_model, machine_serial);
}

static inline void populate_xml_footer(void)
{
	fprintf(result_file, "</DiskDrives>\n");
}

static int get_system_vpd(char *machine_serial,
			  char *machine_type, char **machine_model)
{
	int device_fd;
	int rc;
	int start_index = 0;
	char serial[BUFFER_LENGTH] = {0};
	char model[BUFFER_LENGTH] = {0};
	char *temp;

	/*
	 * Always use ibm,vendor-system-id device-tree property
	 * to get machine serial number. If it does not exist,
	 * then fallback to system-id property to get machine
	 * serial number.
	 */
	device_fd = open(DEVICE_TREE_VENDOR_SYSTEM_ID, O_RDONLY);
	if (device_fd < 0) {
		device_fd = open(DEVICE_TREE_SYSTEM_ID, O_RDONLY);
		if (device_fd < 0)
			return -1;
	}

	rc = read(device_fd, serial, BUFFER_LENGTH);
	close(device_fd);
	if (rc <= 0)
		return -1;

	/*
	 * We are interested in last 5 bytes of serial number not complete
	 * string read from system-id property.
	 */
	if (strlen(serial) > 5)
		start_index = strlen(serial) - 5;
	memcpy(machine_serial, serial + start_index, SERIAL_NUM_LEN);
	machine_serial[SERIAL_NUM_LEN - 1] = '\0';

	/*
	 * Always use ibm,vendor-model device-tree property
	 * to get model info. If it does not exist, then
	 * fallback to model property to get model info.
	 */
	device_fd = open(DEVICE_TREE_VENDOR_MODEL, O_RDONLY);
	if (device_fd < 0) {
		device_fd = open(DEVICE_TREE_MODEL, O_RDONLY);
		if (device_fd < 0)
			return -1;
	}

	rc = read(device_fd, model, BUFFER_LENGTH);
	close(device_fd);
	if (rc <= 0)
		return -1;

	temp = model;
	/* Discard manufacturer name (valid on PowerVM) */
	if (strchr(model, ',') != NULL)
		temp = strchr(model, ',') + 1;

	memcpy(machine_type, temp, MACHINE_MODEL_LEN + 1);
	machine_type[MACHINE_MODEL_LEN] = '\0';
	*machine_model = strchr(machine_type, '-');
	if (*machine_model == NULL) /* Failed to get model name */
		return -1;

	**machine_model = '\0';
	(*machine_model)++;

	return 0;
}

static int get_disk_vpd(int device_fd)
{
	char serial_num[SERIAL_NUM_LEN + 1] = { 0 };
	struct page_3 page3_inq = { 0 };
	char firm_num[FIRM_VER_LEN + 1] = { 0 };
	struct page_c4 pagec4 = { 0 };
	char master_drive_part_num[PART_NUM_LEN + 1] = { 0 };
	int rc;
	char *temp = NULL;

	/* get device specific details */
	fprintf(result_file, "<Drive ");
	rc = do_ses_cmd(device_fd, INQUIRY, 0xC4, 0x01,
			cdb_size[(INQUIRY >> 5) & 0x7], SG_DXFER_FROM_DEV,
			&pagec4, sizeof(pagec4));
	if (!rc) {
		strncpy(serial_num, (char *)pagec4.serial_num,
				sizeof(serial_num - 1));
		strncpy(master_drive_part_num,
				(char *)pagec4.master_drive_part_num,
				sizeof(master_drive_part_num) - 1);
		/* discard data after first occurance of space */
		temp = strchr(master_drive_part_num, ' ');
		if (temp)
			*temp = '\0';
		fprintf(result_file,
				"pn=\"%s\" sn=\"%s\" ",
				master_drive_part_num, serial_num);
	}

	/* firmware details */
	rc = do_ses_cmd(device_fd, INQUIRY, 0x03, 0x01,
			cdb_size[(INQUIRY >> 5) & 0x7], SG_DXFER_FROM_DEV,
			&page3_inq, sizeof(page3_inq));
	if (!rc) {
		snprintf(firm_num, sizeof(firm_num) - 1, "%c%c%c%c",
				page3_inq.release_level[0],
				page3_inq.release_level[1],
				page3_inq.release_level[2],
				page3_inq.release_level[3]);
		fprintf(result_file, "fw=\"%s\"", firm_num);
	}
	fprintf(result_file, ">\n");

	/* get page_34 data */
	rc = get_page_34_data(device_fd);
	fprintf(result_file, "</Drive>\n");

	return 0;
}

static int sysfs_sg_disk_scan(const char *dir_name, char *disk_name)
{
	DIR *d;
	bool disk_found = false;
	int device_fd;
	int rc = 0;
	struct sg_scsi_id sg_dat;
	struct dirent *namelist;

	d = opendir(dir_name);
	if (!d)
		return -errno;

	/* scan and get disk data */
	while ((namelist = readdir(d)) != NULL) {
		if (namelist->d_name[0] == '.')
			continue;

		/* query for specific disk */
		if (disk_name) {
			if (strcmp(disk_name, namelist->d_name))
				continue;
		}

		device_fd = open_sg_device(namelist->d_name);
		if (device_fd < 0)
			continue;

		if (ioctl(device_fd, SG_GET_SCSI_ID, &sg_dat) < 0) {
			close_sg_device(device_fd);
			continue;
		}

		if (sg_dat.scsi_type == TYPE_DISK || sg_dat.scsi_type == 14) {
			disk_found = true;
			get_disk_vpd(device_fd);
		}
		close_sg_device(device_fd);
	}

	if (disk_name && disk_found == false) {
		fprintf(stderr, "\nUnable to validate disk name[%s].\n\n",
				disk_name);
		rc = -1;
	}

	closedir(d);
	return rc;
}

static int remove_old_log_file(void)
{
	DIR *d;
	struct dirent *namelist;
	char filename[PATH_MAX];
	int rc;

	d = opendir(DISK_OUTPUT_PATH);
	if (!d)
		return -errno;

	while ((namelist = readdir(d)) != NULL) {
		if (namelist->d_name[0] == '.')
			continue;

		rc = snprintf(filename, sizeof(filename) - 1, "%s/%s",
				DISK_OUTPUT_PATH, namelist->d_name);
		if (rc < 0 || rc >= (sizeof(filename) - 1)) {
			fprintf(stderr, "%s:%d - Unable to format %s\n",
				__func__, __LINE__, namelist->d_name);
			continue;
		}

		if (unlink(filename) < 0) {
			fprintf(stderr,
			"\nUnable to remove old log file[%s]. continuing.\n\n",
			filename);
		}
	}

	closedir(d);
	dir_sync(DISK_OUTPUT_PATH);
	return 0;
}

/*
 * @param: it takes disk name as an input. If disk_name is NULL, it collects
 * data for all available disk.
 * @return : zero on success. non zero on failure.
 */
int diag_disk(char *disk_name)
{
	char serial_num[SERIAL_NUM_LEN + 1];
	char mach_type_model[MACHINE_MODEL_LEN + 1];
	char *mach_model = NULL;
	char time_stamp[256];
	char xml_filename[PATH_MAX];
	struct tm tm;
	int ret;

	/* get system vpd */
	ret = get_system_vpd(serial_num, mach_type_model, &mach_model);
	if (ret) {
		fprintf(stderr, "Unable to read system vpd.\n");
		return -1;
	}

	/* get current time */
	ret = get_current_time(&tm);
	if (ret) {
		fprintf(stderr, "Unable to read current time.\n");
		return -1;
	}

	snprintf(time_stamp, sizeof(time_stamp) - 1, "%d-%d-%dT%d:%d:%dZ\n",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		 tm.tm_hour, tm.tm_min, tm.tm_sec);

	/* file format */
	snprintf(xml_filename, sizeof(xml_filename) - 1,
		 "%s~%s~%s~diskAnalytics.xml",
		 mach_type_model, mach_model, serial_num);

	/* Try to remove old log file. We will continue even if this fails */
	remove_old_log_file();

	/* open file */
	ret = open_output_xml_file(xml_filename);
	if (ret) {
		fprintf(stderr, "Unable to open output xml file.\n");
		return -1;
	}

	/* populate xml header */
	populate_xml_header(xml_filename, time_stamp, mach_type_model,
			mach_model, serial_num);

	/* get scsi device name */
	sysfs_sg_disk_scan(SYSFS_SG_PATH, disk_name);

	/* populate xml footer */
	populate_xml_footer();

	/* close output xml file descriptor */
	close_output_xml_file();

	return 0;
}
