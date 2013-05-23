/* Copyright (C) 2009, 2012 IBM Corporation */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <sys/wait.h>
#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>

#include "encl_util.h"
#include "diag_encl.h"

static struct option long_options[] = {
	{"cmp_prev",		no_argument,		NULL, 'c'},
	{"fake",		required_argument,	NULL, 'f'},
	{"help",		no_argument,		NULL, 'h'},
	{"leds",		no_argument,		NULL, 'l'},
	{"serv_event",		no_argument,		NULL, 's'},
	{"verbose",		no_argument,		NULL, 'v'},
	{"version",		no_argument,		NULL, 'V'},
	{0, 0, 0, 0}
};

struct cmd_opts cmd_opts;

static struct {
	char *mtm;
	int (*func)(int, struct dev_vpd *);
} encl_diags[] = {
	{"7031-D24/T24", diag_7031_D24_T24},	/* Pearl enclosure */
	{"5888", diag_bluehawk},		/* Bluehawk enclosure */
	{"EDR1", diag_bluehawk},		/* Bluehawk enclosure */
	{NULL, NULL},
};

/**
 * print_usage
 * @brief Print the usage message for this command
 *
 * @param name the name of this executable
 */
static void
print_usage(const char *name) {
	printf("Usage: %s [-h] [-V] [-s [-c][-l]] [-v] [-f <path.pg2>]"
							" [<scsi_enclosure>]\n"
		"\n\t-h: print this help message\n"
		"\t-s: generate serviceable events for any failures and\n"
		"\t      write events to the servicelog\n"
		"\t-c: compare with previous status; report only new failures\n"
		"\t-l: turn on fault LEDs for serviceable events\n"
		"\t-v: verbose output\n"
		"\t-V: print the version of the command and exit\n"
		"\t-f: for testing, read SES data from path.pg2 and VPD\n"
		"\t      from path.vpd\n"
		"\t<scsi_enclosure>: the sg device on which to operate, such\n"
		"\t                    as sg7; if not specified, all such\n"
		"\t                    devices will be diagnosed\n", name);
}

/**
 * print_raw_data
 * @brief Dump a section of raw data
 *
 * @param ostream stream to which output should be written
 * @param data pointer to data to dump
 * @param data_len length of data to dump
 * @return number of bytes written
 */
int
print_raw_data(FILE *ostream, char *data, int data_len)
{
	char *h, *a;
	char *end = data + data_len;
	unsigned int offset = 0;
	int i, j;
	int len = 0;

	len += fprintf(ostream, "\n");

	h = a = data;

	while (h < end) {
		/* print offset */
		len += fprintf(ostream, "0x%04x:  ", offset);
		offset += 16;

		/* print hex */
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (h < end)
					len += fprintf(ostream, "%02x", *h++);
				else
					len += fprintf(ostream, "  ");
			}
			len += fprintf(ostream, " ");
		}

		/* print ascii */
		len += fprintf(ostream, "    [");
		for (i = 0; i < 16; i++) {
			if (a <= end) {
				if ((*a >= ' ') && (*a <= '~'))
					len += fprintf(ostream, "%c", *a);
				else
					len += fprintf(ostream, ".");
				a++;
			} else
				len += fprintf(ostream, " ");
		}
		len += fprintf(ostream, "]\n");
	}

	return len;
}

/**
 * add_callout
 * @brief Create a new sl_callout struct
 *
 * @param callouts address of pointer to callout list
 * @param priority callout priority
 * @param type callout type
 * @param proc_id callout procedure ID
 * @param location callout location code
 * @param fru callout FRU number
 * @param sn callout FRU serial number
 * @param ccin callout FRU ccin
 */
