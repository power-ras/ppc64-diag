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

#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <regex.h>
#include <servicelog-1/servicelog.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include "diag_nvme.h"
#include "platform.h"
#include "utils.h"

#define ITEM_DATA_LENGTH	255
#define MIN_HOURS_ON		720

struct notify {
	bool smart_missing;
	uint8_t crit_warn;
	bool write_rate;
};

struct section {
	struct header_section {
		uint8_t tag;
		uint8_t reserved;
		uint16_t length;
	} header;
	unsigned char *data;
} __attribute__((packed));

struct item {
	struct header_item {
		char keyword[2];
		uint8_t reserved;
		uint8_t length;
	} header;
	char data[ITEM_DATA_LENGTH];
} __attribute__((packed));

static int append_item(struct section *section, struct item *item);
static int append_section(unsigned char **raw_data, uint32_t *raw_data_len, struct section *section);
static int check_open_serv_event(char *location);
static int diagnose_nvme(char *device_name, struct notify *notify, char *file_path);
static void fill_item_ld(struct item *item, char *keyword, long double data);
static void fill_item_string(struct item *item, char *keyword, char *data, size_t size);
static int fill_raw_data(unsigned char **raw_data, uint32_t *raw_data_len, struct nvme_smart_log_page *log, struct nvme_ibm_vpd *vpd);
static bool in_range_max (long double value, long double max, char *key);
static bool is_positive (long double value, char *key);
static int log_event(uint64_t *event_id, struct nvme_ibm_vpd *vpd, char *controller_name, uint8_t severity, char *description, unsigned char *raw_data, uint32_t raw_data_len);
static void long_double_to_uint128(long double value, uint8_t *data);
static void print_usage(char *command);
static int raw_data_smart(unsigned char **raw_data, uint32_t *raw_data_len, struct nvme_smart_log_page *log);
static int raw_data_vpd(unsigned char **raw_data, uint32_t *raw_data_len, struct nvme_ibm_vpd *vpd);
static int regex_controller(char *controller_name, char *device_name);
static void set_notify(struct notify *notify, struct dictionary *dict, int num_elements);
static long double uint128_to_long_double(uint8_t *data);

