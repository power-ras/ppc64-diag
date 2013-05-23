%{
/*
 * Grammar for reporters catalog
 *
 * Copyright (C) International Business Machines Corp., 2009
 *
 */

#define CATALOGS_IMPLEMENTATION
#include <malloc.h>
#include "catalogs.h"

extern ReporterCtlgParser reporter_ctlg_parser;
static ReporterCtlgParser *pc = &reporter_ctlg_parser;
extern ReporterCatalog reporter_catalog;
static Reporter *reporter;
static MetaReporter *meta_reporter;
static vector<string> *name_list;
static vector<ReporterAlias*> *alias_list;

/* Why doesn't yacc declare this? */
extern int yylex(void);
void yyerror(const char *s);

%}

/* Note: Tokens of type sval are all strdup-ed. */
%union {
	int		ival;	/* keyword, punctuation */
	char		*sval;	/* string, name */
	Reporter	*reporter;
	MetaReporter	*meta_reporter;
	vector<string>	*name_list;
	ReporterAlias	*alias;
	vector<ReporterAlias*> *alias_list;
}

%token <ival> KW_ALIASES KW_META_REPORTER KW_PREFIX_ARGS KW_PREFIX_FORMAT
%token <ival> KW_REPORTER KW_SOURCE KW_VARIANTS KW_DEVICE_ARG
%token <sval> TK_STRING TK_NAME
%token <ival> ERRTOK

%type <reporter> reporter_entry reporter_stmt
%type <meta_reporter> meta_reporter_entry meta_reporter_stmt
%type <ival> source_stmt
%type <name_list> name_list
%type <alias> name_and_lvl
%type <alias_list> alias_list

%%
catalog		: entry
		| catalog entry
		;

entry		: reporter_entry {
				reporter_catalog.register_reporter($1);
			}
		| meta_reporter_entry {
				reporter_catalog.register_meta_reporter($1);
			}
		| error
		;

reporter_entry	: reporter_stmt
			source_stmt
			aliases_stmt
			prefix_format_stmt
			prefix_args_stmt
			device_arg_stmt
				{ $$ = $1; }
		;

meta_reporter_entry : meta_reporter_stmt variants_stmt { $$ = $1; }
		;

meta_reporter_stmt : KW_META_REPORTER ':' TK_NAME {
				meta_reporter = new MetaReporter($3);
				free($3);
				$$ = meta_reporter;
			}
		;

variants_stmt	: KW_VARIANTS ':' name_list {
				meta_reporter->set_variant_names($3);
			}
		;

reporter_stmt	: KW_REPORTER ':' name_and_lvl { 
				reporter = new Reporter($3);
				$$ = reporter;
			}
		;

source_stmt	: KW_SOURCE ':' TK_NAME {
				reporter->set_source($3);
				free($3);
			}
		;

name_list	: TK_NAME	{ 
				name_list = new vector<string>();
				name_list->push_back($1);
				$$ = name_list;
				free($1);
			}
		| name_list TK_NAME	{
				name_list->push_back($2); 
				$$ = name_list;
				free($2);
			}
		;

name_and_lvl	: TK_NAME {
				$$ = new ReporterAlias($1);
				free($1);
			}
		| TK_NAME '(' TK_NAME ')' {
				$$ = new ReporterAlias($1, $3);
				free($1);
				free($3);
			}
		;

alias_list	: name_and_lvl {
				alias_list = new vector<ReporterAlias*>();
				alias_list->push_back($1);
				$$ = alias_list;
			}
		| alias_list name_and_lvl {
				alias_list->push_back($2); 
				$$ = alias_list;
			}
		;

aliases_stmt	: KW_ALIASES ':' alias_list	{ reporter->set_aliases($3); }
		| /*NULL*/
		;

prefix_format_stmt : KW_PREFIX_FORMAT ':' TK_STRING 	{
				reporter->set_prefix_format($3);
				free($3);
			}
		| /*NULL*/
		;

prefix_args_stmt : KW_PREFIX_ARGS ':' name_list	{
				reporter->set_prefix_args($3);
			}
		| /*NULL*/
		;

device_arg_stmt	: KW_DEVICE_ARG ':' TK_NAME {
				reporter->set_device_arg($3);
				free($3);
			}
		| /*NULL*/
		;

%%

/* AKA rrerror() */
void yyerror(const char *s)
{
	fprintf(stderr, "%s:%d: %s\n", pc->pathname, pc->lineno, s);
}
