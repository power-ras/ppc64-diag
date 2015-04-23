/**
 * @file	indicator_opal.c
 * @brief	OPAL indicator manipulation routines
 *
 * Copyright (C) 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "indicator.h"
#include "indicator_ses.h"

/* LED directory */
#define OPAL_LED_SYSFS_PATH	"/sys/class/leds"
#define OPAL_LED_FILE		"brightness"

/* LED Mode */
#define OPAL_LED_MODE_PROPERTY		"/proc/device-tree/ibm,opal/leds/led-mode"
#define OPAL_LED_MODE_LIGHT_PATH	"lightpath"
#define OPAL_LED_MODE_GUIDING_LIGHT	"guidinglight"


/*
 * LEDs are available under /sys/class/leds directory. Open this
 * dir for various LED operations.
 */
static DIR *
open_sysfs_led_dir(void)
{
	int rc;
	DIR *dir;

	rc = access(OPAL_LED_SYSFS_PATH, R_OK);
	if (rc != 0) {
		log_msg("%s : Error accessing led sysfs dir: %s (%d: %s)",
			__func__, OPAL_LED_SYSFS_PATH, errno, strerror(errno));
		return NULL;
	}

	dir = opendir(OPAL_LED_SYSFS_PATH);
	if (!dir)
		log_msg("%s : Failed to open led sysfs dir : %s (%d: %s)",
			__func__, OPAL_LED_SYSFS_PATH, errno, strerror(errno));

	return dir;
}

/* Close LED class dir */
static inline void
close_sysfs_led_dir(DIR *dir)
{
	closedir(dir);
}

/* Parse LED location code */
static struct loc_code *
opal_parse_loc_code(int indicator, struct loc_code *loc, const char *dirname)
{
	char *colon;
	int loc_len;
	struct loc_code *curr = loc;

	colon = strstr(dirname, ":");
	if (!colon) {
		log_msg("%s : Invalid location code", __func__);
		return loc;
	}

	loc_len = colon - dirname;
	if (loc_len <= 0 || loc_len >= LOCATION_LENGTH) {
		log_msg("%s : Invalid location code length", __func__);
		return loc;
	}

	if (!curr) {
		curr = malloc(sizeof(struct loc_code));
		loc = curr;
	} else {
		while (curr->next)
			curr = curr->next;
		curr->next = malloc(sizeof(struct loc_code));
		curr = curr->next;
	}

	if (!curr) {
		log_msg("%s : Out of memory", __func__);
		return loc;
	}

	memset(curr, 0, sizeof(struct loc_code));
	strncpy(curr->code, dirname, loc_len);
	curr->code[loc_len] = '\0';
	curr->length = loc_len + 1; /* Including NULL */
	curr->type = TYPE_OPAL;
	curr->index = indicator;  /* LED_TYPE_{ATTN/IDENT/FAULT} */
	curr->next = NULL;

	return loc;
}

/**
 * Get OPAL platform related indicator list
 *
 * @indicator	identification or attention/fault indicator
 * @loc		pointer to loc_code structure
 *
 * Returns :
 *	0 on success, !0 on failure
 */
static int
opal_get_indices(int indicator, struct loc_code **loc)
{
	const char *indicator_desc;
	int i;
	int rc = -1;
	int nleds;
	struct loc_code *list = *loc;
	struct dirent **dirlist;
	struct dirent *dirent;

	indicator_desc = get_indicator_desc(indicator);
	if (!indicator_desc)
		return rc;

	nleds = scandir(OPAL_LED_SYSFS_PATH, &dirlist, NULL, alphasort);
	if (nleds == -1) {
		log_msg("%s : Failed to read led sysfs dir : %s",
			__func__, OPAL_LED_SYSFS_PATH);
		return rc;
	}

	for (i = 0; i < nleds; i++) {
		dirent = dirlist[i];

		if (dirent->d_type != DT_DIR && dirent->d_type != DT_LNK) {
			free(dirlist[i]);
			continue;
		}

		if (strstr(dirent->d_name, indicator_desc))
			list = opal_parse_loc_code(indicator,
						   list, dirent->d_name);

		free(dirlist[i]);
	}

	*loc = list;
	free(dirlist);
	return 0;
}

