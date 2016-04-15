/*
 * Copyright (C) 2015 IBM Corporation
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
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <arpa/inet.h>

#include "encl_common.h"
#include "encl_util.h"
#include "diag_encl.h"
#include "platform.h"

struct event_severity_map {
	int	severity;
	char	*desc;
};
static struct event_severity_map event_severity_map[] = {
	{SL_SEV_FATAL,		"Fatal"},
	{SL_SEV_ERROR,		"Critical"},
	{SL_SEV_ERROR_LOCAL,	"Critical"},
	{SL_SEV_WARNING,	"Non-Critical"},
	{SL_SEV_EVENT,		"Normal"},
	{SL_SEV_INFO,		"Infomational"},
	{SL_SEV_DEBUG,		"Debug"},
	{-1,			"Unknown"},
};


/** Helper function to print enclosure component status **/

int
status_is_valid(enum element_status_code sc,
		enum element_status_code valid_codes[])
{
	enum element_status_code *v;
	for (v = valid_codes; *v < ES_EOL; v++)
		if (sc == *v)
			return 1;
	return 0;
}

const char *
status_string(enum element_status_code sc,
	      enum element_status_code valid_codes[])
{
	static char invalid_msg[40];	/* So we're not reentrant. */
	if (!status_is_valid(sc, valid_codes)) {
		snprintf(invalid_msg, 40, "(UNEXPECTED_STATUS_CODE=%u)", sc);
		return invalid_msg;
	}

	switch (sc) {
	default:
	case ES_UNSUPPORTED:
		return "UNSUPPORTED";
	case ES_OK:
		return "ok";
	case ES_CRITICAL:
		return "CRITICAL_FAULT";
	case ES_NONCRITICAL:
		return "NON_CRITICAL_FAULT";
	case ES_UNRECOVERABLE:
		return "UNRECOVERABLE_FAULT";
	case ES_NOT_INSTALLED:
		return "(empty)";
	case ES_UNKNOWN:
		return "UNKNOWN";
	case ES_NOT_AVAILABLE:
		return "NOT_AVAILABLE";
	case ES_NO_ACCESS_ALLOWED:
		return "NO_ACCESS_ALLOWED";
	}
	/*NOTREACHED*/
}

void
print_enclosure_status(struct enclosure_status *s,
		       enum element_status_code valid_codes[])
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;

	printf("%s", status_string(sc, valid_codes));

	if (s->failure_requested)
		printf(" | FAILURE_REQUESTED");
	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	printf("\n");
}

void
print_drive_status(struct disk_status *s,
		   enum element_status_code valid_codes[])
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;

	printf("%s", status_string(sc, valid_codes));

	if (s->ready_to_insert)
		printf(" | INSERT");
	if (s->rmv)
		printf(" | REMOVE");
	if (s->app_client_bypassed_a)
		printf(" | APP_CLIENT_BYPASSED_A");
	if (s->app_client_bypassed_b)
		printf(" | APP_CLIENT_BYPASSED_B");
	if (s->bypassed_a)
		printf(" | BYPASSED_A");
	if (s->bypassed_b)
		printf(" | BYPASSED_B");
	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	printf("\n");
}

void
print_esm_status(struct esm_status *s)
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_NOT_INSTALLED, ES_EOL
	};

	printf("%s", status_string(sc, valid_codes));

	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	printf("\n");
}

void
print_temp_sensor_status(struct temperature_sensor_status *s)
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_NOT_INSTALLED,
		ES_UNKNOWN, ES_EOL
	};

	printf("%s", status_string(sc, valid_codes));

	if (s->ot_failure)
		printf(" | OVER_TEMPERATURE_FAILURE");
	if (s->ot_warning)
		printf(" | OVER_TEMPERATURE_WARNING");
	if (cmd_opts.verbose)
		/* between -19 and +235 degrees Celsius */
		printf(" | TEMPERATURE = %dC", s->temperature - 20);
	printf("\n");
}

void
print_fan_status(struct fan_status *s,
		 enum element_status_code valid_codes[], const char *speed[])
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;

	printf("%s", status_string(sc, valid_codes));

	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	if (cmd_opts.verbose)
		printf(" | %s", speed[s->speed_code]);
	printf("\n");
}

