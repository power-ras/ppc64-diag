/**
 * @file guard.c
 * @brief Routines to handle CPU guard RTAS events
 *
 * Copyright (C) 2004 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include "rtas_errd.h"

#define DRMGR_PROGRAM		"/usr/sbin/drmgr"
#define DRMGR_PROGRAM_NOPATH	"drmgr"
#define CONVERT_DT_PROPS_PROGRAM "/usr/sbin/convert_dt_node_props"

#define RTAS_V6_TYPE_RESOURCE_DEALLOC	0xE3

enum resource_dealloc_type {CPU_GUARD, SP_CPU_GUARD, MEM_PAGE, MEM_LMB};
enum event_type {CPUTYPE, MEMTYPE};

/**
 * run_drmgr
 * @brief build correct options followed by fork and exec drmgr
 *
 * @param resource type to deallocate.
 * @param specific drc_name to be de-allocted.
 * @param either quatity or capacity to be deallocated.
 * @param wait do we do a waitpid?
 */
void
run_drmgr(enum resource_dealloc_type resource_type, char *drc_name,
	  unsigned int value, int wait)
{
	pid_t child;
	int status, rc;
	char capacity[6], quant_str[5];
	char *drmgr_args[] = {DRMGR_PROGRAM_NOPATH, "-r", "-c", NULL,
			NULL, NULL, NULL, NULL, NULL};

	dbg("in run_drmgr command: %d %s %d %d", resource_type, drc_name,
	     value, wait);

	if (resource_type == CPU_GUARD) {
		drmgr_args[3] = "cpu";
		if (drc_name != NULL) {
			drmgr_args[4] = "-s";
			drmgr_args[5] = drc_name;
		} else {
			drmgr_args[4] = "-q";
			snprintf(quant_str, 5, "%d", value);
			drmgr_args[5] = quant_str;
		} /* end else */
	}
	else if (resource_type == SP_CPU_GUARD) {
		drmgr_args[3] = "cpu";
		drmgr_args[4] = "-p";
		drmgr_args[5] = "ent_capacity";
		drmgr_args[6] = "-q";
		snprintf(capacity, 5, "%d", value);
		drmgr_args[7] = capacity;
	}
	else if (resource_type == MEM_PAGE) {
		log_msg(NULL, "We should not reach this code path "
			"error in handling of MEM_PAGE");
		/* place holder for future expansion */
	}
	else if (resource_type == MEM_LMB) {
		drmgr_args[3] = "mem";
		drmgr_args[4] = "-s";
		drmgr_args[5] = drc_name;
	}/* end building of drmgr command */

	dbg("Running: %s %s %s %s %s %s %s %s", drmgr_args[0],
		drmgr_args[1], drmgr_args[2], drmgr_args[3],
		drmgr_args[4], drmgr_args[5], drmgr_args[6],
		drmgr_args[7]);

#ifdef DEBUG
	if (no_drmgr)
		return;
#endif

	child = fork();
	if (child == -1) {
		log_msg(NULL, "%s cannot be run to handle a predictive CPU "
			"failure, %s", DRMGR_PROGRAM, strerror(errno));
		return;
	}
	else if (child == 0) {
		/* child process */
		rc = execv(DRMGR_PROGRAM, drmgr_args);

		/* shouldn't get here */
		log_msg(NULL, "Could not exec %s to in response to a "
			"predictive CPU failure, %s", DRMGR_PROGRAM,
			strerror(errno));
		exit(1);
	}

	if (wait) {
		child = waitpid(child, &status, 0);
	}
}

/**
 * can_delete_lmb
 * @brief Counts the number of lmb's and returns an appropriate value
 *
 * This function returns true or false. It counts the lmb's in the system
 * and returns true if there is more than one lmb in the system. Which allows
 * us to call the appropriate delete function.
 * @return true if deletion can occur or false if it is the last lmb.
 */
