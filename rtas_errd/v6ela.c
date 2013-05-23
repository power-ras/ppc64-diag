/**
 * @file v6ela.c
 *
 * Copyright (C) 2005, 2008 IBM Corporation
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <librtasevent.h>
#include <sys/types.h>

#include "rtas_errd.h"
#include "ela_msg.h"
#include "dchrp.h"

#define V6_ERROR_MSG	0
#define V6_EVENT_MSG	1

#define NUM_SUBID	12
#define NUM_SEVERITY	14 
#define NUM_SUBTYPE	14 

/* 
 * Table of V6 Error Messages. Each message, V6_ER_ss_vv,  is addressable 
 * by the Subsystem Id ("ss") and the Error Severity ("vv"). 
 */
char *v6_error[NUM_SUBID][NUM_SEVERITY] = { 
	{V6_ER_10_00, V6_ER_10_10, V6_ER_10_20, V6_ER_10_21, V6_ER_10_22,
	 V6_ER_10_23, V6_ER_10_24, V6_ER_10_40, V6_ER_10_41, V6_ER_10_44,
	 V6_ER_10_45, V6_ER_10_48, V6_ER_10_60, V6_ER_10_61},

	{V6_ER_20_00, V6_ER_20_10, V6_ER_20_20, V6_ER_20_21, V6_ER_20_22,
	 V6_ER_20_23, V6_ER_20_24, V6_ER_20_40, V6_ER_20_41, V6_ER_20_44,
	 V6_ER_20_45, V6_ER_20_48, V6_ER_20_60, V6_ER_20_61},

	{V6_ER_30_00, V6_ER_30_10, V6_ER_30_20, V6_ER_30_21, V6_ER_30_22,
	 V6_ER_30_23, V6_ER_30_24, V6_ER_30_40, V6_ER_30_41, V6_ER_30_44,
	 V6_ER_30_45, V6_ER_30_48, V6_ER_30_60, V6_ER_30_61},

	{V6_ER_40_00, V6_ER_40_10, V6_ER_40_20, V6_ER_40_21, V6_ER_40_22,
	 V6_ER_40_23, V6_ER_40_24, V6_ER_40_40, V6_ER_40_41, V6_ER_40_44,
	 V6_ER_40_45, V6_ER_40_48, V6_ER_40_60, V6_ER_40_61},

	{V6_ER_50_00, V6_ER_50_10, V6_ER_50_20, V6_ER_50_21, V6_ER_50_22,
	 V6_ER_50_23, V6_ER_50_24, V6_ER_50_40, V6_ER_50_41, V6_ER_50_44,
	 V6_ER_50_45, V6_ER_50_48, V6_ER_50_60, V6_ER_50_61},

	{V6_ER_60_00, V6_ER_60_10, V6_ER_60_20, V6_ER_60_21, V6_ER_60_22,
	 V6_ER_60_23, V6_ER_60_24, V6_ER_60_40, V6_ER_60_41, V6_ER_60_44,
	 V6_ER_60_45, V6_ER_60_48, V6_ER_60_60, V6_ER_60_61},

	{V6_ER_70_00, V6_ER_70_10, V6_ER_70_20, V6_ER_70_21, V6_ER_70_22,
	 V6_ER_70_23, V6_ER_70_24, V6_ER_70_40, V6_ER_70_41, V6_ER_70_44,
	 V6_ER_70_45, V6_ER_70_48, V6_ER_70_60, V6_ER_70_61},

	{V6_ER_7A_00, V6_ER_7A_10, V6_ER_7A_20, V6_ER_7A_21, V6_ER_7A_22,
	 V6_ER_7A_23, V6_ER_7A_24, V6_ER_7A_40, V6_ER_7A_41, V6_ER_7A_44,
	 V6_ER_7A_45, V6_ER_7A_48, V6_ER_7A_60, V6_ER_7A_61},

	{V6_ER_80_00, V6_ER_80_10, V6_ER_80_20, V6_ER_80_21, V6_ER_80_22,
	 V6_ER_80_23, V6_ER_80_24, V6_ER_80_40, V6_ER_80_41, V6_ER_80_44,
	 V6_ER_80_45, V6_ER_80_48, V6_ER_80_60, V6_ER_80_61},

	{V6_ER_90_00, V6_ER_90_10, V6_ER_90_20, V6_ER_90_21, V6_ER_90_22,
	 V6_ER_90_23, V6_ER_90_24, V6_ER_90_40, V6_ER_90_41, V6_ER_90_44,
	 V6_ER_90_45, V6_ER_90_48, V6_ER_90_60, V6_ER_90_61},

	{V6_ER_A0_00, V6_ER_A0_10, V6_ER_A0_20, V6_ER_A0_21, V6_ER_A0_22,
	 V6_ER_A0_23, V6_ER_A0_24, V6_ER_A0_40, V6_ER_A0_41, V6_ER_A0_44,
	 V6_ER_A0_45, V6_ER_A0_48, V6_ER_A0_60, V6_ER_A0_61},

	{V6_ER_B0_00, V6_ER_B0_10, V6_ER_B0_20, V6_ER_B0_21, V6_ER_B0_22,
	 V6_ER_B0_23, V6_ER_B0_24, V6_ER_B0_40, V6_ER_B0_41, V6_ER_B0_44,
	 V6_ER_B0_45, V6_ER_B0_48, V6_ER_B0_60, V6_ER_B0_61}
};

