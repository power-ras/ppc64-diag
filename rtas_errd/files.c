/**
 * @file files.c
 * @brief File Manipulation routines for files used by rtas_errd
 *
 * Copyright (C) 2004 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h> 
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "rtas_errd.h"

char *platform_log = "/var/log/platform";
int platform_log_fd = -1;	

/**
 * @var proc_error_log1 
 * @brief File to read RTAS events from.
 */
/**
 * @var proc_error_log2 
 * @brief Alternate file to read RTAS events from.
 */
/**
 * @var proc_error_log_fd
 * @brief File descriptor for proc_error_log.
 */
char *proc_error_log1 = "/proc/ppc64/rtas/error_log";
char *proc_error_log2 = "/proc/ppc64/error_log";
static int proc_error_log_fd = -1;

/**
 * @var rtas_errd_log
 * @brief Message log for the rtas_errd daemon.
 * */
/**
 * @var rtas_errd_log0 
 * @brief Saved ("rolled over") messages log for rtas_errd daemon.
 */
/**
 * @var rtas_errd_log_fd
 * @brief File descriptor for rtas_errd_log
 */
/**
 * @def RTAS_ERRD_LOGSZ
 * @brief Maximum size of the rtas_errd_log
 */
char *rtas_errd_log = "/var/log/rtas_errd.log";
char *rtas_errd_log0 = "/var/log/rtas_errd.log0";
static int rtas_errd_log_fd = -1;
#define RTAS_ERRD_LOGSZ		25000

/* 
 * @var epow_status_file 
 * @brief File used to communicate the current state of an epow event
 * to rc.powerfail.
 */
static int epow_status_fd = -1;
char *epow_status_file = "/var/log/epow_status";

/**
 * @var scanlog
 * @brief buffer to hold scanlog dump path
 * 
 * This is a buffer that is allocated and filled when rtas_errd is 
 * intially exec()'ed via check_scanlog_dump(). The buffer will contain 
 * the path to a scanlog dump and is reported with the first RTAS event 
 * we receive from the kernel.
 */
char *scanlog = NULL;

#ifdef DEBUG
/**
 * @var scenario_file
 * @brief filename specified with the -s option
 */
/**
 * @var scenario_buf
 * @brief buffer of data read in from scenario_file
 */
/**
 * @var scenario_files
 * @brief Array of files listed in the scenario_file
 */
/*
 * @var scenario_index
 * @brief Current index into scenario_files
 */
/*
 * @var scenario_count
 * @brief Total number of entries in scenario_files
 */
char	*scenario_file = NULL;
char	*scenario_buf = NULL;
char	**scenario_files = NULL;
int	scenario_index = 0;
int	scenario_count = 0;

/**
 * @var testing_finished
 * @brief Indicates the last test scenario has been read.
 */
int	testing_finished = 0;

/**
 * event_dump
 * @brief Dump an RTAS event
 *
 * Mainly used for debugging porppuses to get a dump of a RTAS event
 *
 * @param re pointer to a struct event to dump.
 */
void
event_dump(struct event *event)
{
	int i;
	char *bufp = (char *)event->rtas_hdr;
	int len = event->length;

 	/* Print 16 bytes/line in hex, with a space after every 4 bytes */
 	for (i = 0; i < len; i++) {
 		if ((i % 16) == 0) 
			printf("RTAS %d:", i/16);
 
 		if ((i % 4) == 0)
			printf(" ");
 
		printf("%02x", bufp[i]);
 
 		if ((i % 16) == 15)
			printf("\n");
 	}
 	if ((i % 16) != 0)
 		printf("\n");
}

/** 
 * setup_rtas_event_scenario
 * @brief Create a testcase scenario
 * 
 * A scenario file contains a list of files containing	RTAS events 
 * (one per line, no blank lines) that are used to inject a series 
 * of RTAS events into the rtas_errd daemon.
 *		
 * If a scenario is specified this will setup an array of filenames 
 * specified by the file.
 * 
 * @return 0 on success, !0 otherwise
 */
