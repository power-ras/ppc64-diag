/**
 * @file	indicator_rtas.c
 * @brief	RTAS indicator manipulation routines
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
#include <librtas.h>

#include "lp_diag.h"
#include "lp_util.h"

/* RTAS error buffer size */
#define RTAS_ERROR_BUF_SIZE	64

/* Indicator operating mode */
uint32_t	operating_mode;

/**
 * parse_work_area - Parse the working buffer
 *
 * @loc		list to add new data to
 * @buf		working area to parse
 *
 * Returns :
 *	pointer to new loc_code list
 */
static struct loc_code *
parse_rtas_workarea(struct loc_code *loc, const char *buf)
{
	int	i;
	int	num = *(int *)buf;
	struct	loc_code *curr = loc;

	if (curr)
		while (curr->next)
			curr = curr->next;

	buf += sizeof(uint32_t);
	for (i = 0; i < num; i++) {
		if (!curr) {
			curr =
			  (struct loc_code *)malloc(sizeof(struct loc_code));
			loc = curr;
		} else {
			curr->next =
			  (struct loc_code *)malloc(sizeof(struct loc_code));
			curr = curr->next;
		}
		if (!curr) {
			log_msg("Out of memory");
			/* free up previously allocated memory */
			free_indicator_list(loc);
			return NULL;
		}
		memset(curr, 0, sizeof(struct loc_code));

		curr->index = *(uint32_t *)buf;
		buf += sizeof(uint32_t);
		curr->length = *(uint32_t *)buf;
		buf += sizeof(uint32_t);
		strncpy(curr->code, buf, curr->length);
		buf += curr->length;
		curr->code[curr->length] = '\0';
		curr->length = strlen(curr->code) + 1;
		curr->type = TYPE_RTAS;
		curr->next = NULL;
	}

	return loc;
}

/**
 * librtas_error - Check for librtas specific return codes
 *
 * @Note:
 *	librtas_error is based on librtas_error.c in powerpc-util package.
 *	@author Nathan Fontenot <nfont@austin.ibm.com>
 *
 * This will check the error value for a librtas specific return code
 * and fill in the buffer with the appropraite error message.
 *
 * @error	return code from librtas
 * @buf		buffer to fill with error string
 * @size	size of "buffer"
 *
 * Returns :
 *	nothing
 */
static void
librtas_error(int error, char *buf, size_t size)
{
	switch (error) {
	case RTAS_KERNEL_INT:
		snprintf(buf, size, "No kernel interface to firmware");
		break;
	case RTAS_KERNEL_IMP:
		snprintf(buf, size, "No kernel implementation of function");
		break;
	case RTAS_PERM:
		snprintf(buf, size, "Non-root caller");
		break;
	case RTAS_NO_MEM:
		snprintf(buf, size, "Out of heap memory");
		break;
	case RTAS_NO_LOWMEM:
		snprintf(buf, size, "Kernel out of low memory");
		break;
	case RTAS_FREE_ERR:
		snprintf(buf, size, "Attempt to free nonexistant RMO buffer");
		break;
	case RTAS_TIMEOUT:
		snprintf(buf, size, "RTAS delay exceeded specified timeout");
		break;
	case RTAS_IO_ASSERT:
		snprintf(buf, size, "Unexpected librtas I/O error");
		break;
	case RTAS_UNKNOWN_OP:
		snprintf(buf, size, "No firmware implementation of function");
		break;
	default:
		snprintf(buf, size, "Unknown librtas error %d", error);
	}
}

/**
 * get_rtas_list - Gets rtas indicator list
 *
 * @indicator	identification or attention indicator
 * @loc		pointer to loc_code structure
 *
 * Returns :
 *	rtas call return value
 */
