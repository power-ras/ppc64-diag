/*
 * event/message and reporter catalogs for syslog analysis
 *
 * Copyright (C) International Business Machines Corp., 2009
 *
 */

using namespace std;

#include <map>

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include "catalogs.h"
#include <sstream>

enum regex_text_policy regex_text_policy = RGXTXT_READ;

static CatalogCopy *catalog_copy = NULL;

ReporterCtlgParser reporter_ctlg_parser;
extern int rrparse();
extern void rrerror(const char *s);

ReporterCatalog reporter_catalog;

EventCtlgParser event_ctlg_parser;
extern int evparse(void);
extern void everror(const char *s);

EventCatalog event_catalog;
ExceptionCatalog exception_catalog;

Parser *cur_parser = NULL;

Parser::Parser()
{
	pathname = NULL;
	file = NULL;
}

int
Parser::parse_file(const string& path)
{
	int result;

	cur_parser = this;
	pathname = strdup(path.c_str());
	file = fopen(pathname, "r");
	if (!file) {
		fprintf(stderr,
			"can't open catalog file\n");
		perror(pathname);
		return -1;
	}
	init_lex();
	result = parse();
	if (semantic_errors > 0)
		result = semantic_errors;
		fclose(file);
	return result;
}

void
Parser::semantic_error(const string& msg)
{
	error(msg.c_str());
	semantic_errors++;
}

#define MYEOF '\0'

/*
 * Collect the 1-to-3-digit octal number following a \.
 * If is_char_const is true, we're accumulating a char constant, so we flag
 * an error if there are excess digits.
 * Returns value > UCHAR_MAX if there's an error.
 */
int
Parser::get_octal_escape(const char *digits, int is_char_const)
{
	int n = 0;
	int nDigits = 0;
	const char *c;
	for (c = digits; '0' <= *c && *c <= '7'; c++) {
		n *= 8;
		n += *c - '0';
		if (++nDigits == 3 && !is_char_const) {
			return n;
		} else if (nDigits > 3 && is_char_const) {
			return UCHAR_MAX+1;
		}
	}
	return n;
}

int
Parser::get_char_escape(char c)
{
	switch (c) {
	case '\'':	return '\'';
	case '\"':	return '\"';
	case '\?':	return '\?';
	case '\\':	return '\\';
	case 'a':	return '\a';
	case 'b':	return '\b';
	case 'f':	return '\f';
	case 'n':	return '\n';
	case 'r':	return '\r';
	case 't':	return '\t';
	case 'v':	return '\v';
	case '\n':	return -1;
	default:	return c;
	}
}

/*
 * Return a version of s that contains only printable characters, by
 * converting non-printing characters to \ escapes.
 */
string
add_escapes(const string& s)
{
	int i;
	string es;
	int slen = s.length();

	for (i = 0; i < slen; i++) {
		char c = s[i];
		switch (c) {
		case '\\': es += "\\\\"; break;
		case '\"': es += "\\\""; break;
		case '\?': es += "\\?"; break;
		case '\a': es += "\\a"; break;
		case '\b': es += "\\b"; break;
		case '\f': es += "\\f"; break;
		case '\n': es += "\\n"; break;
		case '\r': es += "\\r"; break;
		case '\t': es += "\\t"; break;
		case '\v': es += "\\v"; break;
		default:
			if (isprint(c)) {
				es += c;
			} else {
				char octal_esc[4];
				sprintf(octal_esc, "\\%o", (unsigned char) c);
				es += octal_esc;
			}
			break;
		}
	}
	return es;
}

void
Parser::collect_octal_digits(char *digits, char first_digit)
{
	int nDigits = 1;
	int c;
	digits[0] = first_digit;

	while (nDigits <= 3) {
		c = p_input();
		if ('0' <= c && c <= '7') {
			digits[nDigits++] = c;
		} else {
			p_unput(c);
			break;
		}
	}
	digits[nDigits] = '\0';
}

#define MAXSTRLEN (10*1024)
#define STRSLOP 10	/* e.g., to handle a multibyte character at the end */
static char strbuf[MAXSTRLEN+STRSLOP];

/*
 * We have eaten the leading " in a quoted string.  Collect the characters
 * of the string into strbuf, and then return a strdup-ed copy of the string.
 * We end after eating the terminating ".
 */
char *
Parser::get_string(int quoted) {
	int nc = 0;
	int c;
	int end_of_string = (quoted ? '\"' : MYEOF);

	for (;;) {
		c = p_input();
		if (c == end_of_string) {
			break;
		} else if (c == MYEOF) {
			/* EOF in middle of quoted string. */
			return 0;
		} else if (c == '\\') {
			/* Collect and decode the escape sequence. */
			c = p_input();
			if (c == MYEOF) {
				/* End of input */
				if (quoted) {
					return 0;
				} else {
					/* Allow \ as the last character. */
					strbuf[nc++] = '\\';
					break;
				}
			} else if ('0' <= c && c <= '7') {
				char digits[3+1];
				collect_octal_digits(digits, c);
				strbuf[nc++] = get_octal_escape(digits, 0);
			} else {
				int ce = get_char_escape(c);
				/* Elide escaped newlines (ce == -1). */
				if (ce == -1) {
					lineno++;
				} else {
					strbuf[nc++] = ce;
				}
			}
		} else {
			if (c == '\n') {
				lineno++;
			}
			strbuf[nc++] = c;
		}

		if (nc > MAXSTRLEN) {
			return 0;
		}
	}

	strbuf[nc] = '\0';
	return strdup(strbuf);
}

/*
 * We have already eaten the leading / and *.  Skip past the trailing * and /.
 */
