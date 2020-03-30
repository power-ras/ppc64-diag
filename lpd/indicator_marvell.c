/**
 * @file	indicator_marvell.c
 * @brief	Marvell HDD LEDs (indicators) manipulation routines
 *
 * Copyright (C) 2016 IBM Corporation
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
 * @author	Mauricio Faria de Oliveira <mauricfo@linux.vnet.ibm.com>
 * @author	Douglas Miller <dougmill@us.ibm.com>
 */

#define _GNU_SOURCE

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "indicator.h"
#include "lp_util.h"
#include "utils.h"

/*
 * Path to block devices in sysfs
 */
#define SYS_BLOCK_PATH		"/sys/block"

/*
 * PCI IDs of the Marvell 9235 SATA controller in
 * IBM Power System S822LC for HPC (Garrison/Minsky).
 */
#define MV_PCI_VID		"0x1b4b"
#define MV_PCI_DID		"0x9235"
#define MV_PCI_SVID		"0x1014"
#define MV_PCI_SSID		"0x0612"
#define MV_PCI_ID_BUF_LEN	7	/* "0x3456" and '\0' */

/*
 * Vendor-Specific Registers (VSR) are accessed indirectly
 * through a pair of registers in PCI BAR5.
 *
 * First, set the VSR address in VSR_ADDR;
 * Then, read/write data from/to VSR_DATA.
 */
static const uint32_t mv_pci_bar5_vsr_addr = 0xa8;
static const uint32_t mv_pci_bar5_vsr_data = 0xac;
static const uint32_t mv_pci_bar5_length   = 2048;

/*
 * The LEDs are controlled via GPIO pins or SATA link/activity status (default).
 *
 * This involves 3 VSRs:
 * - The LED mode of a SATA port is set in the PORT_ACTIVE_SEL register.
 *   (either GPIO pin state or SATA link/activity status for all/one SATA port)
 * - The GPIO pin state is set in the DATA_OUT register.
 * - The GPIO pin state output is enabled in the DATA_OUT_ENABLE register.
 *
 * On Garrison/Minsky the identification LED of the SATA port 'N' is wired
 * to the GPIO pin of the SATA port 'N + 2' (the max. number of disks is 2).
 * The PORT_ACTIVE_SEL and DATA_OUT_ENABLE registers are set by default so
 * that SATA ports 3 and 4 (that is, the identification LEDs of SATA ports
 * 1 and 2) are GPIO-pin based, and only the DATA_OUT register needs to be
 * modified to (un)light the identification LEDs (a.k.a. fault LEDs).
 *
 * Register information:
 *
 * GPIO Data Out:
 * - 1 bit per GPIO pin (31-0)
 * - default: 0x00000018 (all low, except for pins 3 and 4)
 * - The LEDs are ON on low and OFF on high.
 *
 * GPIO Data Out Enable:
 * - 1 bit per GPIO bin (31-0)
 * - default: 0xffffffe7 (all high, except for pins 3 and 4)
 * - active low (data output enabled on bit value 0)
 *
 * Port Active Enable (or Port GPIOx Active Select)
 * - bits 31-30: reserved (zero)
 * - bits 29-00: 10x 3-bit mode (SATA ports 9-0)
 * - default: 0x2db6da8d
 */
static const uint32_t mv_gpio_data_out = 0x07071020;
static const uint32_t mv_gpio_data_out_enable = 0x07071024;
static const uint32_t mv_gpio_port_active_sel = 0x07071058;

static const uint32_t mv_port_number_offset = 2;
static const uint32_t mv_port_number_max = 9;

static char *mv_port_mode[] = {
	"All SATAx LINK and ACT",	// 0x0
	"SATA0 LINK and ACT",		// 0x1
	"SATA1 LINK and ACT",		// 0x2
	"SATA2 LINK and ACT",		// 0x3
	"SATA3 LINK and ACT",		// 0x4
	"GPIO_DATA_OUT[n]",		// 0x5
	"Invalid (6)",			// 0x6
	"Invalid (7)"			// 0x7
};

/**
 * mv_mmap_bar5 - open() and mmap() a PCI BAR5 (resource5) file.
 *                the caller should unmap/close (see mv_munmap_bar5()).
 *
 * @path	path to the 'resource5' file
 * @fd		pointer to a file descriptor
 *
 * Return:
 * - success: pointer to the mmap()'ed PCI BAR5
 * - failure: NULL
 */