static int
can_delete_lmb(void)
{
	DIR *dir;
	struct dirent *entry;
	int ret_val, fd, ctr=0;
	char buffer[1024], state[7];

	ret_val = 0; /* default state is false, unless count is positive */

	dir = opendir("/sys/devices/system/memory");

	while ((entry = readdir(dir)) != NULL) {

		/* ignore memory@0 situation */
		if (!strncmp(entry->d_name, "memory@0", 8))
				continue;

		if (!strncmp(entry->d_name, "memory", 6)) {

			snprintf(buffer, 1024, "/sys/devices/system/memory/%s/"
				"state", entry->d_name);

			if ((fd = open(buffer, O_RDONLY)) < 0 ) {
				log_msg(NULL, "Could not open %s, %s",
					buffer, strerror(errno));
				continue;
			}
			if ((read(fd, state, 6)) != 0 ) {
				if(!strncmp(state, "online", 6))
					ctr++;
				/* make sure its not the last lmb */
				if (ctr >= 2) {
					ret_val = 1;
					close(fd);
					break;
				}
			}
			close(fd);
		}
	}

	closedir(dir);
	return ret_val;
}

/**
 * retrieve_drc_name
 * @brief retrieve the drc-name of a cpu
 * 
 * Retrieves a string containing the drc-name of the CPU specified by the ID 
 * passed as a parameter.  Returns 1 on success, 0 on failure.
 *
 * @param event rtas event pointer
 * @param id interrupt server number of the cpu
 * @param buffer storeage for drc-name
 * @param bufsize size of buffer
 * @return 1 on success, 0 on failure
 */
static int
retrieve_drc_name(enum event_type type, struct event *event, unsigned int id,
		      char *buffer, size_t bufsize)
{
	struct stat sbuf;
	int rc, ret=1;
	FILE *fp;
	char cmd[1024];
	uint8_t status;

	if (stat(CONVERT_DT_PROPS_PROGRAM, &sbuf) < 0) {
		log_msg(event, "The command \"%s\" does not exist, %s",
			CONVERT_DT_PROPS_PROGRAM, strerror(errno));
		return 0;
	}

	if (type == CPUTYPE) {
		snprintf(cmd, 1024,"%s --context cpu --from interrupt-server "
		"--to drc-name %u 2>/dev/null", CONVERT_DT_PROPS_PROGRAM, id);
	}
	else { /* event type is mem */
		snprintf(cmd, 1024, "%s --context mem --from drc-index --to "
			 "drc-name 0x%08x 2>/dev/null", CONVERT_DT_PROPS_PROGRAM,
			  id);
	}

	/* Note :
	 *        rtas_errd sets up child signal handler
	 * (setup_sigchld_handler()) which cleanup child process when it
	 * exited. This caused pclose() to return non-zero value always
	 * (No child process ). Resulted this function to return 0
	 * (failure) always. To overcome this issue we will restore
	 * SIGCHLD handler to default action before doing popen() and
	 * setup again after completing pclose().
	 */
	restore_sigchld_default();

	if ((fp = popen(cmd, "r")) == NULL) {
		if (type == CPUTYPE) {
			log_msg(event, "Cannot obtain the drc-name for the "
				"CPU with ID %u; Could not run %s. %s", id,
				CONVERT_DT_PROPS_PROGRAM, strerror(errno));
		}
		else {
			log_msg(event, "Cannot obtain the drc-name for the "
				"Memory with ID %u; Could not run %s. %s", id,
				CONVERT_DT_PROPS_PROGRAM, strerror(errno));
		}
		setup_sigchld_handler();
		return 0;
	}

	rc = fread(buffer, 1, bufsize, fp);
	if (rc == bufsize) {
		if (type == CPUTYPE) {
			log_msg(event, "Cannot obtain the drc-name for the "
				       "CPU with ID %u; Buffer overflow in "
				       "retrieve_drc_name", id);
		}
		else {
			log_msg(event, "Cannot obtain the drc-name for the "
				       "MEMORY with ID %u; Buffer overflow "
				       "in retrieve_drc_name", id);
		}
		buffer[bufsize-1] = '\0';
		ret = 0;
	}
	else {
		buffer[rc-1] = '\0';	/* overwrite the newline at the end */
	}

	rc = pclose(fp);
	status = (int8_t)WEXITSTATUS(rc);

	setup_sigchld_handler();

	if (status != 0) {
		if (type == CPUTYPE) {
			log_msg(event, "Cannot obtain the drc-name for the "
				       "CPU with ID %u; %s returned %d",
					id, CONVERT_DT_PROPS_PROGRAM, rc);
		}
		else {
			log_msg(event, "Cannot obtain the drc-name for the "
					"Memory with ID %u; %s returned %d", 						id, CONVERT_DT_PROPS_PROGRAM, rc);
		}
		ret = 0;
	}

	return ret;
}

