/**
 * @file	lp_diag.c
 * @brief	Light Path Diagnostics
 *
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 *
 * @author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <curses.h>
#include <ncurses/menu.h>
#include <signal.h>

#include "lp_diag.h"
#include "lp_util.h"

/* FRU callout priority as defined in PAPR+
 *
 * Note: Order of the priority is important!
 */
#define FRU_CALLOUT_PRIORITY	"HMABCL"

/* Servicelog command string */
#define SERVICELOG_EVENT_CMD \
	"Run \"/usr/bin/servicelog --query=\"id=%d\"\" for full details."

#define SERVICELOG_REPAIR_CMD \
	"Run \"/usr/bin/servicelog --query=\"repair=%d\"\" for full details."

/* loop */
#define	for_each_callout(callout, callouts) \
	for (callout = callouts; callout; callout = callout->next)

#define for_each_event(e, events) \
	for (e = events; e; e = e->next)

#define for_each_fru(fru, frus) \
	for (fru = frus; fru; fru = fru->next)

/* Row number for the menu begin */
#define BEGIN_Y	5
#define ENTER	10

/* FRU location code */
struct fru {
	char	location[LOCATION_LENGTH];
	struct	fru *next;
};

/* Global variables for LPD UI */
int	nlines;
char	*cur_state;
char	*prev_state;

static int startup_window();

/**
 * set_attn_indicator - Enable/disable system attention indicator
 *
 * @attn_loc	attn location
 * @new_state	indicator state
 *
 * @Returns :
 *	0 on success, !0 on failure
 */
static int
set_attn_indicator(struct loc_code *attn_loc, int new_state)
{
	int	rc;
	int	state;

	rc = get_indicator_state(ATTN_INDICATOR, attn_loc, &state);
	if (rc || state != new_state) {
		rc = set_indicator_state(ATTN_INDICATOR, attn_loc, new_state);

		if (rc)
			indicator_log_write("System Attention Indicator :"
					    "Unable to %s",
					    new_state ? "enable" : "disable");
		else
			indicator_log_write("System Attention Indicator : %s",
					    new_state ? "ON" : "OFF");
	}

	return rc;
}

/**
 * set_fault_indicator - Enable/disable location fault indicator
 *
 * @loc		location code
 * @new_state	indicator state
 *
 * @Returns :
 *	0 on success, !0 on failure
 */
static int
set_fault_indicator(struct loc_code *loc, int new_state)
{
	int	rc;
	int	state;

	rc = get_indicator_state(ATTN_INDICATOR, loc, &state);
	if (rc || state != new_state) {
		rc = set_indicator_state(ATTN_INDICATOR, loc, new_state);

		if (rc)
			indicator_log_write("%s : Unable to %s fault indicator",
					    loc->code,
					    new_state ? "enable" : "disable");
		else
			indicator_log_write("%s : Fault : %s", loc->code,
					    new_state ? "ON" : "OFF");
	}

	return rc;
}

/**
 * service_event_supported - Validate the event type and supported device
 *
 * @event	sl_event structure
 * @event_type	service event type
 *
 * Returns :
 *	1 on success / 0 on failure
 */
static int
service_event_supported(struct sl_event *event)
{
	struct	sl_data_os *os;
	struct	sl_data_enclosure *enclosure;

	/* we handle OS and Enclosure events only */
	switch (event->type) {
	case SL_TYPE_OS:
		os = event->addl_data;

		if (!device_supported(os->subsystem, os->driver)) {
			log_msg("Unsupported device driver : %s", os->driver);
			return 0;
		}
		break;
	case SL_TYPE_ENCLOSURE:
		enclosure = event->addl_data;

		if (!enclosure_supported(enclosure->enclosure_model)) {
			log_msg("Unsupported Enclosure model : %s",
				enclosure->enclosure_model);
			return 0;
		}
		break;
	case SL_TYPE_BMC:
	case SL_TYPE_RTAS:
	case SL_TYPE_BASIC:
	default:
		return 0;
	}

	return 1;
}

/**
 * free_fru_loc_code - Free location code list
 *
 * @frus	fru list
 *
 * Returns :
 *	nothing
 */
