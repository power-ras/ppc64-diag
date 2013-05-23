#ifndef _CATALOGS_H
#define _CATALOGS_H

/*
 * Catalogs: reporters and events/messages, and parsers therefor
 *
 * Copyright (C) International Business Machines Corp., 2009, 2010
 *
 */

using namespace std;

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>

#include <sys/types.h>
#include <syslog.h>
#include <stdio.h>
#include <regex.h>

#define ELA_CATALOG_DIR "/etc/ppc64-diag/message_catalog"

class Parser {
protected:
	virtual void init_lex() = 0;	// defined in .l file

	// yacc-generated parser -- AKA yyparse()
	int (*parse)();

	// flex-generated [yy]input() and unput().
	// input and unput can be magic names in lex.*.c files, so avoid
	// them here.
	int (*p_input)();
	void (*p_unput)(int);

	void (*error)(const char *s);
	int semantic_errors;

	int get_octal_escape(const char *digits, int isCharConst);
	int get_char_escape(char c);
	void collect_octal_digits(char *digits, char firstDigit);
public:
	Parser();
	const char *pathname;
	FILE *file;
	int lineno;
	int parse_file(const string& path);
	void semantic_error(const string& msg);

	/* public because they're called by the lexical analyzers. */
	char *get_string(int quoted);
	int skip_comment();
	char *get_text_block();
};

extern Parser *cur_parser;

/* Parses the reporters catalog */
class ReporterCtlgParser : public Parser {
protected:
	virtual void init_lex();	// defined in .l file
public:
	ReporterCtlgParser();
};

/* Parses a file in the message catalog, yielding an EventCtlgFile */
class EventCtlgParser : public Parser {
protected:
	virtual void init_lex();	// defined in .l file
public:
	EventCtlgParser();
};

struct NameValuePair {
	const char *name;
	int value;
};

class MemberSet {
protected:
	set<void*> seen;
	Parser *parser;
public:
	void tally(void *addr, const string& name);
	void require(void *addr, const string& name);
	MemberSet(Parser *p) { parser = p; }
};

extern int nvp_lookup(string name, NameValuePair *nvp, string member);
extern string nvp_lookup_value(int value, NameValuePair *nvp);

extern string severity_name(int sev);

class Reporter;
class ReporterCatalog;

/* e.g., dev_err is an alias for dev_printk. */
class ReporterAlias {
	friend ostream& operator<<(ostream& os, const ReporterAlias& ra);
public:
	Reporter *reporter;
	string name;
#define LOG_SEV_UNKNOWN 8
#define LOG_SEV_ANY 9	/* used only for catchall-ish messages */
	int severity;	/* LOG_EMERG ... LOG_DEBUG or unknown or any */
	ReporterAlias(const string& nm, const string& sev = "unknown");
};

/*
 * A Reporter is a function or macro like dev_printk or pr_err that
 * is used to report an event.  Different Reporters supply different
 * prefixes.
 */
class Reporter {
	friend class ReporterCatalog;
	friend ostream& operator<<(ostream& os, const Reporter& r);
protected:
	MemberSet members;
	Parser *parser;
	bool prefix_arg_exists(const string& arg);
public:
	string name;
	ReporterAlias *base_alias;
	vector<ReporterAlias*> *aliases;
	bool from_kernel;
	string prefix_format;
	vector<string> *prefix_args;
	string device_arg;

	Reporter(ReporterAlias *ra);
	void set_source(const string& source);
	void set_aliases(vector<ReporterAlias*> *alist);
	void set_prefix_format(const string& format);
	void set_prefix_args(vector<string> *args);
	void set_device_arg(const string& arg);
	void validate(void);
};

/*
 * In some cases, the macro used to log a message may produce either
 * of two (or more) prefix formats.  Such a macro is represented by
 * a MetaReporter, which maps to two or more ReporterAliases, each
 * representing a prefix format.
 */
