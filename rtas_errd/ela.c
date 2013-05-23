/**
 * @file ela.c
 *
 * Copyright (C) 2005 IBM Corporation
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <librtasevent.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include "fru_prev6.h"            
#include "dchrp.h"
#include "dchrp_frus.h"

#include "rtas_errd.h"
#include "ela_msg.h"

/* Function prototypes */
static int analyze_io_bus_error(struct event *, int, int);
static int get_error_type(struct event *, int);
static int report_srn(struct event *, int, struct event_description_pre_v6 *);
static char *get_loc_code(struct event *, int, int *);
static int report_menugoal(struct event *, char *, ...);
static int report_io_error_frus(struct event *, int,
				struct event_description_pre_v6 *, 
				struct device_ela *, struct device_ela *);
static int get_cpu_frus(struct event *);

static int process_v1_epow(struct event *, int); 
static int process_v2_epow(struct event *, int);
static int process_v3_epow(struct event *, int, int);
static int sensor_epow(struct event *, int, int);
static void unknown_epow_ela(struct event *, int);

static int process_v2_sp(struct event *, int);
static int process_v3_logs(struct event *, int);
static int convert_symptom(struct event *, int, int, char **);
static int process_refcodes(struct event *, short *, int);

extern	char *optarg;

/* Macro to determine if there location codes in the buffer */
#define LOC_CODES_OK ((event->loc_codes != NULL) && strlen(event->loc_codes)) 

/* Global saved io error type with Bridge Connection bits. */
int io_error_type = 0;
#define IO_BRIDGE_MASK 0x0FFF0FFF /* mask off byte 13, bits 0-3 */
				  /* of error_type to get io_error_type */

#define PTABLE_SIZE	4
int pct_index = 0;
int percent_table[2][PTABLE_SIZE+1][PTABLE_SIZE] = {
       {{0,0,0,0},
	{100,0,0,0},
	{67,33,0,0},
	{50,30,20,0},
	{40,30,20,10}},
       {{0,0,0,0},
	{100,0,0,0},
	{50,50,0,0},
	{33,33,33,0},
	{25,25,25,25}}
};

/*
 * Table to hold ref codes from the error log entry prior
 * to filling out the error description.
 */
short rctab[MAXREFCODES+1];
/*
 * And a "shadow" table to hold number of location codes per ref code
 */
char  rctabloc[MAXREFCODES+1];

char *
get_tod()
{
	time_t	t;
	
	time(&t);
	return (asctime(localtime(&t)));
}

/**
 * convert_bcd
 * @brief Convert decimal number to hex number
 * 
 * Converts num into hex number that will display as decimal.
 * i.e. binary 0x1010 becomes 0x10000
 *                A              10
 *
 * @param num decimal number to convert
 * @returns converted number
 */
static int 
convert_bcd(int num)
{
	char tempnum[10];

	sprintf(tempnum, "%d", num);
	num = strtol(tempnum, NULL, 16);
	
	return num;
}

/**
 * get_refcode
 *
 */
static char *
get_refcode(struct event_description_pre_v6 *event, int index)
{
        static char rfc[80];

	if (strcmp(event->frus[index].fname, "REF-CODE") == 0) {
		sprintf(rfc, "%04hX%04hX", event->frus[index].fmsg,
			event->frus[index].conf);
		dbg("reference code \"%s\"", rfc);
        	return rfc;
	}

	return NULL;
}

/**
 * add_more_descrs
 *
 */
int
add_more_descrs(struct event *event, struct event_description_pre_v6 *ptr)
{
	int i, rc = 0;
	char *frupn = NULL;
	char *refcode;
	char na[] = "n/a";

	i = 0;
	while (i < MAXFRUS && ptr->frus[i].conf != 0) {
		if (ptr->frus[i].floc[0])
			frupn = diag_get_fru_pn(event,
						ptr->frus[i].floc);
		else
			strcpy(ptr->frus[i].floc, na);

		if (frupn == NULL)
			frupn = na;

		refcode = get_refcode(ptr, i);
		if (refcode == NULL)
			refcode = na;

		add_callout(event, ' ', 0, refcode, ptr->frus[i].floc,
			    frupn, NULL, NULL);

		dbg("loc = \"%s\",\nfrupn = \"%s\",\nrefcode = \"%s\"",
		    ptr->frus[i].floc, frupn, refcode);

		i++;
	}

	return rc;
}

/**
 * set_srn_and_callouts
 * @brief Determines the SRN and creates callout structs
 *
 * Note that the sl_entry struct in event struct (1st parameter) must be
 * allocated before calling this routing.
 *
 * @param event the event on which to operate
 * @param ptr the description of the event as established by ELA
 * @param post_error 0 if not a post error; post error code otherwise
 * @return 0 on success, -1 on failure.
 */
int
set_srn_and_callouts(struct event *event, struct event_description_pre_v6 *ptr,
		     unsigned post_error)
{
	char srn[80];
	int i;
	char *frupn = NULL;
	char *refcode;
	char na[] = "n/a";

	dbg("pre-v6 event: flags = %d, sn = %d, rcode = %d, rmsg = %d\n"
	    "dname = \"%s\"", ptr->flags, ptr->sn, ptr->rcode, ptr->rmsg,
	    ptr->dname);

	if (post_error == 0)
		snprintf(srn, 80, "%03hX-%03hX", ptr->sn, ptr->rcode);
	else
		snprintf(srn, 80, "%0X", post_error);

	event->sl_entry->refcode = (char *)malloc(strlen(srn)+1);
	strncpy(event->sl_entry->refcode, srn, strlen(srn)+1);
	dbg("SRN: \"%s\"", event->sl_entry->refcode);

	i = 0;
	while (i < MAXFRUS && ptr->frus[i].conf != 0) {
		if (ptr->frus[i].floc[0])
			frupn = diag_get_fru_pn(event, ptr->frus[i].floc);
		else
			strcpy(ptr->frus[i].floc, na);

		if (frupn == NULL)
			frupn = na;

		refcode = get_refcode(ptr, i);
		if (refcode == NULL)
			refcode = na;

		dbg("formatting fru list \"%s\", frupn = \"%s\"\n"
		    "refcode = \"%s\"", ptr->frus[i].floc, frupn,
		    refcode);

		add_callout(event, ' ', 0, refcode, ptr->frus[i].floc,
			    frupn, NULL, NULL);

		i++;
	}

	return 0;
}

/**
 * add_cpu_id
 *
 * Adds the physical cpu id number to the logical name procx for an 
 * event that calls out a processor.
 * 
 * @returns 0 if valid cpuid was added to the event data
 * @returns 1 if invalid cpuid was input, so event data changed to proc0
 */
int 
add_cpu_id(struct event_description_pre_v6 *ptr, int fru_num, int cpuid)
{
	int rc = 0;

	if (cpuid > 0) 
		cpuid--;	/* physical cpu id indexed from 1 */
	else {
		cpuid = 0;
		rc = 1;
	}

	/* Stuff the cpuid in for the 'x' of "procx" */
	ptr->frus[fru_num].fname[4] = '0' + cpuid;

	/* Set the fru name is not in the db */
	ptr->frus[fru_num].fname[0] = '\0';

	return rc;
}

/**
 * set_fru_percentages
 *
 * Sets the fru percentages (probablity of failure) from the global 
 * table that is based on the number of FRUs. Used for unknown events
 * and post errors when want to report the physical location codes
 * from RTAS.
 */
void 
set_fru_percentages(struct event_description_pre_v6 *event, int nfrus)
{
	int *percent;
	int i;

	if (nfrus > MAXFRUS) 
		nfrus = MAXFRUS;

	if (nfrus < PTABLE_SIZE) 
		percent = percent_table[pct_index][nfrus];
	else
		percent = percent_table[pct_index][PTABLE_SIZE];

	i = 0;
	while (i < nfrus) {
		event->frus[i].conf = *(percent + i);
		i++;
	}

	while (i < MAXFRUS) { 
		event->frus[i].conf = 0;
		i++;
	}

	return;
}

/**
 * get_register_data
 *
 * 	Look in the product specific area of the CHRP error log for the
 *	register data. The format of the vendor specific area is:
 *		"IBM" NULL location_code_string NULL register_data
 *	where register_data begins on a 4 byte boundary and is (any number of):
 *		length (2 bytes, which includes the length) id (2 bytes) 
 *		and register (length - 4).
 *	The register_data is terminated by length = 2, i.e. no id and no data.
 *
 * RETURNS:
 *	NULL if register for id is not found
 *	Otherwise an integer pointer to the data portion (not the length
 *	or id) of the register data, and rlen is set to the length of the
 *	data returned.
 */
int *
get_register_data(struct event *event, int rid, int *rlen)
{
	char *p, *q;
	char *end;
	int length_id;
	int length;
	int id;
	int *rdata;

	*rlen = 0;
	rdata = NULL;

	end = event->event_buf + event->length;

	/* Find start of register data fields
	 * look for location code id and string 
	 */
	if ((p = strstr(&event->event_buf[I_IBM], "IBM")) != NULL) {
		q = p + strlen(p) + 1;	/* step over "IBM" */
		/*
		 * The location codes are padded to end on 4 byte
		 * boundary. So look for NULL as last byte of 4 bytes.
		 */
		while ((*(int *)q & 0xFF && q < end)) 
			q += 4;			

		p = q + 4;	/* points to 1st register data field */
		/* Make sure pointing to meaningful data */
		if (p >= end)
			return NULL;
	} 
	else
		/* There is no register data */
		return NULL;	
	
	/* Look at each register data field until 
	 * the requested id is found.  
	 * */
	do {
		length_id = *(int *)p; /* 2 bytes length, 2 bytes id */
		length = length_id >> 16;
		
		dbg("found register data in log, length = 0x%x", length);

		/* Make sure pointing can reach meaningful data */
		if (length <= 0)
			return NULL;
		
		id = length_id & 0xFFFF;
		dbg("id = 0x%x", id);
		
		p += sizeof(int);	/* next 4 bytes after length_id */ 
		length -= 2;		/* subtract the length of the length */
		if (length > 0) {		/* is there data to inspect? */ 
			length -= 2;	/* subtract the length of the id */
			if (id == rid) {
				/* This is the requested register */
				*rlen = length;
				rdata = (int *)p;
				break;	/* found, so out of loop */
			} 
			else {
				/* Not it, so ignore this register data */
				p = p + length;
			}
		}
		/* nothing found, and going beyond the data */
		if (p >= end)
			return NULL;
	} while (length > 0);

	return rdata;
}

/* 
 * Data to manage the EPOW reset error log entries.
 */
struct epow_reset {
	struct epow_reset *next;
	unsigned	err_type;
	int		valid_reset;
	uint		err_sequence;
	uint		err_id;
};

static struct epow_reset *epow_reset_list = NULL;

/**
 * save_epow_reset
 *
 *	If this is Auto ELA, go ahead and search for the original EPOW this
 *	one is reseting. If not Auto ELA, save the reset for reference when
 *	the original EPOW is processed.
 *
 * RETURNS:
 *	1 if diagnostic conclusion,
 *	otherwise 0, unless an error, then -1.
 *      
 */
int 
save_epow_reset(struct event *event, int error_type)
{
	struct epow_reset *new_reset;

	new_reset = (struct epow_reset *)malloc(sizeof(*new_reset));
	if (new_reset == NULL)
		return -1;
	
	new_reset->err_type = error_type;
	new_reset->valid_reset = 1;
	new_reset->err_sequence = event->errdata.sequence;
	new_reset->err_id = event->errdata.err_id;
	
	/* Add to the front of the list */
	new_reset->next = epow_reset_list;
	epow_reset_list = new_reset;

	return 0;
}

/**
 * has_epow_reset
 *
 *	Determine if this error (the input) has been reset by an earilier
 *	EPOW reset.
 *
 * RETURNS:
 *	1 if diagnostic conclusion,
 *	otherwise 0. unless an error, then -1.
 */
int 
has_epow_reset(struct event *event, int error_type)
{
	struct epow_reset *erptr;
	uint	   corrected_seq;

	erptr = epow_reset_list;

	/* Search for an occurance of the error_type */
	while (erptr != NULL) {
		if (erptr->err_type == (error_type & 0xFFFFFF00)) {
			/* found a match */
			erptr->valid_reset = 0;	/* don't report again */
			
			/* Store the sequence number of the EPOW_SUS_CHRP.
			   This sequence number is used in the menugoal text. */
			corrected_seq = event->errdata.sequence;
			
			/* Link the menugoal with the sequence number and error
			   ID of the EPOW_RES_CHRP. */
			event->errdata.sequence = erptr->err_sequence;
			event->errdata.err_id = erptr->err_id;
			report_menugoal(event, MSGMENUG202, corrected_seq);
			return 1;
		}
		erptr = erptr->next;
	}	

	return 0;
}
/**
 * process_epow
 *
 * @brief Determine the version of the EPOW error log and process accordingly.
 *
 */
