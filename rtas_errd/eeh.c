/*
 * @file eeh.c
 * @brief Routine to handle EEH notification RTAS events
 *
 * Copyright (C) 2004 IBM Corporation
 */

#include <stdio.h>
#include <string.h>
#include <librtasevent.h>
#include "rtas_errd.h"

/**
 * @struct event_desc
 * @brief Definition for a SRC value and its corresponding description.
 */
struct event_desc {
	char	*src_code;
	char	*desc;
};

/**
 * @var event_descs
 * @brief Array of event_desc structs for the SRC values to search 
 * in an EEH related RTAS event.
 */
struct event_desc event_descs[] = {
	{"BA188001", "EEH recovered a failing I/O adapter"},
	{"BA188002", "EEH could not recover the failed I/O adapter"},
	{"BA180010", "PCI probe error, bridge in freeze state"},
	{"BA180011", "PCI bridge probe error, bridge is not usable"},
	{"BA180012", "PCI device runtime error, bridge in freeze state"},
	{NULL, NULL}
};

/**
 * check_eeh
 * @brief Check a RTAS event for EEH event notification.
 *
 * Parse a RTAS event to see if this is an EEH event notification.  
 * If so, then update the platform log file with additional 
 * information about the EEH event.
 *
 * @param event pointer to the RTAS event
 */
void
check_eeh(struct event *event)
{
	struct rtas_event_hdr *rtas_hdr = event->rtas_hdr;
	struct rtas_priv_hdr_scn *privhdr;
	struct rtas_src_scn *src;
	int index = 0;
	
	if (rtas_hdr->version != 6) {
		return;
	}

	src = rtas_get_src_scn(event->rtas_event);
	if (src == NULL) {
		log_msg(event, "Could not retrieve SRC section to check for "
			"an EEH event, skipping");
		return;
	}

	/*
	 * Check for a known EEH event described above in the 
	 * events description array.
	 */
	while (event_descs[index].src_code != NULL) {
		if (strncmp((char *)src->primary_refcode, 
			    event_descs[index].src_code, 8) == 0)
			break;
		index++;
	}

	if (event_descs[index].src_code == NULL)
		return;

	/*
	 * Write the EEH Event notification out to /var/log/platform
	 * Please trust the compiler to get this right 
	 */
	platform_log_write("EEH Event Notification\n");
	
	privhdr = rtas_get_priv_hdr_scn(event->rtas_event);
	if (privhdr == NULL) {
		log_msg(event, "Could not retrieve the RTAS Event information "
			"to report EEH failure date/time");
		platform_log_write("Could not retrieve RTAS Event Date/Time\n");
	} else {
		platform_log_write("%02x/%02x/%04x  %02x:%02x:%02x\n", 
				   privhdr->date.month, privhdr->date.day, 
				   privhdr->date.year, privhdr->time.hour, 
				   privhdr->time.minutes, 
				   privhdr->time.seconds);
	}

	platform_log_write("Event SRC Code: %s\n", 
			   event_descs[index].src_code);
	platform_log_write("    %s\n", event_descs[index].desc);

	if (src->fru_scns != NULL) {
		struct rtas_fru_scn *fru = src->fru_scns;

		platform_log_write("    Device Location Code: %s\n", 
				   fru->loc_code); 

		snprintf(event->addl_text, ADDL_TEXT_MAX, 
			 "%s, Location %s", event_descs[index].desc, 
			 fru->loc_code);
	} else {
		platform_log_write("    No Device Location Code provided.\n");

		snprintf(event->addl_text, ADDL_TEXT_MAX, "%s, no "
			"location code provided", event_descs[index].desc);
	}

	return;
}
