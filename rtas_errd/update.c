/**
 * @file update.c
 * @brief Routines to update RTAS evetns to the platform log
 *
 * In certian cases we may need to get RTAS events from /var/log/messages
 * and process them to /var/log/platform. (i.e. an EPOW event that allows
 * enough time for the RTAS event(s) to be written to /var/log/messages but
 * not enough time for rtas_errd to run.
 *
 * The search implemeted here checks the last RTAS event number in 
 * /var/log/messages and /var/log/platform. If they are not equal we process
 * RTAS events from /var/log/messages until they are equal.
 *
 * Copyright (C) 2004 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "rtas_errd.h"

/**
 * @def BM_ARRAY_SIZE
 * @brief Array size for Boyer-Moore algorithm
 */
#define BM_ARRAY_SIZE	256

#define RTAS_START	"RTAS event begin"
#define RTAS_END	"RTAS event end"

/**
 * @var bad_char_start
 * @brief Boyer-Moore bad character array for finding RTAS event starts.
 */
int bad_char_start[BM_ARRAY_SIZE] = {-1};
/**
 * @var bad_char_end
 * @brief Boyer-Moore bad character array for finding RTAS event ends.
 */
int bad_char_end[BM_ARRAY_SIZE] = {-1};

/** 
 * @var messages_log 
 * 
 * Used by update_rtas_msgs() to bring the platform log 
 * up to date with current RTAS events.  
 */
char *messages_log = "/var/log/messages";

/**
 * @var msgs_log_fd
 * @brief File descriptor for messages_log
 */
static int msgs_log_fd = -1;

/**
 * setup_bc
 * @brief Initalize the bad character array for a Boyer-Moore search
 * 
 * @param str search string to initialize the array with
 * @param strlen search string length
 * @param bc bad character array to be initialized
 */
void
setup_bc(char *str, int strlen, int *bc)
{
	int i;
	for (i = 0; i < BM_ARRAY_SIZE; i++)
		bc[i] = strlen;
	for (i = 0; i < (strlen - 1); i++)
		bc[(int)str[i]] = strlen - i - 1;
}

/**
 * find_event
 * @brief Find an RTAS event
 * 
 * Search for a RTAS event in the given text.  This uses a scaled 
 * down version of the Boyer-Moore algorithm.  Since we know the 
 * search strings we also know there really isn't a good suffix worth 
 * looking for.  Thus we do not	use the good suffix rule for this search.
 *
 * @param str string to search for
 * @param strlen length of search string
 * @param text text to search for string in
 * @param textlen length of text to search
 * @param bad_char bad character array for Boyer-Moore search
 * @return pointer to string in text on success, NULL on failure
 */
char *
find_event(char *str, int strlen, char *text, int textlen, int bad_char[])
{
	int	i, j = 0;

	while (j <= (textlen - strlen)) {
		/* for (i = (strlen - 1); i >= 0 && str[i] == text[i+j]; i--);*/
                i = strlen - 1;
                while ((i >= 0) && (str[i] == text[i+j])) {
                    i--;
                }
		if (i < 0) 
			return &text[j];
		else
			/* Since we're not using the good suffix rule
			 * we need to gaurantee progress.
			 */
                        j += MAX(1, bad_char[(int)text[i+j]] - strlen + 1 + i);
	}

	return NULL;
}

/** 
 * find_rtas_start
 * @brief Find the beginning of a RTAS event.
 *
 * @param textstart pointer to starting point of search
 * @param textend pointer to ending point of search
 * @return pointer to RTAS event start on success, NULL on failure.
 */
char *
find_rtas_start(char *textstart, char *textend) 
{
	if (textstart == NULL)
		return NULL;

	if (bad_char_start[0] == -1)
		setup_bc(RTAS_START, strlen(RTAS_START), bad_char_start);

	return find_event(RTAS_START, strlen(RTAS_START), textstart,
			  textend - textstart, bad_char_start);
}

