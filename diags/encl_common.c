/*
 * Copyright (C) 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

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
	case PLATFORM_POWERKVM:
		return syslog_log_event(refcode, sev, vpd);
	case PLATFORM_POWERKVM_GUEST: /* Fall through */
	default:
		break;
	}
	return -1;
}