/* Open /sys/class/leds/<loc_code>:<led_type>/brightnes file */
static int
open_led_brightness_file(struct loc_code *loc, int mode)
{
	const char *indicator_desc;
	char filename[PATH_MAX];
	int len;
	int fd;
	int rc = -1;

	indicator_desc = get_indicator_desc(loc->index);
	if (!indicator_desc)
		return rc;

	len = snprintf(filename, PATH_MAX, "%s/%s:%s/%s",
		       OPAL_LED_SYSFS_PATH, loc->code,
		       indicator_desc, OPAL_LED_FILE);
	if (len >= PATH_MAX) {
		log_msg("%s : Path to LED brightness file is too big",
			__func__);
		return rc;
	}

	fd = open(filename, mode);
	if (fd == -1)
		log_msg("%s : Failed to open %s file", __func__, filename);

	return fd;
}

static inline void
close_led_brightness_file(int fd)
{
	close(fd);
}

/**
 * opal_get_indicator - Get OPAL LED state
 *
 * @indicator	identification or attention indicator
 * @loc		location code of the sensor
 * @state	return sensor state for the given loc
 *
 * Returns :
 *	0 on success, !0 on failure
 */
static int
opal_get_indicator(struct loc_code *loc, int *state)
{
	int fd;
	int rc = -1;
	char buf[4];

	fd = open_led_brightness_file(loc, O_RDONLY);
	if (fd == -1)
		return rc;

	rc = read(fd, buf, 4);
	if (rc == -1) {
		log_msg("%s : Failed to read LED state. (%d : %s)",
			__func__, errno, strerror(errno));
		close_led_brightness_file(fd);
		return rc;
	}

	*state = atoi(buf);
	close_led_brightness_file(fd);
	return 0;
}

/**
 * opal_set_indicator - Set OPAL indicator
 *
 * @indicator	identification or attention indicator
 * @loc		location code of OPAL indicator
 * @new_value	value to update indicator to
 *
 * Returns :
 *	0 on success, !0 on failure
 */
static int
opal_set_indicator(struct loc_code *loc, int new_value)
{
	char buf = '0';
	int fd;
	int rc = -1;

	if (new_value)
		buf = '1';

	fd = open_led_brightness_file(loc, O_WRONLY);
	if (fd == -1)
		return rc;

	rc = write(fd, &buf, 1);
	if (rc == -1) {
		log_msg("%s : Failed to modify LED state. (%d : %s)",
			__func__, errno, strerror(errno));
		close_led_brightness_file(fd);
		return rc;
	}

	close_led_brightness_file(fd);
	return 0;
}


/*
 * opal_indicator_probe - Probe indicator support on OPAL based platform
 *
 * Returns:
 *   0 if indicator is supported, else -1
 */
static int
opal_indicator_probe(void)
{
	int rc = -1;
	DIR *led_dir;
	struct dirent *dirent;

	led_dir = open_sysfs_led_dir();
	if (!led_dir)
		return rc;

	/*
	 * LED is supported if we have at least directory
	 * under /sys/class/leds
	 */
	while ((dirent = readdir(led_dir)) != NULL) {
		if (!strcmp(dirent->d_name, ".") ||
		    !strcmp(dirent->d_name, ".."))
			continue;

		if (dirent->d_type != DT_DIR && dirent->d_type != DT_LNK)
			continue;

		close_sysfs_led_dir(led_dir);
		return 0;
	}

	fprintf(stderr, "Service indicators are not supported on this system."
		"\nMake sure 'leds_powernv' kernel module is loaded.\n");
	close_sysfs_led_dir(led_dir);
	return rc;
}