int main(int argc, char *argv[]) {
	bool dump_opt = false, file_opt = false;
	char *dump_path = NULL, *file_path = NULL;
	int opt, rc = 0, num_elements;

	struct dictionary dict[MAX_DICT_ELEMENTS];
	struct notify notify = { .smart_missing = true, .crit_warn = 0xFF, .write_rate = true };

	DIR *dir;
	struct dirent *dirent;

	if (get_platform() != PLATFORM_PSERIES_LPAR || get_processor() < POWER10) {
		fprintf(stdout, "%s is only supported in PowerVM LPARs and at least Power10 processors\n", argv[0]);
		return 0;
	}

	static struct option long_options[] = {
		{"dump", required_argument, NULL, 'd'},
		{"file", required_argument, NULL, 'f'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "d:f:h", long_options, NULL)) != -1) {
		switch (opt) {
		case 'd':
			dump_opt = true;
			dump_path = optarg;
			break;
		case 'f':
			file_opt = true;
			file_path = optarg;
			break;
		case 'h':
			print_usage(argv[0]);
			return 0;
		case '?':
			print_usage(argv[0]);
			return -1;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if ((dump_opt || file_opt) && optind + 1 != argc) {
		fprintf(stderr, "Specify one NVMe device with -d / -f option\n");
		return -1;
	}
	if (dump_opt) {
		if (dump_smart_data(argv[optind], dump_path)) {
			fprintf(stderr, "Dump SMART data failed, aborting\n");
			return -1;
		}
		if (!file_opt)
			return 0;
	}

	if ((num_elements = read_file_dict(CONFIG_FILE, dict, MAX_DICT_ELEMENTS)) >= 0)
		set_notify(&notify, dict, num_elements);
	else
		return -1;

	if (file_opt)
		return diagnose_nvme(argv[optind], &notify, file_path);

	/* No devices have been specified, look for all NVMe devices in sysfs */
	if (optind == argc) {
		dir = opendir(NVME_SYS_PATH);
		if (!dir) {
			fprintf(stderr, "%s open failed: %s\n", NVME_SYS_PATH, strerror(errno));
			fprintf(stdout, "No NVMe devices detected in system\n");
		}
		else {
			while ((dirent = readdir(dir)) != NULL) {
				if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
					continue;
				rc += diagnose_nvme(dirent->d_name, &notify, NULL);
				fprintf(stdout, "\n");
			}
			closedir(dir);
		}
	}

	/* Only go through the devices specified */
	while (optind < argc) {
		rc += diagnose_nvme(argv[optind], &notify, NULL);
		fprintf(stdout, "\n");
		optind++;
	}

	if (rc == 0)
		fprintf(stdout, "NVMe diag command completed successfully\n");
	else
		fprintf(stderr, "NVMe diag command failed with rc %d\n", rc);

	return rc;
}

/* append_item - Append an item to a section
 * @section - Address of section struct to be filled, user is expected to free memory
 *		allocated for section->data to avoid memory leak
 * @item - Address of item struct to be appended to section
 *
 * User is expected to free memory allocated for section->data
 *
 * Append a new item to the existent section
 *
 * Return - Return 0 on success, -1 if section is full, -2 for other errors
 */
static int append_item(struct section *section, struct item *item) {
	unsigned char *old_data;
	uint16_t length;

	if(!section || !item) {
		fprintf(stderr, "section or item parameter in %s is NULL, aborting", __func__);
		return -2;
	}

	length = le16toh(section->header.length) + sizeof(item->header) + item->header.length;
	if (length < section->header.length) {
		fprintf(stdout, "Warn: Section 0x%.2x can't store additional items, ignoring item: keyword %.*s, data %.*s\n",
				section->header.tag, (int) sizeof(item->header.keyword), item->header.keyword,
				item->header.length, item->data);
		return -1;
	}
	old_data = section->data;
	if (!(section->data = malloc(length)))
		goto err_out;
	memcpy(section->data, old_data, le16toh(section->header.length));
	memcpy((section->data + le16toh(section->header.length)), item,
			(sizeof(item->header) + item->header.length));
	section->header.length = htole16(length);
	free(old_data);
	return 0;

err_out:
	fprintf(stderr, "Memory allocation failed in %s for section 0x%.2x, keyword %.*s, data %.*s\n",
			__func__, section->header.tag, (int) sizeof(item->header.keyword),
			item->header.keyword, item->header.length, item->data);
	section->data = old_data;
	return -2;
}

/* append_section - Append a section to raw_data
 * @raw_data - Address of pointer to the binary data to be filled, user is expected to free memory
 *		allocated for raw_data to avoid memory leak
 * @raw_data_len - Address of variable to set with length (in bytes) of raw_data that was filled
 * @section - Address of section struct to be appended to raw_data
 *
 * User is expected to free memory allocated for raw_data
 *
 * Append a new section to the existent raw_data
 *
 * Return - Return 0 on success, -1 if raw_data is full, -2 for other errors
 */
static int append_section(unsigned char **raw_data, uint32_t *raw_data_len, struct section *section) {
	unsigned char *old_raw_data;
	uint32_t length;

	if(!section) {
		fprintf(stderr, "section parameter in %s is NULL, aborting", __func__);
		return -2;
	}

	length = *raw_data_len + sizeof(section->header) + le16toh(section->header.length);
	if (length < *raw_data_len) {
		fprintf(stdout, "Warn: raw_data can't store additional section, ignoring section 0x%.2x\n",
				section->header.tag);
		return -1;
	}
	old_raw_data = *raw_data;
	if (!(*raw_data = malloc(length)))
		goto err_out;
	memcpy(*raw_data, old_raw_data, *raw_data_len);
	memcpy((*raw_data + *raw_data_len), section, sizeof(section->header));
	memcpy((*raw_data + *raw_data_len + sizeof(section->header)), section->data,
			le16toh(section->header.length));
	*raw_data_len = length;
	free(old_raw_data);

	return 0;

err_out:
	fprintf(stderr, "Memory allocation failed in %s for section 0x%.2x\n", __func__,
			section->header.tag);
	*raw_data = old_raw_data;
	return -2;
}

/* check_open_serv_event - Check existent serviceable event in open state for device
 * @location - location code of the device
 *
 * Query the servicelog for any existent serviceable events in open state that contains a callout to
 * the location code passed as a parameter.
 *
 * Return - Return 0 if there is no existent open serviceable event, -1 in case there is
 */
static int check_open_serv_event(char *location) {
	int rc;
	struct servicelog *slog;
	struct sl_event *event, *e;
	struct sl_callout *c;

	rc = servicelog_open(&slog, 0);
	if (rc) {
		fprintf(stderr, "Log open error: %s\n", servicelog_error(slog));
		return 0;
	}
	servicelog_event_query(slog, "serviceable = 1 AND closed = 0", &event);

	for (e = event; e; e = e->next) {
		for (c = e->callouts; c; c = c->next) {
			if(!strcmp(c->location, location)) {
				fprintf(stdout, "Event ID %lu in servicelog matched location found for %s\n",
						e->id, location);
				servicelog_event_free(event);
				servicelog_close(slog);
				return -1;
			}
		}
	}
	servicelog_event_free(event);
	servicelog_close(slog);
	return 0;
}

/* dump_smart_data - Dump SMART data of NVMe device into a file
 * @device_name - Name of the device to retrive SMART data from, expected format is nvme[0-9]+
 * @dump_path - Null terminated pathname of file which SMART data will be dumped into
 *
 * Extract SMART data from specified NVMe device and write the contents of it into a file using a
 * key=value format for each of the fields in the SMART data.
 *
 * Return - Return 0 on success, negative number on failure
 */
extern int dump_smart_data(char *device_name, char *dump_path) {
	char dev_path[PATH_MAX];
	int fd, rc;
	FILE *fp;
	struct nvme_smart_log_page smart_log = { 0 };
	char ans;

	/* Read SMART data from device */
	snprintf(dev_path,sizeof(dev_path), "/dev/%s", device_name);
	fd = open_nvme(dev_path);
	if (fd < 0)
		return -1;
	if ((rc = get_smart_log_page(fd, 0xffffffff, &smart_log)) != 0) {
		fprintf(stderr, "SMART log data could not be retrieved for device %s, ioctl failed: 0x%x\n",
				device_name, rc);
		close(fd);
		return -1;
	}
	close(fd);

	/* Dump it into a file */
	if (strlen(dump_path) > PATH_MAX) {
		fprintf(stderr, "Path specified is longer than expected maximum: %d\n", PATH_MAX);
		return -1;
	}
	fp = fopen(dump_path, "wx");
	if (fp == NULL) {
		if (errno == EEXIST) {
			fprintf(stdout, "File %s exists. Overwrite (y/n)? ", dump_path);
			rc = scanf("%c", &ans);
			if (ans == 'y' || ans == 'Y')
				fp = fopen(dump_path, "w");
		}
		if (fp == NULL) {
			fprintf(stderr, "%s open failed: %s\n", dump_path, strerror(errno));
			return -1;
		}
	}
	write_smart_file(fp, &smart_log);
	fclose(fp);
	return 0;
}

/* diagnose_nvme - Diagnose NVMe device health status
 * @device_name - Name of the device to be diagnosed, expected format is nvme[0-9]+
 * @notify - Struct containing which events are to be notified based on diag_nvme.config parsing
 * @file_path - Read SMART data from a file instead of device for testing purpose, set to NULL if
 * 	data should be read from the device.
 *
 * Diagnose will check SMART data retrieved and identify any failures that need to be reported in
 * the servicelog database.
 *
 * Return - Return 0 on success, negative number on failure
 */
static int diagnose_nvme(char *device_name, struct notify *notify, char *file_path) {
	char dev_path[PATH_MAX], controller_name[NAME_MAX] = { '\0' }, description[DESCR_LENGTH];
	double endurance;
	int fd, rc = 0, tmp_rc, time = 0, interval = 5;
	long double power_on_hours, data_units_written, avg_dwpd;
	long int capacity;
	struct nvme_smart_log_page smart_log = { 0 };
	struct nvme_ibm_vpd vpd = { 0 };
	char endurance_s[sizeof(vpd.endurance) + 1], capacity_s[sizeof(vpd.capacity)+1];
        uint64_t event_id;
	uint8_t severity;
	FILE *fp;
	char tr_file_path[PATH_MAX];
	uint32_t raw_data_len = 0;
	unsigned char *raw_data = NULL;

	/*
	 * Skip diag test if NVMe is connected over fabric
	 */
	snprintf(tr_file_path, sizeof(tr_file_path),
			NVME_SYS_PATH"/%s/%s", device_name, "transport");
	fp = fopen(tr_file_path, "r");
	if (fp) {
		char buf[12];
		int n = fread(buf, 1, sizeof(buf), fp);

		if (n) {
			/*
			 * If NVMe transport is anything but pcie then skip the diag test
			 */
			if (strncmp(buf, "pcie", 4) != 0) {
				fprintf(stdout, "Skipping diagnostics for nvmf : %s\n",
						device_name);
				fclose(fp);
				return 0;
			}
		}
		fclose(fp);
	} else {
		fprintf(stderr, "Skipping diagnostics for %s:\n"
				"Unable to find the nvme transport type\n",
				device_name);
		return -1;
	}

	tmp_rc = regex_controller(controller_name, device_name);
	if (tmp_rc != 0)
		return -1;
	snprintf(dev_path,sizeof(dev_path), "/dev/%s", device_name);
	fd = open_nvme(dev_path);
	if (fd < 0)
		return -1;
	fprintf(stdout, "Running diagnostics for %s\n", device_name);

	/* After system boot VPD data might not be ready yet, so wait until VPD log page is ready */
	while ((tmp_rc = get_ibm_vpd_log_page(fd, &vpd)) == VPD_NOT_READY && time <= MAX_TIME) {
	       sleep(interval);
	       time += interval;
	}

	/* If VPD log page fails to be collected fallback to PCIe VPD */
	if (tmp_rc != 0) {
		fprintf(stdout, "Warn: %s VPD log page ioctl failed: 0x%x, fallback to PCIe VPD\n", controller_name, tmp_rc);
		tmp_rc = get_ibm_vpd_pcie(controller_name, &vpd);
        }

	if (tmp_rc != 0)
		fprintf(stdout, "Warn: %s PCIe VPD failed: 0x%x, proceed without device VPD data\n", controller_name, tmp_rc);

	if(!file_path)
		tmp_rc = get_smart_log_page(fd, 0xffffffff, &smart_log);
	else {
		if ((tmp_rc = get_smart_file(file_path, &smart_log)) == -1)
			return -1;
	}
	/* If SMART data can't be retrieved assume device is in a bad state and report an event
	 * SMART file open failure (return code -2 from get_smart_file) will trigger this event
	 */
	if (tmp_rc && notify->smart_missing) {
		fprintf(stderr, "Smart Log retrieve failed: %d\n", tmp_rc);
		severity = SL_SEV_ERROR;
		snprintf(description, sizeof(description), "SMART log data could not be retrieved for device. Device is assumed to be in a bad state.");
		if(!raw_data && fill_raw_data(&raw_data, &raw_data_len, NULL, &vpd) != 0) {
			free(raw_data);
			return -1;
		}
		rc += log_event(&event_id, &vpd, controller_name, severity, description, raw_data, raw_data_len);
		close(fd);
		return rc;
	}
	close(fd);

	/* Check if any critical warning bits are set and if so report an event */
	if (smart_log.critical_warning & CRIT_WARN_TEMP & notify->crit_warn)
		severity = SL_SEV_WARNING;
	if (smart_log.critical_warning & ~CRIT_WARN_TEMP & notify->crit_warn)
		severity = SL_SEV_ERROR;
	if (smart_log.critical_warning & notify->crit_warn) {
		snprintf(description, sizeof(description), "SMART data has critical warning bits set: 0x%x\n"
				"The following is the meaning of each bit:\n"
				"\t0x%.2x - Available spare capacity\n"
				"\t0x%.2x - Temperature\n"
				"\t0x%.2x - NVM subsystem reliability\n"
				"\t0x%.2x - Media read-only\n"
				"\t0x%.2x - Volatile memory backup\n"
				"\t0x%.2x - Persistent Memory Region", smart_log.critical_warning,
				CRIT_WARN_SPARE, CRIT_WARN_TEMP, CRIT_WARN_DEGRADED, CRIT_WARN_RO,
				CRIT_WARN_VOLATILE_MEM, CRIT_WARN_PMR_RO);
		if (!raw_data && fill_raw_data(&raw_data, &raw_data_len, &smart_log, &vpd) != 0) {
			free(raw_data);
			return -1;
		}
		rc += log_event(&event_id, &vpd, controller_name, severity, description, raw_data, raw_data_len);
	}

	/* Create a warning event in case average rate of writing exceeds device endurance for a day
	 * This is an estimate since data units written from SMART data doesn't account for metadata
	 * Initial usage of a device might be intense on the beggining, so only do the calculation
	 * if device has been powered on for a certain amount of time
	 */
	power_on_hours = uint128_to_long_double(smart_log.power_on_hours);
	data_units_written = uint128_to_long_double(smart_log.data_units_written);

	/* Fields in the VPD log might have not been null terminated so add an extra character for it */
	snprintf(endurance_s, sizeof(endurance_s), "%s", vpd.endurance);
	snprintf(capacity_s, sizeof(capacity_s), "%s", vpd.capacity);
	endurance = atof(endurance_s);
	capacity = atol(capacity_s);
	if (power_on_hours >= MIN_HOURS_ON && endurance && capacity && notify->write_rate) {
		avg_dwpd = data_units_written * (1000 * 512 * 24 / (power_on_hours * 1000000000 * capacity));
		if (avg_dwpd > endurance) {
			severity = SL_SEV_WARNING;
			snprintf(description, sizeof(description), "Drive Writes Per Day (DWPD) average are exceeding the device endurance.\n"
					"Current device DWPD average: %.3Lf\n"
					"Device advertised DWPD endurance: %.3lf\n",
					avg_dwpd, endurance);
			if (!raw_data && fill_raw_data(&raw_data, &raw_data_len, &smart_log, &vpd) != 0) {
				free(raw_data);
				return -1;
			}
			rc += log_event(&event_id, &vpd, controller_name, severity, description, raw_data, raw_data_len);
		}
	}

	free(raw_data);
	return rc;
}

/* fill_item_ld - Fill item with long double data
 * @item - Address of item struct to be filled
 * @keyword - Address of keyword string to fill in item struct
 * @data - long double value to fill in item struct
 *
 * Fill item struct with a keyword string and a long double data converted into ASCII.
 */
static void fill_item_ld(struct item *item, char *keyword, long double data) {
	int length;

	if(!item || !keyword)
		return;

	memset(item->header.keyword, '\0', sizeof(item->header.keyword));
	memset(item->data, '\0', sizeof(item->data));

	strncpy(item->header.keyword, keyword, sizeof(item->header.keyword));
	length = snprintf(item->data, sizeof(item->data), "%0.Lf", data);
	item->header.length = (length > ITEM_DATA_LENGTH) ? ITEM_DATA_LENGTH : length;
}

/* fill_item_string - Fill item with string data
 * @item - Address of item struct to be filled
 * @keyword - Address of keyword string to fill in item struct
 * @data - Address of data string to fill in item struct
 * @data_len - Length of data string
 *
 * Fill item struct with a keyword string and the data string.
 */
static void fill_item_string(struct item *item, char *keyword, char *data, size_t data_len) {

	if(!item || !keyword || !data)
		return;

	memset(item->header.keyword, '\0', sizeof(item->header.keyword));
	memset(item->data, '\0', sizeof(item->data));

	strncpy(item->header.keyword, keyword, sizeof(item->header.keyword));
	if (data_len > ITEM_DATA_LENGTH)
		data_len = ITEM_DATA_LENGTH;
	item->header.length = snprintf(item->data, sizeof(item->data), "%.*s", (int) data_len, data);
}

/* fill_raw_data - Fill the raw_data field of a servicelog event
 * @raw_data - Address of pointer to the binary data to be filled, user is expected to free memory
 * 	 	allocated for raw_data to avoid memory leak
 * @raw_data_len - Address of variable to set with length (in bytes) of raw_data that was filled
 * @log - Address of vpd struct containing SMART data to fill raw_data
 * @vpd - Address of vpd struct containing IBM VPD data to fill raw_data
 * @controller_name - Name of the device to log event against, expected format is nvme[0-9]+
 *
 * User is expected to free memory allocated for raw_data
 *
 * Fill the raw_data field with any extra data such as VPD and SMART to be logged in the servicelog
 * database. The data itself is formatted as follows so it provides an extensible way to add more
 * data in the future if needed:
 *
 * The raw_data format is made of sections, each section contains a one-byte tag which identifies
 * what is the data contained in this section, an additional one-byte reserved and two-byte length
 * which represents the length value (in bytes) of all the data fields contained in this section,
 * followed by the data items, so a section has the following format:
 * +------------+----------------------------------------------+
 * |   Offset   |                Field meaning                 |
 * +------------+----------------------------------------------+
 * |   Byte 0   |               Identifier Tag                 |
 * |            |                                              |
 * |   Byte 1   |                  Reserved                    |
 * |            |                                              |
 * |   Byte 2   | Length (in bytes) of data - bits[7:0] (LSB)  |
 * |            |                                              |
 * |   Byte 3   | Length (in bytes) of data - bits[15:8] (MSB) |
 * |            |                                              |
 * | Byte 4 - N |                 Data items                   |
 * +------------+----------------------------------------------+
 *
 * The current tags in use are the following:
 * +---------+------------+
 * | Tag 01h | SMART data |
 * +---------+------------+
 * | Tag 02h |  VPD data  |
 * +---------+------------+
 * | Tag FFh |  End Tag   |
 * +---------+------------+
 * For the end tag (FFh) no additional data items are present along with the header, so it is a
 * section with the other header bytes set to 0.
 *
 * Each data item consists of a four-byte header followed by some amount of data, The format of
 * this header is as follows: a keyword is a two-character (ASCII) mnemonic that uniquely
 * identifies the information in the field, one additional reserved byte and the last byte of the
 * header is binary and represents the length value (in bytes) of the data that follows. The
 * keyword data is provided as ASCII characters. The following is an example:
 *          |      +0       |      +1       |      +2       |      +3       |
 *          |               |               |               |               |
 *          |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
 *          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *          |            Keyword            |   Reserved    |   Length (N)  |
 * Byte 0 ->|                               |               |               |
 *          +-------------------------------+---------------+---------------+
 *          |               Data (byte 4 -> N+3, example here N=4)          |
 * Byte 4 ->|                                                               |
 *          +---------------------------------------------------------------+
 *
 * Return - Return 0 on success, -1 on error
 */
static int fill_raw_data(unsigned char **raw_data, uint32_t *raw_data_len, struct nvme_smart_log_page *log, struct nvme_ibm_vpd *vpd) {
	struct section section = { 0 };

	if (!raw_data || !raw_data_len) {
		fprintf(stderr, "raw_data or raw_data_len parameter for %s is NULL, aborting\n", __func__);
		return -1;
	}

	if (log)
		if(raw_data_smart(raw_data, raw_data_len, log))
			goto err_out;
	if (vpd)
		if(raw_data_vpd(raw_data, raw_data_len, vpd))
			goto err_out;

	/* Append end tag section */
	section.header.tag = 0xFF;
	if(append_section(raw_data, raw_data_len, &section))
		goto err_out;

	return 0;

err_out:
	fprintf(stderr, "raw_data was not filled properly, aborting\n");
	return -1;
}

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

/* get_smart_file - Process SMART data from a file
 * @file_path - Null terminated pathname of file which SMART data will be retrived from
 * @log - Address of log struct to store the SMART data
 *
 * Read SMART data from file and fill the nvme_smart_log_page struct with its content
 *
 * Return - Return 0 on success, -1 on SMART file error, -2 open error
 */
extern int get_smart_file(char *file_path, struct nvme_smart_log_page *log) {
	int num_elements = 0;
	struct dictionary dict[MAX_DICT_ELEMENTS];

	if ((num_elements = read_file_dict(file_path, dict, MAX_DICT_ELEMENTS)) < 0) {
		fprintf(stderr, "read_file_dict failed: %s, rc % d\n",
				file_path, num_elements);
		return num_elements;
	}
	return set_smart_log_field(log, dict, num_elements);
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

static bool in_range_max (long double value, long double max, char *key) {
	bool rc;
	if (!(rc = (0 <= value && value <= max)))
		fprintf(stderr, "Value %.0Lf for key %s is not in expected range of 0 - %.0Lf\n",
				value, key, max);
	return rc;
}

static bool is_positive (long double value, char *key) {
	bool rc;
	if (!(rc = (0 <= value)))
		fprintf(stderr, "Value %.0Lf for key %s should be positive\n", value, key);
	return rc;
}

/* location_code_nvme - Get location code of NVMe controller
 * @location - Address to store the NVMe controller location code, max length is LOCATION_LENGTH
 * @controller_name -  Name of the device to retrieve location code, expected format is nvme[0-9]+
 *
 * Retrieve location code from device tree for the NVMe controller specified, in order to map the
 * device to the proper device tree path we need to look into the device devspec file in sysfs.
 *
 * Return - Return 0 on success, -1 on error
 */
extern int location_code_nvme(char *location, char *controller_name) {
	int fd;
	char sysfs_devspec_path[PATH_MAX], dt_location_path[PATH_MAX];
	char devspec[2*NAME_MAX]= { '\0' }, *newline;

	snprintf(sysfs_devspec_path,sizeof(sysfs_devspec_path), "%s/%s/device/devspec",
			NVME_SYS_PATH, controller_name);
	fd = open(sysfs_devspec_path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s open failed: %s\n", sysfs_devspec_path, strerror(errno));
		return -1;
	}
	if (read(fd, devspec, sizeof(devspec)) <= 0)
		goto read_error;
	close(fd);

	/* devspec might be reported with a trailing newline in case the kernel running contains
	 * commit 14c19b2a40b6 ("PCI/sysfs: Add 'devspec' newline"), so remove it if present */
	newline = strchr(devspec, '\n');
	if (newline)
		*newline = '\0';

	snprintf(dt_location_path, sizeof(dt_location_path), "%s%s/ibm,loc-code",
			DEVICE_TREE_PATH, devspec);
	fd = open(dt_location_path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s open failed: %s\n", dt_location_path, strerror(errno));
		return -1;
	}
	if (read(fd, location, LOCATION_LENGTH) <= 0)
		goto read_error;
	close(fd);

	return 0;

read_error:
	fprintf(stderr, "Failed to read any data from file, location code not retrieved\n");
	close(fd);
	return -1;
}

/* log_event - Create event in servicelog database
 * @event_id - Address to store the event identifier created in the servicelog database
 * @vpd - Address of vpd struct containing IBM VPD data to fill relevant event fields
 * @controller_name - Name of the device to log event against, expected format is nvme[0-9]+
 * @severity - severity to set for the event, one of the SL_SEV_* defined in servicelog.h
 * @description - String to be used as the description for the event, max length is DESCR_LENGTH
 * @raw_data - Address of binary data to be logged along with the event, NULL if not needed
 * @raw_data_len - Length of the raw_data information, 0 if not needed
 *
 * Log an event into the servicelog database by setting all the sl_event, sl_data_os and sl_callout
 * structures with relevant data for the NVMe device
 *
 * Return - Return 0 on success, -1 on error
 */
static int log_event(uint64_t *event_id, struct nvme_ibm_vpd *vpd, char *controller_name, uint8_t severity, char *description, unsigned char *raw_data, uint32_t raw_data_len) {
	char location[LOCATION_LENGTH] = { '\0' };
	int rc = -1, tmp_rc;
	struct servicelog *slog;
	struct sl_event *entry = NULL;
	struct sl_data_os *os = NULL;
	struct utsname uname_data;

	if (!(entry = calloc(1, sizeof(struct sl_event))))
		goto err_out;

	/* Fill sl_data_os struct */
	if (!(os = calloc(1, sizeof(struct sl_data_os))))
		goto err_out;
	entry->addl_data = os;

	tmp_rc = uname(&uname_data);
	if (tmp_rc) {
		fprintf(stderr, "uname error: %d\n", tmp_rc);
		goto out;
	}
	if (!(os->version = strdup(uname_data.release)))
		goto err_out;
	if (!(os->driver = strdup("nvme")))
		goto err_out;
	if (!(os->subsystem = strdup("storage")))
		goto err_out;
	if (!(os->device = strndup(controller_name, NAME_MAX)))
		goto err_out;

	/* Fill sl_callout struct
	 * Callout data fields can't have white spaces or it will mess the parsing in
	 * servevent_parse.pl which split the input through spaces
	 */
        if (!(entry->callouts = calloc(1, sizeof(struct sl_callout))))
                goto err_out;

	if (location_code_nvme(location, controller_name) < 0)
		goto out;
	entry->callouts->priority = 'M';
	if (!(entry->callouts->location = strndup(location, sizeof(location))))
		goto err_out;
	trim_trail_space(entry->callouts->location);
	if (!(entry->callouts->fru = strndup(vpd->fru_pn, sizeof(vpd->fru_pn))))
		goto err_out;
	trim_trail_space(entry->callouts->fru);
	if (!(entry->callouts->ccin = strndup(vpd->ccin, sizeof(vpd->ccin))))
		goto err_out;
	trim_trail_space(entry->callouts->ccin);
	if (!(entry->callouts->serial = strndup(vpd->manufacture_sn, sizeof(vpd->manufacture_sn))))
		goto err_out;
	trim_trail_space(entry->callouts->serial);

	/* Fill sl_event struct */
	time(&entry->time_event);
	entry->type = SL_TYPE_OS;
	entry->severity = severity;

	if (severity >= SL_SEV_ERROR_LOCAL) {
		entry->disposition = SL_DISP_UNRECOVERABLE;
		entry->serviceable = 1;
		entry->call_home_status = SL_CALLHOME_CANDIDATE;
		if (check_open_serv_event(location)) {
			fprintf(stdout, "There is an open serviceable event for %s at %s, no new serviceable event will be reported for this device, when open event is solved use log_repair_action to close the event\n", controller_name, location);
			rc = 0;
			goto out;
		}
	}
	else {
		entry->disposition = SL_DISP_RECOVERABLE;
		entry->serviceable = 0;
		entry->call_home_status = SL_CALLHOME_NONE;
	}
	if (!(entry->description = strndup(description, DESCR_LENGTH)))
		goto err_out;
        if (!(entry->refcode = strdup("0000000")))
		goto err_out;

	if (raw_data && raw_data_len) {
		if (!(entry->raw_data = malloc(raw_data_len)))
			goto err_out;
		entry->raw_data_len = raw_data_len;
		memcpy(entry->raw_data, raw_data, raw_data_len);
	}

	tmp_rc = servicelog_open(&slog, 0);
	if (tmp_rc) {
		fprintf(stderr, "Log open error: %s\n", servicelog_error(slog));
		goto out;
	}
	tmp_rc = servicelog_event_log(slog, entry, event_id);
	servicelog_close(slog);
	if (tmp_rc) {
		fprintf(stderr, "Event logging error: %s\n", servicelog_error(slog));
		goto out;
	}
	else
		fprintf(stdout, "Event ID %lu has been logged for device %s at %s, please check servicelog for more information\n", *event_id, controller_name, location);

	rc = 0;
	goto out;

err_out:
	fprintf(stderr, "Memory allocation failed in %s\n", __func__);
out:
	servicelog_event_free(entry);
	return rc;
}

static void long_double_to_uint128(long double value, uint8_t *data) {
	int i;

	for (i = 0; i < 16; i++) {
		data[i] = fmodl(value, 256);
		value /= 256;
	}
}

/* open_nvme - Open NVMe device
 * @dev_path - Full path to NVMe device, expected format is /dev/nvme*
 *
 * The function tries to do some sanity check to make sure the device opened is at least a block or
 * character device prior to return the file descriptor index
 *
 * Return - Return file descriptor index on success, -1 on error
 */
extern int open_nvme(char *dev_path) {
	int fd;
        struct stat stat;

	if (strncmp(dev_path, NVME_DEV_PATH, strlen(NVME_DEV_PATH)) != 0) {
		fprintf(stderr, "%s not a NVMe device\n", dev_path);
		return -1;
	}

	fd = open(dev_path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s open failed: %s\n", dev_path, strerror(errno));
		return -1;
	}
	if (fstat(fd, &stat) < 0) {
		fprintf(stderr, "%s fstat failed: %s\n", dev_path, strerror(errno));
		close(fd);
		return -1;
	}
	switch (stat.st_mode & S_IFMT) {
		case S_IFBLK:
		case S_IFCHR:
			return fd;
		default:
			fprintf(stderr, "%s is not a block or character device\n", dev_path);
			close(fd);
			return -1;
	}
}

static void print_usage(char *command) {
	printf("Usage: %s [-h] [<nvmen ...>]\n"
		"\t-h or --help: print this help message\n"
		"\t<nvmen ...>: the NVMe devices on which to operate, for\n"
		"\t                  example nvme0; if not specified, all detected\n"
		"\t                  nvme devices will be diagnosed\n", command);
}

/* raw_data_smart - Fill the raw_data field with SMART information
 * @raw_data - Address of pointer to the binary data to be filled, user is expected to free memory
 *              allocated for raw_data to avoid memory leak
 * @raw_data_len - Address of variable to set with length (in bytes) of raw_data that was filled
 * @vpd - Address of vpd struct containing IBM VPD data to fill raw_data
 *
 * User is expected to free memory allocated for raw_data
 *
 * Fill the raw_data field with SMART section (tag 0x01). Data format to be followed is described
 * in fill_raw_data function description.
 *
 * Return - Return 0 on success, -1 on error
 */
static int raw_data_smart(unsigned char **raw_data, uint32_t *raw_data_len, struct nvme_smart_log_page *log) {
	int rc = -1;
	struct section section = { 0 };
	struct item item = { 0 };

	section.header.tag = 0x01;

	fill_item_ld(&item, "CW", log->critical_warning);
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "T0", le16toh(log->composite_temp));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "AS", log->avail_spare);
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "ST", log->avail_spare_threshold);
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "PU", log->percentage_used);
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "EG", log->endurance_group_crit_warn_summary);
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "DR", uint128_to_long_double(log->data_units_read));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "DW", uint128_to_long_double(log->data_units_written));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "HR", uint128_to_long_double(log->host_reads_cmd));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "HW", uint128_to_long_double(log->host_writes_cmd));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "BT", uint128_to_long_double(log->ctrl_busy_time));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "PC", uint128_to_long_double(log->power_cycles));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "PH", uint128_to_long_double(log->power_on_hours));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "US", uint128_to_long_double(log->unsafe_shutdowns));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "DI", uint128_to_long_double(log->media_data_integrity_err));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "EI", uint128_to_long_double(log->num_err_info_log_entries));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "TW", le32toh(log->warn_composite_temp_time));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "TC", le32toh(log->crit_composite_temp_time));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "T1", le16toh(log->temp_sensor1));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "T2", le16toh(log->temp_sensor2));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "T3", le16toh(log->temp_sensor3));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "T4", le16toh(log->temp_sensor3));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "T5", le16toh(log->temp_sensor5));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "T6", le16toh(log->temp_sensor6));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "T7", le16toh(log->temp_sensor7));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "T8", le16toh(log->temp_sensor8));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "M1", le32toh(log->thrm_mgmt_temp1_trans_count));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "M2", le32toh(log->thrm_mgmt_temp2_trans_count));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "M3", le32toh(log->total_time_thm_mgmt_temp1));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_ld(&item, "M4", le32toh(log->total_time_thm_mgmt_temp2));
	if(append_item(&section, &item) < -1)
		goto err_out;

	if(append_section(raw_data, raw_data_len, &section) < -1)
		goto err_out;
	rc = 0;

