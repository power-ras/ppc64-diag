using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <string>
#include <vector>
#include <iostream>

#include "catalogs.h"

static const char *progname;
bool debug = 0;
extern ReporterCatalog reporter_catalog;
extern EventCatalog event_catalog;

static void usage_message(FILE *out)
{
	fprintf(out, "usage: %s [-b date] [-e date] [-m msgfile | -M]\n"
				"\t[-C catalog_dir] [-h] [-d]\n", progname);
}

static void usage(void)
{
	usage_message(stderr);
	exit(1);
}

static void report_event(SyslogEvent *event, SyslogMessage *msg,
							const char *line)
{
	MatchVariant *mv = event->matched_variant;
	Reporter *reporter = mv->reporter_alias->reporter;

	cout << endl << line;
	cout << "matches: " << event->reporter_name
		<< " \"" << event->escaped_format << "\"" << endl;

	size_t nr_prefix_args = msg->prefix_args.size();
	if (nr_prefix_args > 0) {
		unsigned int i;
		for (i = 0; i < nr_prefix_args; i++) {
			string arg_name = reporter->prefix_args->at(i);
			if (i > 0)
				cout << "  ";
			cout << arg_name << "=" << msg->prefix_args[arg_name];
		}
		cout << endl;

		if (msg->set_devspec_path(event) == 0)
			cout << "devspec: " << msg->devspec_path << endl;
	}

	cout << "subsystem: " << event->driver->subsystem << endl;
	cout << "severity: " << severity_name(mv->severity) << endl;
	if (event->source_file)
		cout << "file: " << "\"" << *(event->source_file) << "\""
								<< endl;

	ExceptionMsg *em = event->exception_msg;
	string description, action;
	if (em) {
		description = em->description;
		action = em->action;
	} else {
		description = event->description;
		action = event->action;
	}
	cout << "description:" << endl << indent_text_block(description, 2)
								<< endl;
	cout << "action:" << endl << indent_text_block(action, 2) << endl;
}

static time_t
parse_date_arg(const char *date_str, const char *arg_name)
{
	char *date_end = NULL;
	time_t t = parse_syslogish_date(date_str, &date_end);
	if (!t || *date_end != '\0') {
		cerr << "unrecognized date format for " << arg_name
			<< " option" << endl;
		exit(1);
	}
	return t;
}

static void
print_help(void)
{
	usage_message(stdout);
	printf(
"-b begin_time\tIgnore messages with timestamps prior to begin_time.\n"
"-C catalog_dir\tUse message catalog in catalog_dir.  Defaults to\n"
"\t\t\t/etc/ppc64-diag/message_catalog.\n"
"-d\t\tPrint debugging output on stderr.\n"
"-e end_time\tStop upon reading message with timestamp after end_time.\n"
"-h\t\tPrint this help text and exit.\n"
"-m message_file\tRead syslog messages from message_file, not stdin.\n"
"-M\t\tRead syslog messages from /var/log/messages.\n"
	);
}

int main(int argc, char **argv)
{
	int c;
	time_t begin_date = 0, end_date = 0;
	const char *catalog_dir = ELA_CATALOG_DIR;
	const char *msg_path = NULL;
	FILE *msg_file = stdin;
	vector<SyslogEvent*>::iterator ie;

	progname = argv[0];
	opterr = 0;
	while ((c = getopt(argc, argv, "b:C:de:hm:M")) != -1) {
		switch (c) {
		case 'b':
			begin_date = parse_date_arg(optarg, "-b");
			break;
		case 'C':
			catalog_dir = optarg;
			break;
		case 'd':
			debug = true;
			break;
		case 'e':
			end_date = parse_date_arg(optarg, "-e");
			break;
		case 'h':
			print_help();
			exit(0);
		case 'm':
			msg_path = optarg;
			break;
		case 'M':
			msg_path = "/var/log/messages";
			break;
		case '?':
			usage();
		}
	}
	if (optind != argc)
		usage();

	if (msg_path) {
		msg_file = fopen(msg_path, "r");
		if (!msg_file) {
			perror(msg_path);
			exit(2);
		}
	}

	if (EventCatalog::parse(catalog_dir) != 0)
		exit(2);

	if (debug) {
		vector<Reporter*>::iterator ir;
		for (ir = reporter_catalog.rlist.begin();
				ir < reporter_catalog.rlist.end(); ir++) {
			cout << "-----" << endl;
			cout << **ir;
		}

		vector<MetaReporter*>::iterator imr;
		for (imr = reporter_catalog.mrlist.begin();
				imr < reporter_catalog.mrlist.end(); imr++) {
			cout << "-----" << endl;
			cout << **imr;
		}

		for (ie = event_catalog.events.begin();
				ie < event_catalog.events.end(); ie++) {
			cout << "-----" << endl;
			cout << **ie;
		}
	}

#define LINESZ 256
	char line[LINESZ];
	int skipped = 0;
	bool prev_line_truncated = false, cur_line_truncated;
	while (fgets(line, LINESZ, msg_file)) {
		if (strchr(line, '\n'))
			cur_line_truncated = false;
		else {
			/*
			 * syslog-ng "Log statistics" messages can be very
			 * long, so don't complain about such monstrosities
			 * by default.
			 */
			if (debug)
				cerr << "message truncated to " << LINESZ-1
						<< " characters!" << endl;
			line[LINESZ-2] = '\n';
			line[LINESZ-1] = '\0';
			cur_line_truncated = true;
		}
		bool skip_fragment = prev_line_truncated;
		prev_line_truncated = cur_line_truncated;
		if (skip_fragment)
			continue;

		SyslogMessage msg(line);
		if (!msg.parsed) {
			if (debug)
				cerr << "unparsed message: " << line;
			skipped++;
			continue;
		}
		if (begin_date && difftime(msg.date, begin_date) < 0)
			continue;
		if (end_date && difftime(msg.date, end_date) > 0) {
			/*
			 * We used to break here (i.e., skip all the rest
			 * of the lines in the file).  But timestamps in
			 * syslog files sometimes jump backward, so it's
			 * possible to find lines in the desired timeframe
			 * even after we hit lines that are beyond it.
			 */
			continue;
		}
		SyslogEvent *unreported_exception = NULL;
		bool reported = false;
		for (ie = event_catalog.events.begin();
					ie < event_catalog.events.end(); ie++) {
			SyslogEvent *event = *ie;
			if (event->exception_msg
				&& (reported || unreported_exception)) {
				/*
				 * We've already matched an event, so
				 * don't bother trying to match exception
				 * catch-alls.
				 */
				continue;
			}
			if (event->match(&msg, true)) {
				if (skipped > 0) {
					cout << endl << "[Skipped " << skipped
						<< " unrecognized messages]"
						<< endl;
					skipped = 0;
				}

				if (event->exception_msg)
					unreported_exception = event;
				else {
					report_event(event, &msg, line);
					reported = true;
				}
			}
		}
		if (!reported) {
			if (unreported_exception)
				report_event(unreported_exception, &msg, line);
			else
				skipped++;
		}
	}
	if (skipped > 0)
		cout << endl << "[Skipped " << skipped
				<< " unrecognized messages]" << endl;
	exit(0);
}