static void
free_fru_loc_code(struct fru *frus)
{
	struct fru *temp = frus;

	while (frus) {
		temp = frus;
		frus = frus->next;
		free(temp);
	}
}

/**
 * get_fru_indicator - get FRU indicator location code
 *
 * Truncates the last few characters off of a location code;
 * if fault indicator doesn't exist at the original location,
 * perhaps one exists at the location closer to the CEC.
 *
 * Returns:
 *	loc_code structure on success, NULL on failure
 */
static struct loc_code *
get_fru_indicator(struct loc_code *list, char *location, int *truncated)
{
	struct loc_code *loc_led;

retry:
	loc_led = get_indicator_for_loc_code(list, location);
	if (!loc_led) {	/* No indicator at original location */
		if (truncate_loc_code(location)) {
			*truncated = 1;
			goto retry;
		}
		return NULL;
	}
	return loc_led;
}

/**
 * build_callout_loc_code - Build location code list
 *
 * @event	sl_event
 * @list	loc_code structure
 * @frus	fru list
 * @attn_state	attention indicator state
 *
 * Returns :
 *	fru loc list, on success / NULL, on failure
 */
static struct fru *
build_callout_loc_code(struct sl_event *event, struct loc_code *list,
					 struct fru *frus, int *attn_state)
{
	int	found;
	int	truncated = 0;
	char	location[LOCATION_LENGTH];
	struct	fru *fru;
	struct	sl_callout *callout;
	struct	loc_code *loc_led;

	if (!event->callouts) {	/* Empty callout */
		*attn_state = 1;
		return frus;
	}

	for_each_callout(callout, event->callouts) {
		/* valid loc code ? */
		if (!callout->location || !strcmp(callout->location, "")) {
			*attn_state = 1;
			continue;
		}

		/* get FRUs nearest fault indicator */
		strncpy(location, callout->location, LOCATION_LENGTH);
		loc_led = get_fru_indicator(list, location, &truncated);
		if (!loc_led) { /* No indicator found for the given loc code */
			*attn_state = 1;
			continue;
		}

		found = 0;
		for_each_fru(fru, frus)
			/* location is added to callout list ? */
			if (!strcmp(fru->location, location)) {
				found = 1;
				break;
			}

		if (!found) {	/* Add location code to fru list */
			fru = frus;
			if (fru)
				while (fru->next)
					fru = fru->next;

			if (!fru) {
				fru = (struct fru *)malloc(sizeof
							   (struct fru));
				frus = fru;
			} else {
				fru->next = (struct fru *)malloc(sizeof
							       (struct fru));
				fru = fru->next;
			}
			if (!fru) {
				log_msg("Out of memory");
				free_fru_loc_code(frus);
				return NULL;
			}

			memset(fru, 0, sizeof(struct fru));
			strncpy(fru->location, location, LOCATION_LENGTH);
		}
	}

	return frus;
}

/**
 * event_fru_callout -	Parse the FRU callout list and enable
 *			location indicator
 *
 * @callouts		sl_callouts structure
 * @list		loc_code list
 * @fru_priority	FRU callout priority
 * @attn_on		Attention indicator state
 *
 * @Returns :
 *	1 on success / 0 on failure
 */
static int
event_fru_callout(struct sl_callout *callouts, struct loc_code *list,
					  char fru_priority, int *attn_on)
{
	int	rc = 0;
	int	truncated = 0;
	char	location[LOCATION_LENGTH];
	struct	sl_callout *callout;
	struct	loc_code *loc_led = NULL;

	/* Go over callout list */
	for_each_callout(callout, callouts) {
		if (callout->priority != fru_priority)
			continue;

		/* valid location code ? */
		if (!callout->location || !strcmp(callout->location, "")) {
			indicator_log_write("Empty location code in callout");
			*attn_on = 1;
			continue;
		}

		/* get FRUs nearest fault indicator */
		strncpy(location, callout->location, LOCATION_LENGTH);
		loc_led = get_fru_indicator(list, location, &truncated);
		if (!loc_led) { /* No indicator found for the given loc code */
			*attn_on = 1;
			indicator_log_write("%s does not have fault indicator",
					    callout->location);
			indicator_log_write("Could not truncate and get "
					    "location code closer to CEC");
			continue;
		}

		if (truncated) {
			indicator_log_write("%s does not have fault indicator",
							    callout->location);
			indicator_log_write("Truncated location : %s",
								location);
		}

		rc = set_fault_indicator(loc_led, INDICATOR_ON);
	}
	return rc;
}