class MetaReporter {
	friend class ReporterCatalog;
	friend ostream& operator<<(ostream& os, const MetaReporter& mr);
protected:
	MemberSet members;
	Parser *parser;
	vector<string> *variant_names;
	void handle_nested_meta_reporter(MetaReporter *mr);

public:
	string name;
	vector<ReporterAlias*> variants;

	MetaReporter(const string& nm);
	void set_variant_names(vector<string> *vnames);
	void validate(ReporterCatalog *catalog);
};

class ReporterCatalog {
public:
	vector<Reporter*> rlist;
	vector<MetaReporter*> mrlist;
	map<string, ReporterAlias*> rmap;
	map<string, MetaReporter*> mrmap;
	void register_reporter(Reporter*);
	void register_meta_reporter(MetaReporter*);
	void register_alias(ReporterAlias *ra, Reporter*);
	ReporterAlias *find(const string& name);
	MetaReporter *find_meta_reporter(const string& name);
};

class EventCtlgFile;
class SyslogEvent;
class SyslogMessage;
class CatalogCopy;

class ExceptionMsg {
public:
	string type;
	string description;
	string action;
	ExceptionMsg(const string& ty, const string& desc, const string &act) {
		type = ty;
		description = desc;
		action = act;
	}
};

class ExceptionCatalog {
protected:
	map<string, ExceptionMsg*> exceptions;
public:
	void add(Parser *parser, const string& type,
			const string& description, const string &action);
	ExceptionMsg *find(const string& type);
};

enum ErrorClass { SYCL_HARDWARE, SYCL_SOFTWARE, SYCL_FIRMWARE, SYCL_UNKNOWN };
enum ErrorType { SYTY_PERM=1, SYTY_TEMP, SYTY_CONFIG, SYTY_PEND, SYTY_PERF,
					SYTY_INFO, SYTY_UNKNOWN, SYTY_BOGUS=0 };
#ifndef SL_SEV_FATAL
/* defines for sl_event.severity */
#define SL_SEV_FATAL		7
#define SL_SEV_ERROR		6
#define SL_SEV_ERROR_LOCAL	5
#define SL_SEV_WARNING		4
#define SL_SEV_EVENT		3
#define SL_SEV_INFO		2
#define SL_SEV_DEBUG		1
#endif

/*
 * This contains the information necessary to match a SyslogMessage to a
 * SyslogEvent.  Typically, there is just one per SyslogEvent.  But if the
 * ReporterAlias named in the message statement is actually a MetaReporter,
 * then a MatchVariant is created for each of the MetaReporter's
 * ReporterAliases.
 */
class MatchVariant {
	friend class SyslogEvent;
protected:
//	string regex_text;

	int resolve_severity(int msg_severity);
	void compute_regex_text(void);
	void compile_regex(void);
public:
        string regex_text; 
	ReporterAlias *reporter_alias;
	int severity;		// from ReporterAlias
	regex_t regex;
	SyslogEvent *parent;

	MatchVariant(ReporterAlias *ra, int msg_severity, SyslogEvent *pa);
	bool match(SyslogMessage*, bool get_prefix_args);
	void report(ostream& os, bool sole_variant);
	void set_regex(const string& rgxtxt);
};

/* A message/event from the message catalog */
class SyslogEvent {
	friend class MatchVariant;
	friend ostream& operator<<(ostream& os, const SyslogEvent& e);
protected:
	Parser *parser;
	MemberSet members;
	vector<MatchVariant*> match_variants;

	string paste_copies(const string &text);
	void mk_match_variants(const string& rp, const string& sev);
public:
	string reporter_name;	// Could be a reporter, alias, or meta-reporter
	MatchVariant *matched_variant;	// set by match()
	bool from_kernel;

	EventCtlgFile *driver;
	string format;
	string escaped_format;	/* e.g., newline replaced with slash, n */
	string *source_file;
	string description;
	string action;
	ErrorClass err_class;
	// err_type or sl_severity, not both
	ErrorType err_type;
	int sl_severity;	// one of SL_SEV_*
	string refcode;
	char priority;
	ExceptionMsg *exception_msg;