int
Parser::skip_comment()
{
	int c;
	int orig_lineno = lineno;

	while ((c = p_input()) != MYEOF) {
		if (c == '\n') {
			lineno++;
		}

		/* Correctly handle multiple *s followed by /. */
	check_star:
		while (c == '*') {
			c = p_input();
			if (c == '/') {
				/* End of comment */
				return 0;
			} else if (c == MYEOF) {
				/* EOF after a '*'. */
				return -1;
			} else if (c == '\n') {
				lineno++;
			}
		}
		/* This is just to warn about nested comments. */
		while (c == '/') {
			c = p_input();
			if (c == '*') {
				fprintf(stderr, "%s:%d: warning: comment here"
					" nested inside comment starting at"
					" line %d\n", pathname, lineno,
					orig_lineno);
				goto check_star;
			} else if (c == MYEOF) {
				return -1;
			} else if (c == '\n') {
				lineno++;
			}
		}
	}

	/* End of file */
		return -1;
}

/*
 * We've eaten the leading {{ in a text block.  Collect the characters
 * of the block into strbuf, and then return a strdup-ed copy of the string.
 * We end after eating the terminating }}.  Leading and trailing space
 * characters, including newlines, are stripped off.
 */
char *
Parser::get_text_block(void)
{
	int nc = 0;
	int c;
	int last_non_space = -1;

	for (;;) {
		c = p_input();
		if (c == 0) {
			/* EOF in middle of text block */
			return NULL;
		} else if (isspace(c)) {
			if (c == '\n')
				lineno++;
			if (last_non_space >= 0)
				strbuf[nc++] = c;
		} else {
			if (c == '}') {
				c = p_input();
				if (c == '}') {
					/* End of block */
					break;
				} else {
					/* Lone } */
					p_unput(c);
					c = '}';
				}
			}
			last_non_space = nc;
			strbuf[nc++] = c;
		}
		if (nc > MAXSTRLEN)
			return NULL;
	}
	if (last_non_space < 0)
	return strdup("");
	strbuf[last_non_space+1] = '\0';
	return strdup(strbuf);
	}

	ReporterCtlgParser::ReporterCtlgParser() : Parser() {
	parse = rrparse;
	error = rrerror;
}

EventCtlgParser::EventCtlgParser() : Parser() {
	parse = evparse;
	error = everror;
}

void
MemberSet::tally(void *addr, const string& name)
{
	if (seen.find(addr) == seen.end())
			seen.insert(addr);
	else
	parser->semantic_error(name + " statement seen multiple times"
						" in same catalog entry.");
}

void
MemberSet::require(void *addr, const string& name)
{
	if (seen.find(addr) == seen.end())
		parser->semantic_error(name
			+ " statement required but missing.");
}

int
nvp_lookup(string name, NameValuePair *nvp, string member)
{
	const char *nm = name.c_str();
	int i;
	for (i = 0; nvp[i].name != NULL; i++) {
		if (!strcmp(nm, nvp[i].name))
			return nvp[i].value;
	}
	cur_parser->semantic_error("unrecognized value for " + member + ": "
							+ name);
	return nvp[i].value;
}

string
nvp_lookup_value(int value, NameValuePair *nvp)
{
	int i;
	for (i = 0; nvp[i].name != NULL; i++) {
		if (nvp[i].value == value)
			return string(nvp[i].name);
	}
	return "badval";
}

static struct NameValuePair severity_nvp[] = {
	{ "emerg", LOG_EMERG },
	{ "alert", LOG_ALERT },
	{ "crit", LOG_CRIT },
	{ "err", LOG_ERR },
	{ "warning", LOG_WARNING },
	{ "notice", LOG_NOTICE },
	{ "info", LOG_INFO },
	{ "debug", LOG_DEBUG },
	{ "unknown", LOG_SEV_UNKNOWN },
	{ "any", LOG_SEV_ANY },
	{ "KERN_EMERG", LOG_EMERG },
	{ "KERN_ALERT", LOG_ALERT },
	{ "KERN_CRIT", LOG_CRIT },
	{ "KERN_ERR", LOG_ERR },
	{ "KERN_WARNING", LOG_WARNING },
	{ "KERN_NOTICE", LOG_NOTICE },
	{ "KERN_INFO", LOG_INFO },
	{ "KERN_DEBUG", LOG_DEBUG },
	{ "LOG_EMERG", LOG_EMERG },
	{ "LOG_ALERT", LOG_ALERT },
	{ "LOG_CRIT", LOG_CRIT },
	{ "LOG_ERR", LOG_ERR },
	{ "LOG_WARNING", LOG_WARNING },
	{ "LOG_NOTICE", LOG_NOTICE },
	{ "LOG_INFO", LOG_INFO },
	{ "LOG_DEBUG", LOG_DEBUG },
	{ NULL, LOG_SEV_UNKNOWN }
};

string
severity_name(int sev)
{
	return nvp_lookup_value(sev, severity_nvp);
}

ReporterAlias::ReporterAlias(const string& nm, const string& sev)
{
	reporter = NULL;
	name = nm;
	severity = nvp_lookup(sev, severity_nvp, "severity level");
}

ostream& operator<<(ostream& os, const ReporterAlias& ra)
{
	os << ra.name << '(' << severity_name(ra.severity) << ')';
	return os;
}

static struct NameValuePair source_nvp[] = {
	{ "kernel", 1 },
	{ "user", 0 },
	{ NULL, 1 }
};

Reporter::Reporter(ReporterAlias *ra) : members(&reporter_ctlg_parser)
{
	name = ra->name;
	base_alias = ra;
	aliases = NULL;
	from_kernel = false;
	prefix_format = "";
	prefix_args = NULL;
	device_arg = "";
	parser = &reporter_ctlg_parser;
}

void
Reporter::set_source(const string& source)
{
	members.tally(&from_kernel, "source");
	from_kernel = nvp_lookup(source, source_nvp, "source");
}

void
Reporter::set_aliases(vector<ReporterAlias*> *alist)
{
	members.tally(&aliases, "aliases");
	if (!aliases)
		aliases = alist;
}

void
Reporter::set_prefix_format(const string& format)
{
	members.tally(&prefix_format, "prefix_format");
	prefix_format = format;
}

void
Reporter::set_prefix_args(vector<string> *args)
{
	members.tally(&prefix_args, "prefix_args");
	if (!prefix_args)
	prefix_args = args;
}