void
print_power_supply_status(struct power_supply_status *s)
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_NOT_INSTALLED, ES_EOL
	};

	printf("%s", status_string(sc, valid_codes));

	if (s->dc_fail)
		printf(" | DC_FAIL");
	if (s->dc_over_voltage)
		printf(" | DC_OVER_VOLTAGE");
	if (s->dc_under_voltage)
		printf(" | DC_UNDER_VOLTAGE");
	if (s->dc_over_current)
		printf(" | DC_OVER_CURRENT");
	if (s->ac_fail)
		printf(" | AC_FAIL");
	CHK_IDENT_LED(s);
	CHK_FAULT_LED(s);
	printf("\n");
}

void
print_voltage_sensor_status(struct voltage_sensor_status *s)
{
	enum element_status_code sc =
				(enum element_status_code) s->byte0.status;
	static enum element_status_code valid_codes[] = {
		ES_OK, ES_CRITICAL, ES_NONCRITICAL, ES_NOT_INSTALLED,
		ES_UNKNOWN, ES_EOL
	};

	printf("%s", status_string(sc, valid_codes));

	if (s->warn_over)
		printf(" | NON_CRITICAL_OVER_VOLTAGE");
	if (s->warn_under)
		printf(" | NON_CRITICAL_UNDER_VOLTAGE");
	if (s->crit_over)
		printf(" | CRITICAL_OVER_VOLTAGE");
	if (s->crit_under)
		printf(" | CRITICAL_UNDER_VOLTAGE");
	if (cmd_opts.verbose)
		/* between +327.67 to -327.68
		 */
		printf(" | VOLTAGE = %.2f volts", ntohs(s->voltage) / 100.0);
	printf("\n");
}


/* Helper functions for reporting faults to servicelog start here. */

/*
 * Factor new status into the composite status cur.  A missing element
 * (ES_NOT_INSTALLED) is ignored.  A non-critical status is less severe
 * than critical.  Otherwise assume that an increasing value of
 * element_status_code indicates an increasing severity.  Return the more
 * severe of new and cur.
 */
static enum element_status_code
worse_element_status(enum element_status_code cur, enum element_status_code new)
{
	if (new == ES_OK || new == ES_NOT_INSTALLED)
		return cur;
	if ((cur == ES_OK || cur == ES_NONCRITICAL) && new > ES_OK)
		return new;
	return cur;
}

/*
 * Calculate the composite status for the nel elements starting at
 * address first_element.  We exploit the fact that every status element
 * is 4 bytes and starts with an element_status_byte0 struct.
 */
enum element_status_code
composite_status(const void *first_element, int nel)
{
	int i;
	const char *el = (const char *)first_element;
	enum element_status_code s = ES_OK;
	const struct element_status_byte0 *new_byte0;

	for (i = 0; i < nel; i++, el += 4) {
		new_byte0 = (const struct element_status_byte0 *) el;
		s = worse_element_status(s,
				(enum element_status_code) new_byte0->status);
	}
	return s;
}

static int
status_worsened(enum element_status_code old, enum element_status_code new)
{
	return (worse_element_status(old, new) != old);
}

/*
 * b is the address of a status byte 0 in dp (i.e., the status page we just
 * read from the SES).  If prev_dp has been populated, compare the old and
 * new status, and return 1 if the new status is worse, 0 otherwise.  If
 * prev_dp isn't valid, return 1.
 */
static int
element_status_reportable(const struct element_status_byte0 *new,
			  const char * const dp, const char * const prev_dp)
{
	ptrdiff_t offset;
	struct element_status_byte0 *old;

	if (!prev_dp)
		return 1;
	offset = ((char *) new) - dp;
	old = (struct element_status_byte0 *) (prev_dp + offset);
	return status_worsened((enum element_status_code) old->status,
				(enum element_status_code) new->status);
}

/*
 * If the status byte indicates a fault that needs to be reported, return
 * the appropriate servicelog status and start the description text
 * accordingly.  Else return 0.
 */
static uint8_t
svclog_status(enum element_status_code sc, char *crit)
{
	if (sc == ES_CRITICAL) {
		strncpy(crit, "Critical", ES_STATUS_STRING_MAXLEN - 1);
		crit[ES_STATUS_STRING_MAXLEN - 1] = '\0';
		return SL_SEV_ERROR;
	} else if (sc == ES_NONCRITICAL) {
		strncpy(crit, "Non-critical", ES_STATUS_STRING_MAXLEN - 1);
		crit[ES_STATUS_STRING_MAXLEN - 1] = '\0';
		return SL_SEV_WARNING;
	} else
		return 0;
}