/**
 * repair_fru_callout - Disable indicator
 *
 * @fru			FRU location code
 * @list		loc_code list
 * @attn_disable	Attention indicator state
 *
 * @Returns :
 *	1 on success / 0 on failure
 */
static int
repair_fru_callout(struct fru *fru, struct loc_code *list, int *attn_disable)
{
	int	rc = 0;
	struct	loc_code *loc_led = NULL;

	loc_led = get_indicator_for_loc_code(list, fru->location);
	if (loc_led)
		rc = set_fault_indicator(loc_led, INDICATOR_OFF);
	else {
		indicator_log_write("%s does not have fault indicator",
				    fru->location);
		*attn_disable = 1;
	}

	return rc;
}

/**
 * parse_service_event - Analyze events and enable LED if required
 *
 * @event_id	servicelog event ID
 *
 * Returns :
 *	0 on success, !0 on failure
 */
static int
parse_service_event(int event_id)
{
	int	i;
	int	rc;
	int	attn_on = 0;
	struct	sl_event *event = NULL;
	struct	loc_code *list = NULL;
	struct	loc_code *attn_loc;

	log_msg("Service event ID = %d", event_id);

	/* get servicelog event */
	rc = get_service_event(event_id, &event);
	if (rc)
		return rc;

	if (!event) {
		log_msg("Unable to read service event");
		return -1;
	}

	if (!service_event_supported(event)) {
		rc = 0; /* return success */
		goto event_out;
	}

	indicator_log_write("---- Service event begin ----");

	/* build indicator list */
	rc = get_indicator_list(ATTN_INDICATOR, &list);
	if (rc) {
		log_msg("Unable to retrieve fault indicators");
		indicator_log_write("Unable to retrieve fault indicators");
		goto indicator_out;
	}

	/* First loc code is system attention indicator */
	attn_loc = &list[0];

	if (operating_mode == LIGHT_PATH_MODE) {
		if (event->callouts)
			/* Run over FRU callout priority in order and
			 * enable fault indicator
			 */
			for (i = 0; FRU_CALLOUT_PRIORITY[i]; i++)
				rc = event_fru_callout(event->callouts, list,
						       FRU_CALLOUT_PRIORITY[i],
						       &attn_on);
		else {
			/* No callout list, enable check log indicator */
			indicator_log_write("Empty callout list");
			attn_on = 1;
		}

		if (attn_on)	/* check log indicator */
			rc = set_attn_indicator(attn_loc, INDICATOR_ON);
	} else {
		log_msg("Guiding Light mode");
		rc = set_attn_indicator(attn_loc, INDICATOR_ON);
	}

	if (rc)
		log_msg("Unable to enable fault indicator");

	/* free indicator list */
	free_indicator_list(list);

indicator_out:
	indicator_log_write(SERVICELOG_EVENT_CMD, event_id);
	indicator_log_write("---- Service event end ----");

event_out:
	/* free up servicelog event */
	servicelog_event_free(event);

	return rc;
}

/**
 * parse_repair_event -	Analyse the servicelog repair events and
 *			disable indicator if required
 *
 * @repair_id	servicelog repair ID
 *
 * Returns :
 *	0 on sucess, !0 on failure
 */