/* 
 * Table of V6 Event Messages. Each message, V6_EV_ss_tt,  is addressable 
 * by the Subsystem Id ("ss") and the Event Subtype ("tt"). 
 */
char *v6_event[NUM_SUBID][NUM_SUBTYPE] = {
	{V6_EV_10_00, V6_EV_10_01, V6_EV_10_08, V6_EV_10_10, V6_EV_10_20,
	 V6_EV_10_21, V6_EV_10_22, V6_EV_10_30, V6_EV_10_40, V6_EV_10_60,
	 V6_EV_10_70, V6_EV_10_80, V6_EV_10_D0, V6_EV_10_E0},

	{V6_EV_20_00, V6_EV_20_01, V6_EV_20_08, V6_EV_20_10, V6_EV_20_20,
	 V6_EV_20_21, V6_EV_20_22, V6_EV_20_30, V6_EV_20_40, V6_EV_20_60,
	 V6_EV_20_70, V6_EV_20_80, V6_EV_20_D0, V6_EV_20_E0},

	{V6_EV_30_00, V6_EV_30_01, V6_EV_30_08, V6_EV_30_10, V6_EV_30_20,
	 V6_EV_30_21, V6_EV_30_22, V6_EV_30_30, V6_EV_30_40, V6_EV_30_60,
	 V6_EV_30_70, V6_EV_30_80, V6_EV_30_D0, V6_EV_30_E0},

	{V6_EV_40_00, V6_EV_40_01, V6_EV_40_08, V6_EV_40_10, V6_EV_40_20,
	 V6_EV_40_21, V6_EV_40_22, V6_EV_40_30, V6_EV_40_40, V6_EV_40_60,
	 V6_EV_40_70, V6_EV_40_80, V6_EV_40_D0, V6_EV_40_E0},

	{V6_EV_50_00, V6_EV_50_01, V6_EV_50_08, V6_EV_50_10, V6_EV_50_20,
	 V6_EV_50_21, V6_EV_50_22, V6_EV_50_30, V6_EV_50_40, V6_EV_50_60,
	 V6_EV_50_70, V6_EV_50_80, V6_EV_50_D0, V6_EV_50_E0},

	{V6_EV_60_00, V6_EV_60_01, V6_EV_60_08, V6_EV_60_10, V6_EV_60_20,
	 V6_EV_60_21, V6_EV_60_22, V6_EV_60_30, V6_EV_60_40, V6_EV_60_60,
	 V6_EV_60_70, V6_EV_60_80, V6_EV_60_D0, V6_EV_60_E0},

	{V6_EV_70_00, V6_EV_70_01, V6_EV_70_08, V6_EV_70_10, V6_EV_70_20,
	 V6_EV_70_21, V6_EV_70_22, V6_EV_70_30, V6_EV_70_40, V6_EV_70_60,
	 V6_EV_70_70, V6_EV_70_80, V6_EV_70_D0, V6_EV_70_E0},

	{V6_EV_7A_00, V6_EV_7A_01, V6_EV_7A_08, V6_EV_7A_10, V6_EV_7A_20,
	 V6_EV_7A_21, V6_EV_7A_22, V6_EV_7A_30, V6_EV_7A_40, V6_EV_7A_60,
	 V6_EV_7A_70, V6_EV_7A_80, V6_EV_7A_D0, V6_EV_7A_E0},

	{V6_EV_80_00, V6_EV_80_01, V6_EV_80_08, V6_EV_80_10, V6_EV_80_20,
	 V6_EV_80_21, V6_EV_80_22, V6_EV_80_30, V6_EV_80_40, V6_EV_80_60,
	 V6_EV_80_70, V6_EV_80_80, V6_EV_80_D0, V6_EV_80_E0},

	{V6_EV_90_00, V6_EV_90_01, V6_EV_90_08, V6_EV_90_10, V6_EV_90_20,
	 V6_EV_90_21, V6_EV_90_22, V6_EV_90_30, V6_EV_90_40, V6_EV_90_60,
	 V6_EV_90_70, V6_EV_90_80, V6_EV_90_D0, V6_EV_90_E0},

	{V6_EV_A0_00, V6_EV_A0_01, V6_EV_A0_08, V6_EV_A0_10, V6_EV_A0_20,
	 V6_EV_A0_21, V6_EV_A0_22, V6_EV_A0_30, V6_EV_A0_40, V6_EV_A0_60,
	 V6_EV_A0_70, V6_EV_A0_80, V6_EV_A0_D0, V6_EV_A0_E0},

	{V6_EV_B0_00, V6_EV_B0_01, V6_EV_B0_08, V6_EV_B0_10, V6_EV_B0_20,
	 V6_EV_B0_21, V6_EV_B0_22, V6_EV_B0_30, V6_EV_B0_40, V6_EV_B0_60,
	 V6_EV_B0_70, V6_EV_B0_80, V6_EV_B0_D0, V6_EV_B0_E0}
};

/**
 * get_message_id
 *
 * FUNCTION:	Look up the message id for the given message type and UH 
 *		section. From the UH section, the subsystem id and either
 *		the error severity or the event subtype is used to find
 *		the message id.
 *
 * RETURNS:	the message id for the dchrp.msg file
 *
 */
char *
get_message_id(int type, struct rtas_usr_hdr_scn *usrhdr)
{
	int i,j,k;
	int supported_severity[NUM_SEVERITY] = { 0, 0x10, 0x20, 0x21, 0x22, 
	                                         0x23, 0x24, 0x40, 0x41, 0x44,
	                                         0x45, 0x48, 0x60, 0x61}; 
	int supported_subtype[NUM_SUBTYPE]   = { 0, 0x01, 0x08, 0x10, 0x20, 
	                                         0x21, 0x22, 0x30, 0x40, 0x60, 
	                                         0x70, 0x80, 0xD0, 0xE0};

	/* Subsystem IDs start at 0x10 according to the PAPR.
	   Anything below that is invalid */
	if (usrhdr->subsystem_id < 0x10)
		return (V6_INVALID_SUBID);

	/* Subsystem IDs in the range 0xB0 to 0xFF are reserved */
	if (usrhdr->subsystem_id > 0xAF) 
		return (V6_RESERVED_SUBID);

	/* Use upper 4 bits for message index */
	i = usrhdr->subsystem_id >> 4;
	
	if (usrhdr->subsystem_id > 0x79) 
		i++;
	
	/* Decrement subsystem ID index into message arrays
	   (index 0 = message for subsystem ID 0x10 - 0x1F, etc) */
	i--;

	if (type == V6_ERROR_MSG) {
		for (k = 0, j = -1; k < NUM_SEVERITY && j == -1; k++)
			if (supported_severity[k] == usrhdr->event_severity)
				j = k;
		if (j == -1)
			j = 0;

		return (v6_error[i][j]);
	} 
	else {
		/* V6_EVENT_MSG */
		for (k = 0, j = -1; k < NUM_SUBTYPE && j == -1; k++) 
			if (supported_subtype[k] == usrhdr->event_type)
				j = k;
		if (j == -1)
			j = 0;

		return (v6_event[i][j]);
	}
}

/**
 * report_src
 * @brief Create the servicelog entry for a v6 event
 *
 * @return 0 on success
 */
static int
report_src(struct event *event, struct rtas_priv_hdr_scn *privhdr,
	       struct rtas_usr_hdr_scn *usrhdr)
{
	struct rtas_fru_scn *fru;
	struct rtas_src_scn *src;
	struct sl_data_rtas *rtas_data = event->sl_entry->addl_data;
	int rc = 0;
	char *msg;

	src = rtas_get_src_scn(event->rtas_event);
	if (src == NULL) {
		log_msg(event, "Could not retrieve SRC section to handle "
			"event, skipping");
		return 1;
	}