bool
Reporter::prefix_arg_exists(const string& arg)
{
	if (!prefix_args)
		return false;

	vector<string>::iterator it;
	for (it = prefix_args->begin(); it < prefix_args->end(); it++) {
		if (*it == arg)
			return true;
	}
	return false;
}

void
Reporter::set_device_arg(const string& arg)
{
	members.tally(&device_arg, "device_arg");
	device_arg = arg;
	if (arg != "none") {
		if (!prefix_arg_exists(arg))
			parser->semantic_error("device_arg " + arg +
							" not in prefix_args");
	}
}

void
Reporter::validate(void)
{
	if (device_arg == "") {
		if (prefix_args) {
			if (prefix_arg_exists("device"))
				device_arg = "device";
			else
				parser->semantic_error(
					"No \"device\" arg in prefix_args, "
					"so device_arg statement must specify "
					"a prefix arg or none.");
		} else
			device_arg = "none";
	}
}

static void
cout_name_list(ostream& os, const vector<string> *list)
{
	if (list) {
		vector<string>::const_iterator it;
		for (it = list->begin(); it < list->end(); it++)
			os << " " << *it;
	} else
		os << " [NONE]";
}

static void
cout_alias_list(ostream& os, const vector<ReporterAlias*> *list)
{
	if (list) {
		vector<ReporterAlias*>::const_iterator it;
		for (it = list->begin(); it < list->end(); it++)
			os << " " << **it;
	} else
		os << " [NONE]";
}

ostream& operator<<(ostream& os, const Reporter& r)
{
	os << "reporter: " << *(r.base_alias) << endl;
	os << "source: " << (r.from_kernel ? "kernel" : "user") << endl;
	os << "aliases:";
	cout_alias_list(os, r.aliases);
	os << endl;
	os << "prefix_format: \"" << r.prefix_format << "\"" << endl;
	os << "prefix_args:";
	cout_name_list(os, r.prefix_args);
	os << endl;
	os << "device_arg: " << r.device_arg << endl;
	return os;
}

MetaReporter::MetaReporter(const string& nm) : members(&reporter_ctlg_parser)
{
	name = nm;
	parser = &reporter_ctlg_parser;
	variant_names = NULL;
}


ostream& operator<<(ostream& os, const MetaReporter& mr)
{
	os << "meta_reporter: " << mr.name << endl;
	os << "variants:";
	cout_alias_list(os, &mr.variants);
	os << endl;
	return os;
}

void
MetaReporter::set_variant_names(vector<string> *vnames)
{
	members.tally(&variant_names, "variants");
	if (!variant_names)
		variant_names = vnames;
}

/*
 * This MetaReporter's variant_names list contains the name of a previously
 * defined MetaReporter, mr.  Copy mr's variants list into this's.  (A
 * MetaReporter's variants list is always ReporterAliases, never MetaReporters.)
 *
 * Note that it's possible to get the same ReporterAlias multiple times in
 * the same MetaReporter's variants list (e.g., if multiple nested
 * MetaReporters refer to it), but that won't break anything.
 */
void
MetaReporter::handle_nested_meta_reporter(MetaReporter *mr)
{
	vector<ReporterAlias*>::iterator it;
	for (it = mr->variants.begin(); it < mr->variants.end(); it++)
		variants.push_back(*it);
}

void
MetaReporter::validate(ReporterCatalog *catalog)
{
	members.require(&variant_names, "variants");
	if (!variant_names)
		return;

	vector<string>::iterator it;
	set<string> names_seen;

	for (it = variant_names->begin(); it < variant_names->end(); it++) {
		string vname = *it;

	if (names_seen.find(vname) != names_seen.end()) {
		parser->semantic_error(
			"duplicate name in variants list:" + vname);
		continue;
	}
		names_seen.insert(vname);

		ReporterAlias *ra = catalog->find(vname);
		if (ra) {
			variants.push_back(ra);
		} else {
			MetaReporter *mr = catalog->find_meta_reporter(vname);
			if (mr)
				handle_nested_meta_reporter(mr);
			else
				parser->semantic_error("unknown reporter: "
								+ vname);
		}
	}

	/* Make sure all variants are consistent about necessary stuff. */
	bool first_variant = true;
	bool from_kernel = true;
	vector<ReporterAlias*>::iterator rit;
	for (rit = variants.begin(); rit < variants.end(); rit++) {
		ReporterAlias *ra = *rit;
		if (first_variant) {
			from_kernel = ra->reporter->from_kernel;
			first_variant = false;
		} else if (ra->reporter->from_kernel != from_kernel)
			parser->semantic_error("meta_reporter variants "
				"can't be from both kernel and user space.");
	}
}

void
ReporterCatalog::register_reporter(Reporter* r)
{
	if (!r)
	/* called as a result of a syntax error */
	return;
	rlist.push_back(r);
	register_alias(r->base_alias, r);

	if (r->aliases) {
		vector<ReporterAlias*>::iterator it;
		for (it = r->aliases->begin(); it < r->aliases->end(); it++)
			register_alias(*it, r);
	}

	r->validate();
}

void
ReporterCatalog::register_meta_reporter(MetaReporter *mr)
{
	if (!mr)
		return;
	mr->validate(this);
	if (find(mr->name) || find_meta_reporter(mr->name)) {
		cur_parser->semantic_error(
			"meta_reporter name already in use: " + mr->name);
	} else {
		mrmap[mr->name] = mr;
		mrlist.push_back(mr);
	}
}

void
ReporterCatalog::register_alias(ReporterAlias *ra, Reporter *reporter)
{
	/*
	 * To avoid the possibility of a mapping to a Reporter that
	 * has been deleted due to a syntax error, we don't register
	 * aliases until the Reporter entry has been successfully
	 * parsed.  This may mean that on a duplicate, the error
	 * message's line number is slightly off.  NBD.
	 */
	ra->reporter = reporter;
	ReporterAlias *dup = find(ra->name);
	if (dup)
		cur_parser->semantic_error("duplicate reporter name: "
								+ ra->name);
	else
		rmap[ra->name] = ra;
}

