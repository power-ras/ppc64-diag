#
# Makefile for lpd/man
#
.SILENT:
include ../../rules.mk

LPD_MAN = lp_diag.8.gz usysattn.8.gz usysident.8.gz usysfault.8.gz

all: $(LPD_MAN)

%.8.gz: %.8
	gzip -c $< > $@

install: all
	@$(call install_man,$(LPD_MAN),$(DESTDIR))

uninstall:
	@$(call uninstall_man,$(LPD_MAN),$(DESTDIR))

clean:
	@rm -f *.gz
