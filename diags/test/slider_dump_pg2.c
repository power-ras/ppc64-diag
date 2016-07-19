#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <sys/ioctl.h>

#include "encl_common.h"
#include "slider.h"
#include "test_utils.h"

char *path;
char dev_sg[20];

void read_write_diag_page(void *dp, int size)
{
	int result, fd;

	fd = open(dev_sg, O_RDWR);
	if (fd < 0) {
		perror(dev_sg);
		exit(3);
	}

	result = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC,
				     2, dp, size);

	if (result != 0) {
		perror("get_diagnostic_page");
		exit(4);
	}

	if (write_page2_to_file(path, dp, size) != 0)
		exit(5);
}


/* Dump slider enclosure pg2 */
int
main(int argc, char **argv)
{
	char *sg;
	struct slider_lff_diag_page2 dpl;
	struct slider_sff_diag_page2 dps;
	struct dev_vpd vpd;

	if (argc != 3) {
		fprintf(stderr, "usage: %s sgN output_file\n", argv[0]);
		exit(1);
	}
	sg = argv[1];
	path = argv[2];
	if (strncmp(sg, "sg", 2) != 0 || strlen(sg) > 6) {
		fprintf(stderr, "bad format of sg arg\n");
		exit(2);
	}
	snprintf(dev_sg, 20, "/dev/%s", sg);

	/* Validate enclosure device */
	if (valid_enclosure_device(dev_sg))
		exit(0);

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
		exit (1);
	}

	if (!strcmp(vpd.mtm, "ESLL"))
		read_write_diag_page(&dpl, sizeof(dpl));
	else
		read_write_diag_page(&dps, sizeof(dps));

	exit(0);
}