/**
 * opal_get_indicator_mode - Gets the service indicator operating mode
 *
 * Returns :
 *	operating mode value
 */
static int
opal_get_indicator_mode(void)
{
	char	buf[128];
	int	rc = 0;
	int	fd;
	int	readsz;

	fd = open(OPAL_LED_MODE_PROPERTY, O_RDONLY);
	if (fd == -1) {
		log_msg("%s : Failed to open led mode DT file : %s (%d : %s)",
			__func__, OPAL_LED_MODE_PROPERTY,
			errno, strerror(errno));
		return -1;
	}

	readsz = read(fd, buf, 128);
	if (readsz == -1) {
		log_msg("%s : Failed to read led mode : %s (%d : %s)",
			__func__, OPAL_LED_MODE_PROPERTY,
			errno, strerror(errno));
		close(fd);
		return -1;
	}

	if (!strcmp(buf, OPAL_LED_MODE_LIGHT_PATH))
		operating_mode = LED_MODE_LIGHT_PATH;
	else if (!strcmp(buf, OPAL_LED_MODE_GUIDING_LIGHT))
		operating_mode = LED_MODE_GUIDING_LIGHT;
	else {
		log_msg("%s : Unknown LED operating mode\n", __func__);
		rc = -1;
	}

	close(fd);
	return rc;
}

/**
 * opal_get_indicator_list - Build indicator list of given type
 *
 * @indicator	identification or fault indicator
 * @list	loc_code structure
 *
 * Returns :
 *	0 on success, !0 otherwise
 */
static int
opal_get_indicator_list(int indicator, struct loc_code **list)
{
	int	rc;

	/*
	 * We treat first indicator in fault indicator list as
	 * check log indicator. Hence parse attention indicator.
	 */
	if (indicator == LED_TYPE_FAULT) {
		rc = opal_get_indices(LED_TYPE_ATTN, list);
		if (rc)
			return rc;
	}

	/* Get OPAL indicator list */
	rc = opal_get_indices(indicator, list);
	if (rc)
		return rc;

	/* FRU fault indicators are not supported in Guiding Light mode */
	if (indicator == LED_TYPE_FAULT &&
	    operating_mode == LED_MODE_GUIDING_LIGHT)
		return rc;

	/* SES indicators */
	get_ses_indices(indicator, list);

	return rc;
}

/**
 * opal_get_indicator_state - Retrieve the current state for an indicator
 *
 * @indicator	identification or attention indicator
 * @loc		location code of the sensor
 * @state	return location for the sensor state
 *
 * Returns :
 *	indicator return code
 */
static int
opal_get_indicator_state(int indicator, struct loc_code *loc, int *state)
{
	switch (loc->type) {
	case TYPE_OPAL:
		return opal_get_indicator(loc, state);
	case TYPE_SES:
		return get_ses_indicator(indicator, loc, state);
	default:
		break;
	}

	return -1;
}

/**
 * opal_set_indicator_state - Set an indicator to a new state (on or off)
 *
 * Call the appropriate routine for setting indicators based on the type
 * of indicator.
 *
 * @indicator	identification or attention indicator
 * @loc		location code of opal indicator
 * @new_value	value to update indicator to
 *
 * Returns :
 *	indicator return code
 */
static int
opal_set_indicator_state(int indicator, struct loc_code *loc, int new_value)
{
	switch (loc->type) {
	case TYPE_OPAL:
		return opal_set_indicator(loc, new_value);
	case TYPE_SES:
		return set_ses_indicator(indicator, loc, new_value);
	default:
		break;
	}

	return -1;
}

struct platform opal_platform = {
	.name			= "opal",
	.probe			= opal_indicator_probe,
	.get_indicator_mode	= opal_get_indicator_mode,
	.get_indicator_list	= opal_get_indicator_list,
	.get_indicator_state	= opal_get_indicator_state,
	.set_indicator_state	= opal_set_indicator_state,
};