static int
parse_repair_event(int repair_id)
{
	int	rc;
	int	loc_match;
	int	attn_disable = 0;
	int	attn_keep = 0;
	struct	fru *r_fru = NULL;
	struct	fru *r_frus = NULL;
	struct	fru *o_fru = NULL;
	struct	fru *o_frus = NULL;
	struct	sl_event *repair_events = NULL;
	struct	sl_event *open_events = NULL;
	struct	sl_event *repair;
	struct	sl_event *event;
	struct	loc_code *attn_loc;
	struct	loc_code *list = NULL;

	log_msg("Repair event ID = %d", repair_id);

	/* get servicelog repair events */
	rc = get_repair_event(repair_id, &repair_events);
	if (rc)
		return rc;

	/* get_repair_event returns success, even if particular
	 * repair event didn't close any serviceable event(s).
	 */
	if (!repair_events) {
		log_msg("Repair event %d did not close any service event(s)",
								repair_id);
		return 0;
	}

	rc = get_all_open_service_event(&open_events);
	if (rc)
		goto repair_out;

	indicator_log_write("---- Repair event begin ----");

	/* build indicator list */
	rc = get_indicator_list(ATTN_INDICATOR, &list);
	if (rc) {
		log_msg("Unable to retrieve fault indicators");
		goto event_out;
	}

	/* First loc code is system attention indicator */
	attn_loc = &list[0];

	if (operating_mode == LIGHT_PATH_MODE) {
		for_each_event(repair, repair_events) {
			if (!service_event_supported(repair))
				continue;
			r_frus = build_callout_loc_code(repair, list,
							r_frus, &attn_disable);
		}

		for_each_event(event, open_events) {
			if (!service_event_supported(event))
				continue;
			o_frus = build_callout_loc_code(event, list,
							o_frus, &attn_keep);
		}

		for_each_fru(r_fru, r_frus) {
			loc_match = 0;
			for_each_fru(o_fru, o_frus) {
				/* Do not disable fault indicator, if there
				 * exists an open serviceable event(s) with
				 * that location code.
				 */
				if (!strcmp(r_fru->location, o_fru->location)) {
					loc_match = 1;
					break;
				}
			}
			if (loc_match)
				continue;

			rc = repair_fru_callout(r_fru, list, &attn_disable);
		}

		if (!attn_keep && attn_disable)
			rc = set_attn_indicator(attn_loc, INDICATOR_OFF);

	} else {
		log_msg("Guiding Light mode");

		if (!open_events) /* No more open events */
			rc = set_attn_indicator(attn_loc, INDICATOR_OFF);
	}

	if (rc)
		log_msg("Unable to disable fault indicator");

	/* free indicator list */
	free_indicator_list(list);

event_out:
	indicator_log_write(SERVICELOG_REPAIR_CMD, repair_id);
	indicator_log_write("---- Repair event end ----");

	servicelog_event_free(open_events);

repair_out:
	servicelog_event_free(repair_events);

	return rc;
}

/* UI_help - Print the help message for each selection
 *
 * @my_menu	list of identify and attention indicators
 *
 */
static void
UI_help(MENU *my_menu)
{
	int	x, y;
	int	c;
	const	char *desc = NULL;
	ITEM	*cur;
	WINDOW	*my_text_win;
	WINDOW	*my_help_win;
	char	*help1 = "This selection will turn all Identify Indicators\n"
			 "off.";
	char	*help2 = "This selection will turn System Attention\nIndicator"
			 " off.";
	char	*help3 = "This selection will toggle the Identify Indicator "
			 "state.\n\n"
			 "If a '+' appears to the left of this selection,\n"
			 "the transaction is waiting to be committed.\n\n"
			 "If an 'I' appears to the left of this selection,\nthe"
			 " Identify Indicator is currently in the \n'identify'"
			 " state.\n\n"
			 "If an 'I' does not appear to the left of this\n"
			 "selection, the Identify Indicator is currently\nin "
			 "the 'normal' state.";

	getmaxyx(stdscr, y, x);
	my_help_win = newwin(18, 55, y - 19, (x - 55)/2);
	my_text_win = newwin(15, 50, y - 17, (x - 50)/2);
	keypad(my_text_win, TRUE);
	box(my_help_win, 0, 0);
	wborder(my_help_win, '|', '|', '-', '-', '+', '+', '+', '+');
	wrefresh(my_help_win);

	mvwprintw(my_text_win, 14, 0, "F3=Cancel\t\tEnter");
	wrefresh(my_text_win);

	cur = current_item(my_menu);
	desc = item_description(cur);

	if (!desc)
		return;

	if (!strcmp(desc, "ident"))
		mvwprintw(my_text_win, 0, 0, help1);
	else if (!strcmp(desc, "attn"))
		mvwprintw(my_text_win, 0, 0, help2);
	else
		mvwprintw(my_text_win, 0, 0, help3);

	wrefresh(my_text_win);

	while ((c = wgetch(my_text_win))) {
		if (c == KEY_F(3) || c == ENTER)
			break;
	}

	delwin(my_text_win);
	delwin(my_help_win);
	touchwin(stdscr);
	wrefresh(stdscr);
	endwin();
}