int
setup_rtas_event_scenario(void)
{
	struct stat	sbuf;
	char	*tmp;
	int	fd, len;
	int	i;

	if (scenario_file == NULL)
		return 0;

	fd = open(scenario_file, O_RDONLY);
	if (fd == -1) {
		log_msg(NULL, "Could not open scenario file %s, %s", 
			scenario_file, strerror(errno));
		return -1;
	}

	if ((fstat(fd, &sbuf)) < 0) {
		log_msg(NULL, "Could not get status of scenario file %s, %s", 
			scenario_file, strerror(errno));	
		close(fd);
		return -1;
	}

	scenario_buf = malloc(sbuf.st_size);
	if (scenario_buf == NULL) {
		log_msg(NULL, "Could not allocate space for RTAS injection "
			"scenario, %s", strerror(errno));
		close(fd);
		return -1;
	}

	len = read(fd, scenario_buf, sbuf.st_size);
	close(fd);

	/* Now, convert all '\n' and EOF chars to '\0' */
	for (i = 0; i < sbuf.st_size; i++) {
		if (scenario_buf[i] == '\n') {
			scenario_buf[i] = '\0';
			scenario_count++;
		}
	}
	
	/* Allocate the scenario array */
	scenario_files = malloc(scenario_count * sizeof(char *));
	if (scenario_files == NULL) {
		log_msg(NULL, "Could not allocate memory for scenario "
			"files, %s", strerror(errno));
		return -1;
	}

	tmp = scenario_buf;
	dbg("Setting up RTAS event injection scenario..");
	for (i = 0; i < scenario_count; i++) {
		scenario_files[i] = tmp;
		dbg("    %s\n", scenario_files[i]);
		tmp += strlen(tmp) + 1;
	}

	proc_error_log1 = scenario_files[scenario_index++];
	proc_error_log2 = NULL;

	return 0;
}
#endif /* DEBUG */

/** 
 * init_files
 * @brief Initialize files used by rtas_errd
 *
 * Open the following files needed by the rtas_errd daemon: 
 *	rtas_errd_log
 *	proc_error_log
 *	platform_log 
 *	epow_status
 *
 * Note: This should only be called once before any rtas_events
 * are read.
 *
 * @return 0 on success, !0 on failure
 */
int
init_files(void)
{
	int rc = 0;

	/* First, open the rtas_errd log file */
	rtas_errd_log_fd = open(rtas_errd_log, O_RDWR | O_CREAT | O_APPEND,
				S_IRUSR | S_IWUSR | S_IRGRP /*0640*/);
	if (rtas_errd_log_fd == -1 && debug)
		fprintf(stderr, "Could not open rtas_errd log file \"%s\".\n",
			rtas_errd_log);

	
	/* Next, open the /proc file */
#ifdef DEBUG
	rc = setup_rtas_event_scenario();
	if (rc) 
		return rc;
#endif
	proc_error_log_fd = open(proc_error_log1, O_RDONLY);
	if (proc_error_log_fd <= 0) 
		proc_error_log_fd = open(proc_error_log2, O_RDONLY);
	
	if (proc_error_log_fd <= 0) {
		log_msg(NULL, "Could not open error log file at either %s or "
			"%s, %s\nThe rtas_errd daemon cannot continue and will "
			"exit", proc_error_log1, proc_error_log2, 
			strerror(errno));
		return -1;
	}

	/* Next, open /var/log/platform */
	platform_log_fd = open(platform_log, O_RDWR | O_SYNC | O_CREAT,
			       S_IRUSR | S_IWUSR | S_IRGRP /*0640*/);
	if (platform_log_fd <= 0) {
		log_msg(NULL, "Could not open log file %s, %s\nThe daemon "
			"cannot continue and will exit", platform_log,
			strerror(errno));
		return -1;
	}

	/* Now, the epow status file. Updating the status to zero will 
	 * have the side effect of also opening the file. 
	 */
	update_epow_status_file(0);

	return rc;
}

