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
#include "bluehawk.h"
#include "homerun.h"
#include "slider.h"

static void get_power_supply_vpd(int fd, void *edp, int size)
{
	int result;

	memset(edp, 0, size);

	result = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 7, edp, size);
	if (result != 0) {
		perror("get_power_supply_vpd");
		fprintf(stderr, "result = %d\n", result);
		exit(5);
	}
}

static void display_encl_element_vpd_data(struct vpd_page *vp)
{
	char temp[20];

	printf("FN=%s\n", strzcpy(temp, vp->fru_number, 8));
	printf("SN=%s\n", strzcpy(temp, vp->serial_number, 12));
	printf("CCIN=%s\n", strzcpy(temp, vp->model_number, 4));
	printf("FL=%s\n", strzcpy(temp, vp->fru_label, 5));
}

#define DISPLAY_ENCL_PS_VPD(element_desc_page, element) \
	do { \
		printf("FN=%s\n", strzcpy(temp, \
				element_desc_page.element.fru_number, 8)); \
		printf("SN=%s\n", strzcpy(temp, \
				element_desc_page.element.serial_number, 12)); \
		printf("FL=%s\n", strzcpy(temp, \
				element_desc_page.element.fru_label, 5)); \
		\
	} while (0)

int main(int argc, char **argv)
{
	int fd;
	int result;
	char *sg;
	char dev_sg[20];
	char temp[20];
	struct vpd_page vp;
	struct bh_element_descriptor_page bh_edp;
	struct hr_element_descriptor_page hr_edp;
	struct slider_lff_element_descriptor_page slider_l_edp;
	struct slider_sff_element_descriptor_page slider_s_edp;
	struct dev_vpd vpd;

	if (argc != 2) {
		fprintf(stderr, "usage: %s sgN\n", argv[0]);
		exit(1);
	}

	sg = argv[1];
	if (strncmp(sg, "sg", 2) != 0 || strlen(sg) > 6) {
		fprintf(stderr, "bad format of sg argument\n");
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
	display_encl_element_vpd_data(&vp);

	printf("\nMidplane:\n");
	result = get_diagnostic_page(fd, INQUIRY, 5, &vp, sizeof(vp));
	if (result != 0) {
		perror("get_vpd_page");
		fprintf(stderr, "result = %d\n", result);
		exit(4);
	}
	display_encl_element_vpd_data(&vp);

	/* Read enclosure vpd */
	memset(&vpd, 0, sizeof(vpd));
	read_vpd_from_lscfg(&vpd, sg);
	if (vpd.mtm[0] == '\0') {
		fprintf(stderr,
			"Unable to find machine type/model for %s\n", sg);
		exit(1);
	}

	if (strcmp(vpd.mtm, "5888") && strcmp(vpd.mtm, "EDR1") &&
		strcmp(vpd.mtm, "5887") && strcmp(vpd.mtm, "ESLL") &&
		strcmp(vpd.mtm, "ESLS")) {
		fprintf(stderr,
				"Not a valid enclosure : %s", sg);
		exit (1);
	}

	if ((!strcmp(vpd.mtm, "5888")) || (!strcmp(vpd.mtm, "EDR1"))) {
		get_power_supply_vpd(fd, &bh_edp, sizeof(bh_edp));
		printf("\nPower supply 0:\n");
		DISPLAY_ENCL_PS_VPD(bh_edp, ps0_vpd);

		printf("\nPower supply 1:\n");
		DISPLAY_ENCL_PS_VPD(bh_edp, ps1_vpd);
	} else if (!strcmp(vpd.mtm, "5887")) {
		get_power_supply_vpd(fd, &hr_edp, sizeof(hr_edp));
		printf("\nPower supply 0:\n");
		DISPLAY_ENCL_PS_VPD(hr_edp, ps0_vpd);

		printf("\nPower supply 1:\n");
		DISPLAY_ENCL_PS_VPD(hr_edp, ps1_vpd);
	} else if (!strcmp(vpd.mtm, "ESLL")) {
		get_power_supply_vpd(fd, &slider_l_edp, sizeof(slider_l_edp));
		printf("\nPower supply 0:\n");
		DISPLAY_ENCL_PS_VPD(slider_l_edp, ps0_vpd);

		printf("\nPower supply 1:\n");
		DISPLAY_ENCL_PS_VPD(slider_l_edp, ps1_vpd);
	} else if (!strcmp(vpd.mtm, "ESLS")) {
		get_power_supply_vpd(fd, &slider_s_edp, sizeof(slider_s_edp));
		printf("\nPower supply 0:\n");
		DISPLAY_ENCL_PS_VPD(slider_s_edp, ps0_vpd);

		printf("\nPower supply 1:\n");
		DISPLAY_ENCL_PS_VPD(slider_s_edp, ps1_vpd);
	}

	exit(0);
}