uint8_t
svclog_element_status(struct element_status_byte0 *b, const char * const dp,
		      const char * const prev_dp, char *crit)
{
	if (!element_status_reportable(b, dp, prev_dp))
		return 0;
	return svclog_status(b->status, crit);
}

/*
 * Like element_status_reportable(), except we return 1 if the status of any
 * of the nel elements has worsened.
 */
static int
composite_status_reportable(const void *first_element, const char * const dp,
			    const char * const prev_dp, int nel)
{
	int i;
	const char *el = (const char *) first_element;

	if (!prev_dp)
		return 1;
	for (i = 0; i < nel; i++, el += 4) {
		if (element_status_reportable(
				(const struct element_status_byte0 *) el,
				(char *) dp, (char *) prev_dp))
			return 1;
	}
	return 0;
}

uint8_t
svclog_composite_status(const void *first_element,  const char * const dp,
			const char * const prev_dp, int nel, char *crit)
{
	if (!composite_status_reportable(first_element, dp, prev_dp, nel))
		return 0;
	return svclog_status(composite_status(first_element, nel), crit);
}


/* Get severity description */
static char *
get_event_severity_desc(int severity)
{
	int i;

	for (i = 0; event_severity_map[i].severity != -1; i++)
		if (severity == event_severity_map[i].severity)
			return event_severity_map[i].desc;

	return "Unknown";
}

/**
 * Get service action based on severity
 *
 * Presently we are clasifying event into two category:
 *   - Service action and call home required
 *   - No service action required
 *
 * In future we may add below event type for WARNING events
 * (similar to ELOG):
 *   - Service action required
 **/
static char *
get_event_action_desc(int severity)
{
	switch (severity) {
	case SL_SEV_FATAL:
	case SL_SEV_ERROR:
	case SL_SEV_ERROR_LOCAL:
		return "Service action and call home required";
	case SL_SEV_WARNING:  /* Fall through */
	case SL_SEV_EVENT:
	case SL_SEV_INFO:
	case SL_SEV_DEBUG:
	default:
		break;
	}
	return "No service action required";
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
		*callouts = calloc(1, sizeof(struct sl_callout));
		c = *callouts;
	} else {
		c = *callouts;
		while (c->next != NULL)
			c = c->next;
		c->next = calloc(1, sizeof(struct sl_callout));
		c = c->next;
	}
	if (c == NULL) {
		fprintf(stderr, "Out of memory\n");
		return;
	}

	c->priority = priority;
	c->type = type;
	if (proc_id) {
		c->procedure = strdup(proc_id);
		if (c->procedure == NULL)
			goto err_out;
	}
	if (location) {
		c->location = strdup(location);
		if (c->location == NULL)
			goto err_out;
	}
	if (fru) {
		c->fru = strdup(fru);
		if (c->fru == NULL)
			goto err_out;
	}
	if (sn) {
		c->serial = strdup(sn);
		if (c->serial == NULL)
			goto err_out;
	}
	if (ccin) {
		c->ccin = strdup(ccin);
		if (c->ccin == NULL)
			goto err_out;
	}

	return;

err_out:
	free(c->procedure);
	free(c->location);
	free(c->fru);
	free(c->serial);
	free(c->ccin);
	free(c);

	fprintf(stderr, "%s : %d - Failed to allocate memory.\n",
			__func__, __LINE__);
}

/* Add a callout with just the location code */
void
add_location_callout(struct sl_callout **callouts, char *location)
{
	add_callout(callouts, 'M', 0, NULL, location, NULL, NULL, NULL);
}

/* Add VPD info from VPD inquiry page */
void
add_callout_from_vpd_page(struct sl_callout **callouts,
			  char *location, struct vpd_page *vpd)
{
	char fru_number[8+1];
	char serial_number[12+1];
	char ccin[4+1];

	strzcpy(fru_number, vpd->fru_number, 8);
	strzcpy(serial_number, vpd->serial_number, 12);
	strzcpy(ccin, vpd->model_number, 4);
	add_callout(callouts, 'M', 0, NULL, location,
		    fru_number, serial_number, ccin);
}

/**
 * servicelog_log_event
 * @brief Generate a serviceable event and an entry to the servicelog
 *
 * @param refcode the SRN or SRC for the serviceable event
 * @param sev the severity of the event
 * @param text the description of the serviceable event
 * @param vpd a structure containing the VPD of the target
 * @param callouts a linked list of FRU callouts
 * @return key of the new servicelog entry
 */