/* UI_make_selection - Select a particular indicator
 *
 * @my_menu_win	window associated with my_menu
 * @my_menu	list of identify and attention indicators
 *
 */
static void
UI_make_selection(WINDOW *my_menu_win, MENU *my_menu)
{
	int	index;
	int	i;
	int	x, y;
	ITEM	*cur;
	const	char *desc = NULL;

	cur = current_item(my_menu);
	index = item_index(cur);
	desc = item_description(cur);
	getyx(my_menu_win, y, x);

	if (desc && !strcmp(desc, "ident")) {
		for (i = 0; i < item_count(my_menu); i++)
			if (cur_state[i] == 'I') {
				prev_state[i] = 'I';
				cur_state[i] = '+';
			}
		for (i = 0; i < nlines; i++)
			mvaddch(i + BEGIN_Y, 0, cur_state[i]);
	} else if (cur_state[index] == '+') {
		cur_state[index] = prev_state[index];
		mvaddch(y + BEGIN_Y, 0, cur_state[index]);
	} else {
		prev_state[index] = cur_state[index];
		cur_state[index] = '+';
		mvaddch(y + BEGIN_Y, 0, cur_state[index]);
	}

	refresh();
}

/* UI_commit - commits the changes made to the indicator states
 *
 * @my_menu_win	window associated with my_menu
 * @my_menu	list of identify and attention indicators
 * @ident_list	list of identify indicators
 * @attn_list	list of system attention and fault indicators
 *
 */
static void
UI_commit(WINDOW *my_menu_win, MENU *my_menu,
	  struct loc_code *ident_list, struct loc_code *attn_list)
{
	int	ident;
	int	index;
	int	i, j;
	int	err = 0;
	int	x, y;
	int	rc;
	int	c;
	char	*name;
	const	char *desc = NULL;
	ITEM	**items;
	ITEM	*cur;
	WINDOW	*my_help_win;
	struct	loc_code *loc;

	getmaxyx(stdscr, y, x);
	my_help_win = newwin(6, 55, y - 7, (x - 55)/2);
	keypad(my_help_win, TRUE);
	box(my_help_win, 0, 0);
	wborder(my_help_win, '|', '|', '-', '-', '+', '+', '+', '+');

	items = menu_items(my_menu);

	for (i = 0; i < item_count(my_menu); i++) {
		desc = item_description(items[i]);

		if (!desc) {
			 if (cur_state[i] == '+') {
				 cur_state[i] = prev_state[i];
				 err = 1;
			 }
			continue;
		}

		if (!strcmp(desc, "loc")) {
			name = (char *)item_name(items[i]);
			loc = get_indicator_for_loc_code(ident_list, name);
			rc = get_indicator_state(IDENT_INDICATOR,
						 loc, &ident);
			if (rc) {
				if (cur_state[i] == '+') {
					cur_state[i] = prev_state[i];
					err = 1;
				}
				continue;
			}
		}

		if (cur_state[i] == '+') {
			mvwprintw(my_help_win, 2, 8, "Processing data ...");
			wrefresh(my_help_win);

			if (!strcmp(desc, "attn")) {
				rc = set_attn_indicator(&attn_list[0], INDICATOR_OFF);
				cur_state[i] = ' ';

				if (rc)
					err = 1;
			} else {
				rc = set_indicator_state(IDENT_INDICATOR, loc,
							 ident ? INDICATOR_OFF : INDICATOR_ON);

				if (rc) {
					err = 1;
					cur_state[i] = ident? 'I' : ' ';
					continue;
				}
				cur_state[i] = ident ? ' ' : 'I';
				indicator_log_write("%s : Identify : %s",
						    loc->code, ident? "OFF" : "ON");
			}
		} else if (!strcmp(desc, "loc")) {
			cur_state[i] = ident? 'I' : ' ';
		}
	}

