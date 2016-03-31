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
#include "bluehawk.h"
#include "test_utils.h"

/* Dump bluehawk enclosure pg2 */
int
main(int argc, char **argv)
{
	char *sg;
	char dev_sg[20];
	char *path;
	int fd;
	struct bluehawk_diag_page2 dp;
	struct dev_vpd vpd;
	int result;

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
	result = read_vpd_from_lscfg(&vpd, sg);
	if (vpd.mtm[0] == '\0') {
		fprintf(stderr,
			"Unable to find machine type/model for %s\n", sg);
		exit(1);
	}

	if (strcmp(vpd.mtm, "5888") && strcmp(vpd.mtm, "EDR1")) {
		fprintf(stderr,
			"Not a valid bluehawk enclosure : %s", sg);
		exit (1);
	}

	fd = open(dev_sg, O_RDWR);
	if (fd < 0) {
		perror(dev_sg);
		exit(3);
	}
	result = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC,
				     2, &dp, sizeof(dp));
	if (result != 0) {
		perror("get_diagnostic_page");
		exit(4);
	}

	if (write_page2_to_file(path, &dp, sizeof(dp)) != 0)
		exit(5);

	exit(0);
}
