/*
 * Copyright (C) 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef _ENCL_COMMON_H
#define _ENCL_COMMON_H

/*
 * This file contains macros, structures and function definations
 * which are common across enclosures. Enclosure specific details
 * can be found in enclosure specific header file.
 *
 * Presently it supports 5887 and EDR1 enclosures.
 */

#include <stdint.h>
#include <servicelog-1/servicelog.h>

#include "encl_util.h"

#define ES_STATUS_STRING_MAXLEN		32
#define EVENT_DESC_SIZE			512

#define FRU_NUMBER_LEN			8
#define SERIAL_NUMBER_LEN		12


/* SRN Format :
 *      FFC-xxx
 *	for SAS FFC = 2667
 *
 * Note:
 *   These SRN's are valid for Bluehawk and Homerun enclosures.
 */

/* Failing Function Code : SAS SCSI Enclosure Services */
#define SRN_FFC_SAS			0x2667

/* Return code for SAS SES components */
#define SRN_RC_CRIT_PS			0x125
#define SRN_RC_CRIT_FAN			0x135
#define SRN_RC_CRIT_ESM			0x155
#define SRN_RC_CRIT_EN			0x175
#define SRN_RC_DEVICE_CONFIG_ERROR	0x201
#define SRN_RC_ENCLOSURE_OPEN_FAILURE	0x202
#define SRN_RC_ENQUIRY_DATA_FAIL	0x203
#define SRN_RC_MEDIA_BAY		0x210
#define SRN_RC_VOLTAGE_THRESHOLD	0x239
#define SRN_RC_PS_TEMP_THRESHOLD	0x145
#define SRN_RC_TEMP_THRESHOLD		0x246

/* Build SRN */
#define SRN_SIZE	16
#define build_srn(srn, element) \
	snprintf(srn, SRN_SIZE, "%03X-%03X", SRN_FFC_SAS, element)

/*
 * If the indicated status element reports a fault, turn on the fault component
 * of the LED if it's not already on.  Keep the identify LED element unchanged.
 */
#define FAULT_LED(poked_leds, dp, ctrl_page, ctrl_element, status_element) \
do { \
	enum element_status_code sc = dp->status_element.byte0.status; \
	if (!dp->status_element.fail && \
			(sc == ES_CRITICAL || sc == ES_NONCRITICAL || \
			 sc == ES_UNRECOVERABLE)) { \
		ctrl_page->ctrl_element.common_ctrl.select = 1; \
		ctrl_page->ctrl_element.rqst_fail = 1; \
		ctrl_page->ctrl_element.rqst_ident = dp->status_element.ident; \
		poked_leds++; \
	} \
} while (0)


#define CHK_IDENT_LED(s) if ((s)->ident) printf(" | IDENT_LED")
#define CHK_FAULT_LED(s) if ((s)->fail) printf(" | FAULT_LED")


enum element_status_code {
	ES_UNSUPPORTED,		/* invalid status */
	ES_OK,
	ES_CRITICAL,		/* usually valid */
	ES_NONCRITICAL,		/* usually valid */
	ES_UNRECOVERABLE,	/* invalid status */
	ES_NOT_INSTALLED,
	ES_UNKNOWN,		/* usually invalid */
	ES_NOT_AVAILABLE,	/* usually invalid */
	ES_NO_ACCESS_ALLOWED,	/* invalid status */
	ES_EOL			/* end of list of status codes */
};

struct element_status_byte0 {
	uint8_t reserved1:1;	/* = 0 */
	uint8_t prdfail:1;	/* not implemented */
	uint8_t disabled:1;	/* not implemented */
	uint8_t swap:1;
	uint8_t status:4;	/* 0/1 or element_status_code */
};

struct overall_disk_status_byte1 {
	uint8_t device_environment:3;	/* = 010b */
	uint8_t slot_address:5;
};

struct disk_element_status_byte1 {
	uint8_t hot_swap:1;		/* = 1 */
	uint8_t slot_address:7;
};

struct disk_status {
	struct element_status_byte0 byte0;
	/* Overall status = worst status of all disks. */

	union {
		struct overall_disk_status_byte1 overall_status;
		struct disk_element_status_byte1 element_status;
	} byte1;

	uint8_t app_client_bypassed_a:1;
	uint8_t do_not_remove:1;
	uint8_t enclosure_bypassed_a:1;
	uint8_t enclosure_bypassed_b:1;
	uint8_t ready_to_insert:1;
	uint8_t rmv:1;
	uint8_t ident:1;
	uint8_t report:1;

	uint8_t app_client_bypassed_b:1;
	uint8_t fault_sensed:1;
	uint8_t fail:1;		/* AKA fault_reqstd */
	uint8_t device_off:1;
	uint8_t bypassed_a:1;
	uint8_t bypassed_b:1;
	uint8_t device_bypassed_a:1;
	uint8_t device_bypassed_b:1;
};

struct enclosure_status {
	struct element_status_byte0 byte0;
	/* status is always 1. */

	uint8_t ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3:6;
	uint8_t fail:1;		/* AKA failure_indication */
	uint8_t warning_indication:1;

	uint8_t reserved4:6;
	uint8_t failure_requested:1;
	uint8_t warning_requested:1;
};

struct esm_status {
	struct element_status_byte0 byte0;