int 
process_epow(struct event *event, int error_type)
{
	int rc = 0;
	
	if ((error_type & 0xFF000000) == EPOWLOG) {
		if ((error_type & 0x0F) == 0) {
			/* This is an EPOW reset, so save it */
			/* and go to the next (older) error  */
			rc = save_epow_reset(event, error_type);
		} 
		else if ((rc = has_epow_reset(event, error_type))) {
			/* this error has been reset */
			/* continue */
		} 
		else if (event->event_buf[0] == 0x01) { 
			/* Version 1 EPOW Log */
			rc = process_v1_epow(event, error_type);
		} 
		else
			/* Version 2 EPOW Log */
			rc = process_v2_epow(event, error_type);
	}
	return rc;
}

/**
 * process_pre_v6
 * @brief Handle older (pre-v6) style events
 *
 * @param event the event to be parsed
 * @return always returns 0
 */
int 
process_pre_v6(struct event *event)
{
	int error_fmt;
	int error_type;
	char *loc;
	int cpuid;
	struct event_description_pre_v6 *e_desc;
	int fru_index;
	struct device_ela sigdev;
	struct device_ela sendev;
	int sid, sed;
	int post_error_code;
	int i;
	short *refc;
	int rlen;
	int deconfig_error = 0;
	int predictive_error = 0;
	int repair_pending = 0;
	int rc;
	int loc_mode;
	int ignore_fru;
	char *devtree = NULL;
	char target[80];

	struct rtas_event_exthdr *exthdr;

	dbg("Processing pre-v6 event");

        /* Reset for this analysis */
        if (event->loc_codes != NULL) {
                free(event->loc_codes);
                event->loc_codes = NULL;
        }

	exthdr = rtas_get_event_exthdr_scn(event->rtas_event);
	if (exthdr == NULL) {
		log_msg(event, "Could not retrieve extended event data");
		return 0;
	}

	/* create and populate an entry to be logged to servicelog */
	event->sl_entry = (struct sl_event *)malloc(sizeof(struct sl_event));
	memset(event->sl_entry, 0, sizeof(struct sl_event));
	event->sl_entry->time_event = get_event_date(event);
	event->sl_entry->type = SL_TYPE_BASIC;
	event->sl_entry->severity = servicelog_sev(event->rtas_hdr->severity);
	event->sl_entry->serviceable = 1;

	exthdr = rtas_get_event_exthdr_scn(event->rtas_event);
	if (exthdr) {
		if (exthdr->predictive)
			event->sl_entry->predictive = 1;

		if (exthdr->recoverable)
			event->sl_entry->disposition = SL_DISP_RECOVERABLE;
		else if (exthdr->unrecoverable_bypassed)
			event->sl_entry->disposition = SL_DISP_BYPASSED;
		else if (exthdr->unrecoverable)
			event->sl_entry->disposition = SL_DISP_UNRECOVERABLE;
	}

	event->sl_entry->raw_data_len = event->length;
	event->sl_entry->raw_data = (unsigned char *)malloc(event->length);
	memcpy(event->sl_entry->raw_data, event->event_buf, event->length);

	deconfig_error = exthdr->unrecoverable_bypassed;
	predictive_error = exthdr->predictive;

	dbg("CHRP log data: recovered_error = 0x%x, deconfigured_error = 0x%x\n"
	    "predictive_error = 0x%x", exthdr->recoverable, deconfig_error,
	    predictive_error);

	/*
	 * Look for the "IBM" vendor tag at end of chrp error log. 
	 * Copy location code string into re->loc_codes.        
	 */
	if ((loc = strstr((char *)&event->event_buf[I_IBM], "IBM")) != NULL) {
		/* loc code is a null terminated string beginning at loc + 4 */
		if (strlen(loc + 4)) {
			event->loc_codes = (char *)malloc(strlen(loc+4)+1);
			if (event->loc_codes == NULL)
				return 0;

			strcpy(event->loc_codes, (loc + 4));
		}
	}

	/*
 	 * First, see if there is any SRC data with the "02" id. 
	 * If so, create an error description from the ref. codes. 
	 */
	refc = (short *)get_register_data(event, SRC_REG_ID_02, &rlen);
	if (refc != NULL) {
		process_refcodes(event, refc, rlen);
		return 0;
	}

	/* Otherwise process the sympton bits in the CHRP error log. */
	io_error_type = 0;
	error_fmt = get_error_fmt(event);
	error_type = get_error_type(event, error_fmt);

	dbg("CHRP log data: error_fmt = 0x%x, error_type = 0x%x, \n"
	    "io_error_type = 0x%x", error_fmt, error_type, io_error_type);

	/*
	 * Any predictive, unrecoverable error-degraded mode error will be
	 * reported in V3 style SRN, regardless	of error log format, so
	 * check for it here.
	 */   
	if (deconfig_error && predictive_error) {
		/* 
		 * Make sure the FRUs in this log have not already been
		 * deconfigured by the platform. Search the device tree for
		 * deconfigured status for all FRUs listed in this error log.
		 * Should only be one FRU, but will handle multiple FRUs. Only
		 * if all FRUs are deconfigured, is this error log ignored.
		 */
		loc_mode = FIRST_LOC; 
		ignore_fru = FALSE; 
		while ((loc = get_loc_code(event, loc_mode, NULL)) != NULL) {
			loc_mode = NEXT_LOC; 
			if (devtree == NULL)
				devtree = get_dt_status(NULL); 
			if (devtree) {
				sprintf(target, "%s fail-offline", loc); 
				if (strstr(devtree, target)) {
			    		/* found FRU and status in devtree */
					/* can ignore this fru */
					ignore_fru = TRUE;
					continue;
				} 
				else {
					/* fru not disabled */
					/* can not ignore this error log */
					ignore_fru = FALSE;
					break;
				}
			}
		}
		if (ignore_fru == TRUE)
			return 0;

		/* FRUs have not been disabled by the platform */
		v3_errdscr[0].sn = SN_V3ELA + error_fmt;
                v3_errdscr[0].sn |= 0x010; /* encode the predictive/deferred bit */
		v3_errdscr[0].rcode = 0x500;
		v3_errdscr[0].rmsg = PREDICT_UNRECOV;

		if (error_fmt == RTAS_EXTHDR_FMT_CPU) { 
			/* check if the "repair pending an IPL" bit is set */
			repair_pending = event->event_buf[I_BYTE24] & 0x80; 
			if (repair_pending) {
				v3_errdscr[0].rcode = 0x560;
				v3_errdscr[0].rmsg = PREDICT_REPAIR_PENDING;
			}
		}

		report_srn(event, 0, v3_errdscr);
		return 0;
	} 
	else if (deconfig_error && error_fmt == RTAS_EXTHDR_FMT_IBM_SP 
		   && event->loc_codes != NULL) {
		/* 
		 * An error was found and the resource has been 
		 * deconfigured to bypass the error.
		 */
		report_srn(event, 0, &bypass_errdscr[0]);
		return 0;
	}

	if ( ! process_v3_logs(event, error_type)  &&	/* v3 and beyond */
	     ! process_epow(event, error_type) &&	/* v1 and v2 EPOWs */
	     ! process_v2_sp(event, error_type)) {

		if (predictive_error) 
			fru_index = 1;
		else
			fru_index = 0;

	    /* all POST and version 1 non-EPOW error logs */
	    switch (error_type) {
	    	case CPUB12b0: 	/* CPU internal error */
			cpuid = event->event_buf[I_CPU];
			e_desc = &cpu610[fru_index];
			add_cpu_id(e_desc, 0, cpuid);
			report_srn(event, 0, e_desc);
			break;

		case CPUB12b1: 	/* CPU internal cache error */
			cpuid = event->event_buf[I_CPU];
			e_desc = &cpu611[fru_index];
			add_cpu_id(e_desc, 0, cpuid);
			report_srn(event, 0, e_desc);
			break;

		case CPUB12b2: 	/* L2 cache parity or multi-bit ECC error */
			e_desc = &cpu612[fru_index];
			report_srn(event, 0, e_desc);
			break;

		case CPUB12b3: 	/* L2 cache ECC single-bit error */
			report_srn(event, 0, &cpu613[fru_index]);
			break;

	    	case CPUB12b4: 	/* TO error waiting for memory controller */
			report_srn(event, 0, &cpu614[0]);
			break;

	    	case CPUB12b5: 	/* Time-out error waiting for I/O */
			report_srn(event, 0, &cpu615[0]);
			break;

	    	case CPUB12b6: 	/* Address/Data parity error on Processor bus */
		case CPUB12b7: 	/* Transfer error on Processor bus */
			/*
			 * The RTAS frus can be planar,cpu,[cpu] in any order.
			 * The err descriptions require planar,cpu,[cpu], so
			 * parse location codes to determine which error
			 * descriptions then arrange RTAS data to fit
			 * error description.
			 */
			rc = get_cpu_frus(event);
			if (rc == RC_PLANAR) {
				if (error_type == CPUB12b6) 
					e_desc = &cpu710[0];
				else    
					e_desc = &cpu713[0];
				report_srn(event, 0, e_desc);
			} 
			else if (rc == RC_PLANAR_CPU) {
				if (error_type == CPUB12b6) 
					e_desc = &cpu711[0];
				else    
					e_desc = &cpu714[0];
				report_srn(event, 0, e_desc);
			} 
			else if (rc == RC_PLANAR_2CPU) {
				if (error_type == CPUB12b6) 
					e_desc = &cpu712[0];
				else    
					e_desc = &cpu715[0];
				report_srn(event, 0, e_desc);
			} 
			else {
				report_srn(event, 0, &cpu619[0]);
			}
			break;

	    	case CPUALLZERO:/* Unknown error detected by CPU Error log */ 
		case MEMALLZERO:/* Unknown error detected by mem. controller */
		case IOALLZERO: /* Unknown error detected by I/O device */
			if (error_type == CPUALLZERO) 
				e_desc = &cpu619[0];
			else if (error_type == MEMALLZERO) 
				e_desc = &mem629[0];
			else
				e_desc = &io639[0];

			report_srn(event, 0, e_desc);
			
			break;

	    	case MEMB12b0: 	/* Uncorrectable Memory Error */ 
			/* log didn't contain dimm element # */
			/* or dimm type is unknown, then */
			/* check memory status  */ 
			report_srn(event, 0, &memtest600[0]);
			break;

		case MEMB12b1: 	/* ECC correctable error */
	    	case MEMB12b2: 	/* Correctable error threshold exceeded */
			pct_index = 1;
			/*
			 * log didn't contain dimm element # or dimm type is
			 * unknown, then check memory status
			 */ 
			report_srn(event, 0, &memtest600[0]);

			pct_index = 0;
			break;

	    	case MEMB12b3: 	/* Memory Controller internal error */
			report_srn(event, 0, &mem624[0]);
			break;

	    	case MEMB12b4: 	/* Memory Address (Bad addr going to memory) */
			report_srn(event, 0, &mem625[0]);
			break;

	    	case MEMB12b4B13b3: /* I/O Bridge T/O and Bad Memory Address */
			report_menugoal(event, MSGMENUG174); 
			return 0;
			break;

	    	case MEMB12b5: 	/* Memory Data    (Bad data going to memory) */
			report_srn(event, 0, &mem626[0]);
			break;

	    	case MEMB12b6: 	/* Memory bus/switch internal error */
				/* memory bus/switch not yet implemented. */
			break;

	    	case MEMB12b7: 	/* Memory time-out error */
			report_srn(event, 0, &mem627[0]);
			break;

	    	case MEMB13b0: 	/* Processor Bus parity error */
			add_cpu_id(&mem722[0], 0, 0);
				/*
				 * calling out processor, but don't know which
				 * one, so default to proc0 and let location 
				 * code point out failing proc
				 */
			report_srn(event, 0, &mem722[0]);
			break;

	    	case MEMB13b1: 	/* Processor time-out error */
			add_cpu_id(&mem628[0], 0, 0);
				/*
				 * calling out processor, but don't know which
				 * one, so default to proc0 and let location 
				 * code point out failing proc
				 */
			report_srn(event, 0, &mem628[0]);
			break;

	    	case MEMB13b2: 	/* Processor bus Transfer error */
			add_cpu_id(&mem723[0], 0, 0);
				/*
				 * calling out processor, but don't know which
				 * one, so default to proc0 and let location 
				 * code point out failing proc
				 */
			report_srn(event, 0, &mem723[0]);
			break;

	    	case MEMB13b3: 	/* I/O Host Bridge time-out error */
			report_srn(event, 0, &mem724[0]);
			break;

	    	case MEMB13b4: 	/* I/O Host Bridge address/data parity error */
			report_srn(event, 0, &mem725[0]);
			break;

		case IOB12b0: 	/* I/O Bus Address Parity Error */ 
		case IOB12b1: 	/* I/O Bus Data Parity Error */ 
		case IOB12b2: 	/* I/O Time-out error */ 
			/* Version 1 and 2 look the same here. */
			analyze_io_bus_error(event, 1, error_type);
			break;
 
		case IOB12b3: 	/* I/O Device Internal Error */
			if (io_error_type == IOB12b3B13b2) {
			    /* Int Err, bridge conn. to i/o exp. bus */
			    report_srn(event, 0, &io634[fru_index]);
			    break; /* out of switch statement */
			}

			e_desc = &io832[0];

			sid = 0;
			sed = 0;
			
			if (sid == 0) {
				e_desc->rcode = sigdev.led;
			} 
			else if (sed == 0) {
				e_desc->rcode = sendev.led;
			} 
			else {
				/* isolated only to pci bus */
				e_desc->rcode = 0x2C9;
			}
			report_srn(event, 0, e_desc);
			break;
				
		case IOB12b4: 	/* I/O Error on non-PCI bus */
			report_srn(event, 0, &io730[0]);
			break;

		case IOB12b5: 	/* Mezzanine/Processor Bus Addr. Parity Error */
			if (io_error_type == IOB12b5B13b1) { 
			    /* Mezzanine/Proc. Bus Addr. Parity Error */
			    report_srn(event, 0, &io733[fru_index]);
			    break;
			}
			if (io_error_type == IOB12b5B13b2) { 
			    /* Mezzanine/Proc. Bus Addr. Parity Error */
			    report_srn(event, 0, &io770[fru_index]);
			    break;
			}
			/* Default for IOB12b5, including IOB12b5B13b0. */
			report_srn(event, 0, &io731[fru_index]);
			break;


		case IOB12b6: 	/* Mezzanine/Processor Bus Data Parity Error */
			if (io_error_type == IOB12b6B13b1) {  
			    /* Mezzanine/Proc. Bus Data Parity Error */
			    report_srn(event, 0, &io734[fru_index]);
			    break;
			}
			if (io_error_type == IOB12b6B13b2) { 
			    /* Mezzanine/Proc. Bus Addr. Parity Error */
			    report_srn(event, 0, &io771[fru_index]);
			    break;
			}
			if (io_error_type == IOB12b6B13b3) { 
			    /* Err detected by i/o exp. bus controller */
			    report_srn(event, 0, &io773[fru_index]);
			    break;
			}
			/* Default for IOB12b6, including IOB12b6B13b0. */
			report_srn(event, 0, &io732[fru_index]);
			break;

		case IOB12b7: 	/* Mezzanine/Processor Bus Time-out Error */
			if (io_error_type == IOB12b7B13b1)  {
		            /* Mezzanine/Processor Bus Time-out Error */
			    report_srn(event, 0, &io736[fru_index]);
			    break;
			}
			if (io_error_type == IOB12b7B13b2) {
			    /* Mezzanine/Proc. Bus Addr. Parity Error */
			    report_srn(event, 0, &io772[fru_index]);
			    break;
			}
			/* Default for IOB12b7, including IOB12b7B13b0. */
			report_srn(event, 0, &io735[fru_index]);
			break;

		case IOB13b4: /* I/O Expansion Bus Parity Error */
			report_srn(event, 0, &io630[fru_index]);
			break;

		case IOB13b5: /* I/O Expansion Bus Time-out Error */
			report_srn(event, 0, &io631[fru_index]);
			break;

		case IOB13b6: /* I/O Expansion Bus Connection Failure */
			report_srn(event, 0, &io632[fru_index]);
			break;

		case IOB13b7: /* I/O Expansion Bus Connection Failure */
			report_srn(event, 0, &io633[fru_index]);
			break;

		case SPB16b0: 	/* T.O. communication response from SP */ 
			report_srn(event, 0, &sp740[0]);
			break;

		case SPB16b1: 	/* I/O (I2C) general bus error */
			report_srn(event, 0, &sp640[0]);
			break;

		case SPB16b2: 	/* Secondary I/O (I2C) general bus error */
			report_srn(event, 0, &sp641[0]);
			break;

		case SPB16b3: 	/* Internal Service Processor memory error */
			report_srn(event, 0, &sp642[0]);
			break;

		case SPB16b4: 	/* SP error accessing special registers */
			report_srn(event, 0, &sp741[0]);
			break;

		case SPB16b5: 	/* SP reports unknown communication error */
			report_srn(event, 0, &sp742[0]);
			break;

		case SPB16b6: 	/* Internal Service Processor firmware error */
			report_srn(event, 0, &sp643[0]);
			break;

		case SPB16b7: 	/* Other internal SP hardware error */
			report_srn(event, 0, &sp644[0]);
			break;

		case SPB17b0: 	/* SP error accessing VPD EEPROM */
			report_srn(event, 0, &sp743[0]);
			break;

		case SPB17b1:	/* SP error accessing Operator Panel */
			report_srn(event, 0, &sp744[0]);
			break;

		case SPB17b2: 	/* SP error accessing Power Controller */
			report_srn(event, 0, &sp745[0]);
			break;

		case SPB17b3: 	/* SP error accessing Fan Sensor */
			report_srn(event, 0, &sp746[0]);
			break;

		case SPB17b4: 	/* SP error accessing Thermal Sensor */
			report_srn(event, 0, &sp747[0]);
			break;

		case SPB17b5: 	/* SP error accessing Voltage Sensor */
			report_srn(event, 0, &sp748[0]);
			break;

		case SPB18b0: 	/* SP error accessing serial port */ 
			report_srn(event, 0, &sp749[0]);
			break;

		case SPB18b1: 	/* SP error accessing NVRAM */
			report_srn(event, 0, &sp750[0]);
			break;

		case SPB18b2: 	/* SP error accessing RTC/TOD */
			report_srn(event, 0, &sp751[0]);
			break;

		case SPB18b3: 	/* SP error accessing JTAG/COP controller */
			report_srn(event, 0, &sp752[0]);
			break;

		case SPB18b4: 	/* SP detect error with tod battery */
			report_srn(event, 0, &sp753[0]);
			break;

		case SPB18b7:	/* SP reboot system due to surv. time-out */
			report_srn(event, 0, &sp760[0]);
			break;

		case POSTALLZERO:
		case POSTB12b0: /* Firmware error, revision level %s */
		case POSTB12b1: /* Configuration error */
		case POSTB12b2: /* CPU POST error */
		case POSTB12b3: /* Memory POST error */
		case POSTB12b4: /* I/O Subsystem POST error */
		case POSTB12b5: /* Keyboard POST error */
		case POSTB12b6: /* Mouse POST error */
		case POSTB12b7: /* Graphic Adapter/Display POST error */
		case POSTB13b0: /* Diskette Initial Program Load error */
		case POSTB13b1: /* Drive Controller IPL error */
		case POSTB13b2: /* CD-ROM Initial Program Load error */
		case POSTB13b3: /* Hard disk Initial Program Load error */
		case POSTB13b4: /* Network Initial Program Load error */
		case POSTB13b5: /* Other Initial Program Load error */
		case POSTB13b7: /* Self-test error in FW extended diagnostics */
			/* All of the post error processing goes here. */
			e_desc = &post[0];

			e_desc->sn = -2; /* have to reset after insert_errdscr */
			e_desc->rcode = 0;
			post_error_code = *(int *)&event->event_buf[I_POSTCODE];

			/* Add any (upto 4) chrp location codes from the log */
			loc = get_loc_code(event, FIRST_LOC, NULL);
			i = 0;
			while (loc && i < 4) {
				strcpy(e_desc->frus[i++].floc, loc);
				loc = get_loc_code(event, NEXT_LOC, NULL);
			}

			set_fru_percentages(e_desc, i);

			if (set_srn_and_callouts(event, e_desc,
						 post_error_code))
				return -1;

			break;
	    }
	}
	return 0;
}

