#
# Makefile for ppc64-diag/diags/doc
#

include ../../rules.mk

MANPAGES = diag_encl.8.gz encl_led.8.gz

all: $(MANPAGES)

%.8.gz: %.8
	gzip -c $< > $@

install: all
	@$(call install_man,$(MANPAGES),$(DESTDIR))

uninstall:
	@$(call uninstall_man,$(MANPAGES),$(DESTDIR))
