/* Copyright (C) 2009, 2012 IBM Corporation */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <scsi/scsi.h>

#include "encl_util.h"
#include "diag_encl.h"

#define OK			0
#define EMPTY			-1
#define FAULT_CRITICAL		1
#define FAULT_NONCRITICAL	2

#define CRIT_PS			0x120
#define CRIT_FAN		0x130
#define CRIT_REPEATER		0x150
#define CRIT_VPD		0x160
#define UNRECOVER_PS		0x220
#define UNRECOVER_FAN		0x230
#define REDUNDANT		0x310
#define NONCRIT_PS		0x320
#define NONCRIT_FAN		0x330

struct pearl_diag_page2 {
	uint32_t page_code:8;
	uint32_t health_status:8;
	uint32_t page_length:16;
	uint32_t reserved:32;
	uint32_t drive_status_overall:32;
	uint32_t drive_status1:32;
	uint32_t drive_status2:32;
	uint32_t drive_status3:32;
	uint32_t drive_status4:32;
	uint32_t drive_status5:32;
	uint32_t drive_status6:32;
	uint32_t power_status_overall:32;
	uint32_t power_status1:32;
	uint32_t power_status2:32;
	uint32_t fan_status_overall:32;
	uint32_t fan_status1:32;
	uint32_t fan_status2:32;
	uint32_t fan_status3:32;
	uint32_t repeater_status:32;
	uint32_t vpd_card_status:32;
};

/**
 * get_enclosure_scsi_id
 * @brief Retrieve the SCSI ID for the Pearl enclosure
 *
 * @param dp the diagnostic page from the enclosure
 * @return the SCSI ID of the enclosure
 */ 
static int
get_enclosure_scsi_id(struct pearl_diag_page2 *dp)
{
	return ((dp->repeater_status & 0x000F0000) >> 16);
}

/**
 * print_drive status
 * @brief Print the status of a drive in the enclosure
 *
 * @param status the status of the drive from the diagnostic page
 * @return OK, EMPTY, FAULT_NONCRITICAL, or FAULT_CRITICAL
 */ 
static int
print_drive_status(uint32_t status)
{
	int fail = 0, printed = 0, rc = OK;

	if ((status & 0x0F000000) == 0x05000000) {
		printf("(empty)  ");
		rc = EMPTY;
	}
	else if ((status & 0x40000000) || (status & 0x00000040) ||
			(status & 0x00000020))
		fail = 1;
	else
		printf("ok  ");

	if (status & 0x40000000) {
		printf("PREDICTIVE_FAIL");
		rc = FAULT_NONCRITICAL;
		printed = 1;
	}
	if (status & 0x00000040) {
		if (printed) printf (" | ");
		printf("FAULT_SENSED");
		rc = FAULT_CRITICAL;
		printed = 1;
	}
	if (status & 0x00000020) {
		if (printed) printf (" | ");
		printf("FAULT_REQUESTED");
		rc = FAULT_CRITICAL;
		printed = 1;
	}
	if (status & 0x00000800) {
		if (printed) printf (" | ");
		printf("INSERT");
		printed = 1;
	}
	if (status & 0x00000400) {
		if (printed) printf (" | ");
		printf("REMOVE");
		printed = 1;
	}
	if (status & 0x00000200) {
		if (printed) printf (" | ");
		printf("IDENTIFY");
		printed = 1;
	}

	printf("\n");
	return rc;
}

/**
 * print_ps_fan status
 * @brief Print the status of a power supply or fan in the enclosure
 *
 * @param status the status of the part from the diagnostic page
 * @return OK, EMPTY, FAULT_NONCRITICAL, or FAULT_CRITICAL
 */ 
static int
print_ps_fan_status(uint32_t status)
{
	int fail = 0, rc = OK;

	if ((status & 0x0F000000) == 0x01000000)
		printf("ok  ");
	else if ((status & 0x0F000000) == 0x02000000) {
		printf("CRITICAL_FAULT");
		rc = FAULT_CRITICAL;
		fail = 1;
	}
	else if ((status & 0x0F000000) == 0x03000000) {
		printf("NON_CRITICAL_FAULT");
		rc = FAULT_NONCRITICAL;
		fail = 1;
	}
	else if ((status & 0x0F000000) == 0x05000000) {
		printf("(empty)  ");
		rc = EMPTY;
	}

	if (status & 0x00000200) {
		if (fail)
			printf(" | ");
		printf("IDENTIFY");
	}

	printf("\n");
	return rc;
}

