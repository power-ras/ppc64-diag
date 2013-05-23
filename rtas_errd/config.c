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

/**
 * get_token
 * @brief return the next token from the provided buffer.
 *
 * This routine is modified parser/tokenizer for interpreting the 
 * ppc64-diag config file.
 *
 * @param str point at which to began looking for tokens
 * @param str_end end of token buffer
 * @param tok buffer to be filled in with the found token
 * @param line_no reference to line number in the config file
 * @return pointer to where the tokenizer stopped in 'str'
 */
static char *
get_token(char *str, char *str_end, char *tok, int *line_no)
{
	char	*start = NULL;

	while (str < str_end) {
	    switch (*str) {
		case '#':
			/* A '#' denotes the beginning of a comment that
			 * continues to the end of the line ('\n')
			 * character.  Skip ahead to the next line and 
			 * continue searching.
			 */
			while ((str < str_end) && (*str++ != '\n'));
			(*line_no)++;
			break;

		case '\\':
			/* A '\' denotes a multi-line string.  If we haven't
			 * started a string yet simply skip ahead to the next 
			 * line and continue searching...
			 */
			if (start == NULL) {
				while ((str < str_end) && (*str++ != '\n'));
				(*line_no)++;
				break;
			}

			/* ...Otherwise, return the token we have already 
			 * started then skip ahead to the next line and
			 * return that point as the end of the search.
			 */
			snprintf(tok, str - start + 1, start);
			
			while ((str < str_end) && (*str++ != '\n'));
			(*line_no)++;

			return str;
			break;
			
		case ' ':
		case '\t':
			/* Whitespace is either is token delimiter if we
			 * have started reading a token, or it is igored.
			 */
			if (start == NULL) {
				str++;
				break;
			}

			snprintf(tok, str - start + 1, start);
			return str + 1;
			break;

		case '\n':
			/* Newlines are either token delimiters if we
			 * have started reading a token, or a seperate token
			 * on its own.
			 */
			if (start == NULL) {
				tok[0] = '\n';
				tok[1] = '\0';
				(*line_no)++;
				str++;
			} else {
				snprintf(tok, str - start + 1, start);
			}

			return str;
			break;

		case '{':
		case '}':
		case '=':
			/* All three ('{', '}', and '=') are both token
			 * delimiters if we have started reading a token or
			 * a token on their own.
			 */
			if (start == NULL) {
				snprintf(tok, 2, str);
				str++;
			} else {
				snprintf(tok, str - start + 1, start);
			}

			return str;
			break;

		case '"':
			/* A quoted string is returned as a single token
			 * containing everything in quotes, or a token
			 * delimiter if we have begun reading in a token.
			 * NOTE, we do not allow quoted strings the span
			 * multiple lines.
			 */
			if (start == NULL) {
				start = str++;
				while ((str < str_end) && (*str != '"')) {
					if (*str == '\n')
						return NULL;
					else
						str++;
				}
				snprintf(tok, str - start + 1, start);
				str++;
			} else {
				snprintf(tok, str - start + 1, start);
			}

			return str;
			break;

		default:
			/* By default everything else is part of a token */
			if (start == NULL)
				start = str;

			str++;
			break;
	    }
	}

	return (char *)EOF;
}

		
/**
 * get_config_string
 * @brief retrieve a string associated with a configuration entry
 *
 * For the purposes of parsing the ppc64-diag config file a string is
 * considered everything that comes after the '=' character up to
 * the newline ('\n') character.  Strings can span multiple lines 
 * if they end in a '\', but this is handled by get_token() and we
 * only need to look for the newline character here.
 *
 * @param start should point to the '=' following an entry name
 * @param end end of buffer containing the string
 * @param buf buffer into which the string is copied
 * @param line_no reference to the line number in the config file
 * @return pointer to where we stopped parsing or NULL on failure
 */
static char *
get_config_string(char *start, char *end, char *buf, int *line_no)
{
	char	tok[1024];
	int	offset = 0;

	/* The first token token should be '=' */
	start = get_token(start, end, tok, line_no);
	if ((start == NULL) || (tok[0] != '='))
		return NULL;

	/* Next token should be the string, this string can be a series
	 * of tokens or one string in quotes.  We just get everything up
	 * to the newline.  The code looks a bit odd here, but that is so
	 * we can add in spaces between tokens which are ignored by the
	 * tokenizer.
	 */
	start = get_token(start, end, tok, line_no);
	if (start == NULL)
		return NULL;

	offset += sprintf(buf, tok);

	start = get_token(start, end, tok, line_no);
	while ((start != NULL) && (tok[0] != '\n')) {
		offset += sprintf(buf + offset, " %s", tok);
		start = get_token(start, end, tok, line_no);
	}

	return start;
}

/**
 * get_config_num
 * @brief Retrieve a numeric value for a configuration entry
 *
 * @param start should point to the '=' following an entry name
 * @param end end of buffer containing thevalue 
 * @param val int reference into which the value is copied
 * @param line_no reference to the line number in the config file
 * @return pointer to where we stopped parsing or NULL on failure
 */