	cur = current_item(my_menu);
	index = item_index(cur);
	getyx(my_menu_win, y, x);

	for (i = index - y, j = 0; i < index - y + nlines; i++, j++)
		mvaddch(j + BEGIN_Y, 0, cur_state[i]);

	refresh();

	if (err) {
		mvwprintw(my_help_win, 2, 4, "Commit failed for one "
						"or more selections.");
		mvwprintw(my_help_win, 4, 10, "F3=Cancel\t\tEnter");

		while ((c = wgetch(my_help_win))) {
		       if (c == ENTER || c == KEY_F(3))
			   break;
		}
		wrefresh(my_help_win);
	}

	delwin(my_help_win);
	touchwin(my_menu_win);
	touchwin(stdscr);
	wrefresh(stdscr);
	wrefresh(my_menu_win);
}

/* UI_update_indicator - Updates the state of the indicator when
 *			the selected item changes
 *
 * @my_menu_win	window associated with my_menu
 * @my_menu	list of identify and attention indicators
 *
 */
static void
UI_update_indicator(WINDOW *my_menu_win, MENU *my_menu)
{
	int	index;
	int	i, j;
	int	x, y;
	int	count;
	ITEM	*cur;

	cur = current_item(my_menu);
	index = item_index(cur);
	count = item_count(my_menu);
	getyx(my_menu_win, y, x);

	if (count > nlines) {
		move(BEGIN_Y - 1, 0);
		clrtoeol();

		if (index - y == 0)
			printw("[TOP]");
		else
			printw("[MORE .. %d]", index - y);

		move(nlines + BEGIN_Y, 0);
		clrtoeol();

		if (index - y == count - nlines)
			printw("[BOTTOM]");
		else
			printw("[MORE .. %d]", count - (index - y + nlines));
	}

	for (i = index - y, j = 0; i < index - y + nlines; i++, j++)
		mvaddch(j + BEGIN_Y, 0, cur_state[i]);

	refresh();
}

/* UI_cleanup - release memory for states, my_items and menu
 *
 * @my_menu	list of identify and attention indicators
 * @my_items	items associated with menu
 *
 */
static void
UI_cleanup(MENU *my_menu, ITEM **my_items)
{
	int i;

	free(cur_state);
	free(prev_state);

	unpost_menu(my_menu);
	free_menu(my_menu);

	for (i = 0; i < item_count(my_menu); i++)
		free_item(my_items[i]);

	free(my_items);
}

void resize_handler(int sig)
{
}

/* create_menu -  Prints the list of location codes and allows
 *		the user to modify the state of the indicator
 *
 * @ident_list	list of identify indicators
 * @attn_list	list of system attention and fault indicators
 *
 * Returns :
 *	0 on success and !0 on failure
 */