ReporterAlias *
ReporterCatalog::find(const string& name)
{
	map<string, ReporterAlias*>::iterator it = rmap.find(name);
	if (it == rmap.end())
		return NULL;
	return it->second;
}

MetaReporter *
ReporterCatalog::find_meta_reporter(const string& name)
{
	map<string, MetaReporter*>::iterator it = mrmap.find(name);
	if (it == mrmap.end())
		return NULL;
	return it->second;
}

void
ExceptionCatalog::add(Parser *pc, const string& type,
			const string& description, const string &action)
{
	if (find(type) == NULL)
		exceptions[type] = new ExceptionMsg(type, description, action);
	else
		pc->semantic_error("multiple entries for exception " + type);
}

ExceptionMsg *
ExceptionCatalog::find(const string& type)
{
	map<string, ExceptionMsg*>::iterator it = exceptions.find(type);
	if (it == exceptions.end())
		return NULL;
	return it->second;
}

/*
 * The severity should be implied by the reporter (alias) or specified
 * explicitly in the message statement, but not both.
 *
 * Report bogus severity name in message stmt in any case.
 */
int
MatchVariant::resolve_severity(int msg_severity)
{
	int reporter_sev, sev;

	sev = reporter_sev = reporter_alias->severity;
	if (reporter_sev == LOG_SEV_UNKNOWN && msg_severity == LOG_SEV_UNKNOWN) 
		parent->parser->semantic_error("message statement must specify"
			" severity because reporter " +
			reporter_alias->name + " does not.");
	else if (reporter_sev != LOG_SEV_UNKNOWN
					&& msg_severity != LOG_SEV_UNKNOWN) {
		parent->parser->semantic_error("reporter "
			+ reporter_alias->name
			+ " specifies severity, so message statement"
			+ " should not.");
		if (reporter_sev != msg_severity)
			parent->parser->semantic_error("severity specified by"
				" message statement conflicts with"
				" severity specified by reporter "
				+ reporter_alias->name);
	} else if (reporter_sev == LOG_SEV_UNKNOWN)
		sev = msg_severity;
	return sev;
}

void
SyslogEvent::mk_match_variants(const string& rp, const string& sev_name)
{
int msg_severity;

	if (!sev_name.compare(""))
		msg_severity = LOG_SEV_UNKNOWN;
	else if (!sev_name.compare("default"))
		/*
		 * Some printks don't include a KERN_* prefix.  printk()
		 * uses default_message_loglevel in those cases.  For all
		 * practical purposes, that's KERN_WARNING.
		 */
		msg_severity = LOG_WARNING;
	else
		msg_severity = nvp_lookup(sev_name, severity_nvp,
							"severity level");

	ReporterAlias *ra = reporter_catalog.find(rp);
	if (ra) {
		match_variants.push_back(new MatchVariant(ra, msg_severity,
									this));
	} else {
		MetaReporter *mr = reporter_catalog.find_meta_reporter(rp);
		if (mr) {
			vector<ReporterAlias*>::iterator it;
			for (it = mr->variants.begin();
						it != mr->variants.end(); it++)
				match_variants.push_back(new MatchVariant(*it,
							msg_severity, this));
		} else {
			parser->semantic_error("logging function not found in"
					" reporter catalog: " + rp);
		}
	}
}

SyslogEvent::SyslogEvent(const string& rp, const string& sev, const string& fmt,
				EventCtlgFile *drv) : members(&event_ctlg_parser)
{
	parser = &event_ctlg_parser;
	driver = drv;
	source_file = driver->cur_source_file;
	format = fmt;
	escaped_format = add_escapes(format);
	reporter_name = rp;
	mk_match_variants(rp, sev);
	err_type = SYTY_BOGUS;	// zero
	sl_severity = 0;
	priority = 'L';
	exception_msg = NULL;

	from_kernel = false;
	if (match_variants.size() > 0) {
		MatchVariant *first = match_variants.front();
		from_kernel = first->reporter_alias->reporter->from_kernel;
	}
}

/*
 * POSIX recommends that portable programs use regex patterns less than 256
 * characters.
 */
#define REGEX_MAXLEN 256

/*
 * Form the full format string by prepending the reporter's prefix;
 * generate the corresponding regular-expression text, and compile
 * the regex.
 *
 * If the associated Reporter supplies prefix args (e.g., dev_err
 * provides driver and device), create and compile the regular expression
 * such that the values of the prefix args can be extracted from a
 * matching message using regexec's pmatch feature.
 *
 * Failures are logged as parser semantic errors.
 *
 * NOTE: compute_regex_text() has been split off from compile_regex()
 * now that regex_text can be read in from the catalog.
 */

void
MatchVariant::compute_regex_text(void)
{
	int get_prefix_args = 0;
	FILE *in;
	char regex_cstr[REGEX_MAXLEN];
	Reporter *reporter = reporter_alias->reporter;
	string full_format = reporter->prefix_format + parent->format;
	size_t nl = full_format.find_last_of('\n');

	// Strip trailing newline. 
	if (nl) {
		if (full_format.substr(nl+1) != "")
		parent->parser->semantic_error(
				"in format string, newline is not last");
		full_format = full_format.substr(0, nl);
	}

	if (reporter->prefix_args && reporter->prefix_args->size() > 0)
		get_prefix_args = 1;

	std::stringstream command; 
	command <<  "regex_converter " << REGEX_MAXLEN << " " << "\""; 
	command << full_format << "\" " << get_prefix_args;

	std::string tmp = command.str();
	const char* cstr = tmp.c_str();

	if (!(in = popen(cstr, "r"))) {
		parent->parser->semantic_error("cannot create regex text,"
				"regex_converter may not be installed");
		return;
	}
 
	fgets(regex_cstr,REGEX_MAXLEN, in);
 
	pclose(in);

	if (!strcmp(regex_cstr, "regex parser failure")) {
		parent->parser->semantic_error(
				"cannot create regex text from format");
		return;
	}

	regex_text = regex_cstr;

	nl = regex_text.find_last_of('\n');

	// Strip trailing newline. 
	if (nl) {
 		regex_text = regex_text.substr(0, nl);
	}
	// Change expr to ^expr$ so we match only the full message. 
	regex_text = "^" + regex_text + "$";
}