/**
 * parse_lparcfg
 * @brief Retrieves the value of the specified parameter from the lparcfg file.
 *
 * @param param lparcfg parameter
 * @return value of param from lparcfg, -1 on error
 */
static long
parse_lparcfg(char *param) {
	FILE *fp;
	int len;
	char buffer[128], *pos;

	if ((fp = fopen("/proc/ppc64/lparcfg", "r")) == NULL) {
		log_msg(NULL, "Could not open /proc/ppc64/lparcfg, %s",
			strerror(errno));
		return -1;
	}

	len = strlen(param);
	while ((fgets(buffer, 128, fp)) != NULL) {
		if (!strncmp(param, buffer, len)) {
			if ((pos = strchr(buffer, '=')) == NULL) {
				log_msg(NULL, "Could not retrieve the value "
					"for %s from /proc/ppc64/lparcfg", 
					param);
			}
			pos++;
			buffer[strlen(buffer)-1] = '\0';
			fclose(fp);
			return strtol(pos, NULL, 0);
		}
	}

	fclose(fp);
	log_msg(NULL, "Could not find the parameter %s in /proc/ppc64/lparcfg",
		param);

	return -1;
}

/**
 * guard_cpu
 * @brief Parse RTAS event for CPU guard information.
 *
 * Parses error information to determine if it represents a predictive CPU
 * failure, which should cause a CPU Guard operation.  DRMGR_PROGRAM is
 * forked to actually remove the CPU from the system.
 *
 * @param event rtas event
 * @param cpu id to locate drc_name
 */
static void
guard_cpu(struct event *event, int cpu_id)
{
	char drc_name[30];
	int n_cpus;

	if (! retrieve_drc_name(CPUTYPE, event, cpu_id, drc_name, 30)) {
		log_msg(event, "A CPU could not be guarded off in "
			"response to a received predictive CPU "
			"failure event %d", cpu_id);
		return;
	}

	/* check to make sure this isn't the last CPU */
	n_cpus = parse_lparcfg("partition_active_processors");
	if (n_cpus == 1) {
		log_msg(event, "A request was received to deallocate a "
			"processor due to a predictive CPU failure. "
			"The request cannot be carried out because "
			"there is only one CPU on this system");
		return;
	}
	else if (n_cpus <= d_cfg.min_processors) {
		log_msg(event, "A request was received to deallocate a "
			"processor due to a predictive CPU failure. "
			"The request is not being carried out because "
			"ppc64-diag is configured to keep a minimum of "
			"%d processors", d_cfg.min_processors);
		return;
	}

	run_drmgr(CPU_GUARD, drc_name, 0, 0);
	log_msg(event, "The following CPU has been offlined due to the "
		"reporting of a predictive CPU failure: logical ID "
		"%d, drc-name %s", cpu_id, drc_name);
	platform_log_write("CPU Deallocation Notification\n");
	platform_log_write("(resulting from a predictive CPU failure)\n");
	platform_log_write("    Logical ID: %d\n", cpu_id);
	platform_log_write("    drc-name:   %s\n", drc_name);

	snprintf(event->addl_text, ADDL_TEXT_MAX,
		"Predictive CPU Failure: deallocated CPU ID %d "
		"(drc-name \"%s\")", cpu_id, drc_name);
	return;
}

/**
 * guard_spcpu
 * @brief Parse RTAS event for SP_CPU guard information.
 *
 * Parses error information to determine if it represents a predictive SPCPU
 * failure, which should cause a SPCPU Guard operation. DRMGR_PROGRAM is
 * forked to actually remove the virtual cpu from the system.
 *
 * @param event rtas event
 * @param entitiled shared processor loss.
 */