err_out:
	free(section.data);
	return rc;
}

/* raw_data_vpd - Fill the raw_data field with VPD information
 * @raw_data - Address of pointer to the binary data to be filled, user is expected to free memory
 *              allocated for raw_data to avoid memory leak
 * @raw_data_len - Address of variable to set with length (in bytes) of raw_data that was filled
 * @vpd - Address of vpd struct containing IBM VPD data to fill raw_data
 *
 * User is expected to free memory allocated for raw_data
 *
 * Fill the raw_data field with VPD section (tag 0x02). Data format to be followed is described in
 * fill_raw_data function description.
 *
 * Return - Return 0 on success, -1 on error
 */
static int raw_data_vpd(unsigned char **raw_data, uint32_t *raw_data_len, struct nvme_ibm_vpd *vpd) {
	int rc = -1;
	struct section section = { 0 };
	struct item item = { 0 };

	section.header.tag = 0x02;

	fill_item_string(&item, "Z1", vpd->endurance, sizeof(vpd->endurance));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_string(&item, "Z2", vpd->capacity, sizeof(vpd->capacity));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_string(&item, "Z3", vpd->warranty, sizeof(vpd->warranty));
	if(append_item(&section, &item) < -1)
		goto err_out;
	fill_item_string(&item, "RM", vpd->firmware_level, sizeof(vpd->firmware_level));
	if(append_item(&section, &item) < -1)
		goto err_out;

	if(append_section(raw_data, raw_data_len, &section) < -1)
		goto err_out;
	rc = 0;

err_out:
	free(section.data);
	return rc;
}

