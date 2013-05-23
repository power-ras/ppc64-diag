#
# Makefile for lpd/man
#
.SILENT:
include ../../rules.mk

MANPAGES = lp_diag.8 usysattn.8 usysident.8 usysfault.8

C_MANPAGES = lp_diag.8.gz usysattn.8.gz usysident.8.gz usysfault.8.gz

all: $(MANPAGES)

install: all
	@$(call install_man,$(MANPAGES),$(DESTDIR))
	for file in $(MANPAGES) ; do \
		gzip -f $(DESTDIR)/$(MAN_DIR)/$$file ;\
	done

uninstall:
	@$(call uninstall_man,$(C_MANPAGES),$(DESTDIR))

clean:
