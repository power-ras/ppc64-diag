#include <stdlib.h>
#include <stdio.h>
#include "bluehawk.h"
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
	if (write_page2_to_file(&healthy_page, argv[1]) != 0)
		exit(2);
	exit(0);
}