int
get_rtas_indices(int indicator, struct loc_code **loc)
{
	int	rc;
	int	index = 1;
	int	next_index;
	char	workarea[BUF_SIZE];
	char	err_buf[RTAS_ERROR_BUF_SIZE];
	struct	loc_code *list = NULL;

	do {
		rc = rtas_get_indices(0, indicator, workarea, BUF_SIZE,
				      index, &next_index);
		switch (rc) {
		case 1:		/* more data available */
			index = next_index;
			/* fall through */
		case 0:		/* success */
			list = parse_rtas_workarea(list, workarea);
			if (!list)
				return -99; /* Out of heap memory */
			break;
		case -1:	/* hardware error */
			log_msg("Hardware error retrieving indicator indices");
			free_indicator_list(list);
			break;
		case RTAS_UNKNOWN_OP:
			/* Yes, this is a librtas return code but it should
			 * be treated the same as a -3 return code, both
			 * indicate that functionality is not supported
			 */
			librtas_error(rc, err_buf, RTAS_ERROR_BUF_SIZE);
			/* fall through */
		case -3:	/* indicator type not supported. */
			log_msg("The %s indicators are not supported "
				"on this system", INDICATOR_TYPE(indicator));

			if (rc == RTAS_UNKNOWN_OP)
				log_msg(",\n%s", err_buf);

			free_indicator_list(list);
			break;
		case -4:	/* list changed; start over */
			free_indicator_list(list);
			list = NULL;
			index = 1;
			break;
		default:
			librtas_error(rc, err_buf, RTAS_ERROR_BUF_SIZE);
			log_msg("Could not retrieve data for %s "
				"indicators,\n%s",
				INDICATOR_TYPE(indicator), err_buf);
			free_indicator_list(list);
			break;
		}

	} while ((rc == 1) || (rc == -4));

	*loc = list;
	return rc;
}

/**
 * get_rtas_sensor - Retrieve a sensor value from rtas
 *
 * Call the rtas_get_sensor or rtas_get_dynamic_sensor librtas calls,
 * depending on whether the index indicates that the sensor is dynamic.
 *
 * @indicator	identification or attention indicator
 * @loc		location code of the sensor
 * @state	return sensor state for the given loc
 *
 * Returns :
 *	rtas call return code
 */
int
get_rtas_sensor(int indicator, struct loc_code *loc, int *state)
{
	int	rc;
	char	err_buf[RTAS_ERROR_BUF_SIZE];

	if (loc->index == DYNAMIC_INDICATOR)
		rc = rtas_get_dynamic_sensor(indicator,
					     (void *)loc, state);
	else
		rc = rtas_get_sensor(indicator, loc->index, state);

	switch (rc) {
	case 0:	/*success  */
		break;
	case -1:
		log_msg("Hardware error retrieving the indicator at %s",
			loc->code);
		break;
	case -2:
		log_msg("Busy while retrieving indicator at %s. "
			"Try again later", loc->code);
		break;
	case -3:
		log_msg("The indicator at %s is not implemented", loc->code);
		break;
	default:
		librtas_error(rc, err_buf, RTAS_ERROR_BUF_SIZE);

		log_msg("Could not get %ssensor %s indicators,\n%s",
			(loc->index == DYNAMIC_INDICATOR) ? "dynamic " : "",
			INDICATOR_TYPE(indicator), err_buf);
		break;
	}

	return rc;
}

/**
 * set_rtas_indicator - Set rtas indicator
 *
 * Call the rtas_set_indicator or rtas_set_dynamic_indicator librtas calls,
 * depending on whether the index indicates that the indicator is dynamic.
 *
 * @indicator	identification or attention indicator
 * @loc		location code of rtas indicator
 * @new_value	value to update indicator to
 *
 * Returns :
 *	rtas call return code
 */
int
set_rtas_indicator(int indicator, struct loc_code *loc, int new_value)
{
	int	rc;
	char	err_buf[RTAS_ERROR_BUF_SIZE];

	if (loc->index == DYNAMIC_INDICATOR)
		rc = rtas_set_dynamic_indicator(indicator,
						new_value, (void *)loc);
	else
		rc = rtas_set_indicator(indicator, loc->index, new_value);

	switch (rc) {
	case 0:	/*success  */
		break;
	case -1:
		log_msg("Hardware error while setting the indicator at %s",
			loc->code);
		break;
	case -2:
		log_msg("Busy while setting the indicator at %s. "
			"Try again later", loc->code);
		break;
	case -3:
		log_msg("The indicator at %s is not implemented", loc->code);
		break;
	default:
		librtas_error(rc, err_buf, RTAS_ERROR_BUF_SIZE);

		log_msg("Could not set %ssensor %s indicators,\n%s",
			(loc->index == DYNAMIC_INDICATOR) ? "dynamic " : "",
			INDICATOR_TYPE(indicator), err_buf);
		break;
	}

	return rc;
}