static int
create_menu(struct loc_code *ident_list, struct loc_code *attn_list)
{
	int	x, y;
	int	i = 0;
	int	c;
	int	rc;
	int	ident;
	int	length = 0;
	int	count;
	struct	loc_code *cur;
	WINDOW	*my_menu_win;
	MENU	*my_menu;
	ITEM	**my_items;

	getmaxyx(stdscr, y, x);
	nlines = y - 13;

	if (ident_list)
		for(cur = ident_list; cur; cur = cur->next)
			length++;

	if (attn_list)
		length += 3;
	else
		length += 2;

	cur_state = (char *)malloc(length);
	prev_state = (char *)malloc(length);

	my_items = (ITEM **)calloc(length, sizeof(ITEM *));

	if (!cur_state || !prev_state || !my_items) {
		log_msg("Out of memory");
		return -1;
	}

	if (ident_list) {
		my_items[i] = new_item("Set ALL Identify "
				       "Indicator to NORMAL", "ident");
		cur_state[i] = ' ';
		i++;

		mvprintw(0, 0, "SYSTEM IDENTIFY INDICATOR");
		mvprintw(2, 0, "Determining system capabilities");
		mvprintw(4, 0, "Please stand by.");
		refresh();
	}

	if (attn_list) {
		my_items[i] = new_item("Set System Attention "
				       "Indicator to NORMAL", "attn");
		cur_state[i] = ' ';
		i++;
	}

	for (cur = ident_list, i; cur; cur = cur->next, i++) {
		/* Determine the identify indicator state */
		rc = get_indicator_state(IDENT_INDICATOR, cur, &ident);

		if (rc)
			cur_state[i] = ' ';
		else
			cur_state[i] = ident ? 'I' : ' ';

		my_items[i] = new_item(cur->code, "loc");
	}

	my_items[i] = (ITEM *)NULL;

	clear();

	if (!ident_list && attn_list)
		mvprintw(0, 0, "ATTENTION INDICATOR");
	else if (attn_list)
		mvprintw(0, 0, "IDENTIFY AND ATTENTION INDICATORS");
	else
		mvprintw(0, 0, "IDENTIFY INDICATORS");

	mvprintw(2, 0, "Make selection(s), use Commit to continue");
	mvprintw(LINES - 3, 0, "F1=Help\t\t\tF4=List\t\t\tF7=Commit\t\t\t"
					"F10=Exit\nF3=Previous Menu");
	my_menu = new_menu((ITEM **)my_items);
	menu_opts_off(my_menu, O_SHOWDESC);
	menu_opts_off(my_menu, O_ONEVALUE);
	my_menu_win = newwin(nlines, 70, BEGIN_Y , 1);
	keypad(my_menu_win, TRUE);
	set_menu_win(my_menu, my_menu_win);
	set_menu_format(my_menu, nlines, 1);
	set_menu_mark(my_menu, " ");
	post_menu(my_menu);

	count = item_count(my_menu);
	if (count > nlines) {
		mvprintw(BEGIN_Y - 1, 0, "[TOP]");
		mvprintw(nlines + BEGIN_Y, 0, "[MORE .. %d]", count - nlines);
	} else {
		nlines = count;
	}

	for (i = 0; i < nlines; i++)
		mvaddch(i + BEGIN_Y, 0, cur_state[i]);

	refresh();

	while ((c = wgetch(my_menu_win)) != KEY_F(10)) {
		switch (c) {
		case KEY_F(1):
			UI_help(my_menu);
			touchwin(my_menu_win);
			break;
		case KEY_DOWN:
			menu_driver(my_menu, REQ_DOWN_ITEM);
			UI_update_indicator(my_menu_win, my_menu);
			break;
		case KEY_UP:
			menu_driver(my_menu, REQ_UP_ITEM);
			UI_update_indicator(my_menu_win, my_menu);
			break;
		case KEY_PPAGE:
			menu_driver(my_menu, REQ_SCR_UPAGE);
			UI_update_indicator(my_menu_win, my_menu);
			break;
		case KEY_NPAGE:
			menu_driver(my_menu, REQ_SCR_DPAGE);
			UI_update_indicator(my_menu_win, my_menu);
				break;
		case ENTER:
			UI_make_selection(my_menu_win, my_menu);
			break;
		case KEY_F(7):
			UI_commit(my_menu_win, my_menu, ident_list, attn_list);
			break;
		case KEY_F(3):
			clear();
			UI_cleanup(my_menu, my_items);
			startup_window();
			return 0;
		}
			wrefresh(my_menu_win);
	}

	UI_cleanup(my_menu, my_items);
	return 0;
}

/* startup_window - Explain the Lightpath instructions and details
 *
 * Returns:
 *	0 on success !0 on error
 */
