#
# Makefile for opal-dump-parse
#

include ../rules.mk

CMDS = opal-dump-parse

OPAL_DUMP_OBJS = opal-dump-parse.o

all : $(CMDS)

opal-dump-parse : opal-dump-parse.c opal-dump-parse.h

install: all
	@$(call install_sbin,$(CMDS),$(DESTDIR))

uninstall:
	@$(call uninstall_sbin,$(CMDS),$(DESTDIR))

clean:
	@echo "Cleaning up opal-dump-parse files..."
	@rm -rf $(CMDS) $(OPAL_DUMP_OBJS)
