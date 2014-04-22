#
# Makefile for ppc64-diag/opal_errd
#

include ../rules.mk

CMDS = opal_errd extract_opal_dump

OPAL_ERRD_OBJS = opal_errd.o
OPAL_ERRD_LIBS = -ludev
OPAL_DUMP_OBJS = extract_opal_dump.o

SUBDIRS = opal-elog-parse

all: $(CMDS)
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d;)

opal_errd: $(OPAL_ERRD_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CC) $(CFLAGS) -o $@ $^ $(OPAL_ERRD_LIBS)

extract_opal_dump: $(OPAL_DUMP_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CC) $(CFLAGS) -o $@ $^ $(OPAL_DUMP_LIBS)

install: all
	@$(call install_sbin,$(CMDS),$(DESTDIR))
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d install;)

uninstall:
	@$(call uninstall_sbin,$(CMDS),$(DESTDIR))
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d uninstall;)

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -rf $(CMDS) $(OPAL_ERRD_OBJS) $(OPAL_DUMP_OBJS)
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d clean;)