static void
guard_spcpu(struct event *event, int ent_loss)
{
	int n_cpus, quant, ent_cap;
	int	min_ent_cap = d_cfg.min_entitled_capacity;

	/*
	 * Ensure that this event will not cause the entitled
	 * capacity to drop below 10 per CPU.  If so, we need to
	 * start dropping virtual CPUs to accommodate the reduced
	 * entitled capacity.  System minimum is one virtual CPU
	 * with 10 units of entitled capacity.
	 */
	ent_cap = parse_lparcfg("partition_entitled_capacity");
	if (ent_cap <= min_ent_cap) {
		log_msg(event, "A request was received to deallocate "
			"entitled capacity due to a predictive CPU "
			"failure. The request cannot be carried out "
			"because this partition is already at the "
			"minimum allowable entitled capacity");
		return;
	}
	n_cpus = parse_lparcfg("partition_active_processors");
	if ((ent_cap - ent_loss) < min_ent_cap) {
		ent_loss = ent_cap - min_ent_cap;
	}
	if ((ent_cap - ent_loss) < (n_cpus * min_ent_cap)) {
		/* need to deallocate virtual CPUs */
		quant = (ent_cap - ent_loss)/10;
		run_drmgr(CPU_GUARD, NULL, quant, 1);

		log_msg(event, "A request was received to deallocate "
			"entitled capacity due to a predictive CPU "
			"failure.  %d virtual CPUs were deallocated "
			"in order to fulfill this request", quant);
		platform_log_write("CPU Deallocation Notification\n");
		platform_log_write("(resulting from a predictive CPU "
			"failure)\n");
		platform_log_write("    number of virtual CPUs "
				   "offlined: %d\n", quant);
		platform_log_write("    number of virtual CPUs "
				   "remaining: %d\n", n_cpus - quant);

		snprintf(event->addl_text, ADDL_TEXT_MAX,
			"Predictive CPU Failure: deallocated %d "
			"virtual CPUs", quant);
		}

	run_drmgr(SP_CPU_GUARD, NULL, ent_loss, 0);
	log_msg(event, "Entitled capacity in the amount of %d has been "
		"offlined due to the reporting of a predictive CPU "
		"failure", ent_loss);
	platform_log_write("Entitled Capacity Deallocation "
			   "Notification\n");
	platform_log_write("(resulting from a predictive CPU failure)\n");
	platform_log_write("    entitled capacity units offlined:  "
			   "%d\n", ent_loss);
	platform_log_write("    entitled capacity units remaining: "
			   "%d\n", ent_cap - ent_loss);

	snprintf(event->addl_text, ADDL_TEXT_MAX,
		"Predictive CPU Failure: deallocated %d entitled "
		"capacity units", ent_loss);
	return;
}

/**
 * guard_mempage
 * @brief Perform mempage guard operation. Currently not supported.
 *
 * A placeholder function. This dealloc type operation is currently
 * not supported. MEMPAGE events cannot be guarded at present.
 *
 * @param event rtas event.
 * @param memory address page that needs to be de-allocated.
 */
static void
guard_mempage( struct event *event, uint64_t memory_address)
{
	log_msg(event, "A request was received to deallocate a memory page, "
		       "which is not currently a supported operation.");
	return;
}

/**
 * guard_memlmb
 * @brief Perform lmb guard operation after proper validation
 *
 * parses error information to determine the lmb that requires
 * guarding operation. At this time only MEMLMB operations may
 * be guarded. DRMGR_PROGRAM is forked to actually remove
 * the LMB from the system.
 *
 * @param event rtas event
 * @param drc index to be guarded.
 */