/**
 * print_repeater_status
 * @brief Print the status of the repeater and SCSI connectors in the enclosure
 *
 * @param status the status of the repeater from the diagnostic page
 * @return OK or FAULT_CRITICAL
 */ 
static int
print_repeater_status(uint32_t status)
{
	int printed = 0, fail = 0, rc = OK;

	if ((status & 0x0F000000) == 0x01000000)
		printf("ok  ");
	else if ((status & 0x0F000000) == 0x02000000) {
		printf("CRITICAL_FAULT");
		rc = FAULT_CRITICAL;
		fail = 1;
		printed = 1;
	}
	if (status & 0x00000040) {
		if (printed) printf (" | ");
		printf("FAULT_SENSED");
		printed = 1;
	}
	if (status & 0x00000020) {
		if (printed) printf (" | ");
		printf("FAULT_REQUESTED");
		printed = 1;
	}

	if (status & 0x00000400) {
		if (printed) printf(" | ");
		printf("DRAWER_IDENTIFY");
		printed = 1;
	}
	if (status & 0x00000200) {
		if (printed) printf(" | ");
		printf("REPEATER_IDENTIFY");
		printed = 1;
	}

	printf("\n\n  SCSI Connector Status");
	printf("\n    Connector 1:     ");
	if (((status & 0x0000000C) >> 2) == 0x00)
		printf("(empty)");
	else if (((status & 0x0000000C) >> 2) == 0x01)
		printf("Connector installed.  Cable not present.");
	else if (((status & 0x0000000C) >> 2) == 0x03)
		printf("ok");

	printf("\n    Connector 2:     ");
	if ((status & 0x00000003) == 0x00)
		printf("(empty)");
	else if ((status & 0x00000003) == 0x01)
		printf("Connector installed.  Cable not present.");
	else if ((status & 0x00000003) == 0x03)
		printf("ok");

	printf("\n");
	return rc;
}

/**
 * print_vpd_card status
 * @brief Print the status of the VPD card in the enclosure
 *
 * @param status the status of the VPD card from the diagnostic page
 * @return OK or FAULT_CRITICAL
 */ 
static int
print_vpd_card_status(uint32_t status)
{
	int fail = 0, rc = OK;

	if ((status & 0x0F000000) == 0x01000000)
		printf("ok  ");
	else if ((status & 0x0F000000) == 0x02000000) {
		printf("CRITICAL_FAULT");
		rc = FAULT_CRITICAL;
		fail = 1;
	}

	if (status & 0x00000200) {
		if (fail)
			printf(" | ");
		printf("IDENTIFY");
	}

	printf("\n");
	return rc;
}

/**
 * pearl_servevent
 * @brief Generate a serviceable event for a fault found on a Pearl enclosure
 *
 * @param failtype the type of failure (CRIT_PS, REDUNDANT, etc)
 * @param devnum the number of the failed devices (e.g. 2 for the 2nd fan)
 * @param interface the SCSI ID of the enclosure
 * @param vpd structure containing the VPD of the enclosure
 */ 
