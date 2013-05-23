using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

/*
 * This is needed for RTAS_FRUID_COMP_* (callout type, which Mike S. thinks
 * is largely ignored).
 */
#include <librtasevent.h>

#include <libvpd-2/vpdretriever.hpp>
#include <libvpd-2/component.hpp>
#include <libvpd-2/dataitem.hpp>
#include <libvpd-2/system.hpp>
#include <libvpd-2/vpdexception.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

using namespace lsvpd;

#include "catalogs.h"
#include <servicelog-1/servicelog.h>

#define SYSLOG_PATH "/var/log/messages"
#define LAST_EVENT_PATH "/var/log/ppc64-diag/last_syslog_event"
#define LAST_EVENT_PATH_BAK LAST_EVENT_PATH ".bak"

static const char *progname;
static bool debug = 0;
static time_t begin_date = 0, end_date = 0;
static const char *catalog_dir = ELA_CATALOG_DIR;
static const char *msg_path = NULL;
static FILE *msg_file = stdin;
static bool follow = false, follow_default = false;
static string last_msg_matched;	// read from LAST_EVENT_PATH
static bool skipping_old_messages;

extern ReporterCatalog reporter_catalog;
extern EventCatalog event_catalog;

static servicelog *slog = NULL;

static void
usage_message(FILE *out)
{
	fprintf(out, "usage: %s [-b date] [-e date | -F] [-m msgfile | -M]\n"
				"\t[-C catalog_dir] [-h] [-d]\n", progname);
}

static void usage(void)
{
	usage_message(stderr);
	exit(1);
}

/*
 * Convert a C++ string to a strdup-ed C string, or a NULL pointer.
 * If -d is specified, don't return any NULL pointers, because we're
 * going to call servicelog_event_print(), and that doesn't handle
 * NULLs well.
 */
static char *
svclog_string(const string& s, bool force=false)
{
	if (s.empty()) {
		if (debug || force)
			return strdup("none");
		return NULL;
	}
	return strdup(s.c_str());
}

/*
 * Used, in debug mode, to ensure we don't pass any null char pointers to
 * servicelog_event_print().
 */
static char *
fake_val(const char *valname)
{
	return strdup(valname);
}

/* Stuff for querying the Vital Product Data (VPD) database starts here. */
static System *vpd_root = NULL;
static bool vpd_uptodate_flag = false;

static bool
vpd_up_to_date(void)
{
	return vpd_uptodate_flag;
}

static System *
collect_vpd(void)
{
	VpdRetriever *vpd = NULL;

	/* Either succeed or give up forever. */
	vpd_uptodate_flag = true;

	try {
		vpd = new VpdRetriever();
	} catch (exception& e) {
		cerr << progname << ": VpdRetriever constructor threw exception"
								<< endl;
		return NULL;
	}
	try {
		vpd_root = vpd->getComponentTree();
	} catch (VpdException& ve) {
		cerr << progname << ": getComponentTree() failed: " << ve.what()
								<< endl;
		vpd_root = NULL;
	}
	delete vpd;
	if (!vpd_root) {
		cerr << progname << ": getComponentTree() returned null root"
								<< endl;
	}
	return vpd_root;
}

/*
 * Recursive search for the Component with the specificed location code,
 * among the specified list of Components, which are the children of the
 * System root or of a Component.  Recursion is a bit weird because a
 * System is not a Component.
 */
static Component *
get_device_by_location_code(const vector<Component*>& components,
						const string& location_code)
{
	vector<Component*>::const_iterator i, end;
	for (i = components.begin(), end = components.end(); i != end; i++) {
		Component *c = *i;
		if (c->getPhysicalLocation() == location_code)
			return c;
		else {
			Component *k = get_device_by_location_code(
						c->getLeaves(), location_code);
			if (k)
				return k;
		}
	}
	return NULL;
}

static Component *
get_device_by_location_code(const System *root, const string& location_code)
{
	if (root)
		return get_device_by_location_code(root->getLeaves(),
								location_code);
	return NULL;
}

static ssize_t
read_thing_from_file(const char *path, char *buf, size_t bufsz)
{
	int fd;
	ssize_t nbytes;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	nbytes = read(fd, buf, bufsz);
	close(fd);
	return nbytes;
}

