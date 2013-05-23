/**
 * @file rtas_errd.c
 * @brief Main entry point for rtas_errd.
 *
 * Copyright (C) 2004 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <librtas.h>
#include "rtas_errd.h"

/**
 * @var debug
 * @brief Debug level to run at for rtas_errd daemon
 */
int debug = 0;

#ifdef DEBUG
/**
 * @var no_drmgr
 * @brief specifies if the '--nodrmgr' flag was specified
 */
int no_drmgr = 0;
/**
 * @var db_dir
 * @brief Specify an alternate path for the servicelog files
 */
char *db_dir = NULL;
#else
const char *db_dir = NULL;
#endif

/**
 * @var slog
 * @brief servicelog struct for libservicelog use
 */
struct servicelog *slog = NULL;

/**
 * daemonize
 * @brief daemonize rtas_errd
 * 
 * Convert the rtas_errd process to a daemon.
 */
static void
daemonize(void)
{
	pid_t pid;
	int   i;

	/* Fork a child process to be the real daemon */
	pid = fork();
	if (pid < 0) 
		goto daemonize_error;
	else if (pid != 0)
		exit(0); /* bye-bye parent */

	/* make ourselves the session leader */
	pid = setsid();
	if (pid == -1)
		goto daemonize_error;

	/* change to a safe dir */
	chdir("/");

	/* clear file mode mask */
	umask(0);

	/* Close all file descriptors we have open and reopen descriptors
	 * 0-2 to redirect to /dev/null
	 */
	for (i = getdtablesize(); i >= 0; --i) 
		close(i);

	i = open("/dev/null", O_RDWR);
	dup(i);
	dup(i);

	return;

daemonize_error:
	/* use fprintf here, the rtas_errd log file has not been set up yet */
	fprintf(stderr, "Cannot daemonize rtas_errd, check system process "
		"usage.\nrtas_errd cannot continue.\n");
	exit(1);
}

/**
 * handle_rtas_event
 * @brief Main routine for processing RTAS events.
 *
 * @param event RTAS event structure to be handled
 */
int
handle_rtas_event(struct event *event)
{
	int rc = 0;
	struct rtas_event_exthdr *exthdr;

	dbg("Handling RTAS event %d", event->seq_num);
		
	/*
	 * check to determine if this is a platform dump notification,
	 * which requires the dump to be copied to the OS;  this must
	 * be done before the error log is written to disk, because
	 * the log will be updated with the path to the dump
	 */
	dbg("Entering check_patform_dump()");
	check_platform_dump(event);

	/* write the event to the platform file */
	rc = print_rtas_event(event);
	if (rc <= 0) {
		log_msg(event, "Could not write RTAS event %d to log file %s", 
			event->seq_num, platform_log);
		log_msg(event, "Rtas_errd is exiting to preserve the current "
			"RTAS event in nvram due to a failed write to %s",
			platform_log);
		return -1;	
	}

	switch (event->rtas_hdr->type) {
	    case RTAS_HDR_TYPE_CACHE_PARITY:
	    case RTAS_HDR_TYPE_RESOURCE_DEALLOC:
		dbg("Entering handle_resource_dealloc()");
		handle_resource_dealloc(event);
		break;
			    
	    case RTAS_HDR_TYPE_EPOW:
		dbg("Entering check_epow()");
		check_epow(event);
		break;

	    case RTAS_HDR_TYPE_PLATFORM_ERROR:
	    case RTAS_HDR_TYPE_PLATFORM_INFO:
		dbg("Entering check_eeh()");
		check_eeh(event);
		break;

	    case RTAS_HDR_TYPE_DUMP_NOTIFICATION:
		/* handled above in check_platform_dump()*/
		break;

	    case RTAS_HDR_TYPE_PRRN:
		dbg("Entring PRRN handler");
		handle_prrn_event(event);

		/* Nothing left to do for PRRN Events, there is no exthdr
		 * for these events and they are not a serviceable event
		 * and as such do not need to be logged.
		 */
		return 0;

	    default:
		/* Nothing to do for this event */
		break;
	}

	exthdr = rtas_get_event_exthdr_scn(event->rtas_event);
	if (exthdr == NULL) {
		log_msg(event, "Could not retrieve extended event data");
		return 0;
	}

	if (event->rtas_hdr->severity == RTAS_HDR_SEV_ALREADY_REPORTED)
		event->flags |= RE_ALREADY_REPORTED;

	if (exthdr->recoverable)
		event->flags |= RE_RECOVERED_ERROR;

	if (exthdr->predictive)
		event->flags |= RE_PREDICTIVE;

	if (event->rtas_hdr->version == 6)
		process_v6(event);
	else
		process_pre_v6(event);

	/* Log the event in the servicelog DB */
	log_event(event);

#if 0
	if (event->flags & RE_ALREADY_REPORTED) {
		platform_log_write("Event %d has already been reported, for "
				   "further details see\n", event->seq_num);
		platform_log_write("the previous report of this error or the"
				   "servicelog.\n");
	}

	if (event->flags & RE_RECOVERED_ERROR) {
		platform_log_write("Event %d is a recovered error "
				   "notification, further details are\n",
				   event->seq_num);
		platform_log_write("available in the servicelog.\n");
	}
#endif

	return 0;
}

