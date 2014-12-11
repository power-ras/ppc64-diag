/**
 * @file config.c
 * @brief Routines for parsing the ppc64-diag.config file
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
#include <librtas.h>
#include <iniparser.h>
#include "config.h"

#define RTAS_PARAM_AUTO_RESTART		21

/**
 * @var config_file
 * @brief default ppc64-diag configuration file
 */
char *config_file = "/etc/ppc64-diag/ppc64-diag.config";

/**
 * @var d_cfg
 * @brief ppc64-diag configuration structure
 */
struct ppc64_diag_config d_cfg;

static const char *rtas_config_keys[] = {
	"MinProcessors",
	"MinEntitledCapacity",
	"ScanlogDumpPath",
	"PlatformDumpPath",
	"AutoRestartPolicy",
};

#define CONFIG_MAX	(sizeof(rtas_config_keys)/sizeof(rtas_config_keys[0]))

/**
 * config_restart_policy
 * @brief Configure the Auto Restart Policy for this machine, if present.
 *
 * Users can specify an AutoRestartPolicy=X entry in the ppc64-diag
 * configuration file.  This will read in that 'X' value and set the
 * corresponding service policy.  This is done either by setting the
 * ibm,os-auto-restart parameter in NVRAM (older systems), or by setting
 * the partition_auto_restart system parameter (more recent systems).
 *
 * @param update_param if zero, the param will not actually be updated
 */
static void config_restart_policy(int update_param)
{
	struct stat	sbuf;
	char		system_arg[80];
	char		param[3];
	int		rc;

	/* Validate the restart value */
	if (! (d_cfg.restart_policy == 0 || d_cfg.restart_policy == 1)) {
		d_cfg.log_msg("Invalid parameter (%d) specified for the "
			      "AutoRestartPolicy in config file "
			      "(%s), expecting a 0 or a 1.  The Auto Restart "
			      "Policy has not been configured",
			      d_cfg.restart_policy, config_file);
		d_cfg.restart_policy = -1;
		return;
	}

	if (!update_param)
		return;

	/* Try to set the system parameter with the ibm,set-sysparm RTAS call */
	*(uint16_t *)param = htobe16(1);
	param[2] = (uint8_t)d_cfg.restart_policy;
	rc = rtas_set_sysparm(RTAS_PARAM_AUTO_RESTART, param);
	switch (rc) {
	case 0:			/* success */
		d_cfg.log_msg("Configuring the Auto Restart Policy to %d",
			      d_cfg.restart_policy);
		return;
	case -1:		/* hardware error */
		d_cfg.log_msg("Hardware error while attempting to set the "
			      "Auto Restart Policy via RTAS; attempting "
			      "to set in NVRAM...");
		break;
	case RTAS_UNKNOWN_OP:	/* RTAS call not available */
		break;
	case -3:		/* system parameter not supported */
		break;
	case -9002:		/* not authorized */
		d_cfg.log_msg("Not authorized to set the Auto Restart Policy "
			      "via RTAS; attempting to set in NVRAM...");
		break;
	case -9999:		/* parameter error */
		d_cfg.log_msg("Parameter error setting the Auto Restart Policy "
			      "via RTAS; attempting to set in NVRAM...");
		break;
	default:
		d_cfg.log_msg("Unknown error (%d) setting the Auto Restart "
			      "Policy via RTAS; attempting to set in NVRAM...",
			      rc);
	}

	/* If the auto_partition_restart system parameter does not
	 * exist, check if the nvram command exists
	 */
	if (stat("/usr/sbin/nvram", &sbuf) < 0) {
		d_cfg.log_msg("Could not configure the Auto Restart Policy "
			      "due to a missing requisite (nvram command)");
		d_cfg.restart_policy = -1;
		return;
	}

	/* See if the ibm,os-auto-restart config variable is in 
	 * nvram on this machine
	 */
	sprintf(system_arg, "/usr/sbin/nvram "
		"--print-config=ibm,os-auto-restart");
	if (system(system_arg) == -1) {
		d_cfg.log_msg("The current system does not support the "
			      "Auto Restart Policy; the AutoRestartPolicy "
			      "configuration entry is being skipped");
		d_cfg.restart_policy = -1;
		return;
	}

	sprintf(system_arg, "%s%d", "/usr/sbin/nvram -p common "
		"--update-config ibm,os-restart-policy=", d_cfg.restart_policy);

	if (system(system_arg)) {
		d_cfg.log_msg("The Auto Restart Policy could not be configured "
			      "due to a failure in running the nvram command",
			      d_cfg.restart_policy);
		d_cfg.restart_policy = -1;
		return;
	}

	d_cfg.log_msg("Configuring the Auto Restart Policy to %d (in NVRAM)",
		      d_cfg.restart_policy);
}

/**
 * parse_config_entries
 * @brief Parse the ppc64-diag config file entries
 *
 * @param config_dict (dictionary object) after parsing the config file
 * @param update_sysconfig if 0, configuration params (in NVRAM) will not update
 */