static char *
get_config_num(char *start, char *end, int *val, int *line_no)
{
	char	tok[1024];

	/* The first token should be '=' */
	start = get_token(start, end, tok, line_no);
	if ((start == NULL) || (tok[0] != '='))
		return NULL;

	/* Next token should be the value */
	start = get_token(start, end, tok, line_no);
	if (start != NULL)
		*val = ((int)strtol(tok, NULL, 10) > 1 ?
				(int)strtol(tok, NULL, 10) : 1);

	return start;
}

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
 * @param start place to start loking
 * @param end pointer to end of 'start' buffer
 * @param line_no refernece to the current line number
 * @param update_param if zero, the param will not actually be updated
 * @returns pointer to current position in the config file, or NULL on failure
 */
static char *
config_restart_policy(char *start, char *end, int *line_no, int update_param)
{
	struct stat	sbuf;
	char		*cur;
	char		system_arg[80];
	char		param[3];
	int		rc;

	cur = get_config_num(start, end, &d_cfg.restart_policy, line_no);
	if (cur == NULL) {
		d_cfg.log_msg("Parsing error for configuration file "
			      "entry \"AutoRestartPolicy\", " "line %d", 
			      line_no);
		return cur;
	}

	/* Validate the restart value */
	if (! (d_cfg.restart_policy == 0 || d_cfg.restart_policy == 1)) {
		d_cfg.log_msg("Invalid parameter (%d) specified for the "
			      "AutoRestartPolicy (line %d), expecting a 0 "
			      "or a 1.  The Auto Restart Policy has not been "
			      "configured", d_cfg.restart_policy, line_no);
		d_cfg.restart_policy = -1;
		return cur;
	}

	if (!update_param)
		return cur;

	/* Try to set the system parameter with the ibm,set-sysparm RTAS call */
	*(uint16_t *)param = 1;
	param[2] = (uint8_t)d_cfg.restart_policy;
	rc = rtas_set_sysparm(RTAS_PARAM_AUTO_RESTART, param);
	switch (rc) {
	case 0:			/* success */
		d_cfg.log_msg("Configuring the Auto Restart Policy to %d",
			      d_cfg.restart_policy);
		return cur;
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
		return cur;
	}

	/* See if the ibm,os-auto-restart config variable is in 
	 * nvram on this machine
	 */
	sprintf(system_arg, "/usr/sbin/nvram "
		"--print-config=ibm,os-auto-restart");
	if (system(system_arg) == -1) {
		d_cfg.log_msg("The current system does not support the "
			      "Auto Restart Policy; the AutoRestartPolicy "
			      "configuration entry (line %d) is being skipped",
			      line_no);
		d_cfg.restart_policy = -1;
		return cur;
	}

	sprintf(system_arg, "%s%d", "/usr/sbin/nvram -p common "
		"--update-config ibm,os-restart-policy=", d_cfg.restart_policy);

	if (system(system_arg)) {
		d_cfg.log_msg("The Auto Restart Policy could not be configured "
			      "due to a failure in running the nvram command",
			      d_cfg.restart_policy);
		d_cfg.restart_policy = -1;
		return cur;
	}

	d_cfg.log_msg("Configuring the Auto Restart Policy to %d (in NVRAM)",
		      d_cfg.restart_policy);

	return cur;
}

/**
 * parse_config_entries
 * @brief Parse the ppc64-diag config file entries
 * 
 * @param buf buffer containg the ppc64-diag config file contents
 * @param buf_end pointer to the end of 'buf'
 * @param update_sysconfig if 0, configuration params (in NVRAM) will not update
 * @returns 0 on success, -1 on error
 */