/**
 * analyze_io_bus_error
 * @brief Analyze the io bus errors, i.e. IOB12b0, IOB12b1, or IOB12b2. 
 */
static int 
analyze_io_bus_error(struct event *event, int version, int error_type)
{
	struct event_description_pre_v6 *e_desc;
	int fru_index;
	struct device_ela sigdev;
	struct device_ela sendev;
	int sid, sed;
	int rc = 0;

	/*
	 * if this is version 3 or beyond, and there is a physical location
	 * code in the log, then just return for SRN encoding
	 */
	if (version >= 3 && LOC_CODES_OK) 
		return 0;

	if (error_type == IOB12b0)
		fru_index = 0;
	else if (error_type == IOB12b1)
		fru_index = 1;
	else if (error_type == IOB12b2)
		fru_index = 2;
	else
		return 0;	/* can't analyze this error type */
				
	sid = 0;
	sed = 0;

	if (sid == 0 && sed == 0) {
		/* check if a device is parent (the bus) of the other device */
		if ( ! strcmp(sigdev.busname, sendev.name)||
		     ! strcmp(sendev.busname, sigdev.name)  ) {
			e_desc = &io9CC[fru_index];
			report_io_error_frus(event, SN9CC, e_desc, &sigdev, 
					     &sendev);
			rc = 1;
		} 
		else {
			/* 2 devices from error log */
			e_desc = &io833[fru_index];
			report_io_error_frus(event, 0x833, e_desc, &sigdev, 
					     &sendev);
			rc = 1;
		}
	} 
	else if (sid == 0 || sed == 0) {
		/* 1 device only */
		e_desc = &io9CC[fru_index];
		report_io_error_frus(event, SN9CC, e_desc, &sigdev, &sendev);
		rc = 1;
	} 
	else {
		/* bus  only */
		e_desc = &iobusonly[fru_index];
		report_io_error_frus(event, SN9CC, e_desc, &sigdev, &sendev);
		rc = 1;
	}

	return rc;
}


/**
 * get_error_fmt
 * @brief Extract the error log format indicator from the chrp error log.
 *
 * @return An integer that is the error log format indicator.
 */
int 
get_error_fmt(struct event *event)
{
	return (event->event_buf[I_FORMAT] & 0x0F); 
}

/**
 * get_error_type
 *
 * Uses the error format to return the error bits as an unique
 * constant for each error.
 *
 * @return The error format and error bits as an unique integer.
 */
static int 
get_error_type(struct event *event, int fmt)
{
	int error_type;

	switch (fmt) {
	    case RTAS_EXTHDR_FMT_IBM_SP:
		error_type = 	fmt << 24 | 
				event->event_buf[I_BYTE18] << 16 | 
		             	event->event_buf[I_BYTE17] << 8  | 
				event->event_buf[I_BYTE16];
		break;

	    case RTAS_EXTHDR_FMT_EPOW:
		error_type = 	fmt << 24 | 
				event->event_buf[I_BYTE17] << 16 | 
		             	event->event_buf[I_BYTE16] << 8  | 
				(event->event_buf[I_BYTE15] & 0x0F);
		break;

	    case RTAS_EXTHDR_FMT_CPU:
		error_type = 	fmt << 24 | event->event_buf[I_BYTE12];
		break;
	
	    case RTAS_EXTHDR_FMT_IO: 
		/*
		 * First, get the complete io error type, which 
		 * includes some info bits. 
		 */
		io_error_type = fmt << 24 |
				event->event_buf[I_BYTE13] << 8 |
				event->event_buf[I_BYTE12];
		/* But, return just the sympton bits. */
		error_type = io_error_type & IO_BRIDGE_MASK;
		break;

	    default:
		error_type = fmt << 24 |
				event->event_buf[I_BYTE13] << 8 |
				event->event_buf[I_BYTE12];
	}

	return error_type;
}


/**
 * report_srn
 */
int
report_srn(struct event *event, int nlocs,
	   struct event_description_pre_v6 *e_desc)
{
	int i;
	char *loc;
	short save_sn;
	int numLocCodes;

