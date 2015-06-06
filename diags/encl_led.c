/*
 * Copyright (C) 2012, 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "encl_util.h"
#include "encl_led.h"
#include "utils.h"
#include "platform.h"

static struct {
	char	*mtm;
	int	(*list_leds)(const char *, const char *, int);
	int	(*set_led)(const char *, const char *, int, int, int);
} encl_list[] = {
	{"5888", bluehawk_list_leds, bluehawk_set_led}, /* Bluehawk enclosure */
	{"EDR1", bluehawk_list_leds, bluehawk_set_led}, /* Bluehawk enclosure */
	{NULL, NULL}
};

const char *progname;

static struct option long_options[] = {
	{ "fault",	required_argument,	NULL, 'f' },
	{ "help",	no_argument,		NULL, 'h' },
	{ "ident",	required_argument,	NULL, 'i' },
	{ "list",	no_argument,		NULL, 'l' },
	{ "verbose",	no_argument,		NULL, 'v' },
	{ "version",	no_argument,		NULL, 'V' },
	{ 0, 0, 0, 0}
};

static void
usage(void)
{
	fprintf(stderr,
		"Usage: %s { -l | settings } [-v] enclosure [ component ]\n"
		"\t-l | --list    : report settings\n"
		"\t-v | --verbose : verbose report\n"
		"\t-V | --version : print the version of the command "
								"and exit\n"
		"\t-h | --help    : print this usage message and exit\n"
		"\tsettings: [ -f on|off ] [ -i on|off ]\n"
		"\t  -f | --fault: turn component's fault LED element on/off.\n"
		"\t  -i | --ident: turn component's ident LED element on/off.\n"
		"\tenclosure: sgN device name or location code of enclosure\n"
		"\tcomponent: component's location sub-code -- e.g., P1-E2\n",
		progname);
}

/**
 * Get SCSI generic device name for the given location code.
 *
 * Returns :
 *	0 on success / -1 on failure
 */
static int
loc_code_device(const char *loccode, char *sg, int sg_size)
{
	int rc, found = 0, status = 0;
	FILE *fp;
	pid_t cpid;
	char buf[128];
	char *dev;
	char *args[] = {LSVPD_PATH, "-l", NULL, NULL};

	/* Command exists and has exec permissions ? */
	if (access(LSVPD_PATH, F_OK|X_OK) == -1) {
		fprintf(stderr, "Failed to obtain the sg device details.\n"
				"Check that %s is installed and has "
				"execute permissions.", LSVPD_PATH);
		return -1;
	}

	args[2] = (char *const) loccode;
	fp = spopen(args, &cpid);

	if (fp == NULL) {
		fprintf(stderr, "Could not obtain the sg device details.\n"
			"Failed to execute \"%s -l %s\" command.\n",
			LSVPD_PATH, loccode);
		return -1;
	}

	while (fgets_nonl(buf, 128, fp) != NULL) {
		if (found)	/* read until pipe becomes empty*/
			continue;
		/* Handle both old and new formats. */
		if (strstr(buf, "/dev/sg") || strstr(buf, "*AX sg")) {
			dev = strstr(buf, "sg");

			/* validate sg device */
			rc = valid_enclosure_device(dev);
			if (rc)
				continue;

			strncpy(sg, dev, sg_size);
			found = 1;
		}
	}

	status = spclose(fp, cpid);
	/* spclose() failed */
	if (status == -1) {
		fprintf(stderr, "%s : %d - failed in spclose(), "
				"error : %s\n", __func__, __LINE__,
				strerror(errno));
		return -1;
	}
	/* spclose() succeeded, but command failed */
	if (status != 0) {
		fprintf(stdout, "%s : %d - spclose() exited with status : "
				"%d\n", __func__, __LINE__, status);
		return -1;
	}

	if (found)
		return 0;

	return -1;
}

/**
 * Get SCSI generic device name
 *
 * @encl	sg device name or location code
 * @sg		sg dev name
 * @sg_size	sizeof(sg)
 *
 * Returns :
 *	0 on success / -1 on failure
 */