void
MatchVariant::compile_regex(void)
{
	int result;
	int regcomp_flags = REG_EXTENDED | REG_NEWLINE;
	Reporter *reporter = reporter_alias->reporter;

	if (!reporter->prefix_args || reporter->prefix_args->size() == 0)
		regcomp_flags |= REG_NOSUB;

	result = regcomp(&regex, regex_text.c_str(), regcomp_flags);
	if (result != 0) {
		char reason[200];
		(void) regerror(result, &regex, reason, 200);
		parent->parser->semantic_error("cannot compile regex: "
							+ string(reason));
	}
}

void
SyslogEvent::except(const string& reason)
{
	exception_msg = exception_catalog.find(reason);
	if (!exception_msg)
		parser->semantic_error("unknown exception type: " + reason);
}

string
SyslogEvent::paste_copies(const string &text)
{
	string s = text;
	string paste = "@paste ";
	size_t i = 0;

	while (i < s.length()) {
		i = s.find(paste, i);
		if (i == string::npos)
			break;
		size_t j, start = i + paste.length();
		size_t end = s.length();
		for (j = start; j < end; j++) {
			char c = s.at(j);
			if (!isalpha(c) && !isdigit(c) && c != '_')
				break;
		}
		/* j points to the first character after the name. */
		if (j == start) {
			parser->semantic_error("malformed @paste");
			return s;
		}
		string name = s.substr(start, j-start);
		string copy = driver->find_text_copy(name);
		if (copy == "") {
			parser->semantic_error(
					"cannot find text copy to paste: "
					+ name);
			return s;
		}
		s.replace(i, j-i, copy);
		i = j;
	}
	return s;
}

void
SyslogEvent::set_description(const string& desc)
{
	members.tally(&description, "description");
	description = paste_copies(desc);
}

void
SyslogEvent::set_action(const string& act)
{
	members.tally(&action, "action");
	action = paste_copies(act);
}

static struct NameValuePair class_nvp[] = {
	{ "unknown", SYCL_UNKNOWN },
	{ "hardware", SYCL_HARDWARE },
	{ "software", SYCL_SOFTWARE },
	{ "firmware", SYCL_FIRMWARE },
	{ NULL, SYCL_UNKNOWN }
};

void
SyslogEvent::set_class(const string& cls)
{
	members.tally(&err_class, "class");
	err_class = (ErrorClass) nvp_lookup(cls, class_nvp, "class");
}

static struct NameValuePair type_nvp[] = {
	{ "unknown", SYTY_UNKNOWN },
	{ "perm", SYTY_PERM },
	{ "temp", SYTY_TEMP },
	{ "config", SYTY_CONFIG },
	{ "pend", SYTY_PEND },
	{ "perf", SYTY_PERF },
	{ "info", SYTY_INFO },
	{ NULL, SYTY_UNKNOWN }
};

void
SyslogEvent::set_type(const string& ty)
{
	members.tally(&err_type, "type");
	err_type = (ErrorType) nvp_lookup(ty, type_nvp, "type");
}

static struct NameValuePair sl_severity_nvp[] = {
	{ "debug", SL_SEV_DEBUG },
	{ "info", SL_SEV_INFO },
	{ "event", SL_SEV_EVENT },
	{ "warning", SL_SEV_WARNING },
	{ "error_local", SL_SEV_ERROR_LOCAL },
	{ "error", SL_SEV_ERROR },
	{ "fatal", SL_SEV_FATAL },
	{ NULL, 0 }
};

void
SyslogEvent::set_sl_severity(const string& s)
{
	members.tally(&sl_severity, "sl_severity");
	sl_severity = nvp_lookup(s, sl_severity_nvp, "sl_severity");
}

void
SyslogEvent::set_refcode(const string& s)
{
	members.tally(&refcode, "refcode");
	refcode = s;
}

static struct NameValuePair priority_nvp[] = {
	{ "H", 'H' },
	{ "M", 'M' },
	{ "A", 'A' },
	{ "B", 'B' },
	{ "C", 'C' },
	{ "L", 'L' },
	{ NULL, '\0' }
};

void
SyslogEvent::set_priority(const string& s)
{
	members.tally(&priority, "priority");
	priority = (char) nvp_lookup(s, priority_nvp, "priority");
}

void
SyslogEvent::verify_complete(void)
{
	if (!exception_msg) {
		members.require(&description, "description");
		members.require(&action, "action");
		members.require(&err_class, "class");
		if ((err_type && sl_severity) || (!err_type && !sl_severity)) {
			parser->semantic_error("You must specify either "
				"type or sl_severity, but not both.");
		}
	}
	vector<MatchVariant*>::iterator it;
	for (it = match_variants.begin(); it != match_variants.end(); it++) {
		if ((*it)->regex_text.empty())
			parser->semantic_error("Catalog doesn't provide regex"
				" for this message (reporter " +
				(*it)->reporter_alias->name +
				") and it can't be computed.");
	}
}

void
SyslogEvent::set_regex(const string& rpt, const string& rgxtxt)
{
	vector<MatchVariant*>::iterator it;
	for (it = match_variants.begin(); it != match_variants.end(); it++) {
		if ((*it)->reporter_alias->name == rpt) {
			(*it)->set_regex(rgxtxt);
			return;
		}
	}
	parser->semantic_error("regex statement: reporter " + rpt +
		+ " is not associated with this message.");
}

/*
 * If msg matches the regular expression of one of the events's MatchVariants,
 * set this->matched_variant to that MatchVariant, and return a pointer to it.
 * If get_prefix_args is true, also populate msg->prefix_args.  Return NULL,
 * and set this->matched_variant=NULL, if no match.
 */