	event->sl_entry->refcode = (char *)malloc(strlen(src->primary_refcode)
						  +1);
	strcpy(event->sl_entry->refcode, src->primary_refcode);

	msg = get_message_id(V6_ERROR_MSG, usrhdr);
	event->sl_entry->description = (char *)malloc(strlen(msg)+1);
	strcpy(event->sl_entry->description, msg);

	rtas_data->addl_words[0] = src->ext_refcode2;
	rtas_data->addl_words[1] = src->ext_refcode3;
	rtas_data->addl_words[2] = src->ext_refcode4;
	rtas_data->addl_words[3] = src->ext_refcode5;
	rtas_data->addl_words[4] = src->ext_refcode6;
	rtas_data->addl_words[5] = src->ext_refcode7;
	rtas_data->addl_words[6] = src->ext_refcode8;
	rtas_data->addl_words[7] = src->ext_refcode9;

	for (fru = src->fru_scns; fru != NULL; fru = fru->next) {
		struct rtas_fru_hdr *fruhdr = fru->subscns;
		struct rtas_fru_id_scn *fru_id;
		int type;
		char proc_id[20], loccode[80], frupn[20], frusn[20], ccin[20];

		proc_id[0] = '\0';
		loccode[0] = '\0';
		frupn[0] = '\0';
		frusn[0] = '\0';
		ccin[0] = '\0';

		/* the fru ID section will be first, if present */
		if (strncmp(fruhdr->id, "ID", 2) != 0)
			continue;

		fru_id = (struct rtas_fru_id_scn *)fruhdr;

		type = fru_id->fruhdr.flags & RTAS_FRUID_COMP_MASK;

		if (fru->loc_code_length)
			strncpy(loccode, fru->loc_code, fru->loc_code_length);

		if (fruid_has_part_no(fru_id)) {
			strncpy(frupn, fru_id->part_no, 20);

			if (fruid_has_ccin(fru_id)) 
				strncpy(ccin, fru_id->ccin, 20);

			if (fruid_has_serial_no(fru_id))
				strncpy(frusn, fru_id->serial_no, 20);
		}

		if (fruid_has_proc_id(fru_id))
			strncpy(proc_id, fru_id->procedure_id, 20); 

		add_callout(event, fru->priority, type, proc_id, loccode,
			    frupn, frusn, ccin);
	}

	if (event->flags & RE_SERVICEABLE)
		event->sl_entry->serviceable = 1;

	if ((usrhdr->action & 0x8000) && (usrhdr->action & 0x0800))
		event->sl_entry->call_home_status = SL_CALLHOME_CANDIDATE;

	return rc;
}

/**
 * report_menugoal
 *
 * FUNCTION:	Report a menugoal from the V6 log data passed as input. The 
 *		input data must be analyzed to determine which menugoal is
 *		used. If the log data contains an SRC and a FRU list, they
 *		are included in the menugoal if the log data indicates an
 *		error instead of an event.
 *
 * RETURNS:	Nothing.
 *
 */
#define  DISPLAY_SRC_SIZE        8      /* Display 8 digits of the SRC */
static void 
report_menugoal(struct event *event, struct rtas_priv_hdr_scn *privhdr,
		struct rtas_usr_hdr_scn *usrhdr)
{
	char buffer[MAX_MENUGOAL_SIZE], menu_num_str[20];
	char *msg;
	int offset = 0;
	uint menu_num = 0;
        long time_loc;
        struct tm *date = NULL; 

	memset(buffer, 0, sizeof(buffer));
	
	/* Create the menugoal text */

