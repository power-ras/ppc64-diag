/**
 * @file fru_prev6.h
 *
 * Copyright (C) 2008 IBM Corporation
 */

#ifndef _H_FRU_PREV6
#define _H_FRU_PREV6

#define NAMESIZE        16
#define LOCSIZE         80

#define MAXFRUS		4
#define ERRD1		4
#define ERRD2		5

/* 
 * A fru_callout_pre_v6 represents a field replaceable unit callout that
 * is associated with an RTAS event (prior to version 6, which is handled
 * differently).
 *
 * conf		confidence (probability) associated with the FRU callout.
 * 
 * fname	device name or configuration database keyword associated
 * 		with the field replaceable unit that is being reported.
 * 
 * floc		location associated with fname
 */
struct fru_callout_pre_v6{
	int	conf;			/* probability of failure */
	char	fname[NAMESIZE];	/* FRU name */
	char	floc[LOCSIZE];		/* location of fname */
	short   fmsg; 			/* text message number for fname */
};

/* 
 * An event_description_pre_v6 struct represents the outcome of the
 * analysis of an RTAS event (prior to version 6 events, which are
 * handled differently).
 *
 * flags	indicates the type of error description being added to the
 * 		system. The following values are defined.
 * 
 * 		ERRD1	The Error Description identifies the
 * 			resource that failed, its parent, and any cables
 * 			needed to attach the resource to its parent.
 * 
 * 		ERRD2	Similar to ERRD1, but does not include the
 * 			parent resource.
 * 
 * sn		source number of the failure.
 * 
 * rcode	reason code associated with the failure.
 * 
 * rmsg		message number of the reason code text.
 * 
 * frus		an array identifying the field replaceable unit callouts
 * 		associated with this event.
 */
struct event_description_pre_v6 {
	char	dname[NAMESIZE];	/* device name */
	short   flags;
	short	sn;			/* source number of the failure */
	short	rcode;			/* reason code for the failure */
	char    *rmsg;			/* failure description */
	struct fru_callout_pre_v6 frus[MAXFRUS];
};

#endif