/* read_file_dict - Read a file and parse the key=value settings
 * @file_name - File to be parsed, expected format of lines in it is a string key and number value
 * @dict - Pointer to the array of dictionary structs to be filled when parsing the file
 * @max_elements - Length of the dict array
 *
 * This function reads a file and extracts the key=value elements found in it and fill the dict
 * array.
 *
 * Return - Return number of elements filled in the array, -1 config file error, -2 open failure
 */
extern int read_file_dict(char *file_name, struct dictionary *dict, int max_elements) {
	FILE *fp;
	int line, num_elements = 0;
	char *line_string = NULL;
	size_t length = 0;

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s open failed: %s\n", file_name, strerror(errno));
		return -2;
	}
	for (line = 1; getline(&line_string, &length, fp) != -1 &&
			num_elements < max_elements; line++) {
		if (sscanf(line_string, " %[\n\r#/]", dict->key))
			continue;
		/* Width in sscanf should be (KEY_LENGTH - 1) when attempting to read dictionary key */
		if (sscanf(line_string, " %127[^= ] = %Lf", dict->key, &dict->value) < 2) {
			fprintf(stderr, "File: %s error at line %d: %s\n", file_name, line, line_string);
			free(line_string);
			fclose(fp);
			return -1;
		}
		dict++, num_elements++;
	}
	free(line_string);
	fclose(fp);
	return num_elements;
}