static int
enclosure_dev_name(const char *encl, char *sg, int sg_size)
{
	if (!strncmp(encl, "sg", 2) && strlen(encl) < PATH_MAX - 6) {
		strncpy(sg, encl, sg_size);
		return valid_enclosure_device(sg);
	}
	return loc_code_device(encl, sg, sg_size);
}

static void
too_many_settings(void)
{
	fprintf(stderr, "%s: cannot set fault or ident multiple times.\n",
								progname);
	exit(1);
}

static int
parse_on_off(const char *on_off)
{
	if (!strcmp(on_off, "on"))
		return LED_ON;
	else if (!strcmp(on_off, "off"))
		return LED_OFF;
	fprintf(stderr, "%s: expected 'on' or 'off'; saw '%s'.\n", progname,
								on_off);
	exit(1);
}

int
main(int argc, char **argv)
{
	int rc, option_index, i;
	int platform = 0;
	int list_opt = 0, verbose = 0, found = 0;
	int fault_setting = LED_SAME, ident_setting = LED_SAME;
	const char *enclosure = NULL, *component = NULL;
	char sg[128];
	struct dev_vpd vpd;

	progname = argv[0];

	platform = get_platform();
	if (platform != PLATFORM_PSERIES_LPAR) {
		fprintf(stderr, "%s is not supported on the %s platform\n",
				argv[0], __power_platform_name(platform));
		exit(1);
	}

	for (;;) {
		option_index = 0;
		rc = getopt_long(argc, argv, "f:hi:lvV", long_options,
							&option_index);
		if (rc == -1)
			break;
		switch (rc) {
		case 'f':
			if (fault_setting != LED_SAME)
				too_many_settings();
			fault_setting = parse_on_off(optarg);
			break;
		case 'h':
			usage();
			exit(0);
		case 'i':
			if (ident_setting != LED_SAME)
				too_many_settings();
			ident_setting = parse_on_off(optarg);
			break;
		case 'l':
			list_opt = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'V':
			printf("%s %s\n", argv[0], VERSION);
			exit(0);
		case '?':
		default:
			usage();
			exit(1);
		}
	}

	if (optind < argc)
		enclosure = argv[optind++];
	else {
		usage();
		exit(1);
	}
	if (optind < argc)
		component = argv[optind++];

	if (optind < argc) {
		usage();
		exit(1);
	}

	if (!list_opt && fault_setting == LED_SAME &&
				    ident_setting == LED_SAME) {
		usage();
		exit(1);
	}

	/* Get sg dev name for the given sg/location code */
	memset(sg, 0, sizeof(sg));
	if (enclosure_dev_name(enclosure, sg, sizeof(sg)) != 0)
		exit(1);

	/* Make sure sg is in running state */
	if (enclosure_maint_mode(sg) != 0)
		exit(1);

	/* Get enclosure type as "Machine Type" from VPD. */
	memset(&vpd, 0, sizeof(vpd));
	if (read_vpd_from_lscfg(&vpd, sg) != 0)
		exit(1);

	for (i = 0; encl_list[i].mtm; i++)
		if (!strcmp(vpd.mtm, encl_list[i].mtm)) {
			found = 1;
			break;
		}

	if (!found) {
		fprintf(stderr, "No indicator support for device type/model :"
				" %s\n", vpd.mtm);
		exit(0);
	}

	if (list_opt) {
		if (fault_setting != LED_SAME || ident_setting != LED_SAME) {
			usage();
			exit(1);
		}
		if (encl_list[i].list_leds(sg, component, verbose) != 0)
			exit(2);
	} else {
		if (geteuid() != 0) {
			fprintf(stderr, "%s: Turning LEDs on/off requires "
					"superuser privileges.\n", progname);
			exit(1);
		}
		if (encl_list[i].set_led(sg, component, fault_setting,
						ident_setting, verbose) != 0)
			exit(2);
	}

	exit(0);
}
