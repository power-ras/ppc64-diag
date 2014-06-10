#
# Makefile for ppc64-diag/opal_errd/opal_elog_parse
#

include ../../rules.mk

Q =

ifndef V
Q = @
else
ifeq ($(V),0)
Q = @
endif
endif

CMDS = opal-elog-parse

OPAL_ELOG_OBJS = parse-opal-event.o opal-elog-parse.o opal-event-log.o \
                 print_helpers.o opal-event-data.o print-opal-event.o \
                 parse_helpers.o opal-datetime.o opal-v6-hdr.o \
                 opal-mtms-scn.o opal-mtms-struct.o opal-priv-hdr-scn.o \
                 opal-lr-scn.o opal-eh-scn.o opal-ep-scn.o opal-sw-scn.o \
                 opal-ud-scn.o opal-hm-scn.o opal-ch-scn.o opal-lp-scn.o \
                 opal-ie-scn.o opal-mi-scn.o opal-ei-scn.o

all: $(CMDS)

opal-elog-parse: $(OPAL_ELOG_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(Q)$(CC) $(CFLAGS) -o $@ $^

opal-elog-parse.o: opal-elog-parse.c parse-opal-event.h libopalevents.h opal-event-data.h
	@echo "CC $(WORK_DIR)/$@"
	$(Q)$(CC) $(CFLAGS) -c $<

parse-opal-event.o: parse-opal-event.c libopalevents.h print-opal-event.h
	@echo "CC $(WORK_DIR)/$@"
	$(Q)$(CC) $(CFLAGS) -c $<

print-opal-event.o: print-opal-event.c print-opal-event.h libopalevents.h opal-event-data.h print_helpers.h
	@echo "CC $(WORK_DIR)/$@"
	$(Q)$(CC) $(CFLAGS) -c $<

#opal-event-data and print_helpers
%.o: %.c %.h
	@echo "CC $(WORK_DIR)/$@"
	$(Q)$(CC) $(CFLAGS) -c $<

install: all
	@$(call install_sbin,$(CMDS),$(DESTDIR))

uninstall:
	@$(call uninstall_sbin,$(CMDS),$(DESTDIR))

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	$(Q)rm -rf $(CMDS) $(OPAL_ELOG_OBJS)
