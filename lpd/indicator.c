/**
 * @file	indicator.c
 * @brief	Indicator manipulation routines
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lp_diag.h"
#include "lp_util.h"
#include "indicator_ses.h"
#include "indicator_rtas.h"

/* Note:
 *	At present, we do not support all SES enclosures/devices. Consider
 *	removing device_supported() and enclosure_supported() once we
 *	have Light Path support for all enclosures/devices.
 */
/* List of all Light Path supported enclosures */
static struct {
	char *model;
} supported_ses_enclosure[] = {
	{"5888"},	/* Bluehawk */
	{"EDR1"},	/* Bluehawk */
	{NULL}
};

/* List of all Light Path supported devices */
static struct {
	char	*subsystem;
	char	*driver;
} os_supported_device[] = {
	{"net", "cxgb3"},	/* Chelsio network card*/
	{"net", "e1000e"},	/* Intel network card*/
	{NULL, NULL}
};

/**
 * device_supported - Check Light Path support for device
 *
 * @subsystem	subsystem
 * @driver	kernel driver
 *
 * Returns :
 *	1 if device is supported / 0 if device is not supported
 */
int
device_supported(const char *subsystem, const char *driver)
{
	int i;

	if (!subsystem || !driver)
		return 0;

	for (i = 0; os_supported_device[i].driver; i++)
		if (!strcmp(os_supported_device[i].subsystem, subsystem) &&
		    !strcmp(os_supported_device[i].driver, driver))
			return 1;
	return 0;
}

/**
 * enclosure_supported - Check Light Path support for enclosure
 *
 * @model	enclosure model
 *
 * Returns :
 *	1 if enclosure is supported / 0 if enclosure is not supported
 */
int
enclosure_supported(const char *model)
{
	int i;

	if (!model)
		return 0;

	for (i = 0; supported_ses_enclosure[i].model; i++)
		if (!strcmp(supported_ses_enclosure[i].model, model))
			return 1;
	return 0;
}

/**
 * get_indicator_state - Retrieve the current state for an indicator
 *
 * Call the appropriate routine for retrieving indicator values based on the
 * type of indicator.
 *
 * @indicator	identification or attention indicator
 * @loc		location code of the sensor
 * @state	return location for the sensor state
 *
 * Returns :
 *	indicator return code
 */
int
get_indicator_state(int indicator, struct loc_code *loc, int *state)
{
	switch (loc->type) {
	case TYPE_RTAS:
		return get_rtas_sensor(indicator, loc, state);
	case TYPE_SES:
		return get_ses_indicator(indicator, loc, state);
	default:
		break;
	}

	return -1;
}

/**
 * set_indicator_state - Set an indicator to a new state (on or off)
 *
 * Call the appropriate routine for setting indicators based on the type
 * of indicator.
 *
 * @indicator	identification or attention indicator
 * @loc		location code of rtas indicator
 * @new_value	value to update indicator to
 *
 * Returns :
 *	indicator return code
 */
int
set_indicator_state(int indicator, struct loc_code *loc, int new_value)
{
	switch (loc->type) {
	case TYPE_RTAS:
		return set_rtas_indicator(indicator, loc, new_value);
	case TYPE_SES:
		return set_ses_indicator(indicator, loc, new_value);
	default:
		break;
	}

	return -1;
}

/**
 * get_all_indicator_state - Get all indicators current state
 *
 * @indicator	identification or attention indicator
 * @loc		location code structure
 *
 * Returns :
 *	nothing
 */
void
get_all_indicator_state(int indicator, struct loc_code *loc)
{
	int	rc;
	int	state;

	while (loc) {
		rc = get_indicator_state(indicator, loc, &state);
		if (rc) /* failed to get indicator state */
			loc->state = -1;
		else
			loc->state = state;
		loc = loc->next;
	}
}

/**
 * set_all_indicator_state - Sets all indicators state
 *
 * @indicator	identification or attention indicator
 * @loc		location code structure
 * @new_value	INDICATOR_ON/INDICATOR_OFF
 *
 * Returns :
 *	0 on success, !0 on failure
 */
void
set_all_indicator_state(int indicator, struct loc_code *loc, int new_value)
{
	int	state;
	int	rc;
	struct	loc_code *encl = &loc[0];

	while (loc) {
		rc = get_indicator_state(indicator, loc, &state);
		if (rc || state != new_value)
			set_indicator_state(indicator, loc, new_value);

		loc = loc->next;
	}

	/* If enclosure identify indicator is turned ON explicitly,
	 * then turning OFF all components identify indicator inside
	 * enclosure does not turn OFF enclosure identify indicator.
	 */
	if (encl && indicator == IDENT_INDICATOR &&
				new_value == INDICATOR_OFF)
		set_indicator_state(indicator, encl, new_value);
}

/**
 * check_operating_mode - Gets the service indicator operating mode
 *
 * Note: There is no defined property in PAPR+ to determine the indicator
 *	 operating mode. There is some work being done to get property
 *	 into PAPR. When that is done we will check for that property.
 *
 *	 At present, we will query RTAS fault indicators. It should return
 *	 at least one fault indicator, that is check log indicator. If only
 *	 one indicator is returned, then Guiding Light mode else Light Path
 *	 mode.
 *
 * Returns :
 *	operating mode value
 */