	/* used in case there are more FRUs than MAXFRUS */
        struct event_description_pre_v6 *temp_event;

	save_sn = e_desc->sn;		/* save before insert_errdscr */

	/* Add the chrp location codes if nlocs != 0 */
	for (i = 0; i < nlocs; i++) {
		if (i == 0) 
			loc = get_loc_code(event, FIRST_LOC, NULL); 
		else 
			loc = get_loc_code(event, NEXT_LOC, NULL); 
		
		if (loc) 
			strcpy(e_desc->frus[i].floc, loc);
		else
			break;
	}

	if (nlocs == 0) {
		/* 
		 * Build a error description that contains a fru for 
		 * each of the physical location codes.
		 */
		loc = get_loc_code(event, FIRST_LOC, &numLocCodes);
		i = 0;
		while (i < MAXFRUS) {
			if (loc != NULL) {
				strcpy(e_desc->frus[i].floc, loc);
				loc = get_loc_code(event, NEXT_LOC, NULL);
			}
			e_desc->frus[i].fmsg = 36;
			i++;
		}
		set_fru_percentages(e_desc, numLocCodes);
	}

	e_desc->sn = save_sn;

	if (set_srn_and_callouts(event, e_desc, 0)) 
		return -1;

        /* if loc is not null, then there are more FRUs than MAXFRUS */
        while (loc != NULL) {
		i = 0;
		numLocCodes = numLocCodes - MAXFRUS;

		/* allocate space for more FRUs */
		temp_event = (struct event_description_pre_v6 *)
			     calloc(1, sizeof(struct event_description_pre_v6));

		/* copy everything over from the original event */
		strcpy(temp_event->dname, e_desc->dname);
		temp_event->flags = e_desc->flags;
		temp_event->sn = e_desc->sn;
		temp_event->rcode = e_desc->rcode;
		temp_event->rmsg = e_desc->rmsg;

		/* Prepare to add more FRUs */
		while (i < MAXFRUS) {
			if (loc != NULL) {
				strcpy(temp_event->frus[i].floc, loc);
				loc = get_loc_code(event, NEXT_LOC, NULL);
			}
 
			temp_event->frus[i].fmsg = 36;
			i++;
		}

		set_fru_percentages(temp_event, numLocCodes);
		temp_event->sn = save_sn;

		/* Log additional error descriptions */
		if (add_more_descrs(event, temp_event))
			return -1;

		free(temp_event);
        }

	event->sl_entry->call_home_status = SL_CALLHOME_CANDIDATE;

	event->sl_entry->description = (char *)malloc(strlen(e_desc->rmsg)+1);
	strcpy(event->sl_entry->description, e_desc->rmsg);

	dbg("srn: \"%s\"", event->sl_entry->refcode);

	return 0;
}

/**
 * get_loc_code
 *
 *	Gets a physical location codes from the chrp error log. This is 
 *	called to get either the first location code or the next location 
 *	code. When called to get the first location code, the number of 
 *	location codes present in the log is returned.
 *
 *	The string of location codes is null terminated. However a special
 *	character, ">", means that the location codes that follow should not
 *	be reported to the user via diagnostics. Therefore, processing of 
 *	location codes will stop at the first occurance of ">" or a null 
 *	terminator.
 *
 * RETURNS:
 *	If nlocs is not NULL, returns the count of location codes in the buffer.
 *	NULL if location code not found. 
 *	Otherwise a pointer to location code string.
 *
 */
char *
get_loc_code(struct event *event, int mode, int *nlocs)
{ 
	char *loc;
	int prev_space = 0;
	static char *copyLCB = NULL;
	static char *save_copyLCB = NULL;
	static char *start_loc = NULL;	/* start or next pointer for loc code */
	static char *end_loc = NULL;	/* end of location code buffer */

	if (! LOC_CODES_OK) { 
		if (nlocs)
			*nlocs = 0;
		return NULL;
	}

	if (mode == FIRST_LOC) {
		if (save_copyLCB) {
			/* throw away the previous allocation of buffer */
			/* even though not used up by get next calls */
			free(save_copyLCB);
			copyLCB = NULL;
		}

		copyLCB = (char *)malloc(strlen(event->loc_codes) + 1);
		if (copyLCB == NULL)
			return NULL;

		save_copyLCB = copyLCB;
		strcpy(copyLCB, event->loc_codes);
		start_loc = copyLCB;

		/* Force processing to stop at special "Hide" character */
		loc = strchr(copyLCB, LOC_HIDE_CHAR);
		if (loc != NULL) 
			*loc = 0;
		end_loc = copyLCB + strlen(copyLCB);

		/*
		 * The number of location codes is determined by the number
		 * of (non-consecutive) spaces in the log, i.e.
		 * 1 space = 2 location codes
		 */
		if (nlocs != NULL) {	
			loc = copyLCB;
			if (*loc) {
				*nlocs = 0;     /* nothing found until !space */
				prev_space = 1; /* handle leading spaces */
				while (*loc) {
					if (*loc == ' ') {
						if (! prev_space)
							*nlocs = *nlocs + 1;
						prev_space = 1;
					} 
					else
						prev_space = 0;
					loc++;
				}
				if (! prev_space)
					/* count last location code */
					*nlocs = *nlocs + 1;
			} 
			else
				*nlocs = 0;	/* null means no loc. codes */
		}
	}

	loc = start_loc;

	if (loc < end_loc && *loc != 0x0) {
		/* loc is the start of a location code */
		/* use start_loc to find end of location code string */
		/* find 1st occurance of null or blank, the loc. code delimit */
		while (*loc == ' ') 
			loc++;	/* skip leading blanks */

		start_loc = loc;
		while (*start_loc && *start_loc != ' ') 
			start_loc++;

		*start_loc = 0x0;		/* terminate loc string */
		start_loc++;			/* ready for next call */
		return loc;			/* return loc */
	} 
	else {
		return NULL;
	}
}

/**
 * get_ext_epow
 * @brief Get the extended epow status bytes from the chrp log. 
 *
 * The extended epow data is 2 bytes stored as the MSB of an integer.
 *
 * @return the extended epow register from the vendor specific area.
 */
int 
get_ext_epow(struct event *event)
{
	int *extepow;
	int extepowlen;

	extepow = get_register_data(event, EXT_EPOW_REG_ID, &extepowlen);
	if (extepow == NULL)
		return 0;

	/* I have 0xaabbccdd */
	/* I want 0x0000aabb */

	return (*extepow >> 16);
}

/**
 * get_fan_number
 */
int 
get_fan_number(int ext_epow)
{
	int fan;

	fan = ext_epow & SENSOR_MASK;
	fan = fan >> 8;

	return fan;
}

/**
 * report_menugoal
 * @brief Report a menu goal with optional variable substitution into the text.
 *
 * The second paramater must always be the menu number (as an uint).
 *
 * @returns Always returns 0
 */
int 
report_menugoal(struct event *event, char *fmt, ...)
{
	va_list argp;
	int offset;
	char buffer[MAX_MENUGOAL_SIZE], *msg, menu_num_str[20];
	int rlen;
	short *refc;
	unsigned refcode;

	buffer[0]='\0';

	va_start(argp, fmt);
	offset = sprintf(buffer, fmt, argp);
	va_end(argp);

	if (strlen(buffer)) {
		/*
		 * There is a menugoal. If this menugoal is from ELA,
		 * check if a ref. code exists in the error log.
		 */
		if (event->errdata.sequence) {
			refc = (short *)get_register_data(event, SRC_REG_ID_04, 
							  &rlen);
			if (refc) {
				int msglen;
				/* There is a refcode; append to the menugoal */
				refcode = *(uint *)refc;
				msglen = strlen(MSGMENUG_REFC) + 
					2 * sizeof(refcode) + 1;

				if (strlen(buffer) + msglen 
						< MAX_MENUGOAL_SIZE) {
					offset += sprintf(buffer + offset, 
							  MSGMENUG_REFC,
							  refcode);
				}

				/*
				 * Check if there are location codes 
				 * supporting the refcode
				 */
                                if (LOC_CODES_OK) {
					/*
					 * append to menugoal as
					 * supporting data
					 */
					msglen = strlen(MSGMENUG_LOC) +
						strlen(event->loc_codes) + 1;

					if (strlen(buffer) + msglen 
							< MAX_MENUGOAL_SIZE) { 
						offset += sprintf(
							buffer + offset, 
							MSGMENUG_LOC, 
							event->loc_codes);
					}
				}
			}
		}

		snprintf(menu_num_str, 20, "#%d", atoi(buffer));
		event->sl_entry->refcode = (char *)malloc(
						strlen(menu_num_str)+1);
		strcpy(event->sl_entry->refcode, menu_num_str);

		msg = strchr(buffer, ' ') + 1;
		event->sl_entry->description = (char *)malloc(strlen(msg)+1);
		strcpy(event->sl_entry->description, msg);

		dbg("menugoal: number = %s, message = \"%s\"", menu_num_str,
		    msg);
	}

	return 0;
}

/**
 * report_io_error_frus
 *
 * 	Build error description for i/o detected errors. The number of frus is
 *	determined by the number of devices returned in the error log.
 */
int 
report_io_error_frus(struct event *event, int sn,
		     struct event_description_pre_v6 *e_desc, 
		     struct device_ela *sig, struct device_ela *sen)
{ 
	char *loc;
	int i;
	char *p;
	int swap_locs = 0;

	if (sig->status == DEVICE_OK && sen->status == DEVICE_OK) {
		/* 2 devices returned from error log */
		/* if one is non integrated, use it for the srn */
		/* otherwise if one is not a bridge device use if the for srn */
		if (sen->slot != 0) {
			e_desc->rcode = sen->led;
			strcpy(e_desc->frus[0].fname, sen->name);
			strcpy(e_desc->frus[1].fname, sig->name);
			if (sn == 0x833)
				strcpy(e_desc->frus[2].fname, sen->busname);
		} else if (sig->slot != 0) {
			e_desc->rcode = sig->led;
			strcpy(e_desc->frus[0].fname, sig->name);
			strcpy(e_desc->frus[1].fname, sen->name);
			if (sn == 0x833)
				strcpy(e_desc->frus[2].fname, sig->busname);
			swap_locs = 1;	/* f/w list the sender 1st in log */
		} else if (sen->devfunc != 0 || sig->devfunc == 0) {
			e_desc->rcode = sen->led;
			strcpy(e_desc->frus[0].fname, sen->name);
			strcpy(e_desc->frus[1].fname, sig->name);
			if (sn == 0x833)
				strcpy(e_desc->frus[2].fname, sen->busname);
		} else {
			e_desc->rcode = sig->led;
			strcpy(e_desc->frus[0].fname, sig->name);
			strcpy(e_desc->frus[1].fname, sen->name);
			if (sn == 0x833)
				strcpy(e_desc->frus[2].fname, sig->busname);
			swap_locs = 1;	/* f/w list the sender 1st in log */
		}
	} else if (sig->status == DEVICE_OK) {
		/* 1 device returned from error log */
		e_desc->rcode = sig->led;
		strcpy(e_desc->frus[0].fname, sig->name);
		strcpy(e_desc->frus[1].fname, sig->busname);
		swap_locs = 1;	/* f/w list the sender 1st in log */
	} else if (sen->status == DEVICE_OK) {
		/* 1 device returned from error log */
		e_desc->rcode = sen->led;
		strcpy(e_desc->frus[0].fname, sen->name);
		strcpy(e_desc->frus[1].fname, sen->busname);
	} else {
		if (sig->status == DEVICE_BUS) { 
			strcpy(e_desc->frus[0].fname, sig->busname);
			p = sig->busname;
			swap_locs = 1;	/* f/w list the sender 1st in log */
		} else {
			strcpy(e_desc->frus[0].fname, sen->busname);
			p = sen->busname;
		}

		/* Convert number in busname, i.e. pcixx, */
		/* where xx is 0 to 99, */
		/* to integer 1xx for the reason code. */
		while (! isdigit (*p) && *p != 0) 
			p++;

		e_desc->rcode = atoi(p) + 0x100;
	}