	uint8_t ident:1;
	uint8_t fail:1;
	uint8_t reserved2:6;

	uint8_t reserved3:7;
	uint8_t report:1;

	uint8_t hot_swap:1;
	uint8_t reserved4:7;
};

struct temperature_sensor_status {
	struct element_status_byte0 byte0;

	uint8_t ident:1;
	uint8_t reserved2:7;

	uint8_t temperature;

	uint8_t reserved3:4;
	uint8_t ot_failure:1;
	uint8_t ot_warning:1;
	uint8_t ut_failure:1;
	uint8_t ut_warning:1;
};

struct fan_status {
	struct element_status_byte0 byte0;

	uint16_t ident:1;
	uint16_t reserved2:4;
	uint16_t fan_speed:11;

	uint8_t hot_swap:1;
	uint8_t fail:1;
	uint8_t rqsted_on:1;
	uint8_t off:1;
	uint8_t reserved3:1;
	uint8_t speed_code:3;
} __attribute__((packed));

struct power_supply_status {
	struct element_status_byte0 byte0;

	uint8_t ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3:4;
	uint8_t dc_over_voltage:1;
	uint8_t dc_under_voltage:1;
	uint8_t dc_over_current:1;
	uint8_t reserved4:1;

	uint8_t hot_swap:1;
	uint8_t fail:1;
	uint8_t rqsted_on:1;
	uint8_t off:1;
	uint8_t ovrtmp_fail:1;
	uint8_t temp_warn:1;
	uint8_t ac_fail:1;
	uint8_t dc_fail:1;
};

struct voltage_sensor_status {
	struct element_status_byte0 byte0;

	uint8_t ident:1;
	uint8_t reserved2:3;
	uint8_t warn_over:1;
	uint8_t warn_under:1;
	uint8_t crit_over:1;
	uint8_t crit_under:1;

	int16_t voltage;
};


/* Diagnostic page 2 layout for SEND DIAGNOSTIC command */

struct common_ctrl {
	uint8_t select:1;
	uint8_t prdfail:1;
	uint8_t disable:1;
	uint8_t rst_swap:1;
	uint8_t reserved1:4;
};

struct enclosure_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3;

	uint8_t reserved4:6;
	uint8_t rqst_fail:1;
	uint8_t rqst_warn:1;
};

struct esm_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t rqst_fail:1;
	uint8_t reserved2:6;

	uint8_t reserved3:7;
	uint8_t select_element:1;

	uint8_t reserved4;
};

struct fan_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3;

	uint8_t reserved4:1;
	uint8_t rqst_fail:1;
	uint8_t rqst_on:1;
	uint8_t reserved5:2;
	uint8_t requested_speed_code:3;
};

struct power_supply_ctrl {
	struct common_ctrl common_ctrl;

	uint8_t rqst_ident:1;
	uint8_t reserved2:7;

	uint8_t reserved3;

	uint8_t reserved4:1;
	uint8_t rqst_fail:1;
	uint8_t reserved5:6;
};


/* Obtains VPD for esm (page 1), midplane (page 5) via INQUIRY command */
struct vpd_page {
	uint8_t peripheral_qualifier;
	uint8_t	page_code;
	uint8_t reserved1;
	uint8_t page_length;
	uint8_t ascii_length;
	char fn[3];		/* "FN " */
	char fru_number[8];
	uint8_t reserved2;
	char sn[3];		/* "SN " */
	char serial_number[12];
	uint8_t reserved3;
	char cc[3];		/* "CC " */
	char model_number[4];	/* CCIN */
	uint8_t reserved4;
	char fl[3];		/* "FL " */
	char fru_label[5];
	uint8_t reserved5;
};


struct power_supply_descriptor {
	uint16_t descriptor_length;
	char fn[3];		/* "FN " */
	char fru_number[8];
	char sn[3];		/* "SN " */
	char serial_number[12];
	char vendor[12];
	char date_of_manufacture[12];
	char fl[3];		/* "FL " */
	char fru_label[5];
} __attribute__((packed));


extern int status_is_valid(enum element_status_code,
			   enum element_status_code []);
extern const char *status_string(enum element_status_code ,
				 enum element_status_code []);

extern void print_enclosure_status(struct enclosure_status *,
				   enum element_status_code []);
extern void print_drive_status(struct disk_status *,
			       enum element_status_code []);
extern void print_esm_status(struct esm_status *);
extern void print_temp_sensor_status(struct temperature_sensor_status *);
extern void print_fan_status(struct fan_status *,
			     enum element_status_code [], const char **);
extern void print_power_supply_status(struct power_supply_status *);
extern void print_voltage_sensor_status(struct voltage_sensor_status *);

extern enum element_status_code composite_status(const void *, int);
extern uint8_t svclog_element_status(struct element_status_byte0 *,
				     const char * const ,
				     const char * const , char *);
extern uint8_t svclog_composite_status(const void *,  const char * const ,
				       const char * const , int , char *);

extern void add_callout(struct sl_callout **, char, uint32_t,
			char *, char *, char *, char *, char *);
extern void add_location_callout(struct sl_callout **, char *);
extern void add_callout_from_vpd_page(struct sl_callout **,
				      char *, struct vpd_page *);
extern uint32_t servevent(const char *, int, const char *,
			  struct dev_vpd *, struct sl_callout *);


#endif	/* _ENCL_COMMON_H */
