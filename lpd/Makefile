#
# Makefile for lpd
#

include ../rules.mk

CMDS = lp_diag usysident usysattn

COMMON_OBJS = files.o lp_util.o indicator_ses.o indicator_rtas.o indicator.o
LP_DIAG_OBJS =  servicelog.o lp_diag.o
USYSIDENT_OBJS = usysident.o

COMMON_LIBS = -lrtas -lrtasevent -lservicelog -lsqlite3
LP_DIAG_LIBS = -lncurses -lmenu

LICENSE = COPYRIGHT

SUBDIRS = scripts man

all: $(CMDS)

lp_diag: $(LP_DIAG_OBJS) $(COMMON_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CC) $(CFLAGS) -o $@ $^ $(COMMON_LIBS) $(LP_DIAG_LIBS)

usysident: $(USYSIDENT_OBJS) $(COMMON_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CC) $(CFLAGS) -o $@ $^ $(COMMON_LIBS)

usysattn: $(USYSIDENT_OBJS) $(COMMON_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CC) $(CFLAGS) -o $@ $^ $(COMMON_LIBS)

install: all
	@$(call install_sbin,$(CMDS),$(DESTDIR))
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d install;)

uninstall:
	@$(call uninstall_sbin,$(CMDS),$(DESTDIR))
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d uninstall;)

clean:
	@echo "Cleaning up lpd files..."
	@rm -rf $(CMDS) $(COMMON_OBJS) $(LP_DIAG_OBJS) $(USYSIDENT_OBJS)