static void parse_config_entries(dictionary *config_dict, int update_sysconfig)
{
	int len, ret;
	int n_secs, i, j;
	char *result;

	n_secs = iniparser_getnsec(config_dict);

	/* Check if any sections are specified */
	if (n_secs > 0) {
		for (i = 0; i < n_secs; i++)
			d_cfg.log_msg("Error in the configuration file %s"
				      " section [%s] not expected", config_file,
				      iniparser_getsecname(config_dict, i));
		return;
	}

	/* Check if any invalid keys are specified */
	for (i = 0; i < config_dict->n; i++) {
		int found = 0;
		/* Discard the ':' stored in dictionary (as section is null) */
		char *dict_key = config_dict->key[i] + 1;

		for (j = 0; j < CONFIG_MAX; j++)
			if (!strncasecmp(dict_key, rtas_config_keys[j],
					strlen(rtas_config_keys[j]))) {
				found = 1;
				break;
			}

		if (!found)
			d_cfg.log_msg("Error in the configuration file %s key"
				      " [%s] not valid", config_file, dict_key);
	}

	ret = iniparser_getint(config_dict, ":MinProcessors", 0);
	if (ret > 0) {
		d_cfg.min_processors = ret;
		d_cfg.log_msg("Configuring Minimum Processors to %d",
			      d_cfg.min_processors);
	} else {
		d_cfg.log_msg("Parsing error for configuration file entry "
			      "\"MinProcessors\" [value = %d]", ret);
	}

	ret = iniparser_getint(config_dict, ":MinEntitledCapacity", -1);
	if (ret > -1) {
		d_cfg.min_entitled_capacity = ret;
		d_cfg.log_msg("Configuring Minimum Entitled Capacity to %d",
			      d_cfg.min_entitled_capacity);
	} else {
		d_cfg.log_msg("Parsing error for configuration file entry "
			      "\"MinEntitledCapacity\" [value = %d]", ret);
	}

	result = iniparser_getstring(config_dict, ":ScanlogDumpPath", NULL);
	if (result != NULL) {
		strcpy(d_cfg.scanlog_dump_path, result);

		/* We need to ensure the path specified ends in a '/' */
		len = strlen(d_cfg.scanlog_dump_path);
		if (d_cfg.scanlog_dump_path[len-1] != '/') {
			d_cfg.scanlog_dump_path[len] = '/';
			d_cfg.scanlog_dump_path[len + 1] = '\0';
		}
		d_cfg.log_msg("Configuring Scanlog Dump Path to "
			      "\"%s\"", d_cfg.scanlog_dump_path);
	} else {
		d_cfg.log_msg("Parsing error for configuration file entry "
			      "\"ScanlogDumpPath\"");
	}

	result = iniparser_getstring(config_dict, ":PlatformDumpPath", NULL);
	if (result != NULL) {
		strcpy(d_cfg.platform_dump_path, result);

		/* We need to ensure the path specified ends in a '/' */
		len = strlen(d_cfg.platform_dump_path);
		if (d_cfg.platform_dump_path[len-1] != '/') {
			d_cfg.platform_dump_path[len] = '/';
			d_cfg.platform_dump_path[len + 1] = '\0';
		}
		d_cfg.log_msg("Configuring Platform Dump Path to "
			      "\"%s\"", d_cfg.platform_dump_path);
	} else {
		d_cfg.log_msg("Parsing error for configuration file entry "
			      "\"PlatformDumpPath\"");
	}

	ret = iniparser_getint(config_dict, ":AutoRestartPolicy", -1);
	if (ret == 0 || ret == 1) {
		d_cfg.restart_policy = ret;
		config_restart_policy(update_sysconfig);
	} else {
		d_cfg.log_msg("Parsing error for configuration file entry "
			      "\"AutoRestartPolicy\" [value = %d]", ret);
	}
}

/**
 * init_d_cfg
 * @brief initialize the ppc64-diag config structure
 */
static void init_d_cfg(void (*log_msg)(char *, ...))
{
	d_cfg.min_processors = 1;
	d_cfg.min_entitled_capacity = 5;

	strcpy(d_cfg.scanlog_dump_path, "/var/log/");
	strcpy(d_cfg.platform_dump_path, "/var/log/dump/");

	d_cfg.restart_policy = -1;

	d_cfg.log_msg = log_msg;
};

/**
 * diag_cfg
 * @brief read config components from ppc64-diag config file
 *
 * The config file is in the format of variable=data.  Comments
 * in the file are lines beginning with '#' or everything after a '#'
 * character in a line.
 *
 * @param update_sysconfig if 0, NVRAM will not be updated
 */
int
diag_cfg(int update_sysconfig, void (*log_msg)(char *, ...))
{
	dictionary *config_dict = NULL;

	/* initialize the configuration variables */
	init_d_cfg(log_msg);

	/* Parse a config file and return an allocated dictionary object.*/
	config_dict = iniparser_load(config_file);
	if (!config_dict) {
		d_cfg.log_msg("Could not parse config file %s (%s)",
			      config_file, strerror(errno));
		return -1;
	}

	/* parse the configuration entries */
	parse_config_entries(config_dict, update_sysconfig);

	iniparser_freedict(config_dict);

	return 0;
}
