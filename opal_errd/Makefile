#
# Makefile for ppc64-diag/opal_errd
#

include ../rules.mk

CMDS = opal_errd
OPAL_ERRD_OBJS = opal_errd.o


all: $(CMDS)

opal_errd: $(OPAL_ERRD_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CC) $(CFLAGS) -o $@ $^ $(OPAL_ERRD_LIBS)

install: all
	@$(call install_sbin,$(CMDS),$(DESTDIR))

uninstall:
	@$(call uninstall_sbin,$(CMDS),$(DESTDIR))

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -rf $(CMDS) $(OPAL_ERRD_OBJS)
