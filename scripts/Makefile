#
# Makefile for ppc64-diag/scripts
#

include ../rules.mk

SCRIPTS = ppc64_diag_setup ppc64_diag_mkrsrc ppc64_diag_notify \
          ppc64_diag_migrate servevent_parse.pl
INIT_FILES = rtas_errd
CFG_FILES = ppc64-diag.config

all: $(SCRIPTS) $(INIT_FILES) $(CFG_FILES)

install: all
	@$(call install_files,$(SCRIPTS),744,$(DESTDIR)/etc/ppc64-diag)
	@$(call install_files,$(INIT_FILES),755,$(DESTDIR)/etc/init.d)
	@$(call install_files,$(CFG_FILES),644,$(DESTDIR)/etc/ppc64-diag)

uninstall: 
	@$(call uninstall_files,$(SCRIPTS),$(DESTDIR)/etc/ppc64-diag)
	@$(call uninstall_files,$(INIT_FILES),$(DESTDIR)/etc/init.d)
	@$(call uninstall_files,$(CFG_FILES),$(DESTDIR)/etc/ppc64-diag)

clean:

