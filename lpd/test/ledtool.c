/**
 * @file	ledtool.c
 * @brief	Light Path Diagnostics testing tool
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#include "lp_diag.h"

/**
 * enable_fault_indicator - Enable fault indicator for the given loc code
 */
int enable_fault_indicator(char *loccode)
{
	int	rc = 0;
	int	truncated = 0;
	struct	loc_code *list = NULL;
	struct	loc_code *loc_led = NULL;

	rc = get_indicator_list(ATTN_INDICATOR, &list);
	if (!list)
		return -1;

	if (loccode == NULL) {	/* attn indicator */
		loc_led = &list[0];
	} else {
retry:
		loc_led = get_indicator_for_loc_code(list, loccode);
		if (!loc_led) {
			if (truncate_loc_code(loccode)) {
				truncated = 1;
				goto retry;
			}
			fprintf(stderr, "There is no fault indicator at "
				"location code %s\n", loccode);
			free_indicator_list(list);
			return 0;
		}
	}

	if (truncated)
		fprintf(stderr, "Truncated location code = %s\n", loccode);

	rc = set_indicator_state(ATTN_INDICATOR, loc_led, 1);
	if (rc == 0) {
		if (loccode == NULL)
			indicator_log_write("System Attention Indicator : ON");
		else
			indicator_log_write("%s : Fault : ON", loc_led->code);
		fprintf(stderr, "%s : \[on]\n", loc_led->code);
	} else
		fprintf(stderr, "Unable to enable fault indicator\n");


	free_indicator_list(list);
	return rc;
}

void print_usage(char *progname)
{
	fprintf(stdout,
		"Enable system attention/fault indicators.\n"
		"\nUsage : %s -f [<loc_code>]\n"
		"\t-f : Fault indicator\n"
		"\t-h : Print this usage message and exit\n"
		"\n\tloc_code : Indicator location code\n",
		progname);
}

/* ledtool command line arguments */
#define LED_TOOL_ARGS    "fh"

/* main */
int main(int argc, char **argv)
{
	int	rc = 0;
	int	c;
	int	fault = 0;
	char	*loccode = NULL;

	opterr = 0;
	while ((c = getopt(argc, argv, LED_TOOL_ARGS)) != -1 ) {
		switch (c) {
		case 'f':
			fault = 1;
			break;
		case 'h':
			print_usage(argv[0]);
			exit(0);
		default:
			print_usage(argv[0]);
			exit(1);
		}
	}

	if (optind < argc)
		loccode = argv[optind++];

	if (optind < argc) {
		print_usage(argv[0]);
		exit(1);
	}

	if (!fault) {
		print_usage(argv[0]);
		exit(1);
	}

	program_name = argv[0];
	lp_error_log_fd = 1; /* log message to stdout */
	rc = init_files();
	if (rc) {
		fprintf(stderr, "Unable to open log file.\n");
		exit (1);
	}

	/* Light Path operating mode */
	if (check_operating_mode() != 0) {
		close_files();
		exit(1);
	}

	/* enable fault indicator */
	rc = enable_fault_indicator(loccode);

	close_files();

	exit(rc);
}