/*
 * Try to find the /sys/.../devspec node associated with the device
 * that this syslog message is about.  That node contains the pathname
 * of the directory in /proc/device-tree for that device.  (The
 * pathname is not null- or newline-terminated.)  The device's
 * (null-terminated) location code is in the file ibm,loc-code in
 * that directory.  Using the location code, we look up the other
 * Vital Product Data for that device, as required to fill out the callout.
 *
 * Return 0 on (at least partial) success.  Caller has nulled out the
 * various members we might populate.
 */
static int
populate_callout_from_vpd(SyslogEvent *sys, SyslogMessage *msg,
			struct sl_event *svc, struct sl_callout *callout)
{
#define PROC_DEVICE_TREE_DIR "/proc/device-tree"
#define LOCATION_CODE_FILE "/ibm,loc-code"
	int result;
	char dev_tree_path[PATH_MAX];
	char *next, *end = dev_tree_path + PATH_MAX;
	char location_code[1000];
	ssize_t nbytes;

	result = msg->set_devspec_path(sys);
	if (result != 0)
		return result;
	next = dev_tree_path;
	(void) strcpy(next, PROC_DEVICE_TREE_DIR);
	next += strlen(PROC_DEVICE_TREE_DIR);

	/* /proc/device-tree^ */

	nbytes = read_thing_from_file(msg->devspec_path.c_str(),
							next, end - next);
	if (nbytes <= 0)
		return -1;

	/* /proc/device-tree^/xxx/yyy -- typically NOT newline-terminated */

	if (!strncmp(next, "none\n", 5))
		return -1;
	next += nbytes;

	/* /proc/device-tree/xxx/yyy^ */
	if ((unsigned)(end - next) < sizeof(LOCATION_CODE_FILE))
		return -1;
	strcpy(next, LOCATION_CODE_FILE);

	/* /proc/device-tree/xxx/yyy^/ibm,loc-code */

	nbytes = read_thing_from_file(dev_tree_path, location_code, 1000);
	if (nbytes < 0 || nbytes >= 1000)
		return -1;

	if (!vpd_up_to_date())
		vpd_root = collect_vpd();
	if (!vpd_root)
		return -1;

	Component *device = get_device_by_location_code(vpd_root,
							location_code);
	if (!device)
		return -1;
	callout->location = svclog_string(location_code);
	callout->fru = svclog_string(device->getFRU());
	callout->serial = svclog_string(device->getSerialNumber());
	const DataItem *ccin = device->getDeviceSpecific("CC");
	callout->ccin = svclog_string(ccin ? ccin->getValue() : "");
	return 0;
}
/* End of VPD query functions */

static bool
is_informational_event(SyslogEvent *sys)
{
	int severity = sys->get_severity();
	if (severity == LOG_DEBUG || severity == LOG_INFO)
		return true;
	/* Don't log catch-all events. */
	if (severity == LOG_SEV_UNKNOWN || severity == LOG_SEV_ANY)
		return true;
	if (sys->sl_severity == SL_SEV_DEBUG ||
				sys->sl_severity == SL_SEV_INFO)
		return true;
	if (sys->err_type == SYTY_INFO)
		return true;
	return false;
}

/*
 * Use the servicelog severity if it's specified.  Otherwise estimate it
 * from the syslog severity and error type.
 */
static uint8_t
get_svclog_severity(SyslogEvent *sys)
{
	if (sys->sl_severity != 0)
		return sys->sl_severity;

	switch (sys->get_severity()) {
	case LOG_DEBUG:
		return SL_SEV_DEBUG;
	case LOG_NOTICE:
	case LOG_INFO:
		return SL_SEV_INFO;
	case LOG_WARNING:
		return SL_SEV_WARNING;
	default:
		/*
		 * If we get here, the syslog error level is at least LOG_ERR.
		 */
		switch (sys->err_type) {
		case SYTY_PERM:
		case SYTY_CONFIG:
		case SYTY_PEND:
		case SYTY_PERF:
		case SYTY_UNKNOWN:
			return SL_SEV_ERROR;
		case SYTY_BOGUS:
		case SYTY_TEMP:
			return SL_SEV_WARNING;
		case SYTY_INFO:
			return SL_SEV_INFO;
		}
	}
	/* NOTREACHED */
	return 0;
}