static int
startup_window(void)
{
	int	x, y;
	int	c;
	int	rc, rv;
	struct	loc_code *attn_list = NULL;
	struct	loc_code *ident_list = NULL;
	char	*msg1 = "This program contains service indicator tasks. "
			"This should be\nused to view and manipulate the"
			" service indicators (LEDs).";
	char	*msg2 = "Several keys are used to control the procedures:\n"
			"- The Enter key continues the procedure or"
			" performs an action.\n"
			"- The cursor keys are used to select an option.\n";

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	signal(SIGWINCH, resize_handler);

	getmaxyx(stdscr, y, x);

	if (y < 24 || x < 80) {
		endwin();
		fprintf(stderr, "Light Path Diagnostics requires minimum "
				"of 24 line by 80 column display.\nCheck "
				"the window size and try again.\n");
		return -1;
	}

	rc = get_indicator_list(ATTN_INDICATOR, &attn_list);

	if (rc)
		log_msg("Unable to get system attention indicator");

	rv = get_indicator_list(IDENT_INDICATOR, &ident_list);

	if (rv)
		log_msg("Unable to get identify indicators");

	if (rc && rv) {
		endwin();
		fprintf(stderr, "System does not support service indicators\n");
		return rc;
	}

	mvprintw(0, 0, "SERVICE INDICATORS VERSION %s", VERSION);
	mvprintw(2, 0, msg1);
	mvprintw(5, 0, msg2);
	mvprintw(9, 0, "Press the F3 key to exit or press Enter to continue.");

	while ((c = getch()) !=  KEY_F(3)) {
		if (c == ENTER) {
			clear();
			rc = create_menu(ident_list, attn_list);
			break;
		}
	}

	free_indicator_list(attn_list);
	free_indicator_list(ident_list);

	endwin();
	return rc;
}

/**
 * print_usage - Print the usage statement
 */
static void
print_usage(const char *cmd)
{
	fprintf(stdout, "Usage:\n"
		"\t%s [-V] [-h]\n", cmd);
	fprintf(stdout, "\nOptions:\n");
	fprintf(stdout, "\t-V: Print the version of the command\n");
	fprintf(stdout, "\t-h: Print this message\n");
}

/* lp_diag command line arguments */
#define LP_DIAG_ARGS	"e:r:hV"

/* Options to be passed to getopt_long function */
static struct option longopts[] = {
	{"event",	required_argument,	NULL, 'e'},
	{"repair",	required_argument,	NULL, 'r'},
	{"help",	no_argument,		NULL, 'h'},
	{"version",	no_argument,		NULL, 'V'},
	{0, 0, 0, 0}
};

/**
 * main -
 */
int
main(int argc, char *argv[])
{
	int	c;
	int	rc = 0;
	int	event_flag = 0;
	int	repair_flag = 0;
	int	event_id = 0;
	int	repair_id = 0;
	char	*next_char;

	opterr = 0;
	while ((c = getopt_long(argc, argv, LP_DIAG_ARGS,
					longopts, NULL)) != EOF) {
		switch (c) {
		case 'e':
			event_flag = 1;
			event_id = (int)strtoul(optarg, &next_char, 10);
			if (optarg[0] == '\0' || *next_char != '\0' ||
							    event_id <= 0) {
				print_usage(argv[0]);
				exit(1);
			}
			break;
		case 'r':
			repair_flag = 1;
			repair_id = (int)strtoul(optarg, &next_char, 10);
			if (optarg[0] == '\0' || *next_char != '\0' ||
							repair_id <= 0) {
				print_usage(argv[0]);
				exit(1);
			}
			break;
		case 'V':
			fprintf(stdout, "%s %s\n", argv[0], VERSION);
			fflush(stdout);
			exit(0);
		case 'h':
			print_usage(argv[0]);
			exit(0);
		case '?':
		default:
			print_usage(argv[0]);
			exit(1);
		}
	}

	if (geteuid() != 0) {
		dbg("%s: Requires superuser privileges.", argv[0]);
		exit(1);
	}

	if (optind < argc) {
		dbg("Unrecognized argument : %s", argv[optind]);
		print_usage(argv[0]);
		exit(1);
	}

	if (event_flag && repair_flag) {
		dbg("The -e and -r options cannot be used together\n");
		print_usage(argv[0]);
		exit(1);
	}

	/* initialize */
	program_name = argv[0];
	rc = init_files();
	if (rc)
		goto cleanup;

	/* Light Path operating mode */
	rc = check_operating_mode();
	if (rc)
		goto cleanup;

	if (argc < 2) {
		log_msg("Starting LPD UI");
		rc = startup_window();
	}

	if (event_flag) {
		log_msg("Service event analysis");
		rc = parse_service_event(event_id);
	}

	if (repair_flag) {
		log_msg("Repair event analysis");
		rc = parse_repair_event(repair_id);
	}

cleanup:
	log_msg("%s exiting", program_name);
	close_files();

	exit(rc);
}
