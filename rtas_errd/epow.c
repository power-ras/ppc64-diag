/**
 * @file epow.c
 * @brief Routines to handle EPOW RTAS events
 *
 * Copyright (C) 2004 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <librtas.h>
#include <librtasevent.h>
#include "rtas_errd.h"

/* Sensor tokens, from RPA section 7.3.7.5 (get-sensor-state) */
#define SENSOR_TOKEN_KEY_SWITCH			1
#define SENSOR_TOKEN_ENCLOSURE_SWITCH		2
#define SENSOR_TOKEN_THERMAL_SENSOR		3
#define SENSOR_TOKEN_POWER_SOURCE		5
#define SENSOR_TOKEN_EPOW_SENSOR		9

/* File paths */
#define EPOW_PROGRAM		"/etc/rc.powerfail"
#define EPOW_PROGRAM_NOPATH	"rc.powerfail"

/**
 * @var time_remaining
 * @brief Time remaining until a system shutdown.
 * 
 * The time_remaining variable is used by the epow timer
 * to track the time remaining until a system shutdown
 */
static int time_remaining = 0;

/**
 * epow_timer_handler
 * @brief Routine to handle SIGALRM timer interrupts.
 *
 * @param sig unused
 * @param siginfo unused
 * @param context unused
 */
void 
epow_timer_handler(int sig, siginfo_t siginfo, void *context)
{
	int rc, state;
	struct itimerval tv;

	if (time_remaining <= 0) {
		/*
		 * Time is up; disable the interval timer.
		 * The EPOW_PROGRAM should have already shut the 
		 * system down by this point.
		 */
		tv.it_interval.tv_sec = 0;
		tv.it_interval.tv_usec = 0;
		tv.it_value = tv.it_interval;
		rc = setitimer(ITIMER_REAL, &tv, NULL);
		return;
	}

	rc = rtas_get_sensor(SENSOR_TOKEN_EPOW_SENSOR, 0, &state);

	if (state < RTAS_EPOW_ACTION_SYSTEM_SHUTDOWN) {
		/* 
		 * Problem resolved; disable the interval timer and
		 * update the epow status file.
		 */ 
		tv.it_interval.tv_sec = 0;
		tv.it_interval.tv_usec = 0;
		tv.it_value = tv.it_interval;
		rc = setitimer(ITIMER_REAL, &tv, NULL);

		if (state == RTAS_EPOW_ACTION_RESET)
			update_epow_status_file(0);
		else if (state == RTAS_EPOW_ACTION_WARN_COOLING)
			update_epow_status_file(8);
		else if (state == RTAS_EPOW_ACTION_WARN_POWER)
			update_epow_status_file(9);

		time_remaining = 0;
		return;
	}

	/*
	 * If we make it this far, the problem (or a worse one) 
	 * still exists. If the problem is worse, the system will 
	 * probably shut down within 20 seconds.  We shouldn't 
	 * overwrite the status so that if a worse problem exists, 
	 * it can be handled appropriately.
	 */
	time_remaining -= 5;
	return;
}

static void
log_epow(struct event *event, char *fmt, ...)
{
	va_list ap;
	char	buf[1024];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	log_msg(event, buf);
	snprintf(event->addl_text, ADDL_TEXT_MAX, buf);
}

/**
 * parse_epow
 * @brief Retrieve the epow status from an RTAS event.
 *
 * This routine returns the value that will be written to the epow_status 
 * file.  The value will be used by the EPOW_PROGRAM script (which can 
 * be modified by a system administrator) to establish actions to be 
 * taken for certain EPOW conditions.  The return value must be 7 digits 
 * or less.
 * 
 * @param event pointer to the RTAS event
 * @return
 * <pre>
 *  Return Value					Action
 *  ==========================			=====================
 *  0   normal state				none
 *  1   loss of primary power			4 ms to shutdown
 *  2   power supply failure			4 ms to shutdown
 *  3   manual power-off switch	activated	4 ms to shutdown
 *  4   thermal high				20 seconds to shutdown
 *  5   primary fan failure			20 seconds to shutdown
 *  6   secondary fan failure			10 minutes to shutdown
 *  7   battery backup				10 minutes to shutdown
 *  8   non-critical cooling problem		none
 *  9   non-critical power problem		none
 *  10  (v6) normal shutdown requested		initiate shutdown immediately
 *  11  (v6) loss of critical system functions	initiate shutdown immediately
 * </pre>
 *
 */