static int
get_svclog_disposition(SyslogEvent *sys)
{
	if (sys->sl_severity != 0) {
		// sl_severity provided in lieu of err_type
		if (sys->sl_severity >= SL_SEV_ERROR_LOCAL)
			return SL_DISP_UNRECOVERABLE;
		else
			return SL_DISP_RECOVERABLE;
	}
	switch (sys->err_type) {
	case SYTY_PERM:
	case SYTY_CONFIG:
	case SYTY_PEND:
		return SL_DISP_UNRECOVERABLE;
	case SYTY_TEMP:
	case SYTY_INFO:
	case SYTY_BOGUS:
		return SL_DISP_RECOVERABLE;
	case SYTY_PERF:
		return SL_DISP_BYPASSED;
	case SYTY_UNKNOWN:
		/* LOG_EMERG = 0, LOG_DEBUG = 7 */
		return (sys->get_severity() <= LOG_ERR ?
			SL_DISP_UNRECOVERABLE : SL_DISP_RECOVERABLE);
	}

	/* NOTREACHED */
	return 0;
}

static uint32_t
get_svclog_callout_type(SyslogEvent *sys)
{
	/* err_type is valid only if sl_severity not specified */
	if (sys->sl_severity == 0 && sys->err_type == SYTY_CONFIG)
		return RTAS_FRUID_COMP_CONFIG_ERROR;
	if (sys->err_class == SYCL_HARDWARE)
		return RTAS_FRUID_COMP_HARDWARE;
	return RTAS_FRUID_COMP_CODE;
}

/* We couldn't find VPD for the device.  Provide null values. */
static void
zap_callout_vpd(struct sl_callout *callout)
{
	callout->location = svclog_string("");
	callout->fru = svclog_string("");
	callout->serial = svclog_string("");
	callout->ccin = svclog_string("");
}

static void
create_svclog_callout(SyslogEvent *sys, SyslogMessage *msg,
			struct sl_event *svc, struct sl_callout *callout)
{
	memset(callout, 0, sizeof(*callout));
	callout->priority = sys->priority;
	callout->type = get_svclog_callout_type(sys);
	callout->procedure = strdup("see explain_syslog");
	if (populate_callout_from_vpd(sys, msg, svc, callout) != 0)
		zap_callout_vpd(callout);
	svc->callouts = callout;
}

static void
create_addl_data(SyslogEvent *sys, SyslogMessage *msg,
			struct sl_event *svc, struct sl_data_os *os)
{
	memset(os, 0, sizeof(*os));
	/* version set by servicelog_event_log() */
	if (debug)
		os->version = fake_val("version");
	os->subsystem = svclog_string(sys->driver->subsystem, true);
	os->driver = svclog_string(sys->driver->name, true);
	os->device = svclog_string(msg->get_device_id(sys->matched_variant),
									true);
	svc->addl_data = (struct sl_data_os*) os;
}

/*
 * servicelog can't handle apostrophes in inserted text strings, so
 * replace them with back-quotes.
 */
static void
sanitize_syslog_line(string& s)
{
	size_t pos, len = s.length();
	for (pos = 0; pos < len; pos++) {
		if (s[pos] == '\'')
			s[pos] = '`';
	}
}