	/*
	 * Add any chrp location code from the log
	 */
	loc = get_loc_code(event, FIRST_LOC, NULL);
	i = 0;
	while (loc && i < 2) {	/* this error description is for 2 frus */ 
		/* 
		 * For the case when the signaling device is non-integrated,
		 * the location codes must be swapped. 
		 */
		if (swap_locs) {
			if (i == 0) {	
				/* 1st loc code goes in 2nd fru */
				/* but also puting loc in 1st fru */
				/* in case  there is only 1 loc. */
				strcpy(e_desc->frus[0].floc, loc);
				strcpy(e_desc->frus[1].floc, loc);
			} else if ( i == 1 ) { 
				/* 2nd loc code goes in 1st fru */
				strcpy(e_desc->frus[0].floc, loc);
			}
		} else {
			/* Here if not swap_locs */
			strcpy(e_desc->frus[i].floc, loc);
		}
		i++;
		loc = get_loc_code(event, NEXT_LOC, NULL);
	}
	if (i)
		set_fru_percentages(e_desc, i);

	if (set_srn_and_callouts(event, e_desc, 0))
		return -1;

	return 0;
}

/* Its a planar if P and (not - and not /) */
#define is_planar(x)	(x != NULL && strchr(x, 'P') && (!strchr(x,'-') && !strchr( x,'/'))) 

/* Its a CPU if 'C' */
#define is_cpu(x)	(x != NULL && strchr(x, 'C'))

#define is_not_fru(x)	(x == NULL || strlen(x) == 0)

/**
 * get_cpu_frus
 *
 * Gets the RTAS generated FRUs from a CPU detected error log
 * and arranges the frus to match the awaiting error description.
 * This should be called for CPUB12Bb6 and CPUB12b7 errors.
 *
 * OUTPUTS:	The location code buffer is rearranged if necessary.
 *
 * RETURNS:	RC_INVALID -	if invalid or unknown combination of frus
 *		RC_PLANAR  -	if only a planar fru is found
 *		RC_PLANAR_CPU -	if a planar and a CPU fru are found
 *		RC_PLANAR_2CPU -if a planar and 2 CPUs frus are found
 */
static int
get_cpu_frus(struct event *event)
{
	char *buf;
	char *loc1 = NULL, *loc2 = NULL, *loc3 = NULL;
	int nlocs = 0;
	int rc = RC_INVALID;

	buf = (char *)malloc(strlen(event->loc_codes)+4);
	if (buf == NULL)
		return 0;

	loc1 = buf;

	strcpy(loc1, get_loc_code(event, FIRST_LOC, &nlocs));

	if (nlocs < 1) {
		free(buf);
		return rc;
	}

	if (nlocs > 1) {
		loc2 = loc1 + strlen(loc1) + 1;
		strcpy(loc2, get_loc_code(event, NEXT_LOC, NULL));
	}

	if ( nlocs > 2 ) {
		loc3 = loc2 + strlen(loc2) + 1;
		strcpy(loc3, get_loc_code(event, NEXT_LOC, NULL));
	}

	if (is_planar(loc1)) {
		if (is_not_fru(loc2))
			rc = RC_PLANAR;
		else if (is_cpu(loc2)) {
			if (is_not_fru(loc3))
				rc = RC_PLANAR_CPU;
			else if (is_cpu(loc3))
				rc = RC_PLANAR_2CPU;
		}
	}
	else if (is_cpu(loc1)) {
		if (is_planar(loc2)) {
			if (is_not_fru(loc3)) {
				/* Rearrange locs as loc2, loc1 */
				if (event->loc_codes != NULL)
					free(event->loc_codes);
				event->loc_codes = (char *)
					malloc(strlen(loc1) +
						     strlen(loc2) + 2);
				if (event->loc_codes == NULL)
					return 0;

				sprintf(event->loc_codes, "%s %s", loc2, loc1);
				rc = RC_PLANAR_CPU;
			} 
			else if (is_cpu(loc3)) {
				/* Rearrange loc as loc2, loc1, loc3 */
				if (event->loc_codes != NULL)
					free(event->loc_codes);
				event->loc_codes = (char *)
					malloc(strlen(loc1) + 
						     strlen(loc2) +
						     strlen(loc3) + 3);
				if (event->loc_codes == NULL)
					return 0;

				sprintf(event->loc_codes, "%s %s %s", 
					loc2, loc1, loc3);
				rc = RC_PLANAR_2CPU;
			} 
		}
	}

	free(buf);
	return rc;
}

/**
 * process_v1_epow
 * @brief Analyze the Version 1 EPOW error log.
 *
 * INPUTS:
 *      The encoded error type.
 *
 * RETURNS:
 *	1 if diagnostic conclusion,
 *	otherwise 0.
 */
int 
process_v1_epow(struct event *event, int error_type) 
{
	int rc = 1;
	int class;
	int ext_epow;
	int ext_epow_nofan;
	int fan_num;

	/*
	 * The following EPOW error logs found in CHRP, Ver. 1
	 * error logs.
	 */
	class = error_type & 0xFF;

	ext_epow = get_ext_epow(event);
	ext_epow_nofan = ext_epow & IGNORE_SENSOR_MASK;

	if (ext_epow == 0) {
		if (class == 5) {
			/* System shutdown due loss of AC power */
			report_srn(event, 0, epow812);
		} else {
			/* This error log is unknow to ELA */
			unknown_epow_ela(event, class);
			rc = -1;
		}
	}
	else switch (ext_epow_nofan) {
		case XEPOW1n11: /* Fan turning slow */
			fan_num = get_fan_number(ext_epow);
			report_srn(event, 0, epow800);
			break;

		case XEPOW1n64: /* Fan stop was detected */
			fan_num = get_fan_number(ext_epow);
			report_srn(event, 0, epow801);
			break;

		case XEPOW2n32: /* Over voltage condition */
			report_srn(event, 0, epow810);
			break;

		case XEPOW2n52: /* Under voltage condition */
			report_srn(event, 0, epow811);
			break;

		case XEPOW3n21: /* Over temperature condition */
			report_srn(event, 0, epow820);
			break;

		case XEPOW3n73: /* Shutdown over max temp. */
			report_srn(event, 0, epow821);
			break;

		default: 	/* This error log is unknow to ELA */
			unknown_epow_ela(event, class);
			rc = -1;
	}

	return rc;
}

/**
 * loss_power_MG
 *
 *      Displays the menu goal used for a Loss of Power EPOW.
 *	Used by both V2 and V3 EPOW ELA.
 */
void
loss_power_MG(struct event *event)
{
	if (LOC_CODES_OK)
		report_menugoal(event, MSGMENUG150, event->loc_codes);
	else
		report_menugoal(event, MSGMENUG151);
}

/**
 * manual_power_off_MG
 *
 *      Displays the menu goal used for Manual activation of power off button.
 *	Used by both V2 and V3 EPOW ELA.
 */
void
manual_power_off_MG(struct event *event)
{
	/* need the timestamp from the error log */
	/* need the error log sequence number */
	/* and report this as a menugoal */
	report_menugoal(event, MSGMENUG162, event->errdata.time_stamp,
			event->errdata.sequence);
}

/**
 * process_v2_epow
 * @brief Analyze the Version 2 and Version 3 EPOW error logs.
 *
 * @returns 1 if diagnostic conclusion,	otherwise 0.
 */
int
process_v2_epow(struct event *event, int error_type)
{
	int rc = 1;
	int class;
	int *reg, rlen, ext_epow;

	class = error_type & 0xFF;

	/*
	 * Look for an EPOW detected by a defined sensor. 
	 */
	if (error_type & EPOWB16b0)
		rc = sensor_epow(event, error_type, 2);
	else if (error_type & EPOWB16b1) {
		/* look for power fault epow */
		/* look for extended debug data in case necessary */
		reg = get_register_data(event, PCI_EPOW_REG_ID, &rlen);
		if (reg)
			ext_epow = *reg & PCIEPOWMASK;
		if (error_type & EPOWB17b0) {
			/* unspecified cause for power fault */
			/* report SRN only if frus in error log */
			if (class < 3) {
				if (LOC_CODES_OK)
					report_srn(event, 0, epow809);
				else
					report_menugoal(event, MSGMENUG175);
			} else {
				if (LOC_CODES_OK)
					report_srn(event, 0, epow824);
				else
					report_menugoal(event, MSGMENUG175);
			}
		} else if (error_type & EPOWB17b1) {
			/* loss of power source */
			/* first checking system specific data */
			if (ext_epow == PCIEPOW111)
				/* AC power to entire system */
				report_srn(event, 0, epow813);
			else if (ext_epow == PCIEPOW100)
				/* AC power to CEC */
				report_srn(event, 0, epow814);
			else if (ext_epow == PCIEPOW011)
				/* AC power to I/O Rack */
				report_srn(event, 0, epow815);
			else
				/* here if no system specific data */
				loss_power_MG(event);
		} else if (error_type & EPOWB17b2) {
			/* internal power supply failure */
			if (ext_epow == PCIEPOW001)
				/* 3/4 ps in I/O rack */
				report_srn(event, 0, epow816);
			else if (ext_epow == PCIEPOW010)
				/* 1/4 ps in I/O rack */
				report_srn(event, 0, epow817);
			else if (error_type & EPOWB16b4) {
				report_srn(event, 0, epow819red);
			} else
				report_srn(event, 0, epow819);
		} else if (error_type & EPOWB17b3)
			/* manual power off via op panel */
			manual_power_off_MG(event);
		else {
			/* Reserved bits in EPOW log */
			/* This error log is unknow to ELA */
			unknown_epow_ela(event, class);
			rc = -1;
		}
	} else if (error_type & EPOWB16b2) {
		if (error_type & EPOWB16b4) {
			/* Loss of redundant fan */
			report_srn(event, 0, epow652820);
		} else if (error_type & EPOWB16b3) {
			/* System shutdown - thermal & fan */
			report_srn(event, 0, epow822);
		} else {
			if (class < 3)
				report_srn(event, 0, epow802);
			else
				report_srn(event, 0, epow823);
		}
	} else if (error_type & EPOWB16b3) {
		/* look for Over temperature conditions */
		/* Over temperature condition */
		report_srn(event, 0, epow821);
	} else if (error_type & EPOWB16b4) {
		/* look for loss of redundancy, without any other indication */
		/* Loss of redundant power supply */
		report_srn(event, 0, epow652810);
	} else
		/* This error log is unknown */
		unknown_epow_ela(event, class);
		rc = -1;

	return rc;
}

/**
 * sensor_epow
 *
 *      Analysis EPOW error logs that were detected by a defined sensor.
 *	The resulting error description is different depending on the error log
 *	version. Supports version 2 and 3.
 *
 * INPUTS:
 *	The encoded error type.
 *
 * @returns 1 if diagnostic conclusion,	otherwise 0.
 *
 */