MatchVariant *
SyslogEvent::match(SyslogMessage *msg, bool get_prefix_args)
{
	matched_variant = NULL;

	assert(msg);
	if (!msg->parsed)
		return NULL;
	if (msg->from_kernel != from_kernel)
		return NULL;

	vector<MatchVariant*>::iterator it;
	for (it = match_variants.begin(); it < match_variants.end(); it++) {
	if ((*it)->match(msg, get_prefix_args)) {
		matched_variant = *it;
		break;
	}
}
return matched_variant;
}

int
SyslogEvent::get_severity(void)
{
	if (matched_variant)
		return matched_variant->severity;
	if (match_variants.size() > 0) {
		MatchVariant *first = match_variants.front();
		return first->severity;
	}
	return LOG_SEV_UNKNOWN;
}

/*
 * Called by SyslogEvent::match() to test this variant.
 */
bool
MatchVariant::match(SyslogMessage *msg, bool get_prefix_args)
{
	bool result;
	size_t nr_prefix_args, nmatch;
	regmatch_t *pmatch;
	Reporter *reporter = reporter_alias->reporter;

	if (get_prefix_args && reporter->prefix_args) {
		nr_prefix_args = reporter->prefix_args->size();
		nmatch = nr_prefix_args + 1;
		pmatch = new regmatch_t[nmatch];
	} else {
		nr_prefix_args = 0;
		nmatch = 0;
		pmatch = NULL;
	}

	result = regexec(&regex, msg->message.c_str(), nmatch, pmatch, 0);
	if (result != 0) {
		if (pmatch)
			delete[] pmatch;
		return 0;
	}
	if (nr_prefix_args > 0) {
		unsigned int i;
		for (i = 0; i < nr_prefix_args; i++) {
			/* pmatch[0] matches the whole line. */
			regmatch_t *subex = &pmatch[i+1];
			string arg_name = reporter->prefix_args->at(i);
			msg->prefix_args[arg_name] =
				msg->message.substr(subex->rm_so,
				subex->rm_eo - subex->rm_so);
		}
		delete[] pmatch;

		if (!parent->driver->message_passes_filters(msg)) {
			/* Message is from a different driver, perhaps. */
			msg->prefix_args.clear();
			return 0;
		}
	}
	return 1;
}

MatchVariant::MatchVariant(ReporterAlias *ra, int msg_severity, SyslogEvent *pa)
{
	assert(pa);
	assert(ra);

	parent = pa;
	reporter_alias = ra;
	severity = resolve_severity(msg_severity);
	if (regex_text_policy != RGXTXT_READ) {
		compute_regex_text();
		compile_regex();
		if (catalog_copy) {
			string regex_stmt = "regex " + ra->name +
				" \"" + add_escapes(regex_text) + "\"\n";
			catalog_copy->inject_text(regex_stmt,
				cur_parser->lineno);
		}
	} 
}

void
MatchVariant::set_regex(const string& rgxtxt)
{
	if (regex_text_policy == RGXTXT_READ) {
		regex_text = rgxtxt;
		compile_regex();
	} else if (regex_text_policy == RGXTXT_WRITE) {
		static bool reported = false;
		if (!reported) {
			cur_parser->semantic_error("Adding regex statements "
				"to file that already has them.");
			reported = true;
		}
	}
}

void
MatchVariant::report(ostream& os, bool sole_variant)
{
	string indent;
	if (sole_variant) {
		indent = "";
	} else {
		// Need to distinguish this variant from the others.
		os << "variant: " << reporter_alias->name << endl;
		indent = "  ";
	}
	os << indent << "regex_text: " << regex_text << endl;
	os << indent << "severity: " << severity_name(severity) << endl;
}

ostream& operator<<(ostream& os, const SyslogEvent& e)
{
	os << "message: " << e.reporter_name <<
		" \"" << e.escaped_format << "\"" << endl;

	bool sole_variant = (e.match_variants.size() == 1);
	vector<MatchVariant*>::const_iterator it;
	for (it = e.match_variants.begin(); it < e.match_variants.end(); it++)
		(*it)->report(os, sole_variant);

	os << "subsystem: " << e.driver->subsystem << endl;
	if (e.source_file)
		os << "file: " << "\"" << *(e.source_file) << "\"" << endl;
	if (e.exception_msg) {
		os << "exception: " << e.exception_msg->type << endl;
		return os;
	}
	os << "description {{" << endl;
	os << e.description << endl << "}}" << endl;
	os << "action {{" << endl;
	os << e.action << endl << "}}" << endl;
	os << "class: " << nvp_lookup_value(e.err_class, class_nvp) << endl;
	if (e.err_type)
		os << "type: " << nvp_lookup_value(e.err_type, type_nvp)
								<< endl;
	else
		os << "sl_severity: " << nvp_lookup_value(e.sl_severity,
						sl_severity_nvp) << endl;
	if (e.priority != '\0')
		os << "priority: " << e.priority << endl;
	os << "refcode: \"" << e.refcode << "\"" << endl;
	return os;
}

MessageFilter::MessageFilter(const string& name, int op, const string& value)
{
	if (op != '=')
		cur_parser->semantic_error("filter op must be '='");
	arg_name = name;
	arg_value = value;
}

/*
 * If msg has a prefix arg named arg_name (e.g., reporter), then that
 * arg's value must be arg_value to pass the filter.
 */
bool
MessageFilter::message_passes_filter(SyslogMessage *msg)
{
	map<string, string>::iterator it = msg->prefix_args.find(arg_name);
	return (it == msg->prefix_args.end() || it->second == arg_value);
}

EventCtlgFile::EventCtlgFile(const string& path, const string& subsys)
{
	pathname = path;
	subsystem = subsys;
	cur_source_file = NULL;

	size_t last_slash = pathname.rfind("/");
	if (last_slash == string::npos)
		name = pathname;
	else
		name = pathname.substr(last_slash+1);
}

void
EventCtlgFile::add_text_copy(const string& name, const string& text)
{
	if (find_text_copy(name) != "")
		cur_parser->semantic_error("duplicate name for text copy: "
								+ name);
	else
		text_copies[name] = text;
}

string
EventCtlgFile::find_text_copy(const string& name)
{
	map<string, string>::iterator it = text_copies.find(name);
	if (it == text_copies.end())
		return "";
	return it->second;
}

