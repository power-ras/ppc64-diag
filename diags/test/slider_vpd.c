#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>

#include "encl_common.h"
#include "encl_util.h"
#include "slider.h"

static void get_power_supply_vpd(int fd, void *edp, int size)
{
	int result;

	memset(edp, 0, size);

	result = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 7, edp, size);
	if (result != 0) {
		perror("get_vpd_page");
		fprintf(stderr, "result = %d\n", result);
		exit(5);
	}
}

int main(int argc, char **argv)
{
	int fd;
	int result;
	char *sg;
	char dev_sg[20];
	char temp[20];
	struct vpd_page vp;
	struct slider_lff_element_descriptor_page edpl;
	struct slider_sff_element_descriptor_page edps;
	struct dev_vpd vpd;

	if (argc != 2) {
		fprintf(stderr, "usage: %s sgN\n", argv[0]);
		exit(1);
	}

	sg = argv[1];
	if (strncmp(sg, "sg", 2) != 0 || strlen(sg) > 6) {
		fprintf(stderr, "bad format of sg arg\n");
		exit(2);
	}
	snprintf(dev_sg, 20, "/dev/%s", sg);
	fd = open(dev_sg, O_RDWR);
	if (fd < 0) {
		perror(dev_sg);
		exit(3);
	}

	printf("ESM/ERM:\n");
	result = get_diagnostic_page(fd, INQUIRY, 1, &vp, sizeof(vp));
	if (result != 0) {
		perror("get_vpd_page");
		fprintf(stderr, "result = %d\n", result);
		exit(4);
	}
	printf("FN=%s\n", strzcpy(temp, vp.fru_number, 8));
	printf("SN=%s\n", strzcpy(temp, vp.serial_number, 12));
	printf("CC=%s\n", strzcpy(temp, vp.model_number, 4));
	printf("FL=%s\n", strzcpy(temp, vp.fru_label, 5));

	printf("\nmidplane:\n");
	result = get_diagnostic_page(fd, INQUIRY, 5, &vp, sizeof(vp));
	if (result != 0) {
		perror("get_vpd_page");
		fprintf(stderr, "result = %d\n", result);
		exit(4);
	}
	printf("FN=%s\n", strzcpy(temp, vp.fru_number, 8));
	printf("SN=%s\n", strzcpy(temp, vp.serial_number, 12));
	printf("CC=%s\n", strzcpy(temp, vp.model_number, 4));
	printf("FL=%s\n", strzcpy(temp, vp.fru_label, 5));

	/* Read enclosure vpd */
	memset(&vpd, 0, sizeof(vpd));
	read_vpd_from_lscfg(&vpd, sg);
	if (vpd.mtm[0] == '\0') {
		fprintf(stderr,
			"Unable to find machine type/model for %s\n", sg);
		exit(1);
	}

	if (strcmp(vpd.mtm, "ESLL") && strcmp(vpd.mtm, "ESLS")) {
		fprintf(stderr,
				"Not a valid slider enclosure : %s", sg);
		exit(1);
	}
	if (!strcmp(vpd.mtm, "ESLL")) {
		get_power_supply_vpd(fd, &edpl, sizeof(edpl));
		printf("\npower supply 0:\n");
		printf("FN=%s\n", strzcpy(temp, edpl.ps0_vpd.fru_number, 8));
		printf("SN=%s\n", strzcpy(temp, edpl.ps0_vpd.serial_number,
								12));
		printf("FL=%s\n", strzcpy(temp, edpl.ps0_vpd.fru_label, 5));

		printf("\npower supply 1:\n");
		printf("FN=%s\n", strzcpy(temp, edpl.ps1_vpd.fru_number, 8));
		printf("SN=%s\n", strzcpy(temp, edpl.ps1_vpd.serial_number,
								12));
		printf("FL=%s\n", strzcpy(temp, edpl.ps1_vpd.fru_label, 5));
	} else {
		get_power_supply_vpd(fd, &edps, sizeof(edps));
		printf("\npower supply 0:\n");
		printf("FN=%s\n", strzcpy(temp, edps.ps0_vpd.fru_number, 8));
		printf("SN=%s\n", strzcpy(temp, edps.ps0_vpd.serial_number,
								12));
		printf("FL=%s\n", strzcpy(temp, edps.ps0_vpd.fru_label, 5));

		printf("\npower supply 1:\n");
		printf("FN=%s\n", strzcpy(temp, edps.ps1_vpd.fru_number, 8));
		printf("SN=%s\n", strzcpy(temp, edps.ps1_vpd.serial_number,
								12));
		printf("FL=%s\n", strzcpy(temp, edps.ps1_vpd.fru_label, 5));
	}

	exit(0);
}