int
sensor_epow(struct event *event, int error_type, int version)
{
	int class, token, index, redundancy, status;
	struct event_description_pre_v6 *e_desc;
	char sensor_name [256];
	char fru_name[256];
	int rc = 1;

	class = error_type & 0xFF;
	token = *(int*)&event->event_buf[I_TOKEN];
	index = *(int*)&event->event_buf[I_INDEX];
	redundancy = (error_type & LOGB16b4);
	status = *(int*)&event->event_buf[I_STATUS];

	dbg("EPOW class = %d, token = %d, index = %d", class, token, index);

	switch (token) {
		case FAN:
			if (version == 3) {
				e_desc = v3_errdscr;
				e_desc->rcode = 0x10;
			} else {
        			e_desc = fan_epow;
				e_desc->rcode = 0x830;
			}
                        break;

                case VOLT:
			sprintf(fru_name, "%s", F_POWER_SUPPLY);
			sprintf(sensor_name, F_EPOW_ONE, F_VOLT_SEN_2,
				index + 1);

			if (version == 3) {
				e_desc = v3_errdscr;
				e_desc->rcode = 0x30;
			} else {
                       		e_desc = volt_epow;
				strcpy(e_desc->frus[0].fname, fru_name);
				strcpy(e_desc->frus[1].fname, sensor_name);
                       		e_desc->rcode = 0x831;
			}
                        break;

                case THERM:
                       	sprintf(sensor_name, F_EPOW_ONE, F_THERM_SEN,
				index + 1);

			if (version == 3) {
				e_desc = v3_errdscr;
				e_desc->rcode = 0x50;
			} else {
                        	e_desc = therm_epow;
                        	strcpy(e_desc->frus[0].fname, sensor_name);
                        	e_desc->rcode = 0x832;
			}
			break;

                case POWER:
                       	sprintf(fru_name, F_EPOW_ONE, F_POWER_SUPPLY,
				index + 1);

			sprintf(sensor_name, "%s", F_POW_SEN);

			if (version == 3) {
				e_desc = v3_errdscr;
				e_desc->rcode = 0x70;
			} else {
                        	e_desc = pow_epow;
                        	strcpy(e_desc->frus[0].fname, fru_name);
				strcpy(e_desc->frus[1].fname, sensor_name);
       		        	e_desc->rcode = 0x833;
			}
                        break;

                default:
			if (version == 3) {
				e_desc = v3_errdscr;
				e_desc->rcode = 0x90;
			} else {
                        	e_desc = unknown_epow;
				e_desc->rcode += 0x839;
			}
                        break;
	}

	if (version != 3)
		e_desc->sn = 0x651;

	if (redundancy) {
		if (version == 3)
			e_desc->sn |= 0x010;
		else
			e_desc->sn = 0x652;

		switch (token) {
		    	case FAN:
				if (version == 3)
					e_desc->rcode = 0x110;
				e_desc->rmsg = MSGRED_FAN;
				break;

		    	case POWER:
				if (version == 3)
					e_desc->rcode = 0x120;
                        	e_desc->rmsg = MSGRED_POWER;
                        	break;

		    	default:
				if (version == 3)
					e_desc->rcode = 0x130;
                        	e_desc->rmsg = MSGRED_UNK;
                        	break;
		}
	} else {
		switch (status) {
			case CRITHI:
			case CRITLO:
				switch (token) {
					case FAN:
						e_desc->rmsg = MSGCRIT_FAN;
						if (version == 3) {
						    if (class > 2) {
							e_desc->rcode = 0x20;
							e_desc->rmsg =
							      MSGCRIT_FAN_SHUT;
						    }
						}
						break;

					case VOLT:
                                        	e_desc->rmsg = MSGCRIT_VOLT;
						if (version == 3) {
						    if (class > 2) {
							e_desc->rcode = 0x40;
							e_desc->rmsg =
							     MSGCRIT_VOLT_SHUT;
						    }
						}
						break;

					case THERM:
						e_desc->rmsg = MSGCRIT_THERM;
						if (version == 3) {
						    if (class > 2) {
							e_desc->rcode = 0x60;
							e_desc->rmsg = 
							    MSGCRIT_THERM_SHUT;
						    }
						}
						break;

					case POWER:
						e_desc->rmsg = MSGCRIT_POWER;
						if (version == 3) {
						    if (class > 2) {
							e_desc->rcode = 0x80;
							e_desc->rmsg =
							    MSGCRIT_POWER_SHUT;
						    }
						}
                                        	break;

					default:
                                        	e_desc->rmsg = MSGCRIT_UNK;
						if (version == 3) {
						    if (class > 2) {
							e_desc->rcode = 0x100;
							e_desc->rmsg =
							      MSGCRIT_UNK_SHUT;
						    }
						}
						break;
			    	}
			    	break;

			case WARNHI:
			case WARNLO:
				if (version == 3)
					e_desc->sn += 0x10;
				else
					e_desc->rcode += 0x10;

                        	switch (token) {
                                    	case FAN:
                                        	e_desc->rmsg = MSGWARN_FAN;
                                        	break;
                                    	case VOLT:
                                        	e_desc->rmsg = MSGCRIT_VOLT;
                                        	break;
                                    	case THERM:
                                        	e_desc->rmsg = MSGCRIT_THERM;
                                        	break;
                                    	case POWER:
                                        	e_desc->rmsg = MSGCRIT_POWER;
                                        	break;
                                    	default:
                                        	e_desc->rmsg = MSGCRIT_UNK;
                                 	       	break;
                                }

                                break;

			default:
				if (status == NORMAL || status == GS_SUCCESS)
					return 0;	/* ignore; not error */
				else {
					/* here if error log indicates error */
					/* from get-sensor-status */
					if (LOC_CODES_OK)
						report_menugoal(event,
							   MSGMENUG152,
                                                           event->loc_codes);
					else
						report_menugoal(event,
								MSGMENUG153);

					return 0;	/* don't report err */
				}
				break;
		}
	}

	if (version != 3)
		/* adds any location codes from log */
		report_srn(event, 0, e_desc);

	/*
	 * Version 3 will be reported after return because there is more to
	 * add to event
	 */

	return rc;
}

/**
 * unknown_epow_ela
 *
 *	Makes a diagnostic conclusion, i.e. menu goal, for an error log that
 *	is unknown to ELA for any of the following:
 *		- non-product specific Version 1 logs
 *		- unknown register debug data in log
 *		- (thought to be) reserved bits set in log
 *
 * INPUTS:
 *      the EPOW class 
 */
void
unknown_epow_ela(struct event *event, int class)
{
	if (class < 3) {
		if (LOC_CODES_OK)
			report_menugoal(event, MSGMENUG156, event->loc_codes);
		else
			report_menugoal(event, MSGMENUG157);
	} else if ( class == 3 ) {
		if (LOC_CODES_OK)
			report_menugoal(event, MSGMENUG158, event->loc_codes,
                                        get_tod());
		else
			report_menugoal(event, MSGMENUG159, get_tod);
	} else {
		if (LOC_CODES_OK)
			report_menugoal(event, MSGMENUG160, event->loc_codes);
		else
			report_menugoal(event, MSGMENUG161);
	}
}

/**
 * process_v2_sp
 * @brief Analyze the Version 2 SP error logs.
 *
 * INPUTS:
 *      The encoded error type.
 *
 * @returns 1 if diagnostic conclusion,	otherwise 0.
 */
int
process_v2_sp(struct event *event, int error_type)
{
	if (event->event_buf[0] != 0x02)
		return 0;		/* not a version 2 log */

	if ((error_type & 0xFF000000) != SPLOG)
		return 0;		/* not a SP log */

	if (event->event_buf[I_BYTE19] & 0x80) {
		/* Byte 19, bit 0 */
		report_srn(event, 0, sp754);
		return 1;
	}

	dbg("Found unknown version 2 SP log");
	return 0;
}

/**
 * process_v3_logs
 * @brief Analyze the Version 3 and beyond error logs.
 *
 * INPUTS:
 *      The encoded error type.
 *
 * @returns 1 if diagnostic conclusion,	otherwise 0.
 */
int 
process_v3_logs(struct event *event, int error_type)
{
	int version;
	int platform_specific;
	int predictive;
	int replace_all;
	int concurrent;
	int sw_also;
	int format_type;
	int errdscr_ready = 0;
	int seqn;
	char *msg;

	version = event->event_buf[0];
	if (version < 0x03)
		return 0;		/* not a version 3 or beyond log */

	/* Does log contain platform specific error code? */
	platform_specific = event->event_buf[I_BYTE1] & 0x80;
	if (platform_specific) {
		/* get the error code nibble */
		platform_specific = event->event_buf[I_BYTE1] & 0x0F;
		platform_error[0].rcode += platform_specific;
		report_srn(event, 0, platform_error);
		/* reset rcode to be correct for additional errors */
		platform_error[0].rcode -= platform_specific;
		return 1;
	}

	/* Get error format type from log */
	format_type = (error_type & 0xFF000000) >> 24;

	/* Treat POST detected errors as Version 1. */
	if (format_type == RTAS_EXTHDR_FMT_POST)
		return 0;

	/* Encode the log format type into the SRN */
	v3_errdscr[0].sn = SN_V3ELA + format_type;

	/* If predictive error, then modify source number */
	predictive = event->event_buf[I_BYTE0] & 0x08; 

	if (format_type == RTAS_EXTHDR_FMT_EPOW) {
		/* if ok, errdscr needs only modifier bits */
		errdscr_ready = process_v3_epow(event, error_type, version);
		if (errdscr_ready == 2) 
			return 1;	/* refcode, so skip the rest */
	} else {
		/* Convert symptom bits to sequence number. */
		seqn = convert_symptom(event, format_type, predictive, &msg);
		if (seqn == 0x100)
			/* a menugoal was used in lieu of a SRN */
			return 1;

		v3_errdscr[0].rcode = (seqn << 4);
		v3_errdscr[0].rmsg = msg;

		if (seqn == 0xFF) {
			/* Check for SRC (ref-code) data */
			short *refc;
			int rlen;
			refc = (short *)get_register_data(event, SRC_REG_ID_04,
							  &rlen);
			if (refc != NULL) {
				process_refcodes(event, refc, rlen);
				return 1;
			}
			/* Don't modify the SRN any further because of */
			/* unknown symptom bits or error log format. */
			report_srn(event, 0, v3_errdscr);
			return 1;
		}
		errdscr_ready = 1;
	}

	/* Ok to continue encoding the SRN. */

	if (predictive)
		v3_errdscr[0].sn |= 0x010;

	/* Get other modifiers from the error log header */
	/* replace all frus in list */
	replace_all = event->event_buf[I_BYTE3] & 0x20;
	/* repair as system running */
	concurrent = event->event_buf[I_BYTE3] & 0x40;
	/* sw also a probable cause */
	sw_also = event->event_buf[I_BYTE3] & 0x80;

	while (sw_also) {
		/* using while statement so can break out if canceled */
		/* further analysis needed for s/w also as a cause */
		report_menugoal(event, MSGMENUG171);
		break;
	}

	/* Combine repair modifiers to nibble for the reason code. */
	v3_errdscr[0].rcode += ((sw_also + concurrent + replace_all) >> 5);

	if (errdscr_ready > 0)
		report_srn(event, 0, v3_errdscr);

	return 1;
}

/**
 * convert_symptom
 *
 * FUNCTION:
 *      Converts the symptom bits into a unique sequence number for the
 *	error log format, and finds the reason code message for the SRN. 
 *
 * INPUTS:
 *	format_type - the format of the error log
 *      predictive - flag used to determine type of message
 *
 * OUTPUTS:
 *      The message number of the reason code message.
 *	predictive indicates whether the SRN should reflect deferred repair.
 *
 * RETURNS:
 *	The BCD sequence number, 0 to 0x99 for encoding into a SRN. 
 *
 * 	If there is any error, the sequence number returned will be 0xFF.
 *	The error could be unknown unknow error log format.
 *	Unknown symptom bits will be defaulted to ALL ZEROS in the
 *	symptom bits, which will be sequence number 0.
 *
 *	0x100 will be returned if a menugoal, or another SRN type was
 * 	displayed in lieu of encoding a SRN.
 *
 */