static int
parse_config_entries(char *buf, char *buf_end, int update_sysconfig)
{
	char	*cur = buf;
	char	tok[1024];
	int	line_no = 1;
	int	rc = 0;

	do {
		cur = get_token(cur, buf_end, tok, &line_no);
		if (cur == (char *)EOF)
			break;

		if (cur == NULL) {
			d_cfg.log_msg("Parsing error in configuration "
				      "file near line %d", line_no);
			rc = -1;
			break;
		}
		
		/* Ignore newlines */
		if (tok[0] == '\n') {
			continue;
		}

		/* MinProcessors */
		if (strcmp(tok, "MinProcessors") == 0) {
			cur = get_config_num(cur, buf_end, 
					     &d_cfg.min_processors, &line_no); 
			if (cur == NULL) {
				d_cfg.log_msg("Parsing error for "
					      "configuration file entry "
					      "\"MinProcessors\", line %d", 
					      line_no);
				rc = -1;
				break;
			} 
			else {
				d_cfg.log_msg("Configuring Minimum "
					      "Processors to %d", 
					      d_cfg.min_processors);
			}
			
		/* MinEntitledCapacity */
		} else if (strcmp(tok, "MinEntitledCapacity") == 0) {
			cur = get_config_num(cur, buf_end, 
					     &d_cfg.min_entitled_capacity,
					     &line_no);
			if (cur == NULL) {
				d_cfg.log_msg("Parsing error for "
					      "configuration file entry "
					      "\"MinEntitledCapacity\", "
					      "line %d", line_no);
				rc = -1;
				break;
			} 
			else {
				d_cfg.log_msg("Configuring Minimum "
					      "Entitled Capacity to %d", 
					      d_cfg.min_entitled_capacity);
			}
			
		/* ScanlogDumpPath */
		} else if (strcmp(tok, "ScanlogDumpPath") == 0) {
			int len;

			cur = get_config_string(cur, buf_end,
						d_cfg.scanlog_dump_path,
						&line_no);
			if (cur == NULL) {
				d_cfg.log_msg("Parsing error for "
					      "configuration file entry "
					      "\"ScanlogDumpPath\", line %d", 
					      line_no);
				rc = -1;
				break;
			}

			/* We need to ensure the path specified ends in a '/' */
			len = strlen(d_cfg.scanlog_dump_path);
			if (d_cfg.scanlog_dump_path[len-1] != '/') {
				d_cfg.scanlog_dump_path[len] = '/';
				d_cfg.scanlog_dump_path[len + 1] = '\0';
			}

			d_cfg.log_msg("Configuring Scanlog Dump Path to "
				      "\"%s\"", d_cfg.scanlog_dump_path);

		/* PlaformDumpPath */
		} else if (strcmp(tok, "PlatformDumpPath") == 0) {
			int len;

			cur = get_config_string(cur, buf_end,
						d_cfg.platform_dump_path,
						&line_no); 

			if (cur == NULL) {
				d_cfg.log_msg("Parsing error for "
					      "configuration file entry "
					      "\"PlatformDumpPath\", line %d", 
					      line_no);
				rc = -1;
				break;
			}

			/* We need to ensure the path specified ends in a '/' */
			len = strlen(d_cfg.platform_dump_path);
			if (d_cfg.platform_dump_path[len-1] != '/') {
				d_cfg.platform_dump_path[len] = '/';
				d_cfg.platform_dump_path[len + 1] = '\0';
			}

			d_cfg.log_msg("Configuring Platform Dump Path to "
				      "\"%s\"", d_cfg.platform_dump_path);

		/* AutoRestartPolicy */
		} else if (strcmp(tok, "AutoRestartPolicy") == 0) {
			cur = config_restart_policy(cur, buf_end, &line_no,
					update_sysconfig);
			if (cur == NULL) {
				rc = -1;
				break;
			}

		} 
		else {
			d_cfg.log_msg("Configuration error: \"%s\", line %d, "
				      "is not a valid configuration name", 
				      tok, line_no);
			rc = -1;
			break;
		}
	} while ((cur != NULL) && (cur < buf_end));

	return rc;
}

/**
 * init_d_cfg
 * @brief initialize the ppc64-diag config structure
 */
static void
init_d_cfg(void (*log_msg)(char *, ...))
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
	struct stat	cfg_sbuf;
	char		*cfg_mmap = NULL;
	char		*cur = NULL, *end;
	int		cfg_fd;

	/* initialize the configuration variables */
	init_d_cfg(log_msg);

	/* open and map the configuration file */
	if ((cfg_fd = open(config_file, O_RDONLY)) < 0) {
		d_cfg.log_msg("Could not open the ppc64-diag configuration "
			      "file \"%s\", %s", config_file, strerror(errno));
		return 0;
	}

	if ((fstat(cfg_fd, &cfg_sbuf)) < 0) {
		d_cfg.log_msg("Could not get status of the ppc64-diag "
			      "configuration file \"%s\", %s. No configuration "
			      "data has been read, using default ppc64-diag "
			      "settings", config_file, strerror(errno));
		close(cfg_fd);
		return 0;	
	}
	
	/* If this is a zero-length file, do nothing */
	if (cfg_sbuf.st_size == 0) {
		close(cfg_fd);
		return 0;
	}

	if ((cfg_mmap = mmap(0, cfg_sbuf.st_size, PROT_READ, MAP_PRIVATE,
			     cfg_fd, 0)) == (char *)-1) {
		d_cfg.log_msg("Could not map ppc64-diag configuration file "
			      "\"%s\", %s. No configuration data has been "
			      "read, using default ppc64-diag settings", 
			      config_file, strerror(errno));
		close(cfg_fd);
		return 0;
	}

	cur = cfg_mmap;
	end = cfg_mmap + cfg_sbuf.st_size;

	if (parse_config_entries(cur, end, update_sysconfig)) {
		d_cfg.log_msg("Due to an error parsing the ppc64-diag "
			      "configuration file, The default configuration "
			      "settings will be used");
		init_d_cfg(log_msg);
	}

	munmap(cfg_mmap, cfg_sbuf.st_size);
	close(cfg_fd);
	return 0;
}
