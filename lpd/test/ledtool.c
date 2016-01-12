/**
 * @file	ledtool.c
 * @brief	Light Path Diagnostics testing tool
 *
 * Copyright (C) 2012 IBM Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#include "indicator.h"

/**
 * enable_fault_indicator - Enable fault indicator for the given loc code
 */
int enable_fault_indicator(char *loccode, int truncate)
{
	int	rc = 0;
	int	truncated = 0;
	struct	loc_code *list = NULL;
	struct	loc_code *loc_led = NULL;

	rc = get_indicator_list(LED_TYPE_FAULT, &list);
	if (!list)
		return -1;

	if (loccode == NULL) {	/* attn indicator */
		loc_led = &list[0];
	} else {
retry:
		loc_led = get_indicator_for_loc_code(list, loccode);
		if (!loc_led) {
			if (truncate && truncate_loc_code(loccode)) {
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

	rc = set_indicator_state(LED_TYPE_FAULT, loc_led, 1);
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
		"\t-t : Truncate location code if necessary\n"
		"\t-h : Print this message and exit\n"
		"\n\tloc_code : Location code\n",
		progname);
}

/* ledtool command line arguments */
#define LED_TOOL_ARGS    "fht"

/* main */
int main(int argc, char **argv)
{
	char	*loccode = NULL;
	int	rc = 0;
	int	c;
	int	fault = 0;
	int	truncate = 0;

	opterr = 0;
	while ((c = getopt(argc, argv, LED_TOOL_ARGS)) != -1) {
		switch (c) {
		case 'f':
			fault = 1;
			break;
		case 'h':
			print_usage(argv[0]);
			exit(0);
		case 't':
			truncate = 1;
			break;
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
		exit(1);
	}

	if (probe_indicator() != 0) {
		close_files();
		exit(1);
	}

	/* Light Path operating mode */
	if (get_indicator_mode() != 0) {
		close_files();
		exit(1);
	}

	/* enable fault indicator */
	rc = enable_fault_indicator(loccode, truncate);

	close_files();

	exit(rc);
}