void
add_callout(struct sl_callout **callouts, char priority, uint32_t type,
	    char *proc_id, char *location, char *fru, char *sn, char *ccin)
{
	struct sl_callout *c;

	if (*callouts == NULL) {
		*callouts = (struct sl_callout *)malloc(sizeof
							(struct sl_callout));
		c = *callouts;
	} else {
		c = *callouts;
		while (c->next != NULL)
			c = c->next;
		c->next = (struct sl_callout *)malloc(sizeof
						      (struct sl_callout));
		c = c->next;
	}
	if (c == NULL) {
		fprintf(stderr, "Out of memory\n");
		return;
	}

	memset(c, 0, sizeof(struct sl_callout));
	c->priority = priority;
	c->type = type;
	if (proc_id) {
		c->procedure = (char *)malloc(strlen(proc_id) + 1);
		strcpy(c->procedure, proc_id);
	}
	if (location) {
		c->location = (char *)malloc(strlen(location) + 1);
		strcpy(c->location, location);
	}
	if (fru) {
		c->fru = (char *)malloc(strlen(fru) + 1);
		strcpy(c->fru, fru);
	}
	if (sn) {
		c->serial = (char *)malloc(strlen(sn) + 1);
		strcpy(c->serial, sn);
	}
	if (ccin) {
		c->ccin = (char *)malloc(strlen(ccin) + 1);
		strcpy(c->ccin, ccin);
	}
}

/**
 * servevent
 * @brief Generate a serviceable event and an entry to the servicelog
 *
 * @param refcode the SRN or SRC for the serviceable event
 * @param sev the severity of the event
 * @param text the description of the serviceable event
 * @param vpd a structure containing the VPD of the target
 * @param callouts a linked list of FRU callouts
 * @return key of the new servicelog entry
 */
uint32_t
servevent(char *refcode, int sev, char *text, struct dev_vpd *vpd,
	  struct sl_callout *callouts)
{
	struct servicelog *slog;
	struct sl_event *entry = NULL;
	struct sl_data_enclosure *encl = NULL;
	uint64_t key;
	int rc;

	if ((refcode == NULL) || (text == NULL) || (vpd == NULL))
		return 0;

	entry = (struct sl_event *)malloc(sizeof(struct sl_event));
	if (entry == NULL) {
		fprintf(stderr, "Out of memory\n");
		return 0;
	}
	memset(entry, 0, sizeof(struct sl_event));

	encl = (struct sl_data_enclosure *)malloc(
					sizeof(struct sl_data_enclosure));
	if (encl == NULL) {
		fprintf(stderr, "Out of memory\n");
		return 0;
	}
	memset(encl, 0, sizeof(struct sl_data_enclosure));

	entry->addl_data = encl;

	entry->time_event = time(NULL);
	entry->type = SL_TYPE_ENCLOSURE;
	entry->severity = sev;
	entry->disposition = SL_DISP_UNRECOVERABLE;
	entry->serviceable = 1;
	entry->call_home_status = SL_CALLHOME_CANDIDATE;
	entry->description = (char *)malloc(strlen(text) + 1);
	strcpy(entry->description, text);
	entry->refcode = (char *)malloc(strlen(refcode) + 1);
	strcpy(entry->refcode, refcode);
	encl->enclosure_model = (char *)malloc(strlen(vpd->mtm) + 1);
	strcpy(encl->enclosure_model, vpd->mtm);
	encl->enclosure_serial = (char *)malloc(strlen(vpd->sn) + 1);
	strcpy(encl->enclosure_serial, vpd->sn);

	entry->callouts = callouts;

	rc = servicelog_open(&slog, 0);
	if (rc != 0) {
		fprintf(stderr, servicelog_error(slog));
		return 0;
	}

	rc = servicelog_event_log(slog, entry, &key);
	servicelog_event_free(entry);
	servicelog_close(slog);

	if (rc != 0) {
		fprintf(stderr, servicelog_error(slog));
		return 0;
	}

	return key;
}

/*
 * Given pg2_path = /some/file.pg2, extract the needed VPD values from
 * /some/file.vpd.
 */
static int
read_fake_vpd(struct dev_vpd *vpd, const char *pg2_path)
{
	char *vpd_path, *dot;
	char *result;
	FILE *f;

	vpd_path = strdup(pg2_path);
	assert(vpd_path);
	dot = strrchr(vpd_path, '.');
	assert(dot && !strcmp(dot, ".pg2"));
	strcpy(dot, ".vpd");

	f = fopen(vpd_path, "r");
	if (!f) {
		perror(vpd_path);
		free(vpd_path);
		return -1;
	}

	result = fgets_nonl(vpd->mtm, 128, f);
	if (!result)
		goto missing_vpd;
	result = fgets_nonl(vpd->full_loc, 128, f);
	if (!result)
		goto missing_vpd;
	result = fgets_nonl(vpd->sn, 128, f);
	if (!result)
		goto missing_vpd;
	result = fgets_nonl(vpd->fru, 128, f);
	if (!result)
		goto missing_vpd;
	fclose(f);
	free(vpd_path);

	trim_location_code(vpd);
	return 0;

missing_vpd:
	fprintf(stderr, "%s lacks acceptable mtm, location code, serial number,"
			" and FRU number.\n", vpd_path);
	fclose(f);
	free(vpd_path);
	return -1;
}