	if (usrhdr->event_severity) {
		struct rtas_epow_scn *epow;
		struct rtas_src_scn *src;
	
		epow = rtas_get_epow_scn(event->rtas_event);

		/* This menu goal is a result of V6 log marked as an error, */
		/* and the action is customer notifiy only. Include any/all */
		/* SRC and FRU call outs in the menu goal text. 	    */
		if (epow != NULL) {
			/* Attempt to map this EPOW events to preexisting */
			/* menu goals SRC and FRU Lists are added later.  */
			switch (epow->action_code) {
				case 0:
					/* No method to match this reset 
					 * to original error */
					msg = MSGMENUG203;
					menu_num = 651203;
					break;
				case 1:
				case 2:
					msg = MSGMENUG157;
					menu_num = 651157;
					break;
				case 3:
					if (epow->event_modifier == 1) {
						msg = MSGMENUG206;
						menu_num = 651206;
					} else if (epow->event_modifier == 2) {
						msg = MSGMENUG205;
						menu_num = 651205;
					} else if (epow->event_modifier == 3) {
						msg = MSGMENUG159;
						menu_num = 651159;
						time_loc = time((long *)0);
						date = localtime(&time_loc);
					}
					break;
				case 4:
				case 5:
				case 7:
					msg = MSGMENUG161;
					menu_num = 651161;
				break;
			}
			
			if (date != NULL)
				offset += sprintf(buffer + offset, msg, 
						  asctime(date)); 
			else
				offset += sprintf(buffer + offset, msg); 
		} 

		if (menu_num == 0) {
			/* Not resolved by EPOW section */
			menu_num = 651300;
			msg = get_message_id(V6_ERROR_MSG, usrhdr);
			offset += sprintf(buffer + offset, "%s%s\n\n",
					  MSGMENUGPEL_ERROR, msg);
		}

		src = rtas_get_src_scn(event->rtas_event);

		if (src != NULL && strlen(src->primary_refcode)) {
			struct rtas_fru_scn *fru;
			char tmp_buf[MAX_MENUGOAL_SIZE/2];
			char tmp_src[DISPLAY_SRC_SIZE+1]; 
			int tmp_off = 0;

			memset(tmp_buf, 0, sizeof(tmp_buf));
			
			strncpy(tmp_src, src->primary_refcode, 8);
			tmp_src[DISPLAY_SRC_SIZE] = '\0';
			offset += sprintf(buffer + offset, 
					  MSGMENUG_SRC, tmp_src);
			
			for (fru = src->fru_scns; fru != NULL; fru = fru->next){
				struct rtas_fru_hdr *fruhdr = fru->subscns;
				struct rtas_fru_id_scn *fru_id;
				char *fru_msg = NULL;

				if (strncmp(fruhdr->id, "ID", 2) != 0)
					continue;

				fru_id = (struct rtas_fru_id_scn *)fruhdr;

				switch (fruhdr->flags & RTAS_FRUID_COMP_MASK) {
					case RTAS_FRUID_COMP_HARDWARE:
						fru_msg = MSGMENUG_FRU;
						break;
					case RTAS_FRUID_COMP_CODE:
						fru_msg = MSGMENUG_CODEFRU;
						break;
					case RTAS_FRUID_COMP_CONFIG_ERROR:
						fru_msg = MSGMENUG_CFG;
						break;
					case RTAS_FRUID_COMP_MAINT_REQUIRED: 
						fru_msg = MSGMENUG_MAINT;
						break;
					case RTAS_FRUID_COMP_EXTERNAL:
						fru_msg = MSGMENUG_EXT_FRU; 
						break;
					case RTAS_FRUID_COMP_EXTERNAL_CODE:
						fru_msg = MSGMENUG_EXT_CODEFRU;
						break;
					case RTAS_FRUID_COMP_TOOL:
						fru_msg = MSGMENUG_TOOL_FRU;
						break;
					case RTAS_FRUID_COMP_SYMBOLIC:
						fru_msg = MSGMENUG_SYM_FRU;
						break;
					default:
						fru_msg = MSGMENUG_RESERVED;
				}

				if (fruid_has_part_no(fru_id)) {
				    	tmp_off += sprintf(tmp_buf + tmp_off, 
							   fru_msg, 
							   fru_id->part_no);

					if (fruid_has_ccin(fru_id)) {
						tmp_off += 
						    sprintf(tmp_buf + tmp_off, 
							    MSGMENUG_CCIN,
							    fru_id->ccin);
					}
					
					if (fruid_has_serial_no(fru_id)) {
						tmp_off += 
						    sprintf(tmp_buf + tmp_off, 
							    MSGMENUG_SERIAL, 
							    fru_id->serial_no);
					}
				} else if (fruid_has_proc_id(fru_id))
				    	tmp_off += sprintf(tmp_buf + tmp_off, 
							  fru_msg,
							  fru_id->procedure_id);

				/* Add location code if present */
				if (fru->loc_code_length) {
				    	tmp_off += sprintf(tmp_buf + tmp_off, 
							   MSGMENUG_LOCATION,
							   fru->loc_code);
				} else
					tmp_off += sprintf(tmp_buf + tmp_off, 
							   "\n");

				offset += sprintf(buffer + offset, 
						  MSGMENUG_PRIORITY, 
						  fru->priority, tmp_buf);

			}
		}
	} else {
		/* This menu goal is a result of V6 log marked as an event, */
		/* so don't include any SRC or FRU call outs because it     */
		/* will appear like an error and cause customer concern.    */
		menu_num = 651301;
		msg = get_message_id(V6_EVENT_MSG, usrhdr); 
		offset += sprintf(buffer + offset, "%s%s\n\n",
				  MSGMENUGPEL_EVENT, msg); 
	}