static uint32_t
servicelog_log_event(const char *refcode, int sev, const char *text,
		     struct dev_vpd *vpd, struct sl_callout *callouts)
{
	struct servicelog *slog;
	struct sl_event *entry = NULL;
	struct sl_data_enclosure *encl = NULL;
	uint64_t key;
	int rc;

	if ((refcode == NULL) || (text == NULL) || (vpd == NULL))
		return 0;

	entry = calloc(1, sizeof(struct sl_event));
	if (entry == NULL) {
		fprintf(stderr, "Out of memory\n");
		return 0;
	}

	encl = calloc(1, sizeof(struct sl_data_enclosure));
	if (encl == NULL) {
		free(entry);
		fprintf(stderr, "Out of memory\n");
		return 0;
	}

	entry->addl_data = encl;

	entry->time_event = time(NULL);
	entry->type = SL_TYPE_ENCLOSURE;
	entry->severity = sev;
	/*
	 * Add service action and call home flag for event
	 * with severity > WARNING
	 */
	if (sev > SL_SEV_WARNING) {
		entry->disposition = SL_DISP_UNRECOVERABLE;
		entry->serviceable = 1;
		entry->call_home_status = SL_CALLHOME_CANDIDATE;
	} else {
		entry->disposition = SL_DISP_RECOVERABLE;
		entry->serviceable = 0;
		entry->call_home_status = SL_CALLHOME_NONE;
	}
	entry->description = strdup(text);
	if (entry->description == NULL)
		goto err0;
	entry->refcode = strdup(refcode);
	if (entry->refcode == NULL)
		goto err1;
	encl->enclosure_model = strdup(vpd->mtm);
	if (encl->enclosure_model == NULL)
		goto err2;
	encl->enclosure_serial = strdup(vpd->sn);
	if (encl->enclosure_serial == NULL)
		goto err3;

	entry->callouts = callouts;

	rc = servicelog_open(&slog, 0);
	if (rc != 0) {
		fprintf(stderr, "%s", servicelog_error(slog));
		servicelog_event_free(entry);
		return 0;
	}

	rc = servicelog_event_log(slog, entry, &key);
	servicelog_event_free(entry);
	servicelog_close(slog);

	if (rc != 0) {
		fprintf(stderr, "%s", servicelog_error(slog));
		return 0;
	}

	return key;

err3:
	free(encl->enclosure_model);
err2:
	free(entry->refcode);
err1:
	free(entry->description);
err0:
	free(entry);
	free(encl);

	fprintf(stderr, "%s : %d - Failed to allocate memory.\n",
			__func__, __LINE__);

	return 0;
}

/**
 * syslog_log_event
 * @brief Generate a serviceable event and an entry to syslog
 *
 * @param refcode the SRN or SRC for the serviceable event
 * @param sev the severity of the event
 * @param vpd a structure containing the VPD of the target
 */
static int
syslog_log_event(const char *refcode, int sev, struct dev_vpd *vpd)
{
	char *action, *severity_desc;
	int log_options;

	severity_desc = get_event_severity_desc(sev);
	action = get_event_action_desc(sev);

	/* syslog initialization */
	setlogmask(LOG_UPTO(LOG_NOTICE));
	log_options = LOG_CONS | LOG_PID | LOG_NDELAY;
	openlog("DIAG_ENCL", log_options, LOG_LOCAL1);

	syslog(LOG_NOTICE, "TM[%s]::SN[%s]::SRN[%s]::%s::%s",
	       vpd->mtm, vpd->sn, refcode, severity_desc, action);
	syslog(LOG_NOTICE, "Run 'diag_encl %s' for the details", vpd->dev);

	closelog();
	return 0;
}

/**
 * servevent
 * @brief Call platform specific event logging function
 */
uint32_t
servevent(const char *refcode, int sev, const char *text,
	  struct dev_vpd *vpd, struct sl_callout *callouts)
{
	if ((refcode == NULL) || (text == NULL) || (vpd == NULL))
		return -1;

	switch (platform) {
	case PLATFORM_PSERIES_LPAR:
		return servicelog_log_event(refcode, sev, text, vpd, callouts);
	case PLATFORM_POWERNV:
		return syslog_log_event(refcode, sev, vpd);
	case PLATFORM_POWERKVM_GUEST: /* Fall through */
	default:
		break;
	}
	return -1;
}