/* regex_controller - Extract controller name from device name
 * @device_name - Name of the NVMe device submitted by user or extracted from sysfs
 * @controller_name - Address to store the NVMe controller string extracted from regex, format is
 * nvme[0-9]+
 *
 * A user might submit the NVMe device name pointing to a namespace or partitition, so attempt to
 * sanitize the input and extract the actual NVMe controller of the string.
 *
 * Return - Return zero on success, otherwise a failure ocurred.
 */
static int regex_controller(char *controller_name, char *device_name) {
	int rc = 0;
	regex_t regex;
	char *re = "nvme[0-9]+";
	regmatch_t  match[1];

	if ((rc = regcomp(&regex, re, REG_EXTENDED))) {
		fprintf(stderr, "regcomp failed with return code %d\n", rc);
		return rc;
	}
	if ((rc = regexec(&regex, device_name, 1, match, 0))) {
		fprintf(stderr, "Controller regex failed for %s, with return code %d, expected format to be encountered is %s\n", device_name, rc, re);
		regfree(&regex);
		return rc;
	}
	if (match[0].rm_eo - match[0].rm_so >= NAME_MAX) {
		fprintf(stderr, "Name of NVMe controller is longer than maximum expected name of %d\n", NAME_MAX);
		regfree(&regex);
		return -1;
	}
	strncpy(controller_name, device_name + match[0].rm_so, match[0].rm_eo - match[0].rm_so);
	regfree(&regex);
	return rc;
}

