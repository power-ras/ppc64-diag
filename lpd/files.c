/**
 * @file	files.c
 * @brief	File manipulation routines
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "lp_diag.h"

/* Program name */
char	*program_name;

/* Indicator event log file/descriptor */
char    *lp_event_log_file = "/var/log/indicators";
int     lp_event_log_fd = -1;

/* Log file/descriptor */
char    *lp_error_log_file = "/var/log/lp_diag.log";
int     lp_error_log_fd = -1;


/**
 * reformat_msg - Re-format a log message to wrap at 80 characters
 *
 * In order to ease the formatting of messages in the lp_diag this
 * will automatically format the messages to wrap at 80 characters.
 *
 * @msg		buffer containing the message to re-format
 * @size	message size
 *
 * Returns :
 *	new buffer length
 */
static int
reformat_msg(char *msg, int size)
{
	char    buf[LP_ERROR_LOG_MAX];
	char	*pos;
	char	*next;
	char	temp;
	char	temp2;
	int     len = strlen(msg);

	if (len > LP_ERROR_LOG_MAX) /* Insufficient temporary buffer size */
		return len;

	if (len > (size - size / 80 + 1)) /* Worst case target size */
		return len;

	if (len < 80) {
		/* no need to reformat */
		msg[len++] = '\n';
		return len;
	}

	memset(buf, 0, LP_ERROR_LOG_MAX);
	/* first copy the msg into our buffer */
	memcpy(buf, msg, len);

	/* zero out the old buffer */
	msg[0] = '\0';

	pos = buf;
	while (strlen(pos) > 80) {
		next = pos + 80;
		do {
			if (*next == ' ')
				*next = '\n';
			if (*next == '\n') {
				temp = *(next + 1);
				*(next + 1) = '\0';
				strcat(msg, pos);
				*(next + 1) = temp;
				pos = next + 1;
				break;
			}
			next--;
		} while (next > pos);

		if (next == pos) {      /* word is longer than line length */
			next = pos + 79;
			temp = *next;
			temp2 = *(next + 1);
			*next = '\n';
			*(next + 1) = '\0';
			strcat(msg, pos);
			*next = temp;
			*(next + 1) = temp2;
			pos = next;
		}
	} /* while loop end */
	strcat(msg, pos);
	len = strlen(msg);
	msg[len++] = '\n';

	return len;
}

/**
 * insert_time - Insert current date and time
 */
static int
insert_time(char *buf, int size)
{
	int	len;
	time_t	cal;

	cal = time(NULL);
	len = snprintf(buf, size, "%s", ctime(&cal));
	len--;	/* remove new line character */
	return len;
}

/**
 * _dbg	- Write debug messages to stdout
 *
 * Provide utility to print debug statements if the debug flag
 * is specified.
 *
 * @fmt	format string to printf()
 * @...	additional argument
 */
void
_dbg(const char *fmt, ...)
{
#ifdef LPD_DEBUG
	int     len = 0;
	char    buf[LP_ERROR_LOG_MAX];
	va_list	ap;

	va_start(ap, fmt);
	len = snprintf(buf, LP_ERROR_LOG_MAX, "DEBUG: ");
	len += vsnprintf(buf + len, LP_ERROR_LOG_MAX - len, fmt, ap);
	if (len < 0 || len >= LP_ERROR_LOG_MAX)
		return;	/* Insufficient buffer size */

	va_end(ap);

	fprintf(stdout, buf);
	fprintf(stdout, "\n");
	fflush(stdout);
#endif
}

/**
 * log_msg - Write log message into lp_diag log file
 *
 * @fmt	format string to printf()
 * @...	additional argument
 */
void
log_msg(const char *fmt, ...)
{
	int	rc;
	int     len = 0;
	char    buf[LP_ERROR_LOG_MAX];
	va_list ap;

#ifndef LPD_DEBUG
	/* In order to make testing easier we will not print time in
	 * debug version of lp_diag.
	 */
	len = insert_time(buf, LP_ERROR_LOG_MAX);
	len += snprintf(buf + len, LP_ERROR_LOG_MAX - len, ": ");
#endif

	/* Add the actual message */
	va_start(ap, fmt);
	len += vsnprintf(buf + len, LP_ERROR_LOG_MAX - len, fmt, ap);
	if (len < 0 || len >= LP_ERROR_LOG_MAX) {
		dbg("Insufficient buffer size");
		return;
	}
	va_end(ap);

	/* Add ending punctuation */
	len += snprintf(buf + len, LP_ERROR_LOG_MAX - len, ".");

	if (lp_error_log_fd == -1) {
		dbg("Log file \"%s\" is not available", lp_error_log_file);
		_dbg(buf);
		return;
	}

	_dbg(buf);

	/* reformat the new message */
	len = reformat_msg(buf, LP_ERROR_LOG_MAX);
	rc = write(lp_error_log_fd, buf, len);
	if (rc == -1)
		dbg("Write to log file \"%s\" failed", lp_error_log_file);
}

