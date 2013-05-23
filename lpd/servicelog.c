/**
 * @file	servicelog.c
 * @brief	Read servicelog database
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <string.h>

#include "lp_diag.h"

/* Query string should be same as "match" field in notification script.
 * see scripts/lp_diag_setup script.
 */
#define SERVICELOG_QUERY_LEN	128
#define SERVICELOG_QUERY_STRING	"disposition>=1 and severity>=4 and " \
				"serviceable=1"

/**
 * get_servicelog_event - Retrieve servicelog event(s)
 *
 * @query	servicelog event query
 * @event	servicelog event structure
 *
 * Returns :
 *	0 on success, !0 on failure
 */
static int
get_servicelog_event(char *query, struct sl_event **event)
{
	int	rc;
	struct	servicelog *slog;

	rc = servicelog_open(&slog, 0);
	if (rc) {
		log_msg("Error opening servicelog db: %s", strerror(rc));
		return rc;
	}

	rc = servicelog_event_query(slog, query, event);
	if (rc)
		log_msg("Error reading servicelog db :%s",
			servicelog_error(slog));

	/* close database */
	servicelog_close(slog);

	return rc;
}

/**
 * get_service_event - Get service event
 */
int
get_service_event(int event_id, struct sl_event **event)
{
	char	query[SERVICELOG_QUERY_LEN];

	snprintf(query, SERVICELOG_QUERY_LEN, "id=%d", event_id);

	return get_servicelog_event(query, event);
}

/**
 * get_all_open_service_event - Get all open serviceable event
 */
int
get_all_open_service_event(struct sl_event **event)
{
	int	len;
	char	query[SERVICELOG_QUERY_LEN];

	len = snprintf(query, SERVICELOG_QUERY_LEN, SERVICELOG_QUERY_STRING);
	snprintf(query + len, SERVICELOG_QUERY_LEN - len, " and closed=0");

	return get_servicelog_event(query, event);
}

/**
 * get_repair_event - Get repair event
 */
int
get_repair_event(int repair_id, struct sl_event **event)
{
	int	len;
	char	query[SERVICELOG_QUERY_LEN];

	len = snprintf(query, SERVICELOG_QUERY_LEN, SERVICELOG_QUERY_STRING);
	snprintf(query + len, SERVICELOG_QUERY_LEN - len,
		 " and repair=%d", repair_id);

	return get_servicelog_event(query, event);
}

/**
 * print_service_event - Print servicelog event
 */
int
print_service_event(struct sl_event *event)
{
	return servicelog_event_print(stdout, event, 1);
}
