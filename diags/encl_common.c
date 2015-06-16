/*
 * Copyright (C) 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encl_common.h"
#include "encl_util.h"

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
		*callouts = malloc(sizeof(struct sl_callout));
		c = *callouts;
	} else {
		c = *callouts;
		while (c->next != NULL)
			c = c->next;
		c->next = malloc(sizeof(struct sl_callout));
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
servevent(const char *refcode, int sev, const char *text,
	  struct dev_vpd *vpd, struct sl_callout *callouts)
{
	struct servicelog *slog;
	struct sl_event *entry = NULL;
	struct sl_data_enclosure *encl = NULL;
	uint64_t key;
	int rc;

	if ((refcode == NULL) || (text == NULL) || (vpd == NULL))
		return 0;

	entry = malloc(sizeof(struct sl_event));
	if (entry == NULL) {
		fprintf(stderr, "Out of memory\n");
		return 0;
	}
	memset(entry, 0, sizeof(struct sl_event));

	encl = malloc(sizeof(struct sl_data_enclosure));
	if (encl == NULL) {
		free(entry);
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
