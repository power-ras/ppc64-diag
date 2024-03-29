/**
 * @file servicelog.c
 * @brief Routines for inserting records into the servicelog database.
 *
 * Copyright (C) 2005 IBM Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <librtasevent.h>
#include <sys/wait.h>
#include "rtas_errd.h"

#define bcd_4b_toint(x)	( ((((x) & 0xf000) >> 12) * 1000) + \
			  ((((x) & 0xf00) >> 8) * 100) +    \
			  ((((x) & 0xf0) >> 4) * 10) +      \
			  ((((x) & 0xf))))
#define bcd_2b_toint(x)	( ((((x) & 0xf0) >> 4) * 10) +      \
			  ((((x) & 0xf))))

/**
 * get_event_date
 * @brief Retrieve the timestamp from an event
 *
 * @param event the event from which to retrieve the timestamp
 * @return the timestamp as time since Epoch, or 0 on failure
 */
time_t
get_event_date(struct event *event)
{
	struct rtas_time *time = NULL;
	struct rtas_date *date = NULL;

	if (event->rtas_hdr->version == 6) {
		struct rtas_priv_hdr_scn *privhdr;
		privhdr = rtas_get_priv_hdr_scn(event->rtas_event);

		if (privhdr == NULL) {
			log_msg(event, "Could not parse RTAS event to "
				"initialize servicelog timestamp.");
		} else {
			time = &privhdr->time;
			date = &privhdr->date;
		}
	} else {
		struct rtas_event_exthdr *exthdr;
		exthdr = rtas_get_event_exthdr_scn(event->rtas_event);

		if (exthdr == NULL) {
			log_msg(event, "Could not parse RTAS event to "
				"initialize servicelog timestamp.");
		} else {
			time = &exthdr->time;
			date = &exthdr->date;
		}
	}

	if (time != NULL) {
		struct tm tm = {0};

		tm.tm_year = bcd_4b_toint(date->year) - 1900;
		tm.tm_mon = bcd_2b_toint(date->month) - 1;
		tm.tm_mday = bcd_2b_toint(date->day);
		tm.tm_hour = bcd_2b_toint(time->hour);
		tm.tm_min = bcd_2b_toint(time->minutes);
		tm.tm_sec = bcd_2b_toint(time->seconds);
		tm.tm_isdst = -1;

		return mktime(&tm);
	}

	return 0;
}

/**
 * servicelog_sev
 * @brief convert RTAS severity to servicelog severity
 *
 * @param rtas_sev RTAS severity to be converted
 * @return servicelog severity number, SL_SEV_INFO if unknown severity
 */
int
servicelog_sev(int rtas_sev)
{
	switch (rtas_sev) {
	case 0:	/* NO_ERROR */
		return SL_SEV_INFO;
	case 1:	/* EVENT */
		return SL_SEV_EVENT;
	case 2:	/* WARNING */
		return SL_SEV_WARNING;
	case 3:	/* ERROR_SYNC */
		return SL_SEV_ERROR;
	case 4:	/* ERROR */
		return SL_SEV_ERROR;
	case 5:	/* FATAL */
		return SL_SEV_FATAL;
	case 6:	/* ALREADY_REPORTED */
		return SL_SEV_INFO;
	}

	return SL_SEV_INFO;
}

/**
 * add_callout
 * @brief Add a new FRU callout to the list for this event
 *
 * @param event event to which to add the callout
 * @param pri priority
 * @param type type
 * @param proc procedure ID
 * @param loc location code
 * @param pn FRU part number
 * @param sn FRU serial number
 * @param ccin FRU ccin
 */