static void set_notify(struct notify *notify, struct dictionary *dict, int num_elements) {
	for (int counter = 1; counter <= num_elements; counter++) {
		if (!strcmp(dict->key, "SMART_LOG_MISSING"))
			notify->smart_missing = dict->value;
		else if (!strcmp(dict->key, "WRITE_RATE"))
			notify->write_rate = dict->value;
		else if (!strcmp(dict->key, "AVAILABLE_SPARE_CAPACITY") && dict->value == 0)
			notify->crit_warn &= ~(CRIT_WARN_SPARE);
		else if (!strcmp(dict->key, "TEMPERATURE_THRESHOLD") && dict->value == 0)
			notify->crit_warn &= ~(CRIT_WARN_TEMP);
		else if (!strcmp(dict->key, "NVM_SUBSYSTEM_RELIABILITY") && dict->value == 0)
			notify->crit_warn &= ~(CRIT_WARN_DEGRADED);
		else if (!strcmp(dict->key, "MEDIA_READ_ONLY") && dict->value == 0)
			notify->crit_warn &= ~(CRIT_WARN_RO);
		else if (!strcmp(dict->key, "VOLATILE_MEMORY_BACKUP") && dict->value == 0)
			notify->crit_warn &= ~(CRIT_WARN_VOLATILE_MEM);
		else if (!strcmp(dict->key, "PERSISTENT_MEMORY_REGION") && dict->value == 0)
			notify->crit_warn &= ~(CRIT_WARN_PMR_RO);
		dict++;
	}
}

