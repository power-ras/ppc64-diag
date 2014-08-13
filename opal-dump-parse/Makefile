#
# Makefile for opal-dump-parse
#

include ../rules.mk

CMDS = opal-dump-parse
OPAL_DUMP_MAN = opal-dump-parse.8.gz

OPAL_DUMP_OBJS = opal-dump-parse.o

all : $(CMDS) $(OPAL_DUMP_MAN)

opal-dump-parse : opal-dump-parse.c opal-dump-parse.h

%.8.gz: %.8
	gzip -c $< > $@

install: all
	@$(call install_sbin,$(CMDS),$(DESTDIR))
	@$(call install_man,$(OPAL_DUMP_MAN),$(DESTDIR))

uninstall:
	@$(call uninstall_sbin,$(CMDS),$(DESTDIR))
	@$(call uninstall_man,$(OPAL_DUMP_MAN), $(DESTDIR))

clean:
	@echo "Cleaning up opal-dump-parse files..."
	@rm -rf $(CMDS) $(OPAL_DUMP_OBJS)
	@rm -rf *.8.gz