	SyslogEvent(const string& rpt, const string& sev, const string& fmt,
							EventCtlgFile *drv);
	void set_description(const string& s);
	void set_action(const string& s);
	void set_class(const string& s);
	void set_type(const string& s);
	void set_sl_severity(const string& s);
	void set_refcode(const string& s);
	void set_priority(const string& s);
	void set_regex(const string& rpt, const string& rgxtxt);
	void except(const string& reason);
	void verify_complete(void);
	MatchVariant *match(SyslogMessage*, bool get_prefix_args);
	int get_severity(void);
};

/* Maps a string such as device ID to the corresponding /sys/.../devspec file */
class DevspecMacro {
protected:
	string name;	// e.g., device, netdev, adapter
	string prefix;	// /sys/...
	string suffix;	// .../devspec
public:
	DevspecMacro(const string& nm, const string& path);
	string get_devspec_path(const string& device_id);
	bool valid;
};

/*
 * Typically used to filter out messages from other drivers, by looking
 * at the "driver" arg of the message prefix.
 */
class MessageFilter {
protected:
	string arg_name;
	string arg_value;
public:
	MessageFilter(const string& name, int op, const string& value);
	bool message_passes_filter(SyslogMessage *msg);
};

/*
 * Contains the header information for a particular catalog file (which
 * typically represents a particular driver)
 */
class EventCtlgFile {
public:
	string pathname;
	string name;	// driver name, from pathname
	string subsystem;
	map<string, string> text_copies;
	map<string, DevspecMacro*> devspec_macros;
	vector<MessageFilter*> filters;
	/* not strictly needed, but handy if we ever want to do destructors */
	vector<string*> source_files;
	string *cur_source_file;

	EventCtlgFile(const string& path, const string& subsys);
	void add_text_copy(const string& name, const string& text);
	string find_text_copy(const string& name);
	void set_source_file(const string& path);
	void add_devspec(const string& nm, const string& path);
	DevspecMacro *find_devspec(const string& name);
	void add_filter(MessageFilter *filter);
	bool message_passes_filters(SyslogMessage *msg);
};

/*
 * The overall event/message catalog, comprising all the EventCtlgFiles
 * in the directory
 */
class EventCatalog {
protected:
	vector<EventCtlgFile*> drivers;
public:
	vector<SyslogEvent*> events;
	EventCatalog() {}
	static int parse(const string& directory);
	void register_driver(EventCtlgFile *driver);
	void register_event(SyslogEvent *event);
};

/* A line of text logged by syslog */
class SyslogMessage {
public:
	string line;
	bool parsed;
	time_t date;
	string hostname;
	bool from_kernel;
	string message;
	map<string, string> prefix_args;
	string devspec_path;	// path to devspec node in /sys

	SyslogMessage(const string& s);
	string echo(void);
	int set_devspec_path(SyslogEvent *event);
	string get_device_id(MatchVariant *mv);
};

/*
 * regex_text_policy = RGXTXT_WRITE adds regex_text statements to
 * the message catalogs.
 */
enum regex_text_policy {
	RGXTXT_COMPUTE,	/* Compute regex_text from format string */
	RGXTXT_WRITE,	/* ... and also write it out to catalog file */
	RGXTXT_READ	/* Read regex_text from catalog file */
};
extern regex_text_policy regex_text_policy;

class CatalogCopy {
protected:
	FILE *orig_file;
	FILE *copy_file;
	string orig_path;
	string copy_path;
	int last_line_copied;	// in orig_file

	int copy_through(int line_nr);
public:
	bool valid;

	CatalogCopy(const string& rd_path, const string& wr_path);
	~CatalogCopy();
	void inject_text(const string& text, int line_nr);
	void finish_copy(void);
};

extern string indent_text_block(const string& s1, size_t nspaces);

extern "C" {
extern time_t parse_date(const char *start, char **end, const char *fmt,
							bool yr_in_fmt);
extern time_t parse_syslog_date(const char *start, char **end);
extern time_t parse_syslogish_date(const char *start, char **end);
}

#endif /* _CATALOGS_H */
