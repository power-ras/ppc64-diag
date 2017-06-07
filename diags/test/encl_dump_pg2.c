#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <scsi/scsi.h>

#include "bluehawk.h"
#include "homerun.h"
#include "slider.h"

/* Dump enclosure pg2 */
int main(int argc, char **argv)
{
	char *sg;
	char dev_sg[20];
	char *path;
	int fd;
	void *dp;
	struct dev_vpd vpd;
	int result;
	int size = 0;

	if (argc != 3) {
		fprintf(stderr, "usage: %s sgN output_file\n", argv[0]);
		exit(1);
	}

	sg = argv[1];
	path = argv[2];
	if (strncmp(sg, "sg", 2) != 0 || strlen(sg) > 6) {
		fprintf(stderr, "bad format of sg argument\n");
		exit(2);
	}
	snprintf(dev_sg, 20, "/dev/%s", sg);

	/* Validate enclosure device */
	if (valid_enclosure_device(sg))
		exit(0);

	/* Read enclosure vpd */
	memset(&vpd, 0, sizeof(vpd));
	result = read_vpd_from_lscfg(&vpd, sg);
	if (vpd.mtm[0] == '\0') {
		fprintf(stderr,
			"Unable to find machine type/model for %s\n", sg);
		exit(1);
	}

	if ((!strcmp(vpd.mtm, "5888")) || (!strcmp(vpd.mtm, "EDR1"))) {
		size = sizeof(struct bluehawk_diag_page2);
	} else if (!strcmp(vpd.mtm, "5887")) {
		size = sizeof(struct hr_diag_page2);
	} else if (!strcmp(vpd.mtm, "ESLL")) {
		size = sizeof(struct slider_lff_diag_page2);
	} else if (!strcmp(vpd.mtm, "ESLS")) {
		size = sizeof(struct slider_sff_diag_page2);
	} else {
		fprintf(stderr,"Not a valid enclosure : %s\n", sg);
		exit (1);
	}

	dp = (char *)malloc(size);
	if (!dp) {
		perror(dp);
		exit(2);
	}

	fd = open(dev_sg, O_RDWR);
	if (fd < 0) {
		free(dp);
		perror(dev_sg);
		exit(3);
	}
	result = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 2, dp, size);
	if (result != 0) {
		free(dp);
		perror("get_diagnostic_page");
		exit(4);
	}

	if (write_page2_to_file(path, dp, size) != 0) {
		free(dp);
		exit(5);
	}

	free(dp);
	exit(0);
}
