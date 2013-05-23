%{
/*
 * Grammars for events catalog and exceptions catalog
 *
 * Copyright (C) International Business Machines Corp., 2009
 *
 */

#define CATALOGS_IMPLEMENTATION
#include <malloc.h>
#include "catalogs.h"

extern EventCtlgParser event_ctlg_parser;
static EventCtlgParser *pc = &event_ctlg_parser;
extern EventCatalog event_catalog;
static SyslogEvent *event;
static EventCtlgFile *driver;
extern ExceptionCatalog exception_catalog;

/* Why doesn't yacc declare this? */
extern int yylex(void);
void yyerror(const char *s);

%}

/* Note: Tokens of type sval are all strdup-ed. */
%union {
	int		ival;	/* keyword, punctuation */
	char		*sval;	/* string, name, text block */
	SyslogEvent	*event;
	EventCtlgFile	*driver;
	MessageFilter	*filter;
}

%token <ival> KW_ACTION KW_CLASS KW_COPY KW_DESCRIPTION
%token <ival> KW_DEVSPEC KW_EXCEPTION KW_FILE KW_FILTER
%token <ival> KW_MESSAGE KW_PRIORITY KW_REFCODE KW_REGEX
%token <ival> KW_SL_SEVERITY KW_SUBSYSTEM KW_TYPE
%token <sval> TK_STRING TK_NAME TK_TEXTBLOCK
%token <ival> ERRTOK

%type <ival> catalog_file driver_file exceptions_file
%type <ival> header exception
%type <driver> subsystem_stmt
%type <filter> filter_expr
%type <event> entry
%type <event> message_stmt message_exception_stmt
%type <sval> description_stmt action_stmt exception_stmt

%%
catalog_file	: driver_file
		| exceptions_file
		;

driver_file	: header entries
		;

header		: subsystem_stmt optional_header_stmts {
				event_catalog.register_driver($1);
				$$ = 0;	/* avoid yacc warning */
			}
		;

subsystem_stmt	: KW_SUBSYSTEM ':' TK_NAME {
				driver = new EventCtlgFile(pc->pathname, $3);
				$$ = driver;
				free($3);
			}

optional_header_stmts: header_stmts
		| /* NULL */
		;

header_stmts	: header_stmt
		| header_stmts header_stmt
		;

header_stmt	: copy
		| devspec_stmt
		| filter_stmt
		;

copy		: '@' KW_COPY TK_NAME TK_TEXTBLOCK {
				driver->add_text_copy($3, $4);
				free($3);
				free($4);
			}
		;

devspec_stmt	: KW_DEVSPEC '(' TK_NAME ')' '=' TK_STRING {
				driver->add_devspec($3, $6);
				free($3);
				free($6);
			}
		;

filter_stmt	: KW_FILTER ':' filter_expr {
				driver->add_filter($3);
			}
		;

filter_expr	: TK_NAME '=' TK_STRING {
				$$ = new MessageFilter($1, '=', $3);
				free($1);
				free($3);
			}
		;

entries		: entry_or_file
		| entries entry_or_file
		;


entry_or_file	: entry { event_catalog.register_event($1); }
		| file_stmt
		;

file_stmt	: KW_FILE ':' TK_STRING {
				driver->set_source_file($3);
				free($3);
			}
		;

entry		: message_stmt optional_regex_stmts explanation { $$ = $1; }
		| message_exception_stmt optional_regex_stmts
		| error	{ $$ = NULL; }
		;

message_stmt	: KW_MESSAGE ':' TK_NAME TK_STRING {
				event = new SyslogEvent($3, "", $4, driver);
				$$ = event;
				free($3);
				free($4);
			}
		| KW_MESSAGE ':' TK_NAME TK_NAME TK_STRING {
				event = new SyslogEvent($3, $4, $5, driver);
				$$ = event;
				free($3);
				free($4);
				free($5);
			}
		;

optional_regex_stmts : regex_stmts
		| /* NULL */
		;

regex_stmts	: regex_stmt
		| regex_stmts regex_stmt
		;

regex_stmt	: KW_REGEX TK_NAME TK_STRING {
				event->set_regex($2, $3);
				free($2);
				free($3);
			}
		;

explanation	: addl_stmts
		| exception_stmt {
				event->except($1);
				free($1);
			}
		;

/*
 * message[defensive]: printk "failed to set up thing\n"
 * is shorthand for
 * message: printk "failed to set up thing\n" exception: defensive
 */
message_exception_stmt: KW_MESSAGE '[' TK_NAME ']' ':' TK_NAME TK_STRING {
				event = new SyslogEvent($6, "", $7, driver);
				$$ = event;
				event->except($3);
				free($3);
				free($6);
				free($7);
			}
		| KW_MESSAGE '[' TK_NAME ']' ':' TK_NAME TK_NAME TK_STRING {
				event = new SyslogEvent($6, $7, $8, driver);
				$$ = event;
				event->except($3);
				free($3);
				free($6);
				free($7);
				free($8);
			}
		;

addl_stmts	: addl_stmt
		| addl_stmts addl_stmt
		;

addl_stmt:	description_stmt {
				event->set_description($1);
				free($1);
				// $$ = 0;
			}
		| action_stmt {
				event->set_action($1);
				free($1);
				// $$ = 0;
			}
		| class_stmt
		| type_stmt
		| sl_severity_stmt
		| priority_stmt
		| refcode_stmt
		;

exception_stmt	: KW_EXCEPTION ':' TK_NAME { $$ = $3; }
		;

description_stmt : KW_DESCRIPTION TK_TEXTBLOCK	{ $$ = $2; }
		;		

action_stmt	: KW_ACTION TK_TEXTBLOCK { $$ = $2; }
		;		

class_stmt	: KW_CLASS ':' TK_NAME {
				event->set_class($3);
				free($3);
			}
		;

type_stmt	: KW_TYPE ':' TK_NAME {
				event->set_type($3);
				free($3);
			}

sl_severity_stmt : KW_SL_SEVERITY ':' TK_NAME {
				event->set_sl_severity($3);
				free($3);
			}

priority_stmt	: KW_PRIORITY ':' TK_NAME {
				event->set_priority($3);
				free($3);
			}

refcode_stmt	: KW_REFCODE ':' TK_STRING {
				event->set_refcode($3);
				free($3);
			}

exceptions_file	: exception
		| exceptions_file exception
		;

exception	: exception_stmt description_stmt action_stmt {
				exception_catalog.add(pc, $1, $2, $3);
				free($1);
				free($2);
				free($3);
				$$ = 0;
			}
		;
%%

/* AKA everror() */
void yyerror(const char *s)
{
	fprintf(stderr, "%s:%d: %s\n", pc->pathname, pc->lineno, s);
}