int
check_operating_mode(void)
{
	int	rc;
	struct	loc_code *list = NULL;

	rc = get_rtas_indices(ATTN_INDICATOR, &list);
	if (rc)
		return -1;

	if (!list)	/* failed */
		return -1;
	else if (!list->next)
		operating_mode = GUIDING_LIGHT_MODE;
	else
		operating_mode = LIGHT_PATH_MODE;

	free_indicator_list(list);
	return 0;
}

/**
 * enable_check_log_indicator - Enable check log indicator
 *
 * Returns :
 *	0 on sucess, !0 on failure
 */
int
enable_check_log_indicator(void)
{
	int	rc;
	struct	loc_code *list = NULL;
	struct	loc_code *clocation;

	rc = get_rtas_indices(ATTN_INDICATOR, &list);
	if (rc)
		return rc;

	/**
	 * The first location code returned by get_rtas_indices RTAS call
	 * is check log indicator.
	 */
	clocation = &list[0];
	rc = set_indicator_state(ATTN_INDICATOR, clocation, INDICATOR_ON);
	free_indicator_list(list);

	return rc;
}

/**
 * disable_check_log_indicator - Disable check log indicator
 *
 * Returns :
 *	0 on sucess, !0 on failure
 */
int
disable_check_log_indicator(void)
{
	int	rc;
	struct	loc_code *list = NULL;
	struct	loc_code *clocation;

	rc = get_rtas_indices(ATTN_INDICATOR, &list);
	if (rc)
		return rc;

	/**
	 * The first location code returned by get_rtas_indices RTAS call
	 * is check log indicator.
	 */
	clocation = &list[0];
	rc = set_indicator_state(ATTN_INDICATOR, clocation, INDICATOR_OFF);
	free_indicator_list(list);

	return rc;
}

/**
 * get_indicator_list - Build indicator list of given type
 *
 * @indicator	identification or attention indicator
 * @list	loc_code structure
 *
 * Returns :
 *	0 on success, !0 otherwise
 */
int
get_indicator_list(int indicator, struct loc_code **list)
{
	int	rc;

	/* Get RTAS indicator list */
	rc = get_rtas_indices(indicator, list);
	if (rc)
		return rc;

	/* FRU fault indicators are not supported in Guiding Light mode */
	if (indicator == ATTN_INDICATOR &&
	    operating_mode == GUIDING_LIGHT_MODE)
		return rc;

	/* SES indicators */
	get_ses_indices(indicator, list);

	return rc;
}

/**
 * get_loc_code_for_dev - Get location code for the given device
 *
 * @device	device name
 * @location	output location code
 * @locsize	location code size
 *
 * Returns :
 *	0 on success / -1 on failure
 */
int
get_loc_code_for_dev(const char *device, char *location, int locsize)
{
	struct	dev_vpd vpd;

	memset(&vpd, 0, sizeof(struct dev_vpd));
	if (device && !read_vpd_from_lsvpd(&vpd, device)) {
		if (location && vpd.location[0] != '\0') {
			strncpy(location, vpd.location, locsize);
			return 0;
		}
	}
	return -1;
}

/**
 * get_indicator_for_loc_code - Compare device location code with indicator list
 *
 * @list	indicator list
 * @location	device location code
 *
 * Returns :
 *	on success, loc_code structure
 *	on failure, NULL
 */
struct loc_code *
get_indicator_for_loc_code(struct loc_code *list, const char *location)
{
	while (list) {
		if (!strcmp(list->code, location))
			return list;
		list = list->next;
	}

	return NULL;
}

/**
 * truncate_loc_code - Truncate the last few characters of a location code
 *
 * Truncates the last few characters off of a location code; if an
 * indicator doesn't exist at the orignal location, perhaps one exists
 * at a location closer to the CEC.
 *
 * @loccode	location code to truncate
 *
 * Returns :
 *	1 - successful truncation / 0 - could not be truncated further
 */
int
truncate_loc_code(char *loccode)
{
	int i;

	for (i = strlen(loccode) - 1; i >= 0; i--) {
		if (loccode[i] == '-') {
			loccode[i] = '\0';
			return 1;	/* successfully truncated */
		}
	}

	return 0;
}

/**
 * free_indicator_list - Free loc_code structure
 *
 * @loc		list to free
 *
 * Return :
 *	nothing
 */
void
free_indicator_list(struct loc_code *loc)
{
	struct loc_code *tmp = loc;

	while (loc) {
		tmp = loc;
		loc = loc->next;
		free(tmp);
	}
}

/**
 * fill_indicators_vpd - Fill indicators vpd data
 */
void
fill_indicators_vpd(struct loc_code *list)
{
	struct	dev_vpd vpd;
	struct	loc_code *curr;

	for (curr = list; curr; curr = curr->next) {
		/* zero out the vpd structure */
		memset(&vpd, 0, sizeof(struct dev_vpd));

		if (read_vpd_from_lsvpd(&vpd, curr->code))
			return;

		strncpy(curr->devname, vpd.dev, DEV_LENGTH);
		strncpy(curr->mtm, vpd.mtm, VPD_LENGTH);
		strncpy(curr->sn, vpd.sn, VPD_LENGTH);
		strncpy(curr->pn, vpd.pn, VPD_LENGTH);
		strncpy(curr->fru, vpd.fru, VPD_LENGTH);
		/* retain existing DS */
		if (curr->ds[0] == '\0')
			strncpy(curr->ds, vpd.ds, VPD_LENGTH);
	}
}

/**
 * is_enclosure_loc_code -
 */
int
is_enclosure_loc_code(struct loc_code *loc)
{
	if (strchr(loc->code, '-') == NULL)
		return 1;

	return 0;
}