static void *
mv_mmap_bar5(const char* path, int *fd)
{
	void *bar5;

	/* open and mmap PCI BAR5 */
	*fd = open(path, O_RDWR);
	if (*fd < 0) {
		log_msg("Unable to open file '%s'", path);
		return NULL;
	}

	bar5 = mmap(NULL, mv_pci_bar5_length, PROT_READ | PROT_WRITE,
			 MAP_SHARED, *fd, 0);
	if (bar5 == MAP_FAILED) {
		log_msg("Unable to mmap file '%s'", path);
		close(*fd);
		return NULL;
	}

	return bar5;
}

/**
 * mv_munmap_bar5 - munmap() and close() a PCI BAR5 (resource5) file.
 *                  (release resources from mv_mmap_bar5()).
 *
 * @bar5	pointer to the mmap()'ed PCI BAR5
 * @fd		file descriptor of the open()'ed/mmap()'ed PCI BAR5
 *
 * Return:
 * - nothing
 */
static void
mv_munmap_bar5(void *bar5, int fd)
{
	/* munmap() and close PCI BAR5 */
	if (munmap(bar5, mv_pci_bar5_length))
		log_msg("Unable to munmap file");

	if (close(fd))
		log_msg("Unable to close file");
}

/**
 * mv_vsr_read - read a VSR
 *
 * @bar5	pointer to mmap()'ed PCI BAR5 (see mv_mmap_bar5())
 * @vsr_addr	VSR address to read from
 *
 * Return:
 * - VSR data read
 */
static uint32_t
mv_vsr_read(void* bar5, uint32_t vsr_addr)
{
	volatile uint32_t *addr = (uint32_t *)(bar5 + mv_pci_bar5_vsr_addr);
	volatile uint32_t *data = (uint32_t *)(bar5 + mv_pci_bar5_vsr_data);

	/* set address and read data */
	*addr = vsr_addr;
	return *data;
}

/**
 * mv_vsr_write - write a VSR
 *
 * @bar5	pointer to mmap()'ed PCI BAR5 (see mv_mmap_bar5())
 * @vsr_addr	VSR address to write to
 * @vsr_data	VSR data to write
 *
 * Return:
 * - nothing
 */
static void
mv_vsr_write(void* bar5, uint32_t vsr_addr, uint32_t vsr_data)
{
	volatile uint32_t *addr = (uint32_t *)(bar5 + mv_pci_bar5_vsr_addr);
	volatile uint32_t *data = (uint32_t *)(bar5 + mv_pci_bar5_vsr_data);

	/* set address and write data */
	*addr = vsr_addr;
	*data = vsr_data;
}

/**
 * mv_get_port_mode - get the LED mode of a SATA port (from register copy)
 *
 * @port_number		number of the SATA port (0-9)
 * @port_mode		pointer to the LED mode (result) (see mv_port_mode[])
 * @port_active_sel	copy of the PORT_ACTIVE_SEL register
 *
 * Return:
 * - -1 on failure
 * -  0 on success
 */
static int
mv_get_port_mode(uint32_t port_number, uint32_t *port_mode,
		 uint32_t port_active_sel)
{
	uint32_t shift, mask;

	if (port_number > mv_port_number_max) {
		log_msg("Invalid port number: '%u'", port_number);
		return -1;
	}

	/* get the 3-bit mode of SATA port N */
	shift = 3 * port_number;
	mask = 0x7 << shift;

	*port_mode = (port_active_sel & mask) >> shift;

	return 0;
}

/**
 * check_pci_id() - checks the value of a PCI ID file.
 *
 * Return:
 * -  0 on success (open/read file, and match value)
 * - -1 on failure
 * - +1 on different values
 */
static int
check_pci_id(char *path, char *pci_id)
{
	FILE *f;
	char *pci_id_rc, pci_id_buf[MV_PCI_ID_BUF_LEN];

	f = fopen(path, "r");
	if (!f) {
		log_msg("Unable to open file '%s'", path);
		return -1;
	}

	pci_id_rc = fgets(pci_id_buf, MV_PCI_ID_BUF_LEN, f);
	if (!pci_id_rc) {
		log_msg("Unable to read file '%s'", path);
		fclose(f);
		return -1;
	}

	fclose(f);
	return !!strncmp(pci_id, pci_id_buf, MV_PCI_ID_BUF_LEN);
}

/**
 * mv_indicator_list - Build Marvell HDD LED (indicator) list for the given disk vpd
 *
 * @list	loc_code structure
 * @vpd		dev_vpd structure
 *
 * Return:
 * -  0 on success or skip vpd (not a Marvell SATA disk)
 * - -1 on failure
 */