/**
 * close_files
 * @brief Close all the files used by rtas_errd
 * 
 * Perform any file cleanup (i.e. close()) and possibly free()'ing
 * buffers needed by rtas_errd	before exiting.  
 */
void
close_files(void)
{
#ifdef DEBUG
	if (scenario_buf != NULL)
		free(scenario_buf);
	if (scenario_files != NULL)
		free(scenario_files);
#endif
	if (rtas_errd_log_fd)
		close(rtas_errd_log_fd);
	if (proc_error_log_fd)
		close(proc_error_log_fd);
	if (platform_log_fd)
		close(platform_log_fd);
	if (epow_status_fd)
		close(epow_status_fd);
}

/** 
 * read_proc_error_log
 * @brief Read data from proc_error_log
 * 
 * Read the data in from the /proc error log file.  This routine
 * also handles the debug case of reading in a test file that
 * contains an ascii representation of a RTAS event.
 *
 * @param buf buffer to read RTAS event in to.
 * @param buflen length of buffer parameter
 * @return number of bytes read.
 */
int
read_proc_error_log(char *buf, int buflen)
{
	int len = 0;
	
#ifdef DEBUG
	/* If we are reading in a test file with an ascii RTAS event,
	 * we need to convert it to binary.  proc_error_log2 should
	 * only be NULL when a debug proc_error_log file is specified
	 */
	if (proc_error_log2 == NULL) {
		struct stat	tf_sbuf;
		char		*data;
		char		*tf_mmap;
		int		seq_num = 1000 + scenario_index;
		int		j = 0, k = 0, ch;
		char		str[4];

		if ((fstat(proc_error_log_fd, &tf_sbuf)) < 0) {
			log_msg(NULL, "Cannot get status of test file %s, %s",
				proc_error_log1, strerror(errno));
			return -1;
		}
				
		tf_mmap = mmap(0, tf_sbuf.st_size, PROT_READ, MAP_PRIVATE,
			       proc_error_log_fd, 0);
		if (tf_mmap == MAP_FAILED) {
			log_msg(NULL, "Cannot map test file %s, %s", 
				proc_error_log1, strerror(errno));
			return -1;
		}

		memcpy(buf, &seq_num, sizeof(int));
		buf += sizeof(int);

		data = tf_mmap;
		str[2] = ' ';
		str[3] = '\0';

		while (&data[j] != (tf_mmap + tf_sbuf.st_size)) {
			if (strncmp(&data[j], "RTAS:", 5) == 0) {
				/* This is either an event begin or an
				 * event end line, move on to the '\n'
				 */
				while (data[j++] != '\n');
				continue;
			}

			if (strncmp(&data[j], "RTAS ", 5) == 0) {
				/* this is fluff at the beginning
				 * of the event output line, get rid of it.
				 */

				/* first skip past the 'RTAS X:'.  The X
				 * part may be multiple digits so we have
				 * to search for the ':' afterwards.
				 */
				j += 6;
				while (data[j++] != ':');
				/* go past the extra space */
				j++;
			}
				
			if ((data[j] == '\n') || (data[j] == ' ')) {
				j++;
				continue;
			}

			str[0] = data[j];
			str[1] = data[j + 1];
			j += 2;
			sscanf(str, "%02x", &ch);
			buf[k++] = ch;
		}

		len = tf_sbuf.st_size;

		if (scenario_files != NULL) {
			if (scenario_index < scenario_count) {
				char	*tmp;
				close(proc_error_log_fd);
				tmp = scenario_files[scenario_index++];
				proc_error_log_fd = open(tmp, O_RDONLY);
				if (proc_error_log_fd <= 0)
					log_msg(NULL, "Could not open scenario "
						"file %s, %s", tmp,
						strerror(errno));
			} 
			else {
				testing_finished = 1;
			}
		} 
		else
			testing_finished = 1;

		if (tf_mmap)
			munmap(tf_mmap, tf_sbuf.st_size);
	} 
	else
#endif
		len = read(proc_error_log_fd, buf, buflen);

	/* event_dump(event); */
	return len;
}

