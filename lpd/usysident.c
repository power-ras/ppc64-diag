/**
 * @file	usysident.c
 * @brief	usysident/usysattn/usysfault
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 *
 * @Note:
 *	This file is the re-implementation of original usysident.c
 *	file in powerpc-utils package.
 *	@author Michael Strosaker <strosake@us.ibm.com>
 *
 **/

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lp_diag.h"

#define CMD_LINE_OPTIONS	"d:l:s:-:thV"
#define CMD_IDENTIFY		"usysident"
#define CMD_FAULT		"usysfault"
#define CMD_ATTN		"usysattn"

/* print indicator state */
#define print_indicator_state(loc) \
	fprintf(stdout, "%s\t [%s]\n", loc->code, loc->state ?  "on" : "off")

/* Log indicator status */
#define indicator_log_state(indicator, loc, state) \
	if (indicator == ATTN_INDICATOR && !strchr(loc, '-')) \
		indicator_log_write("System Attention Indicator : %s", \
				    state == INDICATOR_ON ? "ON" : "OFF"); \
	else \
		indicator_log_write("%s : %s : %s", loc, \
				    indicator == IDENT_INDICATOR ? \
				    "Identify" : "Fault", \
				    state == INDICATOR_ON ? "ON" : "OFF");


/**
 * print_usage - Print the usage statement
 */
static void
print_usage(const char *cmd)
{
	if (strstr(cmd, CMD_IDENTIFY)) {
		fprintf(stdout, "Usage:\n"
		       "  %s [-l <loc_code> [-s {identify|normal}][-t]]\n"
		       "  %s [-d <dev_name> [-s {identify|normal}][-t]]\n"
		       "  %s [-V]\n"
		       "  %s [-h]\n\n", cmd, cmd, cmd, cmd);
		fprintf(stdout, "Options:\n"
		       "  -l <loc_code>   Indicator location code\n"
		       "  -d <dev_name>   Device name\n"
		       "  -s identify     Turn on device/location identify indicator\n"
		       "  -s normal       Turn off device/location identify indicator\n"
		       "  -t              Truncate loc code if necessary\n"
		       "  -V		  Print the version of the command\n"
		       "  -h              Print this message and exit\n");
	} else {
		fprintf(stdout, "Usage:\n"
		       "  %s [-l <loc_code> [-s normal][-t]]\n"
		       "  %s [-V]\n"
		       "  %s [-h]\n\n", cmd, cmd, cmd);
		fprintf(stdout, "Options:\n"
		       "  -l <loc_code>   Indicator location code\n"
		       "  -s normal	  Turn off location fault indicator\n"
		       "  -t              Truncate loc code if necessary\n"
		       "  -V		  Print the version of the command\n"
		       "  -h		  Print this message\n");
	}
}

/**
 * main  -
 */