	event->sl_entry->serviceable = 1;
	event->sl_entry->call_home_status = SL_CALLHOME_NONE;

	snprintf(menu_num_str, 20, "#%d", menu_num);
	event->sl_entry->refcode = (char *)malloc(strlen(menu_num_str)+1);
	strcpy(event->sl_entry->refcode, menu_num_str);

	event->sl_entry->description = (char *)malloc(strlen(msg)+1);
	strcpy(event->sl_entry->description, msg);

	dbg("menugoal: number = %d, message = \"%s\"", menu_num, msg);

	return;
}

/*
 * process_v6
 * @brief Process events in the version 6 style
 *
 * @param event the event to be processed
 * @return 0 on success; !0 on failure
 */
int 
process_v6(struct event *event)
{
	struct rtas_event_exthdr *exthdr;
	struct rtas_priv_hdr_scn *privhdr;
	struct rtas_usr_hdr_scn *usrhdr;
	struct rtas_src_scn *srchdr;
	struct rtas_mt_scn *mt;
	struct sl_data_rtas *rtas_data = NULL;

	dbg("Processing version 6 event");

	/* create and populate the servicelog entry */
	event->sl_entry = (struct sl_event *)malloc(sizeof(struct sl_event));
	rtas_data = (struct sl_data_rtas *)malloc(sizeof(struct sl_data_rtas));

	memset(event->sl_entry, 0, sizeof(struct sl_event));
	memset(rtas_data, 0, sizeof(struct sl_data_rtas));

	event->sl_entry->addl_data = rtas_data;

	rtas_data->kernel_id = event->seq_num;
	event->sl_entry->time_event = get_event_date(event);
	event->sl_entry->type = SL_TYPE_RTAS;
	event->sl_entry->severity = servicelog_sev(event->rtas_hdr->severity);

	mt = rtas_get_mt_scn(event->rtas_event);
	if (mt != NULL) {
		event->sl_entry->machine_model =
				(char *)malloc(strlen(mt->mtms.model)+1);
		if (event->sl_entry->machine_model)
			strcpy(event->sl_entry->machine_model, mt->mtms.model);

		event->sl_entry->machine_serial =
				(char *)malloc(strlen(mt->mtms.serial_no)+1);
		if (event->sl_entry->machine_serial)
			strcpy(event->sl_entry->machine_serial,
			       mt->mtms.serial_no);
	}

	/*
	 * refcode, description, serviceable and call_home_status will be
	 * set in either report_src or report_menugoal later
	 */

	exthdr = rtas_get_event_exthdr_scn(event->rtas_event);
	if (exthdr == NULL) {
		log_msg(event, "Could not retrieve RTAS extended header "
			"section.");
	} else {
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

	/* populate the "additional data" section of the servicelog entry */
	rtas_data->event_type = event->rtas_hdr->type;

	privhdr = rtas_get_priv_hdr_scn(event->rtas_event);
	if (privhdr == NULL) {
		log_msg(event, "No PH (private header) section in this v6 "
			"RTAS event; strange, but not an error.");
	} else {
		rtas_data->platform_id = privhdr->plid;
		rtas_data->creator_id = privhdr->creator_id;
	}

	usrhdr = rtas_get_usr_hdr_scn(event->rtas_event);
	if (usrhdr == NULL) {
		log_msg(event, "No UH (user header) section in this v6 "
			"RTAS event; strange, but not an error.");
	} else {
		rtas_data->action_flags = usrhdr->action;
		rtas_data->subsystem_id = usrhdr->subsystem_id;
		rtas_data->pel_severity = usrhdr->event_severity;
		rtas_data->event_subtype = usrhdr->event_type;
	}

	/*
	 * if a "primary SRC" section exists, this is an SRC; otherwise, it
	 * is a menugoal
	 */
	srchdr = rtas_get_src_scn(event->rtas_event);
	if (srchdr != NULL)
		report_src(event, privhdr, usrhdr);
	else
		report_menugoal(event, privhdr, usrhdr);

	return 0;
}