/**
 * indicator_log_write - write indicate events to log file
 *
 * @fmt	format string to printf()
 * @...	additional argument to printf()
 *
 * Returns :
 *	return code from write() call
 */
int
indicator_log_write(const char *fmt, ...)
{
	int	len;
	int	rc = 0;
	char    buf[LP_ERROR_LOG_MAX];
	va_list ap;

	if (lp_event_log_fd == -1) {
		log_msg("Indicator event log file \"%s\" is not available",
			lp_event_log_file);
		return -1;
	}

	/* Add time */
	len = insert_time(buf, LP_ERROR_LOG_MAX);

	len += snprintf(buf + len, LP_ERROR_LOG_MAX - len, ": %s : ",
			program_name);

	va_start(ap, fmt);
	len += vsnprintf(buf + len, LP_ERROR_LOG_MAX - len, fmt, ap);
	if (len < 0 || len >= LP_ERROR_LOG_MAX) {
		log_msg("Insufficient buffer size");
		return -1;
	}
	va_end(ap);

	/* Add ending punctuation */
	len += snprintf(buf + len, LP_ERROR_LOG_MAX - len, "\n");

	rc = write(lp_event_log_fd, buf, len);
	if (rc == -1)
		log_msg("Write to indicator event log file \"%s\" failed",
			lp_event_log_file);

	return rc;
}

/**
 * rotate_log_file - Check log file size and rotate if required.
 *
 * @lp_log_file	log file name
 *
 * Returns :
 *	nothing
 */
static void
rotate_log_file(const char *lp_log_file)
{
	int	rc;
	char	cmd_buf[128];
	char	lp_log_file0[64];
	struct	stat sbuf;

	rc = stat(lp_log_file, &sbuf);
	if (rc == -1) {
		dbg("Cannot stat log file to rotate logs");
		return;
	}

	if (sbuf.st_size > LP_ERRD_LOGSZ) {
		_dbg("Rotating log file : %s", lp_log_file);
		snprintf(lp_log_file0, sizeof(lp_log_file0), "%s0",
			 lp_log_file);
		snprintf(cmd_buf, sizeof(cmd_buf), "rm -f %s; mv %s %s",
			 lp_log_file0, lp_log_file, lp_log_file0);
		rc = system(cmd_buf);
		if (rc == -1) {
			dbg("An error occured during the rotation of log "
			    "file:\n cmd = %s", cmd_buf);
			_dbg("Exit status : %d", WEXITSTATUS(rc));
		}
	}
}

/**
 * init_files - Open log files
 *
 * Returns :
 *	0 on sucess, !0 on failure
 */
int
init_files(void)
{
	int rc = 0;

	/* lp_diag log file */
	if (lp_error_log_fd != 1) { /* 1 = stdout */
		rotate_log_file(lp_error_log_file);

		lp_error_log_fd = open(lp_error_log_file,
				       O_RDWR | O_CREAT | O_APPEND,
				       S_IRUSR | S_IWUSR | S_IRGRP);

		if (lp_error_log_fd == -1)
			dbg("Could not open log file \"%s\"",
			    lp_error_log_file);
	}

	/* open event log file */
	lp_event_log_fd = open(lp_event_log_file,
			       O_RDWR | O_CREAT | O_APPEND,
			       S_IRUSR | S_IWUSR | S_IRGRP);
	if (lp_event_log_fd == -1) {
		log_msg("Could not open indicator event log file \"%s\"",
			lp_event_log_file);
		return -1;
	}

	return rc;
}

/**
 * close_files - Close all the files used by lp_diag
 *
 * Perform any file cleanup (i.e. close()) and possibly free()'ing
 * buffers needed by log_diag before exiting.
 *
 * Returns :
 *	nothing
 */
void
close_files(void)
{
	if (lp_error_log_fd > 1) /* don't close stdout */
		close(lp_error_log_fd);
	if (lp_event_log_fd != -1)
		close(lp_event_log_fd);
}