void
add_callout(struct event *event, char pri, int type, char *proc, char *loc,
	    char *pn, char *sn, char *ccin)
{
	struct sl_callout *callout = event->sl_entry->callouts;

	if (!callout) {
		event->sl_entry->callouts = calloc(1, sizeof(*callout));
		callout = event->sl_entry->callouts;
	}
	else {
		while (callout->next)
			callout = callout->next;
		callout->next = calloc(1, sizeof(*callout));
		callout = callout->next;
	}

	if (!callout)
		return;

	callout->priority = pri;
	callout->type = type;
	if (proc) {
		callout->procedure = malloc(strlen(proc) + 1);
		if (callout->procedure == NULL)
			goto mem_fail;
		strncpy(callout->procedure, proc, strlen(proc) + 1);
	}
	if (loc) {
		callout->location = malloc(strlen(loc) + 1);
		if (callout->location == NULL)
			goto mem_fail;
		strncpy(callout->location, loc, strlen(loc) + 1);
	}
	if (pn) {
		callout->fru = malloc(strlen(pn) + 1);
		if (callout->fru == NULL)
			goto mem_fail;
		strncpy(callout->fru, pn, strlen(pn) + 1);
	}
	if (sn) {
		callout->serial = malloc(strlen(sn) + 1);
		if (callout->serial == NULL)
			goto mem_fail;
		strncpy(callout->serial, sn, strlen(sn) + 1);
	}
	if (ccin) {
		callout->ccin = malloc(strlen(ccin) + 1);
		if (callout->ccin == NULL)
			goto mem_fail;
		strncpy(callout->ccin, ccin, strlen(ccin) + 1);
	}

	return;

mem_fail:
	if (callout->ccin)
		free(callout->ccin);
	if (callout->serial)
		free(callout->serial);
	if (callout->fru)
		free(callout->fru);
	if (callout->location)
		free(callout->location);
	if (callout->procedure)
		free(callout->procedure);
	free(callout);

	log_msg(event, "Memory allocation failed");
	return;
}

/**
 * log_event
 * @brief log the event in the servicelog DB
 *
 * @param event RTAS event structure
 */
void
log_event(struct event *event)
{
	struct rtas_dump_scn *scn_dump;
	uint64_t key;
	int rc, txtlen;

	/* If the DB isn't available, do nothing */
	if (slog == NULL)
		return;

	if (event->sl_entry == NULL) {
		log_msg(event, "ELA did not generate any data to be logged "
			"for this event.");
		return;
	}

	if (event->flags & RE_PLATDUMP_AVAIL) {
		if ((scn_dump = rtas_get_dump_scn(event->rtas_event)) != NULL) {
			char filename[41];
			uint64_t dump_size = 0;

			memset(filename, 0, 41);
			memcpy(filename, scn_dump->os_id, scn_dump->id_len);
			dump_size = scn_dump->size_hi;
			dump_size <<= 32;
			dump_size |= scn_dump->size_lo;
			snprintf(event->addl_text, ADDL_TEXT_MAX, "A platform "
				 "dump was generated and downloaded to the "
				 "filesystem (%llu bytes):\n%s%s",
				 (long long unsigned int)dump_size,
				 d_cfg.platform_dump_path, filename);
		}
	}

	/* Append any additional text to the output of the analysis */
	txtlen = strlen(event->addl_text);
	if (txtlen > 0) {
		if (event->sl_entry->description) {
			char *new_desc;
			int new_len;

			new_len = strlen(event->sl_entry->description) +
				  txtlen + 3;
			new_desc = malloc(new_len);
			if (new_desc == NULL) {
				log_msg(event, "Memory allocation failed");
				return;
			}
			snprintf(new_desc, new_len, "%s\n\n%s",
				 event->sl_entry->description,
				 event->addl_text);

			free(event->sl_entry->description);
			event->sl_entry->description = new_desc;
		} else {
			event->sl_entry->description = strdup(event->addl_text);
			if (event->sl_entry->description == NULL) {
				log_msg(event, "Memory allocation failed");
				return;
			}
		}
	}

	/* Log the event in the servicelog */
	rc = servicelog_event_log(slog, event->sl_entry, &key);
	servicelog_event_free(event->sl_entry);

	if (rc)
		log_msg(event, "Could not log event to servicelog.\n%s\n",
			servicelog_error(slog));
	else
		log_msg(event, "servicelog key %llu", key);
}