/**
 * read_rtas_event
 * @brief Main routine to retrieve RTAS events from the kernel
 * 
 * Responsable for reading RTAS events from the kernel (via /proc)
 * and calling handle_rtas_event() to process the event.
 */
int
read_rtas_events()
{
	struct event event = {0};
	ssize_t len;
	int retries = 0;

	memset(&event, 0, sizeof(event));

	while (1) {
		/*
		 * Passing a reference to re to the read routine is correct.
		 * see rtas_errd.h for details.
		 */
		len = read_proc_error_log((char *)&event, RTAS_ERROR_LOG_MAX);
		if (len <= 0) {
			retries++;
			if (retries >= 3) {
				log_msg(NULL, "Could not read error log file");
				return -1;
			}
			continue;
		}
		
		retries = 0;

		event.rtas_event = parse_rtas_event(event.event_buf, len);
		if (event.rtas_event == NULL) {
			log_msg(&event, "Could not parse RTAS event");
			return -1;
		}

		event.rtas_hdr = rtas_get_event_hdr_scn(event.rtas_event);
		if (event.rtas_hdr == NULL) {
			log_msg(&event, "Could not retrieve event header");
			cleanup_rtas_event(event.rtas_event);
			return -1;
		}

		event.length = event.rtas_event->event_length;

		if (scanlog != NULL)
			event.flags |= RE_SCANLOG_AVAIL;

                dbg("Received RTAS event %d", event.seq_num);

		/*
		 * Mark ourselves as not being able to handle SIGHUP
		 * signals while handling the RTAS event
		 */
		d_cfg.flags &= ~RE_CFG_RECFG_SAFE;
		handle_rtas_event(&event);
		d_cfg.flags |= RE_CFG_RECFG_SAFE;

		if (d_cfg.flags & RE_CFG_RECEIVED_SIGHUP) {
			diag_cfg(1, &cfg_log);
			d_cfg.flags &= ~RE_CFG_RECEIVED_SIGHUP;
		}

		/* cleanup the RTAS event */
		if (event.loc_codes != NULL)
			free(event.loc_codes);
		free_diag_vpd(&event);
		cleanup_rtas_event(event.rtas_event);
		memset(&event, 0, sizeof(event));

#ifdef DEBUG
		/*
		 * If we are reading a fake rtas event from a test file
		 * we only want to read it once
		 */
		if (testing_finished) 
			break;
#endif
	}

	return 0;
}

static struct option longopts[] = {
{
	.name = "debug",
	.has_arg = 0,
	.flag = NULL,
	.val = 'd'
},
#ifdef DEBUG
{
	.name = "epowfile",
	.has_arg = 1,
	.flag = NULL,
	.val = 'e'
},
{
	.name = "file",
	.has_arg = 1,
	.flag = NULL,
	.val = 'f'
},
{
	.name = "logfile",
	.has_arg = 1,
	.flag = NULL,
	.val = 'l'
},
{
	.name = "msgsfile",
	.has_arg = 1,
	.flag = NULL,
	.val = 'm'
},
{
	.name = "nodrmgr",
	.has_arg = 0,
	.flag = NULL,
	.val = 'R'
},
{
	.name = "platformfile",
	.has_arg = 1,
	.flag = NULL,
	.val = 'p'
},
{
	.name = "scenario",
	.has_arg = 1,
	.flag = NULL,
	.val = 's'
},
#endif
{
	.name = NULL,
	.has_arg = 0,
	.flag = NULL,
	.val = 0
}
};
		
	
/**
 * main
 * 
 * The main purpose of main() for the rtas_errd daemon is to parse
 * any command line options to the daemon, setup any signal handlers
 * and initialize all files needed for operation.
 */