static int
log_event(SyslogEvent *sys, SyslogMessage *msg)
{
	struct sl_event *svc;
	struct sl_callout *callout;
	struct sl_data_os *os_data;
	string description,action;

	svc = (struct sl_event*) malloc(sizeof(*svc));
	callout = (struct sl_callout*) malloc(sizeof(*callout));
	os_data = (struct sl_data_os*) malloc(sizeof(*os_data));
	if (!svc || !callout || !os_data) {
		free(svc);
		free(callout);
		free(os_data);
		cerr << "Failed to log servicelog event: out of memory."
								<< endl;
	}
	memset(svc, 0, sizeof(*svc));
	/* next, id set by servicelog_event_log() */
	(void) time(&svc->time_event);
	/* time_last_update set by servicelog_event_log() */
	svc->type = SL_TYPE_OS;
	svc->severity = get_svclog_severity(sys);
	/*
	 * platform, machine_serial, machine_model, nodename set by
	 * servicelog_event_log()
	 */
	if (debug) {
		svc->platform = fake_val("platform");
		svc->machine_serial = fake_val("mserial");
		svc->machine_model = fake_val("model");
		svc->nodename = fake_val("nodename");
	}
	svc->refcode = svclog_string(sys->refcode);

	string msg_line = msg->line;
	sanitize_syslog_line(msg_line);
        description = sys->description;
        action = sys->action;
	svc->description = svclog_string("Message forwarded from syslog:\n"
		+ msg_line
		+ "\n Description: " + description
		+ "\n Action: " + action );

	svc->serviceable = (svc->severity >= SL_SEV_ERROR_LOCAL);
	svc->predictive = (sys->err_type == SYTY_PEND
			|| sys->err_type == SYTY_PERF
			|| sys->err_type == SYTY_UNKNOWN
			|| sys->err_type == SYTY_TEMP);
	svc->disposition = get_svclog_disposition(sys);
	svc->call_home_status = (svc->serviceable ? SL_CALLHOME_CANDIDATE
						: SL_CALLHOME_NONE);
	svc->closed = 0;
	/* repair set by servicelog_event_log() */
	create_svclog_callout(sys, msg, svc, callout);
	svc->raw_data_len = 0;
	svc->raw_data = NULL;
	create_addl_data(sys, msg, svc, os_data);

	int result = 0;
	if (debug)
		servicelog_event_print(stdout, svc, 1);
	else
		result = servicelog_event_log(slog, svc, NULL);
	if (result != 0) {
		cerr << "servicelog_event_log() failed, returning "
							<< result << endl;
		cerr << servicelog_error(slog) << endl;
	}
	servicelog_event_free(svc);
	return result;
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

/* Note: Call this only with skipping_old_messages == true. */
static bool
is_old_message(const char *line)
{
	time_t t = parse_syslog_date(line, NULL);
	if (!t || difftime(t, begin_date) < 0)
		return true;
	if (t == begin_date && !last_msg_matched.empty()) {
		if (line == last_msg_matched)
			/* This is the last one we have to skip. */
			skipping_old_messages = false;
		return true;
	}
	skipping_old_messages = false;
	return false;
}

/* Rename @path to @path_bak, and write @data to new file called @path. */
static void
safe_overwrite(const string& data, const string& path, const string& path_bak)
{
	const char *cpath = path.c_str();
	const char *cpath_bak = path_bak.c_str();

	if (rename(cpath, cpath_bak) != 0) {
		if (errno != ENOENT) {
			if (debug) {
				string msg = "Can't rename " + path + " to "
								+ path_bak;
				perror(msg.c_str());
			}
			return;
		}
	}
	FILE *f = fopen(cpath, "w");
	if (!f) {
		if (debug)
			perror(cpath);
		goto recover;
	}
	if (fputs(data.c_str(), f) == EOF) {
		if (debug)
			perror(cpath);
		goto recover;
	}
	fclose(f);
	return;

recover:
	if (rename(cpath_bak, cpath) != 0 && debug) {
		string msg = "Can't recover " + path + " from " + path_bak;
		perror(msg.c_str());
	}
}

static void
remember_matched_event(const string& msg)
{
	if (msg_path && !strcmp(msg_path, SYSLOG_PATH))
		safe_overwrite(msg, LAST_EVENT_PATH, LAST_EVENT_PATH_BAK);
}

static void
compute_begin_date(void)
{
	if (msg_path && !strcmp(msg_path, SYSLOG_PATH)) {
		/*
		 * Read the saved copy of the last syslog message we matched.
		 * Use that message's date as the begin date, and don't
		 * match any events before or at that line in the message file.
		 */
		FILE *f = fopen(LAST_EVENT_PATH, "r");
		if (!f) {
			if (errno != ENOENT && debug)
				perror(LAST_EVENT_PATH);
			return;
		}
		char line[256];
		if (fgets(line, 256, f)) {
			last_msg_matched = line;
			begin_date = parse_syslog_date(line, NULL);
		}
		if (!begin_date) {
			fprintf(stderr, "Cannot read date from %s\n",
							LAST_EVENT_PATH);
			exit(3);
		}
		fclose(f);
	}
}

/*
 * Read msg_path by piping it through tail -F.  This returns the result of
 * the popen() call.
 */
static FILE *
tail_message_file(void)
{
	/*
	 * Avoid stuff like popen("tail -F ... file; rm -rf /", "r")
	 * when nasty msg_path = 'file; rm -rf /'.  To be extra safe,
	 * we enclose the pathname in quotes.
	 */
	const char *bad_chars = ";|'";
	if (strpbrk(msg_path, bad_chars)) {
		fprintf(stderr, "message pathname must not contain any of"
			" these characters: %s\n", bad_chars);
		return NULL;
	}

	/*
	 * tail -F will get us past interruptions injected by logrotate
	 * and such, but we require that the message file exist when we
	 * start up.
	 */
	int fd = open(msg_path, O_RDONLY);
	if (fd < 0) {
		perror(msg_path);
		return NULL;
	}
	close(fd);

	string tail_command = string("/usr/bin/tail -F -n +0 -s 2 '") +
							msg_path + "'";
	FILE *p = popen(tail_command.c_str(), "r");
	if (!p) {
		perror(tail_command.c_str());
		return NULL;
	}
	return p;
}

static FILE *
open_message_file(void)
{
	if (follow)
		return tail_message_file();
	FILE *f = fopen(msg_path, "r");
	if (!f)
		perror(msg_path);
	return f;
}

static void
close_message_file(void)
{
	if (msg_file) {
		if (follow) {
			if (pclose(msg_file) != 0)
				perror("tail -F of message file");
		} else
			fclose(msg_file);
	}
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
"-F\t\tDon't stop at EOF; process newly logged messages as they occur.\n"
"-h\t\tPrint this help text and exit.\n"
"-m message_file\tRead syslog messages from message_file, not stdin.\n"
"-M\t\tRead syslog messages from /var/log/messages.\n"
	);
}