extern int set_smart_log_field(struct nvme_smart_log_page *log, struct dictionary *dict, int num_elements) {
	for (int counter = 1; counter <= num_elements; counter++) {
		if (!strcmp(dict->key, "CRIT_WARN")) {
			if (!in_range_max(dict->value, UINT8_MAX, dict->key))
				return -1;
			log->critical_warning = dict->value;
		} else if (!strcmp(dict->key, "COMP_TEMP")) {
			if (!in_range_max(dict->value, UINT16_MAX, dict->key))
				return -1;
			log->composite_temp = htole16(dict->value);
		} else if (!strcmp(dict->key, "AVAIL_SPARE")) {
			if (!in_range_max(dict->value, UINT8_MAX, dict->key))
				return -1;
			log->avail_spare = dict->value;
		} else if (!strcmp(dict->key, "AVAIL_SPARE_THRES")) {
			if (!in_range_max(dict->value, UINT8_MAX, dict->key))
				return -1;
			log->avail_spare_threshold = dict->value;
		} else if (!strcmp(dict->key, "PCT_USED")) {
			if (!in_range_max(dict->value, UINT8_MAX, dict->key))
				return -1;
			log->percentage_used = dict->value;
		} else if (!strcmp(dict->key, "END_GRP_CRIT_WARN")) {
			if (!in_range_max(dict->value, UINT8_MAX, dict->key))
				return -1;
			log->endurance_group_crit_warn_summary = dict->value;
		} else if (!strcmp(dict->key, "UNITS_READ")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->data_units_read);
		} else if (!strcmp(dict->key, "UNITS_WRITTEN")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->data_units_written);
		} else if (!strcmp(dict->key, "HOST_READ_CMD")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->host_reads_cmd);
		} else if (!strcmp(dict->key, "HOST_WRITE_CMD")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->host_writes_cmd);
		} else if (!strcmp(dict->key, "CTRL_BUSY_TIME")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->ctrl_busy_time);
		} else if (!strcmp(dict->key, "PWR_CYCLES")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->power_cycles);
		} else if (!strcmp(dict->key, "PWR_ON_HOURS")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->power_on_hours);
		} else if (!strcmp(dict->key, "UNSAFE_SHUTDOWN")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->unsafe_shutdowns);
		} else if (!strcmp(dict->key, "MEDIA_ERR")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->media_data_integrity_err);
		} else if (!strcmp(dict->key, "ERR_LOG")) {
			if (!is_positive(dict->value, dict->key))
				return -1;
			long_double_to_uint128(dict->value, log->num_err_info_log_entries);
		} else if (!strcmp(dict->key, "WARN_TEMP_TIME")) {
			if (!in_range_max(dict->value, UINT32_MAX, dict->key))
				return -1;
			log->warn_composite_temp_time = htole32(dict->value);
		} else if (!strcmp(dict->key, "CRIT_TEMP_TIME")) {
			if (!in_range_max(dict->value, UINT32_MAX, dict->key))
				return -1;
			log->crit_composite_temp_time = htole32(dict->value);
		} else if (!strcmp(dict->key, "TEMP_SENSOR1")) {
			if (!in_range_max(dict->value, UINT16_MAX, dict->key))
				return -1;
			log->temp_sensor1 = htole16(dict->value);
		} else if (!strcmp(dict->key, "TEMP_SENSOR2")) {
			if (!in_range_max(dict->value, UINT16_MAX, dict->key))
				return -1;
			log->temp_sensor2 = htole16(dict->value);
		} else if (!strcmp(dict->key, "TEMP_SENSOR3")) {
			if (!in_range_max(dict->value, UINT16_MAX, dict->key))
				return -1;
			log->temp_sensor3 = htole16(dict->value);
		} else if (!strcmp(dict->key, "TEMP_SENSOR4")) {
			if (!in_range_max(dict->value, UINT16_MAX, dict->key))
				return -1;
			log->temp_sensor4 = htole16(dict->value);
		} else if (!strcmp(dict->key, "TEMP_SENSOR5")) {
			if (!in_range_max(dict->value, UINT16_MAX, dict->key))
				return -1;
			log->temp_sensor5 = htole16(dict->value);
		} else if (!strcmp(dict->key, "TEMP_SENSOR6")) {
			if (!in_range_max(dict->value, UINT16_MAX, dict->key))
				return -1;
			log->temp_sensor6 = htole16(dict->value);
		} else if (!strcmp(dict->key, "TEMP_SENSOR7")) {
			if (!in_range_max(dict->value, UINT16_MAX, dict->key))
				return -1;
			log->temp_sensor7 = htole16(dict->value);
		} else if (!strcmp(dict->key, "TEMP_SENSOR8")) {
			if (!in_range_max(dict->value, UINT16_MAX, dict->key))
				return -1;
			log->temp_sensor8 = htole16(dict->value);
		} else if (!strcmp(dict->key, "THRM_TEMP1_TRANS")) {
			if (!in_range_max(dict->value, UINT32_MAX, dict->key))
				return -1;
			log->thrm_mgmt_temp1_trans_count = htole32(dict->value);
		} else if (!strcmp(dict->key, "THRM_TEMP2_TRANS")) {
			if (!in_range_max(dict->value, UINT32_MAX, dict->key))
				return -1;
			log->thrm_mgmt_temp2_trans_count = htole32(dict->value);
		} else if (!strcmp(dict->key, "TOTAL_THM_TEMP1")) {
			if (!in_range_max(dict->value, UINT32_MAX, dict->key))
				return -1;
			log->total_time_thm_mgmt_temp1 = htole32(dict->value);
		} else if (!strcmp(dict->key, "TOTAL_THM_TEMP2")) {
			if (!in_range_max(dict->value, UINT32_MAX, dict->key))
				return -1;
			log->total_time_thm_mgmt_temp2 = htole32(dict->value);
		}
		dict++;
	}
	return 0;
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