DevspecMacro *
EventCtlgFile::find_devspec(const string& name)
{
	map<string, DevspecMacro*>::iterator it = devspec_macros.find(name);
	if (it == devspec_macros.end())
		return NULL;
	return it->second;
}

void
EventCtlgFile::add_devspec(const string& nm, const string& path)
{
	DevspecMacro *dm = new DevspecMacro(nm, path);
	if (!dm->valid)
		delete dm;
	else if (find_devspec(nm)) {
		cur_parser->semantic_error("duplicate devspec entry for " + nm);
		delete dm;
	} else
		devspec_macros[nm] = dm;
}

void
EventCtlgFile::add_filter(MessageFilter *filter)
{
	filters.push_back(filter);
}

bool
EventCtlgFile::message_passes_filters(SyslogMessage *msg)
{
	vector<MessageFilter*>::iterator it;
	for (it = filters.begin(); it != filters.end(); it++) {
		if (!(*it)->message_passes_filter(msg))
			return false;
	}
	return true;
}

void
EventCtlgFile::set_source_file(const string& path)
{
	cur_source_file = new string(path);
	source_files.push_back(cur_source_file);
}

DevspecMacro::DevspecMacro(const string& nm, const string& path)
{
	valid = false;
	name = nm;
	string embedded_name = "/$" + nm + "/";
	size_t pos1 = path.find(embedded_name);
	if (pos1 == string::npos) {
		cur_parser->semantic_error("could not find $" + nm +
			" component of " + path);
		return;
	}
	size_t pos2 = path.rfind("/devspec");
	if (pos2 == string::npos || pos2+strlen("/devspec") != path.length()) {
		cur_parser->semantic_error("devspec is not final component of "
								+ path);
		return;
	}
	prefix = path.substr(0, pos1 + 1);
	suffix = path.substr(pos1 + embedded_name.length() - 1);
	valid = true;
}

string
DevspecMacro::get_devspec_path(const string& device_id)
{
	if (!valid)
		return "";
	return prefix + device_id + suffix;
}

/*
 * Parse all the catalog files in the specified directory, populating
 * reporter_catalog, exceptions catalog, and event_catalog.
 */
int
EventCatalog::parse(const string& directory)
{
	string path;
	string dir_w_regex, event_ctlg_dir;
	int result;
	DIR *d;
	struct dirent *dent;

	path = directory + "/reporters";
	result = reporter_ctlg_parser.parse_file(path);
	if (result != 0)
		return result;

	path = directory + "/exceptions";
	result = event_ctlg_parser.parse_file(path);
	if (result != 0)
		return result;

	dir_w_regex = directory + "/with_regex";
	if (regex_text_policy == RGXTXT_READ)
		event_ctlg_dir = dir_w_regex;
	else
		event_ctlg_dir = directory;

	d = opendir(event_ctlg_dir.c_str());
	if (!d) {
		perror(event_ctlg_dir.c_str());
		return -1;
	}
	while ((dent = readdir(d)) != NULL) {
		string name = dent->d_name;
		if (name == "reporters" || name == "exceptions")
			continue;
		path = event_ctlg_dir + "/" + name;

		/* Skip directories and such. */
		struct stat st;
		if (stat(path.c_str(), &st) != 0) {
			perror(path.c_str());
			continue;
		}
		if (!S_ISREG(st.st_mode))
			continue;

		if (regex_text_policy == RGXTXT_WRITE) {
			/*
			 * As we parse this catalog, emit a copy of it that
			 * adds a regex statement for each computed regex.
			 * We assume that dir_w_regex has been created and
			 * has appropriate permissions.
			 */
			catalog_copy = new CatalogCopy(path,
					dir_w_regex + "/" + name);
			result |= event_ctlg_parser.parse_file(path);
			catalog_copy->finish_copy();
			delete catalog_copy;
			catalog_copy = NULL;
		} else
			result |= event_ctlg_parser.parse_file(path);
	}
	(void) closedir(d);
	return result;
}

void
EventCatalog::register_driver(EventCtlgFile *driver)
{
	if (driver)
		drivers.push_back(driver);
}

void
EventCatalog::register_event(SyslogEvent *event)
{
	if (event) {
		events.push_back(event);
		event->verify_complete();
	}
}

// regex to match "[%5lu.%06lu] ", as used by printk()
static const char *printk_timestamp_regex_text =
	// "^\\[[ ]{0,4}[0-9]{1,}\\.[0]{0,5}[0-9]{1,}] ";
	"^\\[[ ]{0,4}[0-9]{1,}\\.[0-9]{6}] ";
static regex_t printk_timestamp_regex;
static bool printk_timestamp_regex_computed = false;

static void
compute_printk_timestamp_regex(void)
{
	int result = regcomp(&printk_timestamp_regex,
				printk_timestamp_regex_text,
				REG_EXTENDED | REG_NOSUB | REG_NEWLINE);
	if (result != 0) {
		char reason[100];
		(void) regerror(result, &printk_timestamp_regex, reason, 100);
		fprintf(stderr, "Internal error: can't compile regular "
			"expression for printk timestamp:\n%s\n", reason);
		exit(2);
	}
	printk_timestamp_regex_computed = true;
}

/*
 * If the message begins with what looks like a timestamp emitted by printk()
 * under the CONFIG_PRINTK_TIME option, skip past that.  Return a pointer to
 * where the message seems to start.
 */
static const char*
skip_printk_timestamp(const char *msg)
{
	if (!printk_timestamp_regex_computed)
		compute_printk_timestamp_regex();
	if (msg[0] == '[' && regexec(&printk_timestamp_regex, msg,
							0, NULL, 0) == 0) {
		msg = strstr(msg, "] ");
		assert(msg);
		msg += 2;
	}
	return msg;
}

