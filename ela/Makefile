#
## Makefile for ppc64-diag/ela
#

include ../rules.mk

CMDS = explain_syslog add_regex
CATALOG = message_catalog/cxgb3 message_catalog/e1000e \
	  message_catalog/exceptions message_catalog/reporters \
	  message_catalog/gpfs
CATALOG_REGEX = message_catalog/with_regex/cxgb3 \
	        message_catalog/with_regex/e1000e \
		message_catalog/with_regex/gpfs
MANPAGES = doc/explain_syslog.8.gz doc/syslog_to_svclog.8.gz	
ARCH := $(shell uname -p)
ifeq ($(ARCH), ppc)
        CMDS += syslog_to_svclog
endif
ifeq ($(ARCH), ppc64)
        CMDS += syslog_to_svclog
endif
 
COMMON_OBJS = rr.tab.o lex.rr.o ev.tab.o lex.ev.o date.o
ES_OBJS = explain_syslog.o catalogs.o $(COMMON_OBJS)
SS_OBJS = syslog_to_svclog.o catalogs.o $(COMMON_OBJS)
AR_OBJS = add_regex.o catalogs.o $(COMMON_OBJS)
SS_LIBS = -lservicelog -lvpd -lvpd_cxx -lsqlite3 -lrtasevent

all: $(CMDS) $(CATALOG) $(CATALOG_REGEX)

explain_syslog: $(ES_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CXX) $(CXXFLAGS) -o $@ $^ 

syslog_to_svclog: $(SS_OBJS) 
	@echo "LD $(WORK_DIR)/$@"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SS_LIBS)

add_regex: $(AR_OBJS)
	@echo "LD $(WORK_DIR)/$@" 
	$(CXX) $(CXXFLAGS) -o $@ $^

rr.tab.o: rr.tab.c rr.tab.h catalogs.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c rr.tab.c

ev.tab.o: ev.tab.c ev.tab.h catalogs.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ev.tab.c

rr.tab.c: rr.tab.h

ev.tab.c: ev.tab.h

rr.tab.h: reporter_gram.y
	yacc -dl -b rr -p rr reporter_gram.y

ev.tab.h: event_gram.y
	yacc -dl -b ev -p ev event_gram.y

lex.rr.o: lex.rr.c rr.tab.h catalogs.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c lex.rr.c

lex.ev.o: lex.ev.c ev.tab.h catalogs.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c lex.ev.c

lex.rr.c: reporter_lex.l
	flex -Prr reporter_lex.l

lex.ev.c: event_lex.l
	flex -Pev event_lex.l

catalogs.o: catalogs.h

install: all
	@$(call install_sbin,$(CMDS),$(DESTDIR))        
	@$(call install_files,$(CATALOG),744,$(DESTDIR)/etc/ppc64-diag/message_catalog)
	@$(call install_files,$(CATALOG_REGEX),744,$(DESTDIR)/etc/ppc64-diag/message_catalog/with_regex)
	@$(call install_man,$(MANPAGES),$(DESTDIR))
	$(INSTALL) -d -m 755 /var/log/ppc64-diag

clean:
	@rm -rf $(CMDS) *.o lex.*.c *.tab.c *.tab.h

uninstall:
	@$(call uninstall_sbin,$(CMDS),$(DESTDIR))        
	@$(call uninstall_files,$(CATALOG),$(DESTDIR)/etc/ppc64-diag/)
	@$(call uninstall_files,$(CATALOG_REGEX),$(DESTDIR)/etc/ppc64-diag/)
	@$(call uninstall_man,$(MANPAGES),$(DESTDIR)/usr/share/man/man8)
 