#define DIAG_ENCL_PREV_PAGES_DIR "/etc/ppc64-diag/ses_pages/"
static void
make_prev_path(const char *encl_loc)
{
	free(cmd_opts.prev_path);
	cmd_opts.prev_path = (char *) malloc(sizeof(DIAG_ENCL_PREV_PAGES_DIR) +
					strlen(encl_loc) + 4);
	strcpy(cmd_opts.prev_path, DIAG_ENCL_PREV_PAGES_DIR);
	strcat(cmd_opts.prev_path, encl_loc);
	strcat(cmd_opts.prev_path, ".pg2");
}

static void
free_dev_vpd(struct dev_vpd *vpd)
{
	struct dev_vpd *tmp;

	while (vpd) {
		tmp = vpd;
		vpd = vpd->next;
		free(tmp);
	}
}

/*
 * enclosure_maint_mode
 * @brief Check the state of SCSI enclosure
 *
 * Returns:
 *	-1 on failure
 *	1 if sg is offline
 *	0 if sg is running
 */
static int
enclosure_maint_mode(const char *sg)
{
	char devsg[128], sgstate[128];
	FILE *fp;

	snprintf(devsg, 128, "/sys/class/scsi_generic/%s/device/state", sg);
	fp = fopen(devsg, "r");
	if (!fp) {
		perror(devsg);
		fprintf(stderr, "Unable to open enclosure "
				"state file : %s\n", devsg);
		return -1;
	}

	if (fgets_nonl(sgstate, 128, fp) == NULL) {
		fprintf(stderr, "Unable to read the state of "
				"enclosure device : %s\n", sg);
		fclose(fp);
		return -1;
	}

	/* Check device state */
	if (!strcmp(sgstate, "offline")) {
		fprintf(stderr, "Enclosure \"%s\" is offline."
				" Cannot run diagnostics.\n", sg);
		fclose(fp);
		return 1;
	}

	fclose(fp);
	return 0;
}

/**
 * diagnose
 * @brief Diagnose a specific SCSI generic enclosure
 *
 * @param sg the SCSI generic device to diagnose (e.g. "sg7")
 * @return 0 for no failure, 1 if there is a failure on the enclosure
 */
static int
diagnose(const char *sg, struct dev_vpd **diagnosed)
{
	int rc = 0, fd, found = 0, i;
	char devsg[128];
	struct dev_vpd *vpd = NULL;
	struct dev_vpd *v;

	printf("DIAGNOSING %s\n", sg);

	vpd = (struct dev_vpd *)malloc(sizeof(struct dev_vpd));
	if (vpd == NULL) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	}
	memset(vpd, 0, sizeof(struct dev_vpd));

	if (cmd_opts.fake_path)
		rc = read_fake_vpd(vpd, cmd_opts.fake_path);
	else
		rc = read_vpd_from_lscfg(vpd, sg);

	if (vpd->mtm[0] == '\0') {
		fprintf(stderr, "Unable to find machine type/model for %s\n",
				sg);
		goto error_out;
	}
	if (cmd_opts.serv_event && vpd->location[0] == '\0') {
		fprintf(stderr, "Unable to find location code for %s; needed "
				"for -s\n", sg);
		goto error_out;
	}
	if (rc != 0)
		fprintf(stderr, "Warning: unable to find all relevant VPD for "
				"%s\n", sg);

	printf("\tModel    : %s\n\tLocation : %s\n\n", vpd->mtm, vpd->full_loc);

	for (i = 0; encl_diags[i].mtm; i++) {
		if (!strcmp(vpd->mtm, encl_diags[i].mtm)) {
			for (v = *diagnosed; v; v = v->next) {
				if (!strcmp(v->location, vpd->location)) {
					printf("\t(Enclosure already diagnosed)\n\n");
					free(vpd);
					return 0;
				}
			}

			/* fake patch ? */
			if (cmd_opts.fake_path)
				fd = -1;
			else {
				/* Skip diagnostics if the enclosure is
				 * temporarily disabled for maintenance.
				 */
				if (enclosure_maint_mode(sg))
					goto error_out;

				/* Open sg device */
				snprintf(devsg, 128, "/dev/%s", sg);
				fd = open(devsg, O_RDWR);
				if (fd <= 1) {
					fprintf(stderr, "Unable to open %s\n\n",
							devsg);
					goto error_out;
				}
			}

			/* diagnose */
			if (cmd_opts.serv_event)
				make_prev_path(vpd->location);
			found = 1;
			rc += encl_diags[i].func(fd, vpd);

			if (fd != -1)
				close(fd);
			break;
		}
	}

	if (found) {
		vpd->next = *diagnosed;
		*diagnosed = vpd;
	} else {
		free(vpd);
		fprintf(stderr, "Unable to diagnose devices of machine "
				"type/model %s\n\n", vpd->mtm);
	}
	return rc;