/** 
 * find_rtas_end
 * @brief Find the ending of a RTAS event.
 * 
 * @param textstart pointer to starting point of search
 * @param textend pointer to ending point of search
 * @return pointer to RTAS event end on success, NULL on failure.
 */
char *
find_rtas_end(char *textstart, char *textend)
{
	if (textstart == NULL)
		return NULL;

	if (bad_char_end[0] == -1)
		setup_bc(RTAS_END, strlen(RTAS_END), bad_char_end);
	
	return find_event(RTAS_END, strlen(RTAS_END), textstart,
			  textend - textstart, bad_char_end);
}

/**
 * get_rtas_no
 * @brief Retrieve an RTAS event numnber
 *
 * Retrieve the RTAS event number from an RTAS event.  NOTE: this
 * routine assumes the pointer passed in is the result from
 * a call to find_rtas_start()
 * 
 * @param ptr pointer to RTAS event
 * @return event number on success, 0 on failure
 */
int
get_rtas_no(char *ptr)
{
	/* move back until we hit a colon */
	while (*ptr != ':')
		ptr--;

	/* move to the event number */
	ptr += 2;

	return strtoul(ptr, NULL, 10);
}

/**
 * update_rtas_msgs
 * @brief Update RTAS messages in the platfrom log
 *
 * Update the file /var/log/platform with any RTAS events
 * found in /var/log/messages that have not been handled by
 * rtas_errd.
 */