static int
mv_indicator_list(struct loc_code **list, struct dev_vpd *vpd)
{
	struct	loc_code *list_curr, *list_new;
	char	*block_path, path[PATH_MAX], symlink[PATH_MAX];
	char	*ata_device, *host;
	ssize_t	len;

	/* check for an 'sdX' device name */
	if (strncmp(vpd->dev, "sd", 2))
		return 0;

	/* read the '/sys/block/sdX' symlink to '../device/pci.../sdX' */
	if (asprintf(&block_path, "%s/%s", SYS_BLOCK_PATH, vpd->dev) < 0) {
		log_msg("Out of memory");
		return -1;
	}

	len = readlink(block_path, symlink, PATH_MAX);
	if (len < 0) {
		log_msg("Unable to read the contents of symbolic link '%s'", block_path);
		free(block_path);
		return -1;
	}
	free(block_path);
	symlink[len] = '\0';

	/*
	 * check for an 'ataX' subdir (w/ trailing 'X' digit); for example 'ata1' in
	 * '../devices/pci<...>/0001:08:00.0/ata1/host3/target3:0:0/3:0:0:0/block/sdj'
	 */

	ata_device = strstr(symlink, "/ata");
	if (!ata_device || !isdigit(ata_device[4]))
		return 0;

	host = strstr(symlink, "/host");
	if (!host || !isdigit(host[5]))
		return 0;

	/* split symlink into relative path for PCI device dir and ataX device */
	ata_device[0] = '\0';	/* end symlink on leading '/' of '/ataX/' */
	ata_device++;		/* skip the leading '/' of '/ataX/' */
	host[0] = '\0';		/* end ata_device on trailing '/' of '/ataX/',
				 * which is the leading '/' of '/hostY/' */

	/*
	 * get the absolute path for the PCI device dir from symlink,
	 * which is relative to /sys/block (e.g., '../devices/pci...'),
	 * so skip the leading '../' (3 chars)
	 */
	len = snprintf(path, PATH_MAX, "%s/%s", "/sys", &symlink[3]);
	if (len < 0) {
		log_msg("Unable to format absolute pathname of '%s'", symlink);
		return -1;
	}

	/*
	 * check for the PCI IDs of the Marvell 9235 SATA controller.
	 *
	 * concatenate the PCI ID files' basename after the dirname
	 * with strncpy() (string length + 1 for '\0').
	 */
	strncpy(&path[len], "/vendor", 8);
	if (check_pci_id(path, MV_PCI_VID))
		return 0;

	strncpy(&path[len], "/device", 8);
	if (check_pci_id(path, MV_PCI_DID))
		return 0;

	strncpy(&path[len], "/subsystem_vendor", 18);
	if (check_pci_id(path, MV_PCI_SVID))
		return 0;

	strncpy(&path[len], "/subsystem_device", 18);
	if (check_pci_id(path, MV_PCI_SSID))
		return 0;

	path[len] = '\0'; /* restore path as dirname of PCI device dir */

	/* Allocate one loc_code element, and insert/append it to the list */
	list_new = calloc(1, sizeof(struct loc_code));
	if (!list_new) {
		log_msg("Out of memory");
		return 1;
	}

	if (*list) {
		/* position list_curr in the last element of the list */
		list_curr = *list;
		while (list_curr->next)
			list_curr = list_curr->next;

		/* append the new element to the list */
		list_curr->next = list_new;
	} else {
		/* null list; insert the new element at the list head */
		*list = list_new;
	}

	/* set the new element's properties */
	list_new->type = TYPE_MARVELL;
	strncpy(list_new->code, vpd->location, LOCATION_LENGTH);  /* loc. code */
	strncpy(list_new->devname, vpd->dev, DEV_LENGTH);   /* sdX device name */
	snprintf(list_new->dev, DEV_LENGTH, "%s/resource5", path); /* PCI BAR5 */
	list_new->index = (uint32_t)atoi(&ata_device[3]); /* use for ATA index */

	dbg("Marvell HDD LED:");
	dbg("- location code: '%s'", list_new->code);
	dbg("- device name (disk): '%s'", list_new->devname);
	dbg("- device name (pci bar5): '%s'", list_new->dev);
	dbg("- ata index: '%u'", list_new->index);

	return 0;
}

/**
 * get_mv_indices - Get Marvell HDD LEDs (indicators) list
 *
 * @indicator	led type
 * @loc_list	loc_code structure
 *
 * Return:
 * - nothing
 */