static void
guard_memlmb(struct event *event, unsigned int drc_index)
{
	char drc_name[30];

	/* check to make sure there is more than one lmb to delete */
	if (can_delete_lmb()==FALSE) {
		log_msg(event, "A request was received to deallocate a "
			"LMB partition due to a predictive MEM failure "
			"The request cannot be carried out because "
			"there is only one LMB on this system");
		return;
	}

	if (!retrieve_drc_name(MEMTYPE, event, drc_index, drc_name, 30)) {
		log_msg(event, "A LMB could not be guarded off in "
			"response to a received predictive MEM "
			"failure event 0x%08x", drc_index);
		return;
	}

	run_drmgr(MEM_LMB, drc_name, 0,0);
	log_msg(event, "The following LMB has been offlined due to the "
		       "reporting of a predictive memory failure:"
			"0x%08x, drc-name %s", drc_index, drc_name);

	platform_log_write("MEM Deallocation Notification\n");
	platform_log_write("(resulting from a predictive MEM failure)\n");
	platform_log_write("    Logical ID: 0x%08x\n", drc_index);
	platform_log_write("    drc-name:   %s\n", drc_name);

	snprintf(event->addl_text, ADDL_TEXT_MAX,
		"Predictive MEM Failure: deallocated LMB ID %u "
		"(drc-name \"%s\")", drc_index, drc_name);
	return;
}


/**
 * handle_resource_dealloc
 * @brief Parse RTAS event for CPU guard information.
 *
 * Parses error information to determine if it represents a predictive CPU
 * failure, which should cause a CPU Guard operation.  DRMGR_PROGRAM is
 * forked to actually remove the CPU from the system.
 *
 * @param event rtas event
 */
void
handle_resource_dealloc(struct event *event)
{
	struct rtas_event_hdr *rtas_hdr = event->rtas_hdr;
	struct rtas_event_exthdr *exthdr;
	struct rtas_cpu_scn *cpu;
	struct rtas_lri_scn *lri;
	uint64_t address_range;

	/*
	 * The following conditional is to handle pre-v6 predictive CPU
	 * failure events; it has been designed to specification, but has
	 * not been tested. version 1 to 5.
	 */
	if (rtas_hdr->severity == RTAS_HDR_SEV_WARNING &&
	    rtas_hdr->disposition == RTAS_HDR_DISP_FULLY_RECOVERED &&
	    rtas_hdr->extended &&
	    rtas_hdr->initiator == RTAS_HDR_INIT_CPU &&
	    rtas_hdr->type == RTAS_HDR_TYPE_CACHE_PARITY &&
	    rtas_hdr->version < 6) {

		/* Check extended error information */
		exthdr = rtas_get_event_exthdr_scn(event->rtas_event);

		cpu = rtas_get_cpu_scn(event->rtas_event);

		if (cpu == NULL) {
			log_msg(event, "Could not retrieve CPU section to "
				"check for a CPU guard event");
			return;
		}

		if (exthdr->unrecoverable_bypassed &&
		    exthdr->predictive && exthdr->power_pc &&
		    cpu->extcache_ecc) {
			guard_cpu(event, cpu->id);
		}
	}

	if (rtas_hdr->version >= 6) {
		if (rtas_hdr->type == RTAS_V6_TYPE_RESOURCE_DEALLOC) {
			lri = rtas_get_lri_scn(event->rtas_event);
			if (lri == NULL) {
				log_msg(event, "Could not retrieve a Logical "
					"Resource Identification section from "
					"a v6 event to determine the resource "
					"to be deallocated");
				return;
			}

			switch (lri->resource) {
			    case RTAS_LRI_RES_PROC:
				/* invoke a CPU guard operation */
				guard_cpu(event, lri->lri_cpu_id);
				break;
			    case RTAS_LRI_RES_SHARED_PROC:
				/* shared processor CPU guard operation */
				guard_spcpu(event, lri->capacity);
				break;
			    case RTAS_LRI_RES_MEM_PAGE:
				/* Mem page remove operation */
				address_range = lri->lri_mem_addr_hi;
				address_range <<= 32;
				address_range |= lri->lri_mem_addr_lo;
				guard_mempage(event, address_range);
				break;
			    case RTAS_LRI_RES_MEM_LMB:
				/* LMB remove operation */
				guard_memlmb(event, lri->lri_drc_index);
				break;
			}
		}
	}
}