int
main(int argc, char **argv)
{
	int	c;
	int	state;
	int	index;
	int	indicator;
	int	rc = 0;
	int	trunc = 0;
	int	truncated = 0;
	char	temp[LOCATION_LENGTH];
	char	dloc[LOCATION_LENGTH];
	char	*dvalue = NULL;
	char	*lvalue = NULL;
	char	*svalue = NULL;
	char	*othervalue = NULL;
	struct	loc_code *current;
	struct	loc_code *list = NULL;

	opterr = 0;
	while ((c = getopt(argc, argv, CMD_LINE_OPTIONS)) != -1) {
		switch (c) {
		case 'd':
			/* Device name */
			dvalue = optarg;
			break;
		case 'l':
			/* Location code */
			lvalue = optarg;
			break;
		case 's':
			/* Enable/disable */
			svalue = optarg;
			break;
		case '-':
			/* All location code */
			othervalue = optarg;
			break;
		case 't':
			/* truncate location code */
			trunc = 1;
			break;
		case 'V':
			fprintf(stdout, "%s %s\n", argv[0], VERSION);
			fflush(stdout);
			return 0;
		case 'h':
			print_usage(argv[0]);
			return 0;
		case '?':
			if (isprint(optopt))
				fprintf(stderr,
					"Unrecognized option: -%c\n\n",
					optopt);
			else
				fprintf(stderr,
					"Unrecognized option character %x\n\n",
					optopt);
			print_usage(argv[0]);
			return 1;
		default:
			print_usage(argv[0]);
			return 1;
		}
	}

	/* Option checking */
	for (index = optind; index < argc; index++) {
		fprintf(stderr,
			"Unrecognized argument : %s\n\n", argv[index]);
		print_usage(argv[0]);
		return 1;
	}

	if (dvalue && !strstr(argv[0], CMD_IDENTIFY)) {
		fprintf(stderr, "Unrecognized option: -d\n\n");
		print_usage(argv[0]);
		return 1;
	}

	if (dvalue && lvalue) {
		fprintf(stderr,
			"The -d and -l options cannot be used together.\n\n");
		print_usage(argv[0]);
		return 1;
	}

	if (svalue && strstr(argv[0], CMD_IDENTIFY)) {
		if (!strcmp(svalue, "identify"))
			c = INDICATOR_ON;
		else if (!strcmp(svalue, "normal"))
			c = INDICATOR_OFF;
		else {
			fprintf(stderr,
				"The -s option must be either "
				"\"identify\" or \"normal\".\n\n");
			print_usage(argv[0]);
			return 1;
		}
	}

	if (svalue && (strstr(argv[0], CMD_FAULT) ||
		       strstr(argv[0], CMD_ATTN))) {
		if (!strcmp(svalue, "normal"))
			c = INDICATOR_OFF;
		else {
			fprintf(stderr,
				"The -s option must be \"normal\".\n\n");
			print_usage(argv[0]);
			return 1;
		}
	}

	if (svalue && !(dvalue || lvalue)) {
		if (strstr(argv[0], CMD_IDENTIFY))
			fprintf(stderr,
				"The -s option requires the -d or -l "
				"option to also be used.\n\n");
		else
			fprintf(stderr,
				"The -s option requires the -l "
				"option to also be used.\n\n");
		print_usage(argv[0]);
		return 1;
	}

	if (svalue && geteuid() != 0) {
		fprintf(stderr,
			"%s: Turning indicator on/off requires "
			"superuser privileges.\n\n", argv[0]);
		return 1;
	}

	if (trunc && !(dvalue || lvalue)) {
		if (strstr(argv[0], CMD_IDENTIFY))
			fprintf(stderr,
				"The -t option requires the -d or -l "
				"option to also be used.\n\n");
		else
			fprintf(stderr,
				"The -t option requires the -l "
				"option to also be used.\n\n");
		print_usage(argv[0]);
		return 1;
	}

	if (othervalue && strstr(argv[0], CMD_IDENTIFY)) {
		if (!strcmp(othervalue, "all-on"))
			c = INDICATOR_ON;
		else if (!strcmp(othervalue, "all-off"))
			c = INDICATOR_OFF;
		else {
			fprintf(stderr,
				"Unrecognized option: --%s\n\n", othervalue);
			print_usage(argv[0]);
			return 1;
		}
	}

	if (othervalue && (strstr(argv[0], CMD_ATTN) ||
			   strstr(argv[0], CMD_FAULT))) {
		if (!strcmp(othervalue, "all-off"))
			c = INDICATOR_OFF;
		else {
			fprintf(stderr,
				"Unrecognized option: --%s\n\n", othervalue);
			print_usage(argv[0]);
			return 1;
		}
	}

	if (othervalue && argc > 2) {
		fprintf(stderr,
			"\"--%s\" cannot be used with any other options.\n\n",
			othervalue);
		print_usage(argv[0]);
		return 1;
	}

	if (strstr(argv[0], CMD_IDENTIFY))
		indicator = IDENT_INDICATOR;
	else if (strstr(argv[0], CMD_FAULT) || strstr(argv[0], CMD_ATTN))
		indicator = ATTN_INDICATOR;
	else
		return 1;

	/* initialize */
	program_name = argv[0];
	lp_error_log_fd = 1; /* log message to stdout */
	rc = init_files();
	if (rc) {
		fprintf(stderr, "Unable to open log file.\n");
		return 1;
	}

	/* Light Path operating mode */
	if (indicator == ATTN_INDICATOR) {
		rc = check_operating_mode();
		if (rc)
			return rc;
	}

	/* get indicator list */
	rc  = get_indicator_list(indicator, &list);
	if (rc)
		goto file_cleanup;

	if (argc == 1) {
		current = list;
		while (current) {
			/* get and print all indicators current state */
			rc = get_indicator_state(indicator, current, &state);
			if (rc) /* failed to get indicator state */
				current->state = -1;
			else
				current->state = state;

			print_indicator_state(current);
			current = current->next;
		}
	}

	/* Turn on/off indicator based on device name */
	if (dvalue) {
		if (get_loc_code_for_dev(dvalue, dloc, LOCATION_LENGTH) != 0) {
			fprintf(stderr,
				"\"%s\" is not a valid device or "
				"it does not have location code.\n", dvalue);
			rc = 2;
		} else {
			lvalue = dloc;
			fprintf(stdout, "%s is at location code %s.\n",
				dvalue, lvalue);
		}
	}

	/* Turn on/off indicator based on location code */
	if (lvalue) {
		strncpy(temp, lvalue, LOCATION_LENGTH);

retry:
		current = get_indicator_for_loc_code(list, lvalue);
		if (!current) {
			if (trunc) {
				if (truncate_loc_code(lvalue)) {
					truncated = 1;
					goto retry;
				}
			}
			fprintf(stdout, "There is no %s indicator at location "
				"code %s.\n", INDICATOR_TYPE(indicator), temp);
			rc = 1;
		} else { /* Found location code */
			if (truncated)
				fprintf(stdout, "Truncated location code : "
					"%s\n", lvalue);

			if (svalue) {
				rc = get_indicator_state(indicator, current,
							 &state);

				if (rc || state != c) {
					rc = set_indicator_state(indicator,
								 current, c);
					if (rc)
						goto indicator_cleanup;
					indicator_log_state(indicator,
							    current->code, c);
				}
			}

			rc = get_indicator_state(indicator, current, &state);
			if (!rc) {
				if (dvalue)
					fprintf(stdout, "%s\t[%s]\n", lvalue,
					       state ? "on" : "off");
				else
					fprintf(stdout, "%s\n",
					       state ? "on" : "off");
			}
		} /* if-else end */
	} /* lvalue end */

	/* Turn on/off all indicators */
	if (othervalue) {
		current = list;
		while (current) {
			rc = get_indicator_state(indicator, current, &state);
			if (rc) /* failed to get indicator state */
				current->state = -1;
			else
				current->state = state;

			if (state != c) {
				set_indicator_state(indicator, current, c);
				rc = get_indicator_state(indicator, current,
							 &state);
				if (rc) /* failed to get indicator state */
					current->state = -1;
				else
					current->state = state;
			}

			print_indicator_state(current);
			current = current->next;
		}

		/* If enclosure ident indicator is turned ON explicitly,
		 * then turning OFF all components ident indicator inside
		 * enclosure does not turn OFF enclosure ident indicator.
		 */
		if (indicator == IDENT_INDICATOR && c == INDICATOR_OFF)
			set_indicator_state(indicator, &list[0], c);

		indicator_log_write("All %s Indicators : %s",
				    indicator == IDENT_INDICATOR ? "Identify" : "Fault",
				    c == INDICATOR_ON ? "ON" : "OFF");
	}

indicator_cleanup:
	free_indicator_list(list);
file_cleanup:
	close_files();

	return rc;
}
