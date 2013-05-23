/*
 * Copyright (C) 2012 IBM Corporation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "encl_util.h"
#include "encl_led.h"

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

#define DEVSG_MAXLEN 32
int
open_sg_device(const char *encl)
{
	char dev_sg[DEVSG_MAXLEN];
	int fd;

	snprintf(dev_sg, DEVSG_MAXLEN, "/dev/%s", encl);
	fd = open(dev_sg, O_RDWR);
	if (fd < 0)
		perror(dev_sg);
	return fd;
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
	int found = 0;
	char buf[128];
	char *dev;
	FILE *fp;
	struct dev_vpd vpd;

	fp = popen("lsvpd | grep sg", "r");
	if (fp == NULL) {
		fprintf(stderr, "Could not obtain a list of sg devices."
				 " Ensure that lsvpd is installed.\n");
		return -1;
	}

	while (fgets_nonl(buf, 128, fp) != NULL) {
		if (found)	/* read until pipe becomes empty*/
			continue;
		/* Handle both old and new formats. */
		if (strstr(buf, "/dev/sg") || strstr(buf, "*AX sg")) {
			dev = strstr(buf, "sg");
			memset(&vpd, 0, sizeof(vpd));
			if (read_vpd_from_lscfg(&vpd, dev) == 0)
				if (!strcmp(vpd.location, loccode)) {
					strncpy(sg, dev, sg_size);
					found = 1;
				}
		}
	}

	pclose(fp);

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
	if (!strncmp(encl, "sg", 2) && strlen(encl) < DEVSG_MAXLEN - 6) {
		strncpy(sg, encl, sg_size);
		return 0;
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
	int list_opt = 0, verbose = 0, found = 0;
	int fault_setting = LED_SAME, ident_setting = LED_SAME;
	const char *enclosure = NULL, *component = NULL;
	char sg[128];
	struct dev_vpd vpd;

	progname = argv[0];
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
	if (enclosure_dev_name(enclosure, sg, sizeof(sg)) != 0) {
		fprintf(stderr, "%s: %s is not a valid enclosure device\n",
				progname, enclosure);
		exit(1);
	}

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