/**
 * reformat_msg
 * @brief Re-format a log message to wrap at 80 characters.
 *
 * In order to ease the formatting of messages in the rtas_errd daemon
 * this will automatically format the messages to wrap at 80 characters.
 *
 * @param msg buffer containing the message to re-format
 * @return new buffer length
 */
int
reformat_msg(char *msg)
{
	char	buf[RTAS_ERROR_LOG_MAX], *pos, *next, temp, temp2;
	int	len = strlen(msg);

	if (len < 80) {
		/* no need to reformat */
		msg[len++] = '\n';
		return len;
	}

	memset(buf, 0, RTAS_ERROR_LOG_MAX);
	
	/* first copy the msg into our buffer */
	memcpy(buf, msg, len);

	/* zero out the old buffer */
	memset(msg, 0, RTAS_ERROR_LOG_MAX);

	pos = buf;
	while (strlen(pos) > 80) {
		next = pos+80;
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

		if (next == pos) {	/* word is longer than line length */
			next = pos+79;
			temp = *next;
			temp2 = *(next + 1);
			*next = '\n';
			*(next + 1) = '\0';
			strcat(msg, pos);
			*next = temp;
			*(next + 1) = temp2;
			pos = next;
		}
	}
	strcat(msg, pos);
	len = strlen(msg);
	msg[len++] = '\n';

	return len;
}

/**
 * _log_msg
 * @brief The real routine to write messages to rtas_errd_log
 * 
 * This is a common routine for formatting messages that go to the
 * /var/log/rtas_errd.log file.  Users should pass in a reference to
 * the rtas_event structure if this message is directly related a rtas
 * event and a formatted message a la printf() style.  Please make sure
 * that the message passed in does not have any ending punctuation or
 * ends with a newline.  It sould also not have any internal newlines.
 *
 * This routine will do several things to the message before printing
 * it out;
 * - Add a timestamp 
 * - If a rtas_event reference is passed in, a sequenbce number is added
 * - If errno is set, the results of perror are added.
 * - The entire message is then formatted to fit in 80 cols. 
 * 
 * @param event reference to event
 * @param fmt formatted string a la printf()
 * @param ... additional args a la printf()
 */
static void 
_log_msg(struct event *event, const char *fmt, va_list ap)
{
	struct stat sbuf;
	char	buf[RTAS_ERROR_LOG_MAX];
	int	len = 0, rc;

	if (rtas_errd_log_fd == -1) {
		dbg("rtas_errd log file is not available");
		
		vsprintf(buf, fmt, ap);
		_dbg(buf);
	
		return;
	}

#ifndef DEBUG
	{
		time_t	cal;
		/* In order to make testing easier we don't print the date
		 * to the log file for debug versions of rtas_errd.  This helps
		 * avoid lots of ugly date munging when comparing files.
		 */
		cal = time(NULL);
		len = sprintf(buf, "%s ", ctime(&cal));
	}
#endif

	/* Add the sequence number */
	if (event)
		len += sprintf(buf, "(Sequence #%d) ",
			       event->seq_num);

	/* Add the actual message */
	len += vsprintf(buf + len, fmt, ap);
	
	/* Add ending punctuation */
	len += sprintf(buf + len, ".");

	_dbg(buf);

	/* reformat the new message */
	len = reformat_msg(buf);

	rc = write(rtas_errd_log_fd, buf, len);
	if (rc == -1)
		dbg("rtas_errd: write to rtas_errd log failed");
	
	/* After each write check to see if we need to to do log rotation */
	rc = fstat(rtas_errd_log_fd, &sbuf);
	if (rc == -1) {
		dbg("rtas_errd: Cannot stat rtas_errd log file " 
		    "to rotate logs");
		return;
	}

	if (sbuf.st_size >= RTAS_ERRD_LOGSZ) {
		char	cmd_buf[1024];
		int	rc;
		int8_t	status;

		close(rtas_errd_log_fd);

		memset(cmd_buf, 0, 1024);
		sprintf(cmd_buf, "rm -f %s; mv %s %s", rtas_errd_log0,
			rtas_errd_log, rtas_errd_log0); 
		rc = system(cmd_buf);
		if (rc == -1) {
			status = WEXITSTATUS(rc);
			log_msg(NULL, "An error occured during the rotation of "
				"the rtas_errd logs:\n cmd = %s\nexit status: "
				"%d)", cmd_buf, status);
		}

		rtas_errd_log_fd = open(rtas_errd_log, 
					O_RDWR | O_CREAT | O_TRUNC,
					S_IRUSR | S_IWUSR | S_IRGRP);
		if (rtas_errd_log_fd == -1)
			dbg("Could not re-open %s", rtas_errd_log);
	}

	return;
}