SyslogMessage::SyslogMessage(const string& s)
{
	line = s;
	parsed = false;
	char *s2 = strdup(s.c_str());
	assert(s2);
	char *saveptr = NULL;
	char *date_end, *host, *prefix, *colon_space, *final_nul;

	/* Zap newline, if any. */
	char *nl = strchr(s2, '\n');
	if (nl) {
		if (strcmp(nl, "\n") != 0) {
			fprintf(stderr, "multi-line string passed to "
				"SyslogMessage constructor\n");
			goto done;
		}
		*nl = '\0';
	}
	final_nul = s2 + strlen(s2);

	/* ": " should divide the prefix from the message. */
	colon_space = strstr(s2, ": ");
	if (!colon_space) {
		/*
		 * Could be a line like
		 * "Sep 21 11:56:10 myhost last message repeated 3 times"
		 * which we currently ignore.
		 */
		goto done;
	}

	/* Assume the date is the first 3 words, and the hostname is the 4th. */
	date = parse_syslog_date(s2, &date_end);
	if (!date)
		goto done;
	host = strtok_r(date_end, " ", &saveptr);
	if (!host)
		goto done;
	hostname = host;

	/*
	 * Careful here.  If the message is not from the kernel, the prefix
	 * could be multiple words -- e.g., gconfd messages.
	 */
	prefix = strtok_r(NULL, " ", &saveptr);
	if (!prefix || prefix > colon_space)
		goto done;
	if (!strcmp(prefix, "kernel:")) {
		from_kernel = true;
		message = skip_printk_timestamp(colon_space + 2);
	} else {
		/*
		 * For non-kernel messages, the message includes the prefix.
		 * So the last strtok_r() call probably replaced a space with
		 * a null character in our message.  Fix that.
		 */
		from_kernel = false;
		char *nul = prefix + strlen(prefix);
		if (nul < final_nul)
			*nul = ' ';
		message = prefix;
	}
	parsed = true;

done:
	free(s2);
}

string
SyslogMessage::echo(void)
{
	char cdate[32];
	struct tm tm;
	(void) localtime_r(&date, &tm);
	(void) strftime(cdate, 32, "%b %d %T", &tm);
	string sdate(cdate);

	if (from_kernel)
		return sdate + " " + hostname + " kernel: " + message;
	else
		return sdate + " " + hostname + " " + message;
}

/*
 * Compute the path to the /sys/.../devspec node for the device (if any)
 * specified in this message.  We assume that this message has been
 * matched to the specified event.  Returns 0 if this->devspec_path is
 * successfully set, or -1 otherwise.
 */
int
SyslogMessage::set_devspec_path(SyslogEvent *event)
{
	EventCtlgFile *driver;

	if (!event)
		return -1;
	driver = event->driver;
	if (!driver || driver->devspec_macros.size() == 0)
		return -1;
	if (prefix_args.size() == 0)
		return -1;

	/*
	 * Iterate through the driver's devspec macros, trying to find
	 * a prefix arg in this message that will plug in.  Use the
	 * first match we find.
	 */
	map<string, DevspecMacro*>::iterator it;
	for (it = driver->devspec_macros.begin();
			it != driver->devspec_macros.end(); it++) {
		string name = it->first;
		map<string, string>::iterator arg = prefix_args.find(name);
		if (arg != prefix_args.end()) {
			DevspecMacro *dm = it->second;
			devspec_path = dm->get_devspec_path(arg->second);
			return 0;
		}
	}
	return -1;
}

/* Get the device ID from the (matched) message. */
string
SyslogMessage::get_device_id(MatchVariant *mv)
{
	if (!mv)
		return "";
	Reporter *reporter = mv->reporter_alias->reporter;
	if (reporter->device_arg == "none")
		return "";
	return  prefix_args[reporter->device_arg];
}

CatalogCopy::CatalogCopy(const string& rd_path, const string& wr_path)
{
	valid = false;
	orig_file = NULL;
	copy_file = NULL;

	orig_path = rd_path;
	copy_path = wr_path;
	orig_file = fopen(orig_path.c_str(), "r");
	if (!orig_file) {
		fprintf(stderr,
			"can't open catalog file\n");
		perror(orig_path.c_str());
		return;
	}
	copy_file = fopen(copy_path.c_str(), "w");
	if (!copy_file) {
		fprintf(stderr,
			"can't open new catalog file\n");
		perror(copy_path.c_str());
		return;
	}
	last_line_copied = 0;
	valid = true;
}

/*
 * Copy lines from orig_file to copy_file 'til we've copied through
 * line line_nr in orig_file.  Returns 0 if all lines copied, EOF
 * otherwise.  A negative line_nr says copy through EOF (and return EOF).
 */
int
CatalogCopy::copy_through(int line_nr)
{
	int copied;
	char buf[1024];		// Longer lines are OK.

	if (line_nr < 0)
		line_nr = 1000*1000;
	copied = last_line_copied;
	while (copied < line_nr && fgets(buf, 1024, orig_file)) {
		fputs(buf, copy_file);
		if (strchr(buf, '\n'))
			copied++;
	}
	last_line_copied = copied;
	return (last_line_copied == line_nr ? 0 : EOF);
}

/*
 * Copy through line line_nr in orig_file, then append text, which is
 * assumed to end in a newline, if appropriate.
 */
void
CatalogCopy::inject_text(const string& text, int line_nr)
{
	if (!valid)
		return;
	if (copy_through(line_nr) == EOF) {
		fprintf(stderr, "%s truncated at line %d\n",
			orig_path.c_str(), last_line_copied);
		valid = false;
	}
	fputs(text.c_str(), copy_file);
}

void
CatalogCopy::finish_copy(void)
{
	if (valid)
		(void) copy_through(-1);
}

CatalogCopy::~CatalogCopy()
{
	if (orig_file)
		fclose(orig_file);
	if (copy_file)
		fclose(copy_file);
}

string
indent_text_block(const string& s1, size_t nspaces)
{
	if (nspaces == 0)
		return s1;

	string s2 = s1;
	size_t pos = 0, skip_nl = 0;

	do {
		s2.insert(pos + skip_nl, nspaces, ' ');
		pos += nspaces + skip_nl;
		skip_nl = 1;
	} while ((pos = s2.find_first_of('\n', pos)) != string::npos);
	return s2;
}