void
update_rtas_msgs(void)
{
	struct stat	log_sbuf, msgs_sbuf;
	char		*log_mmap = NULL, *log_mmap_end;
	char		*msgs_mmap = NULL, *msgs_mmap_end;
	char		*rtas_msgs_end, *rtas_msgs_start, *msgs_p;
	char		*log_p;
	char		*last_p;
	int		last_rtas_log_no, last_rtas_msgs_no, cur_rtas_no;

	/* open and map /var/log/messages */
	if ((msgs_log_fd = open(messages_log, O_RDONLY)) < 0) {
		log_msg(NULL, "Could not open %s to update RTAS events, %s",
			messages_log, strerror(errno));
		goto cleanup;
	}

	if ((fstat(msgs_log_fd, &msgs_sbuf)) < 0) {
		log_msg(NULL, "Cannot get status of %s to update RTAS events, "
			"%s", messages_log, strerror(errno));
		goto cleanup;
	}

	/* abort if file size is zero */
	if (msgs_sbuf.st_size == 0) {
		goto cleanup;
	}
		
	if ((msgs_mmap = mmap(0, msgs_sbuf.st_size, PROT_READ, MAP_PRIVATE, 
			      msgs_log_fd, 0)) == (char *)-1) {
		log_msg(NULL, "Cannot map %s to update RTAS events", 
			messages_log);
		msgs_mmap = NULL;
		goto cleanup;
	}

        msgs_p = msgs_mmap;
	msgs_mmap_end = msgs_mmap + msgs_sbuf.st_size;

	if ((fstat(platform_log_fd, &log_sbuf)) < 0) {
		log_msg(NULL, "Cannot get status of %s to update RTAS events",
			platform_log);
		goto cleanup;
	}

	/* nothing to do if file size is zero */
	if (log_sbuf.st_size == 0)
		goto cleanup;

	if ((log_mmap = mmap(0, log_sbuf.st_size, PROT_READ, MAP_PRIVATE, 
			     platform_log_fd, 0)) == (char *)-1) {
		log_msg(NULL, "Cannot map %s to update RTAS events, %s", 
			platform_log, strerror(errno));
		log_mmap = NULL;
		goto cleanup;
	}

	log_p = log_mmap;
	log_mmap_end = log_mmap + log_sbuf.st_size;

	/* find the last RTAS event in /var/log/platform */
	last_p = NULL;
	log_p = find_rtas_start(log_p, log_mmap_end);
	while (log_p != NULL) {
		last_p = log_p;
		log_p = find_rtas_start(log_p + sizeof(RTAS_START), 
					log_mmap_end);
	}
	
	if (last_p == NULL)
		last_rtas_log_no = 0;
	else
		last_rtas_log_no = get_rtas_no(last_p);

	/* We're finished with /var/log/platform; unamp it */
	munmap(log_mmap, log_sbuf.st_size);
	log_mmap = NULL;

	/* find the last RTAS event in /var/log/messages */
	last_p = NULL;
	msgs_p = find_rtas_start(msgs_p, msgs_mmap_end);
	while (msgs_p != NULL) {
		last_p = msgs_p;
		msgs_p = find_rtas_start(msgs_p + strlen(RTAS_START),
					 msgs_mmap_end);
	}

	if (last_p == NULL) {
		dbg("%s does not conatin any RTAS events", messages_log);
		goto cleanup;
	}

	last_rtas_msgs_no = get_rtas_no(last_p);

	if (last_rtas_log_no >= last_rtas_msgs_no)
		goto cleanup;

	/*
	 *  If we get here we know there are some events that have not
	 *  been processed by rtas_errd, process them now.  NOTE:  There
	 *  are scenarios in which we will process events that have 
	 *  already been processed.  There is not much we can do about
	 *  this, just accept it and move along.
	 */
	
	/* Move to the first event that has not been handled so we
	 * can process them in order.
	 */
	cur_rtas_no = 0;
	rtas_msgs_start = msgs_mmap;
	while (cur_rtas_no <= last_rtas_log_no) {
		rtas_msgs_start = 
			find_rtas_start(rtas_msgs_start + strlen(RTAS_START), 
					msgs_mmap_end);
		cur_rtas_no = get_rtas_no(rtas_msgs_start);
	}

	rtas_msgs_end = find_rtas_end(rtas_msgs_start, msgs_mmap_end);
	
	/* Retrieve RTAS events from /var/log/messages */
	while (rtas_msgs_start != NULL) {
		struct event event;
		unsigned long	*out_buf;
		char		*tmp = rtas_msgs_start;

		memset(&event, 0, sizeof(event));

		out_buf = (unsigned long *)event.event_buf;
			
		/* put event number in first */
		cur_rtas_no = get_rtas_no(rtas_msgs_start);
		

                /* skip past the "RTAS event begin" message */
                tmp += strlen(RTAS_START);
	
		while (tmp < rtas_msgs_end) {
                        int i;

			/* find the word "RTAS" */
			for ( ; *tmp != 'R'; tmp++);
			if (strncmp(tmp++, "RTAS", 4) != 0)
				continue;

			/* we found "RTAS", go to the colon */
			for( ; *tmp != ':'; tmp++);

			/* add two to get to the first value */
			tmp += 2;

			/* parse the values */
                        for (i = 0; i < 4; i++) {
				*out_buf = strtoul(tmp, NULL, 16);
				out_buf++;
				tmp += 9; /* char hex value + space */
			}
		}

		/* Initializethe fields of the rtas event */
		event.seq_num = cur_rtas_no;
		
		event.rtas_event = parse_rtas_event(event.event_buf, 
						    RTAS_ERROR_LOG_MAX);
		if (event.rtas_event == NULL) {
			log_msg(NULL, "Could not update RTAS Event %d to %s", 
				cur_rtas_no, platform_log);
		} else {
			log_msg(NULL, "Updating RTAS event %d to %s", 
				cur_rtas_no, platform_log);

			event.rtas_hdr = 
				rtas_get_event_hdr_scn(event.rtas_event);
			event.length = event.rtas_hdr->ext_log_length + 8;

			handle_rtas_event(&event);
		}
			
		rtas_msgs_start = find_rtas_start(rtas_msgs_end, msgs_mmap_end);
		rtas_msgs_end = find_rtas_end(rtas_msgs_start, msgs_mmap_end);
	}
		
cleanup:
	if (msgs_mmap)
		munmap(msgs_mmap, msgs_sbuf.st_size);
	if (msgs_log_fd != -1)
		close(msgs_log_fd);

	return;
}
