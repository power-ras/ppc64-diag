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

/* from diag_encl.h */
struct sense_data_t {
        uint8_t error_code;
        uint8_t segment_numb;
        uint8_t sense_key;
        uint8_t info[4];
        uint8_t add_sense_len;
        uint8_t cmd_spec_info[4];
        uint8_t add_sense_code;
        uint8_t add_sense_code_qual;
        uint8_t field_rep_unit_code;
        uint8_t sense_key_spec[3];
        uint8_t add_sense_bytes[0];
};

/* Dump bluehawk enclosure pg2 */
int
main(int argc, char **argv)
{
	char *sg;
	char dev_sg[20];
	char *path;
	int fd;
	struct bluehawk_diag_page2 dp;
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
	fd = open(dev_sg, O_RDWR);
	if (fd < 0) {
		perror(dev_sg);
		exit(3);
	}
	result = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 2,
		 &dp, sizeof(dp));
	if (result != 0) {
		perror("get_diagnostic_page");
		exit(4);
	}
	if (write_page2_to_file(path, &dp, sizeof(dp)) != 0)
		exit(5);
	exit(0);
}
