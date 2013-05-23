#
# Makefile for lpd/scripts
#
include ../../rules.mk

SCRIPTS = lp_diag_setup lp_diag_notify

all: $(SCRIPTS)

install: all
	@$(call install_files,$(SCRIPTS),744,$(DESTDIR)/etc/ppc64-diag)

uninstall:
	@$(call uninstall_files,$(SCRIPTS),$(DESTDIR)/etc/ppc64-diag)

clean:
