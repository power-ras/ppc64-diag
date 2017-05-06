BUILT_SOURCE = ela/ev.tab.h ela/rr.tab.h \
	       ela/ev.tab.cc ela/rr.tab.cc \
	       ela/lex.rr.cc ela/lex.ev.cc

ela_h_files = ela/catalogs.h

CATALOG = ela/message_catalog/cxgb3 ela/message_catalog/e1000e \
	  ela/message_catalog/exceptions ela/message_catalog/reporters \
	  ela/message_catalog/gpfs

CATALOG_REGEX = ela/message_catalog/with_regex/cxgb3 \
		ela/message_catalog/with_regex/e1000e \
		ela/message_catalog/with_regex/gpfs

ela/rr.tab.h: ela/rr.tab.cc
ela/ev.tab.h: ela/ev.tab.cc

ela/ev.tab.cc: ela/event_gram.y
	yacc -dl -b ev -p ev ela/event_gram.y
	mv ev.tab.c ela/ev.tab.cc
	mv ev.tab.h ela/ev.tab.h

ela/rr.tab.cc: ela/reporter_gram.y
	yacc -dl -b rr -p rr ela/reporter_gram.y
	mv rr.tab.c ela/rr.tab.cc
	mv rr.tab.h ela/rr.tab.h

ela/lex.rr.cc: ela/reporter_lex.l
	flex -Prr ela/reporter_lex.l
	mv lex.rr.c ela/lex.rr.cc

ela/lex.ev.cc: ela/event_lex.l
	flex -Pev ela/event_lex.l
	mv lex.ev.c ela/lex.ev.cc

sbin_PROGRAMS += ela/explain_syslog ela/add_regex ela/syslog_to_svclog

ela_explain_syslog_SOURCES = ela/explain_syslog.cpp \
			     ela/catalogs.cpp \
			     ela/date.c \
			     $(BUILT_SOURCE) \
			     $(ela_h_files)
ela_explain_syslog_LDADD = -lstdc++

ela_syslog_to_svclog_SOURCES = ela/syslog_to_svclog.cpp \
			       ela/catalogs.cpp \
			       ela/date.c \
			       $(BUILT_SOURCE) \
			       $(ela_h_files)
ela_syslog_to_svclog_LDADD = -lservicelog -lvpd -lvpd_cxx -lrtasevent

ela_add_regex_SOURCES = ela/add_regex.cpp \
			ela/catalogs.cpp \
			ela/date.c \
			$(BUILT_SOURCE) \
			$(ela_h_files)

dist_man_MANS += ela/man/explain_syslog.8 ela/man/syslog_to_svclog.8

clean-local-ela:
	rm -f $(BUILT_SOURCE)

CLEAN_LOCALS += clean-local-ela

install-exec-hook-ela:
	install -d --mode=755 $(DESTDIR)/etc/ppc64-diag/message_catalog/with_regex/
	install -D --mode=744 $(CATALOG) $(DESTDIR)/etc/ppc64-diag/message_catalog/
	install -D --mode=744 $(CATALOG_REGEX) \
		$(DESTDIR)/etc/ppc64-diag/message_catalog/with_regex/

INSTALL_EXEC_HOOKS += install-exec-hook-ela

uninstall-hook-ela:
	rm -f $(DESTDIR)/etc/ppc64-diag/message_catalog/cxgb3
	rm -f $(DESTDIR)/etc/ppc64-diag/message_catalog/e1000e
	rm -f $(DESTDIR)/etc/ppc64-diag/message_catalog/exceptions
	rm -f $(DESTDIR)/etc/ppc64-diag/message_catalog/reporters
	rm -f $(DESTDIR)/etc/ppc64-diag/message_catalog/gpfs
	rm -f $(DESTDIR)/etc/ppc64-diag/message_catalog/with_regex/cxgb3
	rm -f $(DESTDIR)/etc/ppc64-diag/message_catalog/with_regex/e1000e
	rm -f $(DESTDIR)/etc/ppc64-diag/message_catalog/with_regex/gpfs

UNINSTALL_HOOKS += uninstall-hook-ela

EXTRA_DIST += ela/README ela/message_catalog \
	      ela/event_lex.l ela/event_gram.y \
	      ela/reporter_lex.l ela/reporter_gram.y