static void
pearl_servevent(int failtype, int devnum, int interface, struct dev_vpd *vpd)
{
	char srn[16], *desc;
	struct sl_callout *callouts = NULL;
	int sev = SL_SEV_INFO;

	if (failtype == REDUNDANT)
		devnum = 1;

	/* build up the SRN */
	snprintf(srn, 16, "807-%03X", failtype + ((devnum - 1) * 4) +
			(interface - 1));

	switch(failtype) {
	case CRIT_PS:
		desc = "A critical power supply failure has occurred. "
			"Refer to the system service documentation for more "
			"information.";
		sev = SL_SEV_ERROR;
		break;
	case CRIT_FAN:
		desc = "A critical fan failure has occurred. "
			"Refer to the system service documentation for more "
			"information.";
		sev = SL_SEV_ERROR;
		break;
	case CRIT_REPEATER:
		desc = "A critical repeater card failure has occurred. "
			"Refer to the system service documentation for more "
			"information.";
		sev = SL_SEV_ERROR;
		break;
	case CRIT_VPD:
		desc = "A critical vpd module failure has occurred. "
			"Refer to the system service documentation for more "
			"information.";
		sev = SL_SEV_ERROR;
		break;
	case UNRECOVER_PS:
		desc = "An unrecoverable power supply failure has occurred. "
			"Refer to the system service documentation for more "
			"information.";
		sev = SL_SEV_ERROR;
		break;
	case UNRECOVER_FAN:
		desc = "An unrecoverable fan failure has occurred. "
			"Refer to the system service documentation for more "
			"information.";
		sev = SL_SEV_ERROR;
		break;
	case REDUNDANT:
		desc = "There is a redundant power supply or fan failure. "
			"Refer to the system service documentation for more "
			"information.";
		sev = SL_SEV_WARNING;
		break;
	case NONCRIT_PS:
		desc = "A non-critical power supply failure has occurred. "
			"Refer to the system service documentation for more "
			"information.";
		sev = SL_SEV_WARNING;
		break;
	case NONCRIT_FAN:
		desc = "A non-critical fan failure has occurred. "
			"Refer to the system service documentation for more "
			"information.";
		sev = SL_SEV_WARNING;
		break;
	default:
		desc = "Unknown failure.";
		break;
	}

	add_callout(&callouts, ' ', 0, "n/a", vpd->location, vpd->fru, "", "");
	servevent(srn, sev, desc, vpd, callouts);

	return;
}

/**
 * diag_7031_D24_T24
 * @brief Diagnose an enclosure with MTM 7031-D24/T24 (a.k.a. Pearl)
 *
 * @param fd a file descriptor to the SCSI generic file (e.g. /dev/sg7)
 * @param vpd structure containing the VPD of the enclosure
 * @return 0 if no faults were found, or 1 if faults were found
 */ 
int
diag_7031_D24_T24(int fd, struct dev_vpd *vpd)
{
	struct pearl_diag_page2 dp;
	int failure = 0, rc, encl_id;
	int buf_len = sizeof(dp);
	int ps1, ps2, fan1, fan2, fan3, rpt, vpd_card;

	if (cmd_opts.fake_path) {
		fprintf(stderr, "No support for -f option with "
				"enclosure type %s\n", vpd->mtm);
		return 1;
	}

	/* Usage warning and continue diagnostics */
	if (cmd_opts.cmp_prev || cmd_opts.leds)
		fprintf(stderr, "Option(s) ignored : No support for -c or -l "
				"options with enclosure type %s\n", vpd->mtm);

	/* the necessary data is on diagnostic page 2 */
	rc = get_diagnostic_page(fd, RECEIVE_DIAGNOSTIC, 2, (void *)&dp,
								buf_len);

	encl_id = get_enclosure_scsi_id(&dp);

	printf("  Enclosure SCSI ID: %d\n", encl_id);
	printf("  Overall Status:    ");
	if (dp.health_status == 0)
		printf("ok");
	if (dp.health_status & 0x02)
		printf("CRITICAL_FAULT");
	else if (dp.health_status & 0x04)
		printf("NON_CRITICAL_FAULT");
	else if (dp.health_status & 0x06)
		printf("CRITICAL_FAULT | NON_CRITICAL_FAULT");

	printf("\n\n  Drive Status\n");

	printf("    Slot SCSI ID %02d: ", (dp.drive_status1 & 0x000F0000)>>16);
	rc = print_drive_status(dp.drive_status1);
	printf("    Slot SCSI ID %02d: ", (dp.drive_status2 & 0x000F0000)>>16);
	rc = print_drive_status(dp.drive_status2);
	printf("    Slot SCSI ID %02d: ", (dp.drive_status3 & 0x000F0000)>>16);
	rc = print_drive_status(dp.drive_status3);
	printf("    Slot SCSI ID %02d: ", (dp.drive_status4 & 0x000F0000)>>16);
	rc = print_drive_status(dp.drive_status4);
	printf("    Slot SCSI ID %02d: ", (dp.drive_status5 & 0x000F0000)>>16);
	rc = print_drive_status(dp.drive_status5);
	printf("    Slot SCSI ID %02d: ", (dp.drive_status6 & 0x000F0000)>>16);
	rc = print_drive_status(dp.drive_status6);

	printf("\n  Power Supply Status\n");
	printf("    Power Supply 1:  ");
	ps1 = print_ps_fan_status(dp.power_status1);
	printf("    Power Supply 2:  ");
	ps2 = print_ps_fan_status(dp.power_status2);

	printf("\n  Fan Status\n");
	printf("    Fan 1:           ");
	fan1 = print_ps_fan_status(dp.fan_status1);
	printf("    Fan 2:           ");
	fan2 = print_ps_fan_status(dp.fan_status2);
	printf("    Fan 3:           ");
	fan3 = print_ps_fan_status(dp.fan_status3);

	printf("\n  Repeater Status:   ");
	rpt = print_repeater_status(dp.repeater_status);
	printf("\n  VPD Card Status:   ");
	vpd_card = print_vpd_card_status(dp.vpd_card_status);

	if (cmd_opts.verbose) {
		printf("\n\nRaw diagnostic page:\n");
		print_raw_data(stdout, (char *)&dp, buf_len);
	}

	printf("\n");

	if ((ps1 > 0) || (ps2 > 0) || (fan1 > 0) || (fan2 > 0) ||
			(fan3 > 0) || (rpt > 0) || (vpd_card > 0))
		failure = 1;

	if (failure && cmd_opts.serv_event) {
		/* Generate serviceable event(s) */

		/* Check power supply 1 */
		if (ps1 == FAULT_CRITICAL) {
			if (ps2 != OK)
				pearl_servevent(CRIT_PS, 1, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 1, encl_id, vpd);
		}
		else if (ps1 == FAULT_NONCRITICAL) {
			if (ps2 != OK)
				pearl_servevent(NONCRIT_PS, 1, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 1, encl_id, vpd);
		}

		/* Check power supply 2 */
		if (ps2 == FAULT_CRITICAL) {
			if (ps1 != OK)
				pearl_servevent(CRIT_PS, 2, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 2, encl_id, vpd);
		}
		else if (ps2 == FAULT_NONCRITICAL) {
			if (ps1 != OK)
				pearl_servevent(NONCRIT_PS, 2, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 2, encl_id, vpd);
		}

		/* Check fan 1 */
		if (fan1 == FAULT_CRITICAL) {
			if ((fan2 != OK) || (fan3 != OK))
				pearl_servevent(CRIT_FAN, 1, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 1, encl_id, vpd);
		}
		else if (fan1 == FAULT_NONCRITICAL) {
			if ((fan2 != OK) || (fan3 != OK))
				pearl_servevent(NONCRIT_FAN, 1, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 1, encl_id, vpd);
		}

		/* Check fan 2 */
		if (fan2 == FAULT_CRITICAL) {
			if ((fan1 != OK) || (fan3 != OK))
				pearl_servevent(CRIT_FAN, 2, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 2, encl_id, vpd);
		}
		else if (fan2 == FAULT_NONCRITICAL) {
			if ((fan1 != OK) || (fan3 != OK))
				pearl_servevent(NONCRIT_FAN, 2, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 2, encl_id, vpd);
		}

		/* Check fan 3 */
		if (fan3 == FAULT_CRITICAL) {
			if ((fan1 != OK) || (fan2 != OK))
				pearl_servevent(CRIT_FAN, 3, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 3, encl_id, vpd);
		}
		else if (fan3 == FAULT_NONCRITICAL) {
			if ((fan1 != OK) || (fan2 != OK))
				pearl_servevent(NONCRIT_FAN, 3, encl_id, vpd);
			else
				pearl_servevent(REDUNDANT, 3, encl_id, vpd);
		}

		/* Check repeater card */
		if (rpt == FAULT_CRITICAL) {
			pearl_servevent(CRIT_REPEATER, 1, encl_id, vpd);
		}

		/* Check VPD card */
		if (vpd_card == FAULT_CRITICAL) {
			pearl_servevent(CRIT_VPD, 1, encl_id, vpd);
		}
	}

	return failure;
}