int 
main(int argc, char *argv[])
{
	struct sigaction sigact;
	int rc = 0;
	int c;
#ifdef DEBUG
	int f_flag = 0, s_flag = 0;
#endif

	while ((c = getopt_long(argc, argv, RTAS_ERRD_ARGS, 
				longopts, NULL)) != EOF) {
		switch(c) {
			case 'd':
				debug++;
				break;
#ifdef DEBUG 
			case 'c': /* ppc64-diag config file */
				config_file = optarg;
				break;

			case 'e': /* EPOW update file */
				epow_status_file = optarg;
				break;

			case 'f': /* RTAS test event */
				if (s_flag) {
					dbg("Only use the -f or -s flag");
					goto error_out;
				}

				f_flag++;
				proc_error_log1 = optarg;
				proc_error_log2 = NULL;
				break;
				
			case 'l': /* debug rtas_errd.log file */
				rtas_errd_log = optarg;
				break;
				
			case 'm': /* debug messages file */
				messages_log = optarg;
				break;
				
			case 'p': /* debug platform file */
				platform_log = optarg;
				break;
				
			case 's': /* RTAS test scenario */
				if (f_flag) {
					dbg("Only use the -f or -s flag");
					goto error_out;
				}

				s_flag++;
				scenario_file = optarg;
				break;

			case 'R': /* No drmgr */
				no_drmgr = 1;
				break;
#endif
			default:
				return -1;
		}
	}

	/*
	 * The debug option can be specified multiple times which will
	 * indicate the debug level to set for librtas.
	 * rtas_set_debug(1) if debug level >= 2,
	 * rtas_set_debug(2) if debug level >= 3, ...
	 */
	if (debug - 1)
		rtas_set_debug(debug - 1);

	if (! debug)
		daemonize();

	/* Initialize all the files used by rtas_errd */ 
	rc = init_files();
	if (rc)
		goto error_out;

	/* Set up a signal handler for SIGALRM to handle EPOW events */
	sigact.sa_handler = (void *)epow_timer_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sigact, NULL)) {
		log_msg(NULL, "Could not initialize signal handler for "
			"certian EPOW events (SIGALRM), %s", strerror(errno));
		goto error_out;
	}

	/* Set up a signal handler for SIGHUP to re-read the config file */
	sigact.sa_handler = (void *)sighup_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGHUP, &sigact, NULL)) {
		log_msg(NULL, "Could not initialize signal handler for "
			"re-reading the ppc64-diag config file (SIGHUP), %s",
			strerror(errno));
	}

	/* Ignore SIGPIPE */
	sigact.sa_handler = SIG_IGN;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGPIPE, &sigact, NULL)) {
		log_msg(NULL, "Cannot ignore SIGPIPE, %s", strerror(errno));
	}

	/* Set up a signal handler for SIGCHLD to handle terminating children */
	setup_sigchld_handler();

	/* Read any configuration options from the config file */
	rc = diag_cfg(1, &cfg_log);
	if (rc)
		goto error_out;

	/* Open the servicelog database */
	rc = servicelog_open(&slog, 0);
	if (rc) {
		log_msg(NULL, "Could not open the servicelog database, events "
			"will only be written to /var/log/platform.\n%s\n",
			strerror(rc));
		slog = NULL;
	}

	/* update the RTAS events from /var/log/messages */
	update_rtas_msgs();

#if 0
	/*
	 * Get amount of filesystem space (in bytes) to reserve for 
	 * platform dumps via the ibm,get-system-parameter RTAS call.
	 */
	rc = rtas_get_sysparm (TOKEN_PLATDUMP_MAXSIZE, 10, data);
	if (rc != 0) {
		log_msg(NULL, "Librtas returned an error code: %d"
			"The minimum amount of space to reserve for "
			"platform dumps could not be established", rc);
		return;
	}
#endif

#if 0
	/* 
	 * Check to see if a new scanlog dump is available;  if so, copy it to
	 * the filesystem and associate the dump with the first error processed.
	 */
	check_scanlog_dump();
#endif

	rc = read_rtas_events();

error_out:
	errno = 0;
	log_msg(NULL, "The rtas_errd daemon is exiting");
	close_files();

	if (slog != NULL)
		servicelog_close(slog);

	return rc;
}