int main(int argc, char **argv)
{
	int c, result;
	int args_seen[0x100] = { 0 };

	progname = argv[0];
	opterr = 0;
	while ((c = getopt(argc, argv, "b:C:de:Fhm:M")) != -1) {
		if (isalpha(c))
			args_seen[c]++;
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
		case 'F':
			follow = true;
			break;
		case 'h':
			print_help();
			exit(0);
		case 'm':
			msg_path = optarg;
			break;
		case 'M':
			msg_path = SYSLOG_PATH;
			follow_default = true;
			break;
		case '?':
			usage();
		}
	}
	if (optind != argc)
		usage();

	for (c = 0; c < 0x100; c++) {
		if (args_seen[c] > 1) {
			cerr << progname << ": duplicate " << (char) c
				<< " args" << endl;
			usage();
		}
	}
	if (args_seen['m'] && args_seen['M'])
		usage();
	if (follow && !msg_path) {
		cerr << progname << ": cannot specify -F when messages come"
						" from stdin" << endl;
		exit(1);
	}
	if (end_date && follow) {
		cerr << progname << ": cannot specify both -e and -F" << endl;
		exit(1);
	}
	if (begin_date && end_date && difftime(begin_date, end_date) > 0) {
		// Note: ctime stupidly appends a newline.
		cerr << progname << ": end date = " << ctime(&end_date)
			<< "precedes begin date = " << ctime(&begin_date)
			<< endl;
		exit(1);
	}
	/* follow defaults to true for /var/log/messages, false for others. */
	if (!end_date && !follow)
		follow = follow_default;

	if (!begin_date)
		compute_begin_date();

	if (msg_path) {
		msg_file = open_message_file();
		if (!msg_file) {
			perror(msg_path);
			exit(1);
		}
	}

	if (EventCatalog::parse(catalog_dir) != 0) {
		close_message_file();
		exit(2);
	}

	result = servicelog_open(&slog, 0);
	if (result != 0) {
		cerr << "servicelog_open() failed, returning "
							<< result << endl;
		close_message_file();
		exit(3);
	}

#define LINESZ 512
	char line[LINESZ];
	skipping_old_messages = (begin_date != 0);
	vector<SyslogEvent*>::iterator ie;

	while (fgets(line, LINESZ, msg_file)) {
		if (!strchr(line, '\n')) {
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
		}
		if (skipping_old_messages && is_old_message(line))
			continue;
		SyslogMessage msg(line);
		if (!msg.parsed) {
			if (debug)
				cerr << "unparsed message: " << line;
			continue;
		}
		if (end_date && difftime(msg.date, end_date) > 0)
			break;
		for (ie = event_catalog.events.begin();
					ie < event_catalog.events.end(); ie++) {
			SyslogEvent *event = *ie;
			if (event->match(&msg, true)) {
				remember_matched_event(line);
				if (!event->exception_msg
					&& !is_informational_event(event))
					log_event(event, &msg);
				break;
			}
		}
	}

	servicelog_close(slog);
	close_message_file();
	exit(0);
}