#define NCPUSYMPTOMS	 9
#define NMEMSYMPTOMS	17
#define NIOSYMPTOMS	17
#define NSPSYMPTOMS	33
#define NSPSYMPTOMS_ADDITIONAL	9
int
convert_symptom(struct event *event, int format_type, int predictive,
		char **msg)
{
	int seqn;
	int sbits;
	int signature;
	int msg_index;
	int error_type;

	/* Look up tables for sequence number to reason code messages. */
	int cpu_log_sig[NCPUSYMPTOMS] = {0x00, 0x80, 0x40, 0x20, 0x10,
					0x08, 0x04, 0x02, 0x01};
	char *cpu_log[NCPUSYMPTOMS][2]= {
		{ MSGCPUALLZERO, DEFER_MSGALLZERO},
		{ MSGCPUB12b0, DEFER_MSGCPUB12b0},
		{ MSGCPUB12b1, DEFER_MSGCPUB12b1},
		{ MSGCPUB12b2, DEFER_MSGCPUB12b2},
		{ MSGCPUB12b3, DEFER_MSGCPUB12b3},
		{ MSGCPUB12b4, DEFER_MSGCPUB12b4},
		{ MSGCPUB12b5, DEFER_MSGCPUB12b5},
		{ MSGCPUB12b6, DEFER_MSGCPUB12b6},
		{ MSGCPUB12b7, DEFER_MSGCPUB12b7}
	};

	int mem_log_sig[NMEMSYMPTOMS] = {0x0000, 0x8000, 0x4000, 0x2000, 0x1000,
					 0x0800, 0x0400, 0x0200, 0x0100,
					 0x0080, 0x0040, 0x0020, 0x0010,
					 0x0008, 0x0004, 0x0002, 0x0001};
	char *mem_log[NMEMSYMPTOMS][2] = {
		{ MSGMEMALLZERO, DEFER_MSGALLZERO},
		{ MSGMEMB12b0, DEFER_MSGMEMB12b0},
		{ MSGMEMB12b1, DEFER_MSGMEMB12b1},
		{ MSGMEMB12b2, DEFER_MSGMEMB12b2},
		{ MSGMEMB12b3, DEFER_MSGMEMB12b3},
		{ MSGMEMB12b4, DEFER_MSGMEMB12b4},
		{ MSGMEMB12b5, DEFER_MSGMEMB12b5},
		{ MSGMEMB12b6, DEFER_MSGMEMB12b6},
		{ MSGMEMB12b7, DEFER_MSGMEMB12b7},
		{ MSGMEMB13b0, DEFER_MSGMEMB13b0},
		{ MSGMEMB13b1, DEFER_MSGMEMB13b1},
		{ MSGMEMB13b2, DEFER_MSGMEMB13b2},
		{ MSGMEMB13b3, DEFER_MSGMEMB13b3},
		{ MSGMEMB13b4, DEFER_MSGMEMB13b4},
		{ MSGRESERVED, DEFER_MSGRESERVED},
		{ MSGMEMB13b6, DEFER_MSGMEMB13b6},
		{ MSGMEMB13b7, DEFER_MSGMEMB13b7},
	};

	/* I/O Byte 13, bits 0-2 are describtions */
	/* that are masked off the symptom bits. */
	int io_log_sig[NIOSYMPTOMS] = {0x0000, 0x8000, 0x4000, 0x2000, 0x1000,
				       0x0800, 0x0400, 0x0410, 0x0200, 0x0210,
				       0x0100, 0x0110, 0x0010,
				       0x0008, 0x0004, 0x0002, 0x0001};
	char * io_log[NIOSYMPTOMS][3] = {
		{ MSGIOALLZERO, DEFER_MSGALLZERO},
		{ MSGIOB12b0, DEFER_MSGIOB12b0},
		{ MSGIOB12b1, DEFER_MSGIOB12b1},
		{ MSGIOB12b2, DEFER_MSGIOB12b2},
		{ MSGIOB12b3, DEFER_MSGIOB12b3},
		{ MSGIOB12b4, DEFER_MSGIOB12b4},
		{ MSGIOB12b5, DEFER_MSGIOB12b5},
		{ MSGIOB12b5B13b3, DEFER_MSGIOB12b5B13b3},
		{ MSGIOB12b6, DEFER_MSGIOB12b6},
		{ MSGIOB12b6B13b3, DEFER_MSGIOB12b6B13b3},
		{ MSGIOB12b7, DEFER_MSGIOB12b7},
		{ MSGIOB12b7B13b3, DEFER_MSGIOB12b7B13b3},
		{ MSGIOB13b3, DEFER_MSGIOB13b3},
		{ MSGIOB13b4, DEFER_MSGIOB13b4},
		{ MSGIOB13b5, DEFER_MSGIOB13b5},
		{ MSGIOB13b6, DEFER_MSGIOB13b6},
		{ MSGIOB13b7, DEFER_MSGIOB13b7},
	};

	int sp_log_sig[NSPSYMPTOMS] = { 0x00000000,
		0x80000000, 0x40000000, 0x20000000, 0x10000000,
		0x08000000, 0x04000000, 0x02000000, 0x01000000,
		0x00800000, 0x00400000, 0x00200000, 0x00100000,
		0x00080000, 0x00040000, 0x00020000, 0x00010000,
		0x00008000, 0x00004000, 0x00002000, 0x00001000,
		0x00000800, 0x00000400, 0x00000200, 0x00000100,
		0x00000080, 0x00000040, 0x00000020, 0x00000010,
		0x00000008, 0x00000004, 0x00000002, 0x00000001};
	char *sp_log[NSPSYMPTOMS][2] = {
		{ MSGSPALLZERO, DEFER_MSGALLZERO},
		{ MSGSPB16b0, DEFER_MSGSPB16b0},
		{ MSGSPB16b1, DEFER_MSGSPB16b1},
		{ MSGSPB16b2, DEFER_MSGSPB16b2},
		{ MSGSPB16b3, DEFER_MSGSPB16b3},
		{ MSGSPB16b4, DEFER_MSGSPB16b4},
		{ MSGSPB16b5, DEFER_MSGSPB16b5},
		{ MSGSPB16b6, DEFER_MSGSPB16b6},
		{ MSGSPB16b7, DEFER_MSGSPB16b7},
		{ MSGSPB17b0, DEFER_MSGSPB17b0},
		{ MSGSPB17b1, DEFER_MSGSPB17b1},
		{ MSGSPB17b2, DEFER_MSGSPB17b2},
		{ MSGSPB17b3, DEFER_MSGSPB17b3},
		{ MSGSPB17b4, DEFER_MSGSPB17b4},
		{ MSGSPB17b5, DEFER_MSGSPB17b5},
		{ MSGRESERVED,DEFER_MSGRESERVED},
		{ MSGRESERVED,DEFER_MSGRESERVED},
		{ MSGSPB18b0, DEFER_MSGSPB18b0},
		{ MSGSPB18b1, DEFER_MSGSPB18b1},
		{ MSGSPB18b2, DEFER_MSGSPB18b2},
		{ MSGSPB18b3, DEFER_MSGSPB18b3},
		{ MSGSPB18b4, DEFER_MSGSPB18b4},
		{ MSGRESERVED,DEFER_MSGRESERVED},
		{ MSGSPB18b6, DEFER_MSGSPB18b6},
		{ MSGSPB18b7, DEFER_MSGSPB18b7},
		{ MSGSPB19b0, DEFER_MSGSPB19b0},
		{ MSGSPB19b1, DEFER_MSGSPB19b1},
		{ MSGRESERVED, DEFER_MSGRESERVED},
		{ MSGRESERVED, DEFER_MSGRESERVED},
		{ MSGSPB19b4, DEFER_MSGSPB19b4},
		{ MSGSPB19b5, DEFER_MSGSPB19b5},
		{ MSGSPB19b6, DEFER_MSGSPB19b6},
		{ MSGRESERVED, DEFER_MSGRESERVED},
	};

	int sp_log_additional_sig[NSPSYMPTOMS_ADDITIONAL] = {0x00,
						0x80, 0x40, 0x20, 0x10,
						0x08, 0x04, 0x02, 0x01};
	char *sp_log_additional[NSPSYMPTOMS_ADDITIONAL][2] = {
		{ MSGSPALLZERO, DEFER_MSGALLZERO},
		{ MSGSPB28b0, DEFER_MSGSPB28b0},
		{ MSGSPB28b1, DEFER_MSGSPB28b1},
		{ MSGSPB28b2, DEFER_MSGSPB28b2},
		{ MSGSPB28b3, DEFER_MSGSPB28b3},
		{ MSGSPB28b4, DEFER_MSGSPB28b4},
		{ MSGSPB28b5, DEFER_MSGSPB28b5},
		{ MSGSPB28b6, DEFER_MSGSPB28b6},
		{ MSGSPB28b7, DEFER_MSGSPB28b7},
	};

	*msg = NULL;
	seqn = 1;
	msg_index = 0;

	if (predictive)
		msg_index = 1;

	switch (format_type) {
		case RTAS_EXTHDR_FMT_CPU:
			sbits = event->event_buf[I_BYTE12];
			while (seqn < NCPUSYMPTOMS) {
				signature = cpu_log_sig[seqn];
				if ((sbits & signature) == signature)
					break;
				seqn++;
			}
			if (seqn == NCPUSYMPTOMS)
				seqn = 0;
			*msg = cpu_log[seqn][msg_index];
			break;

		case RTAS_EXTHDR_FMT_MEMORY:
			sbits = (event->event_buf[I_BYTE12] << 8) |
				 event->event_buf[I_BYTE13];

			while (seqn < NMEMSYMPTOMS) {
				signature = mem_log_sig[seqn];
				if ((sbits & signature) == signature)
					break;
				seqn++;
			}
			if (seqn == NMEMSYMPTOMS)
				seqn = 0;
			*msg = mem_log[seqn][msg_index];
			break;

		case RTAS_EXTHDR_FMT_IO:
			/* first check for io bus error that isolates */
			/* only to a bus, i.e. produces SRN 9CC-1xx */
			error_type = get_error_type(event, format_type);
			if (analyze_io_bus_error(event, 3, error_type)) {
				return 0x100;
			}

			sbits = (event->event_buf[I_BYTE12] << 8) |
				 event->event_buf[I_BYTE13];
			sbits &= 0x0FF1F;

			while (seqn < NIOSYMPTOMS) {
				signature = io_log_sig[seqn];
				if ((sbits & signature) == signature)
					break;
				seqn++;
			}
			if (seqn == NIOSYMPTOMS)
				seqn = 0;
			*msg = io_log[seqn][msg_index];
			break;

		case RTAS_EXTHDR_FMT_IBM_SP:
			sbits =((event->event_buf[I_BYTE16] << 24) |
				(event->event_buf[I_BYTE17] << 16) |
				(event->event_buf[I_BYTE18] << 8 ) |
				event->event_buf[I_BYTE19]);
			if (sbits) {
				while (seqn < NSPSYMPTOMS) {
					signature = sp_log_sig[seqn];
					if ((sbits & signature) == signature)
						break;
					seqn++;
				}
				if (seqn >= NSPSYMPTOMS)
					seqn = 0;
				*msg = sp_log[seqn][msg_index];
				break;
			} else {
				/* use additional symptom bits */
				sbits = event->event_buf[I_BYTE28];
				if (!sbits) {
					seqn = 0;
					*msg =
					    sp_log_additional[seqn][msg_index];
				} else {
				  while ((seqn < NSPSYMPTOMS_ADDITIONAL)) {
					signature = sp_log_additional_sig[seqn];
					if ((sbits & signature) == signature)
						break;
					seqn++;
				  }
				  
				  if (seqn >= NSPSYMPTOMS_ADDITIONAL) {
					seqn = 0;
				  	*msg =
					    sp_log_additional[seqn][msg_index];
				  } else {
				  	*msg =
					    sp_log_additional[seqn][msg_index];
				  	seqn += (NSPSYMPTOMS - 1);
					/* after original symptom bits */
				  }
				}
				break;
			}

		default:
			/*
			 * Should not get here unless the format is
			 * unsupported for Version 3 logs. It would
			 * be an architecture violation by RTAS.
			 *
			 * Assign a sequence number of FF and a
			 * message that indicates the error log is
			 * unknown, and the SRN will be undocumented.
			 * The SRN will look like:
			 *    A00-FF0
			 */

			seqn = 0xFF;
			*msg = MSG_UNKNOWN_V3;
			return seqn;
	}

	/* convert sequence number to BCD */
	seqn = convert_bcd(seqn);

	return seqn;
}

/**
 * process_v3_epow 
 * @brief Analyze Versions 3 and beyond EPOW error logs.
 *
 * INPUTS:
 *      The encoded error type.
 *	The version of the RPA error log format.
 *
 * OUTPUTS:
 *      Fills in the v3_errdscr or reports SRN with refcodes.
 *
 * RETURNS:
 *	1 if errdscr is ready,
 *	2 if refcode was reported
 *	otherwise 0, i.e. a menu goal was reported instead.
 *
 */

/* 
 * This macro will process any "04" tagged debug data for Ref Codes,
 * contained in a Version 4 or greater RPA error log. The intend is
 * to call out the ref codes, unless the analysis would be a menugoal only.
 * For a menugoal, any ref code will be appended to the menugoal for
 * reference if the problem can not be solved by report_menugoal.
 */
#define PROCESS_V4_REFCODE	\
	if (version > 3 && \
	   (refc = (short *)get_register_data(event, SRC_REG_ID_04, &rlen)) != NULL )\
	{ \
		process_refcodes(event, refc, rlen); \
		return 2; \
	} 