/**
 * cfg_log
 * @brief dummy interface for calls to diag_cfg
 *
 * @param fmt formatted string a la printf()
 * @param ... additional args a la printf()
 */
void
cfg_log(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_log_msg(NULL, fmt, ap);
	va_end(ap);
}

/**
 * log_msg
 * @brief Log messages to rtas_errd_log
 *
 * @param event reference to event
 * @param fmt formatted string a la printf()
 * @param ... additional args a la printf()
 */
void
log_msg(struct event *event, char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_log_msg(event, fmt, ap);
	va_end(ap);
}

/**
 * dbg
 * @brief Write debug messages to stdout
 * 
 * Provide utility to print debug statements if the debug flag 
 * is specified.  
 *
 * @param fmt format string a la printf()
 * @param ... args a la printf()
 */
void 
_dbg(const char *fmt, ...)
{
	va_list ap;
	char	buf[RTAS_ERROR_LOG_MAX + 8];
	int	len;

	if (debug == 0)
		return;

	memset(buf, 0, sizeof(buf));

	va_start(ap, fmt);
	len = sprintf(buf, "DEBUG: ");
	len += vsnprintf(buf + len, RTAS_ERROR_LOG_MAX, fmt, ap);
	buf[len] = '\0';
	va_end(ap);

	len = reformat_msg(buf);
	fprintf(stdout, buf);
	fflush(stdout);
}

/**
 * print_rtas_event
 * @brief Print an RTAS event to the platform log
 * 
 * Prints the binary hexdump of an RTAS event to the PLATFORM_LOG file.
 * 
 * @param event pointer to the struct event to print
 * @return 0 on success, !0 on failure
 */