static int 
parse_epow(struct event *event)
{
	struct rtas_event_hdr *rtas_hdr = event->rtas_hdr;
	struct rtas_epow_scn *epow;
	struct itimerval tv;
	char	*event_type;
	int	rc, state;

	/*
	 * Check the sensor state;  this will be used to ensure
	 * that the problem in question is current, and isn't being retrieved
	 * from NVRAM after a reboot when there wasn't time to handle the
	 * problem (e.g. the power cable was yanked).
	 */
	rc = rtas_get_sensor(SENSOR_TOKEN_EPOW_SENSOR, 0, &state);

        if (rtas_hdr->extended == 0) {
		if (state > 0) {
			/* Assume a system halt */
			log_epow(event, "Recieved shortened EPOW event, "
				 "assuming power failure");
			return 10;
		} 
		else {
			log_epow(event, "Received shortened EPOW event, EPOW "
				"sensor not in shutdown mode");
			return 0;
		}
        }

	epow = rtas_get_epow_scn(event->rtas_event);
	if (epow == NULL) {
		log_msg(event, "Could not retrieve EPOW section to handle "
			"incoming EPOW event, skipping");
		return 0;
	}

	switch (epow->action_code) {
	    case RTAS_EPOW_ACTION_RESET:
		log_epow(event, "Received EPOW action code reset (no action)");
		return 0;
		break;
		
	    case RTAS_EPOW_ACTION_WARN_COOLING:
		log_epow(event, "Received non-critical cooling EPOW action "
			 "code (8)");
		return 8;
		break;
		
	    case RTAS_EPOW_ACTION_WARN_POWER:
		log_epow(event, "Received non-critical power EPOW action "
			 "code (9)");
		return 9;
		break;
		
	    case RTAS_EPOW_ACTION_SYSTEM_SHUTDOWN:
		if (state != RTAS_EPOW_ACTION_SYSTEM_SHUTDOWN) {
			log_epow(event, "EPOW event: system shutdown "
				 "requested, but the sensor state indicates "
				 "the problem was resolved");
			return 0;
		}

		/*
		 * We have 10 minutes to shutdown.  If we are already running
		 * an interval timer we'll just just increase the length of 
		 * time we check for status updates instead of starting a new
		 * timer.
		 */
		if (time_remaining <= 0) {
			/* Set up an interval timer to update the epow 
			 * status file every 5 seconds.
			 */
			tv.it_interval.tv_sec = 5;
			tv.it_interval.tv_usec = 0;
			tv.it_value = tv.it_interval;
			rc = setitimer(ITIMER_REAL, &tv, NULL);
		}

		time_remaining = 600;	/* in seconds */

		if (epow->fan) {
			log_epow(event, "EPOW event: system shutdown will "
				 "begin in %d seconds due to a fan failure", 
				 time_remaining);
			return 6;
		}
		
		if (epow->power_fault) {
			log_epow(event, "EPOW event: system shutdown will "
				 "begin in %d seconds due to a power fault", 
				 time_remaining);
			return 7;
		}
			
		if (epow->event_modifier == RTAS_EPOW_MOD_NORMAL_SHUTDOWN) {
			log_epow(event, "EPOW event: system shutdown will "
				 "begin in %d seconds due to a normal "
				 "shutdown request", time_remaining);
			return 10;
		}
		
		if (epow->event_modifier == RTAS_EPOW_MOD_UTILITY_POWER_LOSS) {
			log_epow(event, "EPOW event: system shutdown will "
				 "begin in %d seconds due to a battery "
				 "failure", time_remaining);
			return 7;
		}
		
		if (epow->event_modifier == RTAS_EPOW_MOD_CRIT_FUNC_LOSS) {
			log_epow(event, "EPOW event: system shutdown will "
				 "begin in %d seconds due to a loss of system "
				 "critical functions", time_remaining);
			return 11;
		}

#ifdef RTAS_EPOW_MOD_AMBIENT_TEMP
		if (epow->event_modifier == RTAS_EPOW_MOD_AMBIENT_TEMP) {
			log_epow(event, "EPOW event: system shutdown will "
				 "begin in %d seconds due to ambient "
				 "temperature being too high", time_remaining);
			return 4;
		}
#endif

		log_epow(event, "EPOW event: system shutdown will begin in %d "
			 "seconds due to an unknown cause", time_remaining);
		
		return 7;
		break;
		
	    case RTAS_EPOW_ACTION_SYSTEM_HALT:
		if (state != RTAS_EPOW_ACTION_SYSTEM_HALT) {
			log_epow(event, "EPOW event: system halt requested, "
				 "but the sensor state indicates the problem "
				 "was resolved");
			return 0;
		}

		if (rtas_hdr->version < 6) {
			if (epow->fan) {
				log_epow(event, "EPOW event: system halting "
					 "due to a fan failure");
				return 5;
			}
			if (epow->temp) {
				log_epow(event, "EPOW event: system halting "
					 "due to a thermal condition");
				return 4;
			}

			log_epow(event, "EPOW event: system halting due to "
				 "an unknown condition");
			return 4;
		} 
		else {
			log_epow(event, "EPOW event: system halting due to "
				 "an unspecified condition");
			return 4;
		}

		break;

	    case RTAS_EPOW_ACTION_MAIN_ENCLOSURE:
		event_type = "main enclosure";
		/* fall through */
	    case RTAS_EPOW_ACTION_POWER_OFF:
		event_type = "power off";
		if (state != RTAS_EPOW_ACTION_MAIN_ENCLOSURE && 
		    state != RTAS_EPOW_ACTION_POWER_OFF) {
			log_epow(event, "EPOW event: %s requested, but the "
				 "sensor state indicates the problem was "
				 "resolved", event_type);
			return 0;
		}

		/* System will likely lose power before this code is ever run */
		if ((rtas_hdr->version < 6) && (epow->power_fault)) {
			if (epow->power_loss) {
				log_epow(event, "EPOW event: power loss due "
					 "to a power source fault");
				return 1;
			}
			if (epow->power_supply) {
				log_epow(event, "EPOW event: power loss due "
					 "to a power failure");
				return 2;
			}
			if (epow->power_switch) {
				log_epow(event, "EPOW event: power loss due "
					 "to a power switch fault");
				return 3;
			}
		} 
		else {
			log_epow(event, "EPOW event: power loss due to a %s "
				 "event", event_type);
			return 1;
		}

		if (epow->action_code == RTAS_EPOW_ACTION_MAIN_ENCLOSURE)
			log_epow(event, "EPOW event: power loss due to an "
				 "unknown main enclosure condition");
		else
			log_epow(event, "EPOW event: power loss due to an "
				 "unknown poer supply condition");
		 
		return 2;
		break;

	    default:
		/* Unknown action code */
		log_epow(event, "EPOW event: unknown action code (%d); "
			 "event cannot be handled", epow->action_code);
		return 0;
		break;
	}

	return 0;
}

/**
 * check_epow
 * @brief Check an RTAS event for EPOW data.
 *
 * Parses error information to determine if it represents an EPOW event.  
 * If it is, the epow_status_file is updated with the appropriate 
 * condition, and EPOW_PROGRAM is invoked to take the appropriate system 
 * action (shutdown, etc).
 * 
 * @param event pointer to the RTAS event
 */
void 
check_epow(struct event *event)
{
	pid_t	child;
	char	*childargs[2];
	int	rc, current_status;

	/*
	 * Dissect the EPOW extended error information;
	 * if the error is serious enough to warrant further action,
	 * fork and exec the script to handle it
	 */
	current_status = parse_epow(event); 
	update_epow_status_file(current_status);

	if (current_status > 0) {
		childargs[0] = EPOW_PROGRAM_NOPATH;
		childargs[1] = NULL;
		child = fork();
		if (child == -1) {
			log_msg(event, "Fork error, %s cannot be run to handle"
				"an incoming EPOW event, %s", EPOW_PROGRAM,
				strerror(errno));
		} 
		else if (child == 0) {
			/* child process */
			rc = execv(EPOW_PROGRAM, childargs);

			/* shouldn't get here */
			log_msg(event, "Could not exec %s, %s.  Could not "
				"handle an incoming EPOW event", EPOW_PROGRAM,
				strerror(errno));
			exit(1);
		}
	}

	return;
}
