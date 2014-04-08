#
# Makefile for ppc64-diag/opal_errd/opal_elog_parse
#

include ../../rules.mk

CMDS = opal-elog-parse

OPAL_ELOG_OBJS = parse-opal-event.o opal-elog-parse.o print-opal-event.o print_helpers.o opal-event-data.o

all: $(CMDS)

opal-elog-parse: $(OPAL_ELOG_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CC) $(CFLAGS) -o $@ $^

install: all
	@$(call install_sbin,$(CMDS),$(DESTDIR))

uninstall:
	@$(call uninstall_sbin,$(CMDS),$(DESTDIR))

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -rf $(CMDS) $(OPAL_ELOG_OBJS)