int
process_v3_epow(struct event *event, int error_type, int version)
{
	int rc = 1;
	int class;
	int rlen;
	short *refc;

	class = error_type & 0xFF;

	if (class == 0) {
		/*
		 * This is an EPOW reset, so save it and go to the next
		 * (older) error
		 */
		save_epow_reset(event, error_type);
		return 0;
	} else if (has_epow_reset(event, error_type)) {
		/* this error has been reset */
		return 0; 	 /* continue */
	}

	v3_errdscr[0].rcode = 0;
	v3_errdscr[0].rmsg = MSGEPOWALLZERO;

	/* Look for an EPOW detected by a defined sensor. */
	if (error_type & EPOWB16b0) {
		PROCESS_V4_REFCODE /* returns if callout is refcode */
		rc = sensor_epow(event, error_type, version);
	} else if (error_type & EPOWB16b1) {
		/* look for power fault epow */
		if (error_type & EPOWB17b0) {
			/* unspecified cause for power fault */
			PROCESS_V4_REFCODE /* returns if callout is refcode */
			v3_errdscr[0].rcode = 0x140;
			if (class < 3) {
				v3_errdscr[0].sn |= 0x10;
				v3_errdscr[0].rmsg = MSGEPOWB17b0C12;
			} else {
				v3_errdscr[0].rmsg = MSGEPOWB17b0C37;
			}
		} else if (error_type & EPOWB17b1) {
			if (error_type & EPOWB16b4) {
				/* loss of redundant input */
				report_menugoal(event, MSGMENUG204);
				rc = 0;
			} else if (class == 2 && version == 4) {
				/* this is Version 4, so refer to
				 * possible battery backup */
				report_menugoal(event, MSGMENUG205);
				rc = 0;
			} else {
				/* loss of power source */
				loss_power_MG(event);
				rc = 0;
			}
		} else if (error_type & EPOWB17b2) {
			/* internal power supply failure */
			PROCESS_V4_REFCODE /* returns if callout is refcode */
			v3_errdscr[0].rcode = 0x160;
			if (error_type & EPOWB16b4) {
				v3_errdscr[0].sn |= 0x10;
				v3_errdscr[0].rcode = 0x170;
				v3_errdscr[0].rmsg = MSGEPOWB17b2RED;
			} else if ( class < 3 ) {
				v3_errdscr[0].sn |= 0x10;
				v3_errdscr[0].rmsg = MSGEPOWB17b2C12;
			} else {
				v3_errdscr[0].rmsg = MSGEPOWB17b2C37;
			}
		} else if (error_type & EPOWB17b3) {
			/* manual power off via op panel */
			manual_power_off_MG(event);
			rc = 0;
		} else if (error_type & EPOWB17b4) {
			/* internal battery failure */
			PROCESS_V4_REFCODE /* returns if callout is refcode */
			v3_errdscr[0].rcode = 0x240;
			v3_errdscr[0].rmsg = MSGEPOWB16b1B17b4;
			if (class < 3)
				v3_errdscr[0].sn |= 0x10;
		} else {
			/* Reserved bits in EPOW log */
			/* This error log is unknow to ELA */
			PROCESS_V4_REFCODE /* returns if callout is refcode */
			unknown_epow_ela(event, class);
			rc = 0;
		}
	} else if (error_type & EPOWB16b2) {
		/* look for fan failures */
		if (error_type & EPOWB16b4) {
			/* Loss of redundant fan */
			PROCESS_V4_REFCODE /* returns if callout is refcode */
			v3_errdscr[0].sn |= 0x10;
			v3_errdscr[0].rcode = 0x200;
			v3_errdscr[0].rmsg = MSGEPOW1501B16b4;
		}
		else if (error_type & EPOWB16b3) {
		/* look for Over temperature and fan failure */
			/* System shutdown - thermal & fan */
			PROCESS_V4_REFCODE /* returns if callout is refcode */
			v3_errdscr[0].rcode = 0x220;
			v3_errdscr[0].rmsg = MSGEPOWB16b23;
			if (class < 3) {
				v3_errdscr[0].sn |= 0x10;
				v3_errdscr[0].rmsg = MSGEPOWB16b2b3;
			}
		} else {
			PROCESS_V4_REFCODE /* returns if callout is refcode */
			v3_errdscr[0].rcode = 0x190;
			if (class < 3) {
				v3_errdscr[0].sn |= 0x10;
				v3_errdscr[0].rmsg = MSGEPOWB16b2C12;
			} else {
				v3_errdscr[0].rmsg = MSGEPOWB16b2C37;
			}
		}
	} else if (error_type & EPOWB16b3) {
	/* look for Over temperature conditions */
		if (class == 1) {
			/* over temp. warning */
			report_menugoal(event, MSGMENUG102);
			rc = 0;
		} else {
			/* Over temperature condition */
			PROCESS_V4_REFCODE /* returns if callout is refcode */
			v3_errdscr[0].rcode = 0x210;
			v3_errdscr[0].rmsg = MSGXEPOW3n73;
			if (class < 3) {
				v3_errdscr[0].rmsg = MSGEPOWB16b3;
				v3_errdscr[0].sn |= 0x10;
			}
		}
	} else if (error_type & EPOWB16b4) {
		/* look for loss of redundancy, without any other indication */
		if (error_type & EPOWB17b1) {
			report_menugoal(event, MSGMENUG204);
			rc = 0;
		} else {
			/* Loss of redundant power supply */
			PROCESS_V4_REFCODE /* returns if callout is refcode */
			v3_errdscr[0].sn |= 0x10;
			v3_errdscr[0].rcode = 0x230;
			v3_errdscr[0].rmsg = MSGEPOW1502B16b4;
		}
	} else {
		/* This error log is unknow to ELA */
		PROCESS_V4_REFCODE /* returns if callout is refcode */
		unknown_epow_ela(event, class);
		rc = 0;
	}

	return rc;
}

/**
 * clear_edesc_struct
 *
 *	Clear the frus out of the event description. This allows the caller to
 *	recycle the same event to report more than 4 frus.
 */
void 
clear_edesc_struct(struct event_description_pre_v6 *event)
{
	int k;

	for (k = 0; k < MAXFRUS; k++) {
		event->frus[k].conf = 0;
		event->frus[k].fmsg = 0;
		strcpy(event->frus[k].floc, "");
		strcpy(event->frus[k].fname, "");
	}
	return;
}

/**
 * make_refcode_errdscr 
 *
 *      The first time this function is called for a given error log entry,
 *	this function will call adderrdscr and save_davars_ela. Subsequent
 *	calls to this function for the same error log entry result in
 *	add_more_descrs being called.
 *
 * INPUTS:
 *      Pointer to an erro description containing the information to be logged
 *	Pointer to an integer flag that allows this function to know
 *	whether or not it has already been called for this error log entry.
 *
 * OUTPUTS:
 *      Pointer to an integer flag that allows this function to know
 *	whether or not it has already been called for this error log entry.
 *
 * RETURNS:
 *	0 if the errdscr information was added successfully.
 *	-1 if adderrdscr or add_more_descrs failed.
 */
int 
make_refcode_errdscr(struct event *event,
		  struct event_description_pre_v6 *e_desc,
		  int *adderrdscr_called)
{
	int rc = 0;

	if (! (*adderrdscr_called)) {
		*adderrdscr_called = 1;
		if (set_srn_and_callouts(event, e_desc, 0))
			rc = -1;	/* adderrdscr error. */
	} else {
		if (add_more_descrs(event, e_desc))
			rc = -1;	/* add_more_descrs error. */
	}

	return rc;
}

int
process_refcodes(struct event *event, short *refc, int rlen)
{
	int i, j;
	short src_type;
	int one2one;
	char *lcc;
	int predictive_error = 0;
	struct event_description_pre_v6 *e_desc;
	char *loc;
	int nlocs;
	int adderrdscr_called = 0;

	/*
	 * refc points to SRC data in the error log, that looks like:
	 * byte: 0    2    4    6    8    10   12   14   16   18
	 * data: B4xx RRR1 xxxx xxxx xxxx xxxx RRR2 RRR3 RRR4 RRR5
	 * byte: 20   22   24   26   28   30   32   34   36   38
	 * data: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx L1L2 L3L4
	 * byte: 40   42
	 * data: L500 0000
	 * where xx is don't care for ELA, and RRRn are the ref codes.
	 *
	 * L1-L5 are the number of location codes corresponding to
	 * ref codes RRR1-RRR5. If Ln = 0 then ref code 'n' does not
	 * have any location codes, otherwise, for each ref code, the
	 * next 'n' location codes should be extract and reported with
	 * the ref. code.
	 *
	 * Extract the SRC type, that is, the 1st 2 bytes of the
	 * register data.
	 *
	 * Extract the ref codes and save in rctab. Compare against
	 * any previously reported ref code group. Do not report the
	 * same identical group of ref codes again.
	 *
	 * Concerning location codes, when comparing ref code groups,
	 * I am assuming it is sufficient to compare only the number
	 * of location codes per ref code. The exposure is the same
	 * ref code has the same number of location codes, but
	 * different locations.
	 *
	 * If rlen == 36, then location codes are 1-1 correlated.
	 * If rlen == 44, then location codes are not 1-1 correlated,
	 * and the table rctabloc is used.
	 */

	src_type = *refc;		/* SRC type */

	if (rlen == 36)
		one2one = 1;  /* refc 1-1 with loc codes  */
	else
		one2one = 0;  /* Ceate/use shadow table b/c not 1-1 */

	dbg("reg data rlen = %d, one2one = %d", rlen, one2one);

	if (! one2one)
		/* point to start of loc. code counts */
		lcc = (char *)refc + 36;

	refc++;		/* refc now is RRR1, which must be defined. */
	rctab[0] = 1;	/* holds number of ref codes in group */
	rctab[1] = *refc;

	dbg("SRC: type = 0x%x, ref code = 0x%x", src_type, *refc);

	if (! one2one) {
		rctabloc[0] = 0;    /* not used */ 
		rctabloc[1] = *lcc; /* num of loc codes for first ref code */
		dbg("number of loc codes = 0x%x", *lcc);
		lcc++;		       /* next loc code count */
	}

	/*
	 * Put the rest of the ref codes into the table. 
	 * Stop when a ref. code is zero.
	 */
	refc += 5;			/* now refc points to RRR2 */

	while (*refc && rctab[0] < MAXREFCODES) {
		rctab[0]++;		/* increment count of ref codes */
		rctab[rctab[0]] = *refc;

		dbg("ref code = 0x%x", *refc);
		refc++;			/* pointer to next ref code */
		if (! one2one) {
			rctabloc[rctab[0]] = *lcc;
			dbg("number of loc codes = 0x%x", *lcc);
			lcc++;		     /* next loc code count */
		}
	}

	dbg("ref code count = 0x%x", rctab[0]);

	/* Here to report an SRN using group of ref codes. */
	e_desc = &cec_src[0];

	/* flag predictive errors */
	predictive_error = event->event_buf[I_BYTE0] & 0x08;

	if (predictive_error)
		e_desc->sn = 0x652;
	else
		e_desc->sn = 0x651;

	e_desc->rcode = REFCODE_REASON_CUST;

	if (predictive_error)
		e_desc->rmsg = DEFER_MSGREFCODE_CUST;
	else
		e_desc->rmsg = MSGREFCODE_CUST;

	loc = get_loc_code(event, FIRST_LOC, NULL);

	i = 0;	/* counts ref codes in event */
	j = -1;	/* counts frus added to event, will incr. to use */

	/* Clear frus in case this is the second time through. */
	clear_edesc_struct(e_desc);

	/* While there are ref codes to add to this event */
	while (i < rctab[0]) {
		/* Make sure there is room in the event. */
		j++;
		if (j == MAXFRUS) {
			/* This error description structure is full. Log the
			 * info and then clear out the structure.
			 */
			if (make_refcode_errdscr(event, e_desc, &adderrdscr_called))
				return -1;

			clear_edesc_struct(e_desc);
			j = 0;
		}

		/*
		 * Put the ref code into the fru's confidence field
		 * And the src type into the fru's fmsg, and lastly,
		 * flag the controller by setting fname to a "special"
		 * string, i.e. "REF-CODE".
		 */ 
		e_desc->frus[j].conf = rctab[i+1];
		e_desc->frus[j].fmsg = src_type;
		strcpy(e_desc->frus[j].fname, REFCODE_FNAME);

		/* Check if there are any location codes? */
		if (one2one)
			nlocs = 1;
		else
			nlocs = rctabloc[i+1];

		while (nlocs) {
			/* Put loc code into fru */
			if (loc != NULL)
				strcpy(e_desc->frus[j].floc, loc);
			else
				e_desc->frus[j].floc[0] = '\0';

			/* Get ready with next location code */
			loc = get_loc_code(event, NEXT_LOC, NULL);
			nlocs--;
			if (nlocs) {
				/*
				 * More loc codes means add another
				 * fru with same ref code
				 */
				j++;
				if (j == MAXFRUS) {
					/* This err descr structure is full.
					 * Log the info and then clear
					 * out the structure.
					 */
					if (make_refcode_errdscr(event, e_desc,
							&adderrdscr_called))
						return -1;

					clear_edesc_struct(e_desc);
					j = 0;
				}
				/* Repeat the ref code */
				e_desc->frus[j].conf = rctab[i+1];
				e_desc->frus[j].fmsg = src_type;
				strcpy(e_desc->frus[j].fname, REFCODE_FNAME);
			}
		}
		i++;	/* Look for next ref code */
	}

	if (make_refcode_errdscr(event, e_desc, &adderrdscr_called))
		return -1;
	
	/* End of processing SRC data */
	return 1;
}
