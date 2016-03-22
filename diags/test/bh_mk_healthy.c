#include <stdlib.h>
#include <stdio.h>
#include "encl_common.h"
#include "bluehawk.h"
#include "encl_util.h"
#include "test_utils.h"

extern struct bluehawk_diag_page2 healthy_page;

/* healthy pg2 for bluehawk */
int
main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s pathname\n", argv[0]);
		exit(1);
	}
	if (write_page2_to_file(argv[1],
		 &healthy_page, sizeof(healthy_page)) != 0)
		exit(2);
	exit(0);
}