void
get_mv_indices(int indicator, struct loc_code **loc_list)
{
	struct	dev_vpd *vpd_list, *vpd_curr;

	/* support for identification LEDs only */
	if (indicator != LED_TYPE_IDENT)
		return;

	/* get block devices' vpd information */
        vpd_list = read_device_vpd(SYS_BLOCK_PATH);
	if (!vpd_list)
		return;

	/* search for Marvell HDDs, and append to the list */
	for (vpd_curr = vpd_list; vpd_curr; vpd_curr = vpd_curr->next)
		if (vpd_curr->location[0] != '\0')
			if (mv_indicator_list(loc_list, vpd_curr))
				break;

	free_device_vpd(vpd_list);
}

/**
 * get_mv_indicator - get Marvell HDD LED/indicator state
 *
 * @loc		loc_code structure
 * @state	return indicator state of given loc
 *
 * Return:
 * -  0 on success
 * - -1 on failure
 */
int
get_mv_indicator(int indicator, struct loc_code *loc, int *state)
{
	int fd;
	void *bar5;
	uint32_t data_out, data_out_enable, active_sel;
	uint32_t port_number, port_mode;

	/* support for identification LEDs only */
	if (indicator != LED_TYPE_IDENT)
		return -1;

	/* read registers from PCI BAR5 */
	bar5 = mv_mmap_bar5(loc->dev, &fd);
	if (!bar5)
		return -1; /* log_msg() done */

	/* for debugging, read the 3 VSRs; only DATA_OUT is required */
	data_out = mv_vsr_read(bar5, mv_gpio_data_out);
	data_out_enable = mv_vsr_read(bar5, mv_gpio_data_out_enable);
	active_sel = mv_vsr_read(bar5, mv_gpio_port_active_sel);

	/* for debugging, get the LED mode of the SATA port 'N + offset' */
	port_number = loc->index + mv_port_number_offset;

	if (mv_get_port_mode(port_number, &port_mode, active_sel))
		return -1; /* log_msg() done */

	dbg("SATA disk '%s'", (loc->devname) ? loc->devname : "(null)");
	dbg("Data Out: '0x%08x'", data_out);
	dbg("Data Out Enable: '0x%08x'", data_out_enable);
	dbg("Port Active Select: '0x%08x'", active_sel);
	dbg("ATA Index: '%u', Port Number: '%u', Port Mode: '%u' ('%s')",
		loc->index, port_number, port_mode, mv_port_mode[port_mode]);

	/* LED state: ON on low (bit 0) and OFF on high (bit 1) */
	*state = (data_out & (1 << port_number)) ? LED_STATE_OFF : LED_STATE_ON;

	mv_munmap_bar5(bar5, fd);
	return 0;
}

/**
 * set_mv_indicator - set Marvell HDD LED/indicator state
 *
 * @loc		loc_code structure
 * @state	value to update indicator to
 *
 * Return:
 * -  0 on success
 * - -1 on failure
 */
int
set_mv_indicator(int indicator, struct loc_code *loc, int new_value)
{
	int fd;
	void *bar5;
	uint32_t data_out, port_number;

	/* support for identification LEDs only */
	if (indicator != LED_TYPE_IDENT)
		return -1;

	/* sanity check */
	if (new_value != LED_STATE_ON && new_value != LED_STATE_OFF)
		return -1;

	/* read/write registers from/to PCI BAR5 */
	bar5 = mv_mmap_bar5(loc->dev, &fd);
	if (!bar5)
		return -1; /* log_msg() done */

	/*
	 * only changes to the DATA_OUT register are required with the default
	 * values for the DATA_OUT_ENABLE and PORT_ACTIVE_SEL registers on the
	 * Garrison/Minsky system (see header comments).
	 */
	data_out = mv_vsr_read(bar5, mv_gpio_data_out);

	/* set the LED state of the SATA port 'N + offset' */
	port_number = loc->index + mv_port_number_offset;

	/* set/clear the bit in the DATA_OUT register;
	 * LED state: ON on low (bit 0) and OFF on high (bit 1) */
	if (new_value == LED_STATE_ON)
		data_out &= ~(1 << port_number);
	else
		data_out |= 1 << port_number;

	/* update the register */
	mv_vsr_write(bar5, mv_gpio_data_out, data_out);

	/* for debugging, read the register again after writing */
	dbg("Data Out (written to): '0x%08x'", data_out);
	data_out = mv_vsr_read(bar5, mv_gpio_data_out);
	dbg("Data Out (read again): '0x%08x'", data_out);

	mv_munmap_bar5(bar5, fd);
	return 0;
}