static long double uint128_to_long_double(uint8_t *data) {
	int i;
	long double value = 0;

	for (i = 0; i < 16; i++) {
		value *= 256;
		value += data[15 - i];
	}
	return value;
}

extern void write_smart_file(FILE *fp, struct nvme_smart_log_page *log) {
	fprintf(fp, "CRIT_WARN = %u # Length %ld byte\n",
			log->critical_warning, sizeof(log->critical_warning));
	fprintf(fp, "COMP_TEMP = %u # Length %ld byte\n",
			le16toh(log->composite_temp), sizeof(log->composite_temp));
	fprintf(fp, "AVAIL_SPARE = %u # Length %ld byte\n",
			log->avail_spare, sizeof(log->avail_spare));
	fprintf(fp, "AVAIL_SPARE_THRES = %u # Length %ld byte\n",
			log->avail_spare_threshold, sizeof(log->avail_spare_threshold));
	fprintf(fp, "PCT_USED = %u # Length %ld byte\n",
			log->percentage_used, sizeof(log->percentage_used));
	fprintf(fp, "END_GRP_CRIT_WARN = %u # Length %ld byte\n",
			log->endurance_group_crit_warn_summary, sizeof(log->endurance_group_crit_warn_summary));
	fprintf(fp, "UNITS_READ = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->data_units_read), sizeof(log->data_units_read));
	fprintf(fp, "UNITS_WRITTEN = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->data_units_written), sizeof(log->data_units_written));
	fprintf(fp, "HOST_READ_CMD = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->host_reads_cmd), sizeof(log->host_reads_cmd));
	fprintf(fp, "HOST_WRITE_CMD = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->host_writes_cmd), sizeof(log->host_writes_cmd));
	fprintf(fp, "CTRL_BUSY_TIME = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->ctrl_busy_time), sizeof(log->ctrl_busy_time));
	fprintf(fp, "PWR_CYCLES = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->power_cycles), sizeof(log->power_cycles));
	fprintf(fp, "PWR_ON_HOURS = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->power_on_hours), sizeof(log->power_on_hours));
	fprintf(fp, "UNSAFE_SHUTDOWN = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->unsafe_shutdowns), sizeof(log->unsafe_shutdowns));
	fprintf(fp, "MEDIA_ERR = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->media_data_integrity_err), sizeof(log->media_data_integrity_err));
	fprintf(fp, "ERR_LOG = %.0Lf # Length %ld byte\n",
			uint128_to_long_double(log->num_err_info_log_entries), sizeof(log->num_err_info_log_entries));
	fprintf(fp, "WARN_TEMP_TIME = %u # Length %ld byte\n",
			le32toh(log->warn_composite_temp_time), sizeof(log->warn_composite_temp_time));
	fprintf(fp, "CRIT_TEMP_TIME = %u # Length %ld byte\n",
			le32toh(log->crit_composite_temp_time), sizeof(log->crit_composite_temp_time));
	fprintf(fp, "TEMP_SENSOR1 = %u # Length %ld byte\n",
			le16toh(log->temp_sensor1), sizeof(log->temp_sensor1));
	fprintf(fp, "TEMP_SENSOR2 = %u # Length %ld byte\n",
			le16toh(log->temp_sensor2), sizeof(log->temp_sensor2));
	fprintf(fp, "TEMP_SENSOR3 = %u # Length %ld byte\n",
			le16toh(log->temp_sensor3), sizeof(log->temp_sensor3));
	fprintf(fp, "TEMP_SENSOR4 = %u # Length %ld byte\n",
			le16toh(log->temp_sensor4), sizeof(log->temp_sensor4));
	fprintf(fp, "TEMP_SENSOR5 = %u # Length %ld byte\n",
			le16toh(log->temp_sensor5), sizeof(log->temp_sensor5));
	fprintf(fp, "TEMP_SENSOR6 = %u # Length %ld byte\n",
			le16toh(log->temp_sensor6), sizeof(log->temp_sensor6));
	fprintf(fp, "TEMP_SENSOR7 = %u # Length %ld byte\n",
			le16toh(log->temp_sensor7), sizeof(log->temp_sensor7));
	fprintf(fp, "TEMP_SENSOR8 = %u # Length %ld byte\n",
			le16toh(log->temp_sensor8), sizeof(log->temp_sensor8));
	fprintf(fp, "THRM_TEMP1_TRANS = %u # Length %ld byte\n",
			le32toh(log->thrm_mgmt_temp1_trans_count), sizeof(log->thrm_mgmt_temp1_trans_count));
	fprintf(fp, "THRM_TEMP2_TRANS = %u # Length %ld byte\n",
			le32toh(log->thrm_mgmt_temp2_trans_count), sizeof(log->thrm_mgmt_temp2_trans_count));
	fprintf(fp, "TOTAL_THM_TEMP1 = %u # Length %ld byte\n",
			le32toh(log->total_time_thm_mgmt_temp1), sizeof(log->total_time_thm_mgmt_temp1));
	fprintf(fp, "TOTAL_THM_TEMP2 = %u # Length %ld byte\n",
			le32toh(log->total_time_thm_mgmt_temp2), sizeof(log->total_time_thm_mgmt_temp2));
}
