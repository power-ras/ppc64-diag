#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

typedef enum { false, true } bool;
static int cur_year = 0;	// year - 1900
static time_t end_of_cur_year;	// January 1 00:00:00 of next year

/* Called at beginning of time and each time a new year begins. */
static void
compute_cur_year(time_t now)
{
	struct tm tm;
	localtime_r(&now, &tm);
	cur_year = tm.tm_year;

	tm.tm_mon = 0;
	tm.tm_mday = 1;
	tm.tm_year++;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	tm.tm_isdst = -1;
	end_of_cur_year = mktime(&tm);
}

/*
 * Call strptime() to parse the date string starting at start, according
 * to fmt.
 *
 * The year defaults to either this year or last year -- whatever will
 * yield a date in the preceding 12 months.  If yr_in_fmt == true,
 * it's assumed that fmt will provide the year, and that'll be used
 * instead of the default.
 *
 * If end isn't NULL, *end is set pointing to the next character after
 * the parsed date string.  Returns the date as a time_t.
 *
 * If the string can't be parsed according to fmt, *end is unchanged
 * and 0 is returned.
 */
time_t
parse_date(const char *start, char **end, const char *fmt, bool yr_in_fmt)
{
	struct tm tm;
	time_t now, date;
	char *date_end;

	if (!yr_in_fmt) {
		now = time(NULL);
		if (!cur_year || difftime(now, end_of_cur_year) >= 0)
			compute_cur_year(now);
	}
	memset(&tm, 0, sizeof(tm));
	tm.tm_isdst = -1;
	tm.tm_year = cur_year;
	date_end = strptime(start, fmt, &tm);
	if (date_end == NULL)
		return (time_t) 0;
	if (tm.tm_year < 69) {
		/* year < 1969.  Mistook hour for year? */
		return (time_t) 0;
	}
	date = mktime(&tm);
	if (!yr_in_fmt && difftime(date, now) > 0) {
		/* Date is in future.  Assume it's from last year. */
		tm.tm_isdst = -1;
		tm.tm_year--;
		date = mktime(&tm);
	}
	if (end)
		*end = date_end;
	return date;
}

time_t
parse_syslog_date(const char *start, char **end)
{
	return parse_date(start, end, "%b %d %T", false);
}

struct date_fmt {
	const char *fmt;
	bool has_year;
};

/* Order is important: try longest match first. */
static struct date_fmt day_fmts[] = {
	{ "%b %d %Y", true },	// Jan 15 2010
	{ "%b %d", false },
	{ "%Y-%m-%d", true },	// 2010-1-15
	{ "%d %b %Y", true },	// 15 Jan 2010
	{ "%d %b", false },
	{ NULL, false }
};

static struct date_fmt time_fmts[] = {
	{ "%T %Y", true },
	{ "%T", false },
	{ "%H:%M %Y", true },
	{ "%H:%M", false },
	{ "", false },
	{ NULL, false }
};

/*
 * Parse the date and time pointed to by start, trying all valid
 * combinations of date and time formats from the above lists.
 * See parse_date() for more semantics.
 */
time_t
parse_syslogish_date(const char *start, char **end)
{
	time_t t;
	struct date_fmt *day, *time;
	char fmt[100];

	for (day = day_fmts; day->fmt; day++) {
		for (time = time_fmts; time->fmt; time++) {
			if (day->has_year && time->has_year)
				continue;
			(void) strcpy(fmt, day->fmt);
			if (time->fmt[0] != '\0') {
				(void) strcat(fmt, " ");
				(void) strcat(fmt, time->fmt);
			}
			t = parse_date(start, end, fmt,
					(day->has_year || time->has_year));
			if (t)
				return t;
		}
	}
	return 0;
}
#ifdef TEST
main()
{
	time_t t;
	char line[100];
	while (fgets(line, 100, stdin)) {
		t = parse_syslogish_date(line, NULL);
		if (!t)
			printf("no match\n");
		else
			printf("%s", ctime(&t));
	}
	exit(0);
}
#endif /* TEST */
