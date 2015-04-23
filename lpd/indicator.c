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

#include "platform.h"
#include "indicator.h"
#include "lp_util.h"
#include "indicator_rtas.h"

/* Indicator operating mode */
uint32_t	operating_mode;

/* points to platform structure of running platform */
struct platform platform;

/* Map LED type to description. */
struct led_type_map {
	const int	type;
	const char	*desc;
};
static struct led_type_map led_type_map[] = {
	{LED_TYPE_IDENT,	LED_DESC_IDENT},
	{LED_TYPE_FAULT,	LED_DESC_FAULT},
	{LED_TYPE_ATTN,		LED_DESC_ATTN},
	{-1,			NULL},
};


/* Returns LED type */
int
get_indicator_type(const char *indicator_desc)
{
	int i;

	for (i = 0; led_type_map[i].desc != NULL; i++)
		if (!strcmp(led_type_map[i].desc, indicator_desc))
			return led_type_map[i].type;

	return -1;
}

/* Returns LED description */
const char *
get_indicator_desc(int indicator)
{
	int i;

	for (i = 0; led_type_map[i].type != -1; i++)
		if (led_type_map[i].type == indicator)
			return led_type_map[i].desc;

	return "Unknown";
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
 * probe_indicator - Check indicator support on running platform
 *
 * Returns :
 *	0 on success, -1 on failure
 */
int
probe_indicator(void)
{
	int p;
	p = get_platform();

	switch (p) {
	case PLATFORM_PSERIES_LPAR:
		platform = rtas_platform;
		break;
	case PLATFORM_POWERKVM:
	case PLATFORM_POWERKVM_GUEST:
	default:
		fprintf(stderr, "%s is not supported on the %s platform\n",
				program_name, __power_platform_name(p));
		return -1;
	}

	return 0;
}

/**
 * get_indicator_mode - Gets the service indicator operating mode
 *
 * Returns :
 *	operating mode value
 */
int
get_indicator_mode(void)
{
	if (!platform.get_indicator_mode)
		return -1;

	return platform.get_indicator_mode();
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
	if (!platform.get_indicator_list)
		return -1;

	return platform.get_indicator_list(indicator, list);
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
	if (!platform.get_indicator_state)
		return -1;

	return platform.get_indicator_state(indicator, loc, state);
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
	if (!platform.set_indicator_state)
		return -1;

	return platform.set_indicator_state(indicator, loc, new_value);
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
 * @new_value	LED_STATE_ON/LED_STATE_OFF
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
	if (encl && indicator == LED_TYPE_IDENT &&
				new_value == LED_STATE_OFF)
		set_indicator_state(indicator, encl, new_value);
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

	rc = get_indicator_list(LED_TYPE_FAULT, &list);
	if (rc)
		return rc;

	/**
	 * The first location code returned by get_rtas_indices RTAS call
	 * is check log indicator.
	 */
	clocation = &list[0];
	rc = set_indicator_state(LED_TYPE_FAULT, clocation, LED_STATE_ON);
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

	rc = get_indicator_list(LED_TYPE_FAULT, &list);
	if (rc)
		return rc;

	/**
	 * The first location code returned by get_rtas_indices RTAS call
	 * is check log indicator.
	 */
	clocation = &list[0];
	rc = set_indicator_state(LED_TYPE_FAULT, clocation, LED_STATE_OFF);
	free_indicator_list(list);

	return rc;
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