int 
print_rtas_event(struct event *event)
{
	char	*out_buf;
	int	len, buf_size, offset;
	int	i, rc, nlines;

	/* Determine the length of the log */
 	len = event->length;

	/* Some RTAS events (i.e. shortened epow events) only consist
	 * of the rtas header and an extended length of zero.  For these
	 * assume a length of 32.
	 */
	if (len == 0)
		len = 32;

	/* We need to allocate a buffer big enough to hold all the data
	 * from the RTAS event plus the additional strings printed
	 * before and after the RTAS event (roughly 0x40 in length)
	 * _and_ the strings printed in front of every line (roughly 0x10
	 * in length).  
	 *
	 * We calculate the number of lines needed to print the RTAS event, 
	 * mulitply that by the space for the strings then add in the 
	 * beginning/end strings.  For a little extra padding we double the 
	 * len value also.
	 *
	 * size = (len * 2) + (number of lines * 0x10) + (2 * 0x40)
	 */
	nlines = len / 0x10;
	if (len % 0x10)
		nlines++;
	buf_size = (len * 2) + (nlines * 0x10) + (2 * 0x40);
	/* event_dump(event); */

	out_buf = malloc(buf_size); 
	if (out_buf == NULL) {
		log_msg(NULL, "Could not allocate buffer to print RTAS event "
			"%d, %s.  The event will not copied to %s", 
			event->seq_num, strerror(errno),
			platform_log);
		return -1;
	}
	memset(out_buf, 0, buf_size);
	       
	offset = sprintf(out_buf, 
			 "RTAS: %d -------- RTAS event begin --------\n", 
			 event->seq_num);

	if (event->flags & RE_SCANLOG_AVAIL) {
		offset += sprintf(out_buf + offset, "RTAS: %s\n", scanlog);
		free(scanlog);
		scanlog = NULL;
	}
	
 	/* Print 16 bytes/line in hex, with a space after every 4 bytes */
 	for (i = 0; i < len; i++) {
 		if ((i % 16) == 0)
 			offset += sprintf(out_buf + offset, "RTAS %d:", i/16);
 
 		if ((i % 4) == 0)
 			offset += sprintf(out_buf + offset, " ");
 
 		offset += sprintf(out_buf + offset, "%02x", 
				  event->event_buf[i]); 
 
 		if ((i % 16) == 15) 
			offset += sprintf(out_buf + offset, "\n");
 	}
 	if ((i % 16) != 0)
 		offset += sprintf(out_buf + offset, "\n");
	 
 	offset += sprintf(out_buf + offset,
			  "RTAS: %d -------- RTAS event end ----------\n", 
			  event->seq_num);

	dbg("Writing RTAS event %d to %s", event->seq_num, platform_log);
	lseek(platform_log_fd, 0, SEEK_END);
	rc = write(platform_log_fd, out_buf, offset);
	if (rc != offset) {
		log_msg(NULL, "Writing RTAS event %d to %s failed."
			"expected to write %d, only wrote %d. %s", 
			event->seq_num, platform_log, offset, rc, 
			strerror(errno));
	}

	free(out_buf);
	return rc;
}

/** 
 * platform_log_write
 * @brief Write messages to the platform log
 * 
 * Provide a printf() style interface to write messages to platform_log.  
 * All messages are prepended with "ppc64-diag" to match the expected
 * format of the platform log.
 *
 * @param fmt format string a la printf()
 * @param ... additional args a la printf()
 * @return return code from write() call
 */
int
platform_log_write(char *fmt, ...)
{
	va_list	ap;
	char	buf[1024]; /* should be big enough */
	int	len, rc;

	len = sprintf(buf, "%s ", "ppc64-diag:");

	va_start(ap, fmt);
	len += vsprintf(buf + len, fmt, ap);
	va_end(ap);

	rc = write(platform_log_fd, &buf, len);

	return rc;
}

/**
 * update_epow_status_file
 * @brief Update the epow status file.
 * 
 * Used to write the current EPOW status (as defined in the 
 * parse_epow() function (epow.c) comment) to the epow_status file.
 * 
 * @param status value to update epow_status file to.
 */
void
update_epow_status_file(int status)
{
	int	flags = O_CREAT | O_WRONLY | O_TRUNC;
	int	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH /* 0644 */;
        int	size;
        char	buf[15];

        if (epow_status_fd < 0) {
		epow_status_fd = open(epow_status_file, flags, mode);
		if (epow_status_fd <= 0) {
			log_msg(NULL, "Could not open EPOW status file %s, %s",
				epow_status_file, strerror(errno));
			return;
		}
	}

	if (lseek(epow_status_fd, 0, SEEK_SET) == -1) {
		log_msg(NULL, "Could not seek in EPOW status file %s, %s",
			epow_status_file, strerror(errno));
		return;
	}

        size = sprintf(buf, "%d", status);
        if (size > (sizeof(buf)-1))  {
		log_msg(NULL, "Could not write data to EPOW status file %s, %s",
			epow_status_file, strerror(errno));
		return;
	}

        if (write(epow_status_fd, buf, size) < size) {
		log_msg(NULL, "Could not write data to EPOW status file %s, %s",
			epow_status_file, strerror(errno));
	}

	return;
}

