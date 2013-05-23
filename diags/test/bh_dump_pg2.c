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

/* from diag_encl.c */
/**
 * get_diagnostic_page
 * @brief Make the necessary sg ioctl() to retrieve a diagnostic page
 *
 * @param fd a file descriptor to the appropriate /dev/sg<x> file
 * @param diag_page the page to be retrieved
 * @param buf a buffer to contain the contents of the diagnostic page
 * @param buf_len the length of the previous parameter
 * @return 0 on success, -EIO on invalid I/O status, or CHECK_CONDITION
 */
int
get_diagnostic_page(int fd, int diag_page, void *buf, int buf_len)
{
	unsigned char scsi_cmd_buf[16] =
		{	RECEIVE_DIAGNOSTIC,
			0x01,			/* set PCV bit to 1 */
			diag_page,		/* page to be retrieved */
			(buf_len >> 8) & 0xff,	/* most significant byte */
			buf_len & 0xff,		/* least significant byte */
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct sense_data_t sense_data;
	sg_io_hdr_t hdr;
	int i, rc;

	for (i = 0; i < 3; i++) {
		memset(&hdr, 0, sizeof(hdr));
		memset(&sense_data, 0, sizeof(struct sense_data_t));

		hdr.interface_id = 'S';
		hdr.dxfer_direction = SG_DXFER_FROM_DEV;
		hdr.cmd_len = 16;
		hdr.mx_sb_len = sizeof(sense_data);
		hdr.iovec_count = 0;	/* scatter gather not necessary */
		hdr.dxfer_len = buf_len;
		hdr.dxferp = buf;
		hdr.cmdp = scsi_cmd_buf;
		hdr.sbp = (unsigned char *)&sense_data;
		hdr.timeout = 120 * 1000;	/* set timeout to 2 minutes */
		hdr.flags = 0;
		hdr.pack_id = 0;
		hdr.usr_ptr = 0;

		rc = ioctl(fd, SG_IO, &hdr);

		if ((rc == 0) && (hdr.masked_status == CHECK_CONDITION))
			rc = CHECK_CONDITION;
		else if ((rc == 0) && (hdr.host_status || hdr.driver_status))
			rc = -EIO;

		if (rc == 0 || hdr.host_status == 1)
			break;
	}

	return rc;
}

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
	sprintf(dev_sg, "/dev/%s", sg);
	fd = open(dev_sg, O_RDWR);
	if (fd < 0) {
		perror(dev_sg);
		exit(3);
	}
	result = get_diagnostic_page(fd, 2, &dp, sizeof(dp));
	if (result != 0) {
		perror("get_diagnostic_page");
		exit(4);
	}
	if (write_page2_to_file(&dp, path) != 0)
		exit(5);
	exit(0);
}