error_out:
	free(vpd);
	return 1;
}

int
main(int argc, char *argv[])
{
	int failure = 0, option_index, rc;
	char buf[128], *sg;
	FILE *fp;
	struct dev_vpd *diagnosed = NULL;

	memset(&cmd_opts, 0, sizeof(cmd_opts));

	for (;;) {
		option_index = 0;
		rc = getopt_long(argc, argv, "cf:hlsvV", long_options,
				 &option_index);

		if (rc == -1)
			break;

		switch (rc) {
		case 'c':
			cmd_opts.cmp_prev = 1;
			break;
		case 'f':
			if (cmd_opts.fake_path) {
				fprintf(stderr, "Multiple -f options not "
						"supported.\n");
				return -1;
			}
			cmd_opts.fake_path = optarg;
			break;
		case 'h':
			print_usage(argv[0]);
			return 0;
		case 'l':
			cmd_opts.leds = 1;
			break;
		case 's':
			cmd_opts.serv_event = 1;
			break;
		case 'v':
			cmd_opts.verbose = 1;
			break;
		case 'V':
			printf("%s %s\n", argv[0], VERSION);
			return 0;
		case '?':
			print_usage(argv[0]);
			return -1;
		default:
			/* Shouldn't get here. */
			fprintf(stderr, "huh?\n");
			print_usage(argv[0]);
			return -1;
		}
	}

	if (cmd_opts.cmp_prev && !cmd_opts.serv_event) {
		fprintf(stderr, "No -c option without -s\n");
		return -1;
	}

	if (cmd_opts.leds && !cmd_opts.serv_event) {
		fprintf(stderr, "No -l option without -s\n");
		return -1;
	}

	if ((cmd_opts.serv_event || cmd_opts.leds) && geteuid() != 0) {
		fprintf(stderr, "-s and -l options require superuser "
				"privileges\n");
		return -1;
	}

	if (cmd_opts.fake_path) {
		const char *dot = strrchr(cmd_opts.fake_path, '.');
		if (!dot || strcmp(dot, ".pg2") != 0) {
			fprintf(stderr, "Name of file with fake diagnostic "
					"data must end in '.pg2'.\n");
			return -1;
		}
		if (optind + 1 != argc) {
			fprintf(stderr, "Please specify an sg device with the "
					"-f pathname. It need not be an "
					"enclosure.\n");
			return -1;
		}
		failure += diagnose(argv[optind++], &diagnosed);
	} else if (optind < argc) {
		while (optind < argc)
			failure += diagnose(argv[optind++], &diagnosed);

	} else {
		/* use lsvpd to find all sg devices */
		fp = popen("lsvpd | grep sg", "r");
		if (fp == NULL) {
			fprintf(stderr, "Could not obtain a list of sg devices."
					" Ensure that lsvpd is installed.\n");
			return -1;
		}
		while (fgets_nonl(buf, 128, fp) != NULL) {
			/* Handle both old and new formats. */
			if (strstr(buf, "/dev/sg") || strstr(buf, "*AX sg")) {
				sg = strstr(buf, "sg");
				failure += diagnose(sg, &diagnosed);
			}
		}
		pclose(fp);
	}

	free(cmd_opts.prev_path);
	free_dev_vpd(diagnosed);

	return failure;
}
