#
# Makefile for lp/test
#

.SILENT:
include ../../rules.mk

CMDS = ledtool sesdevices

INCLUDE = -I..
CFLAGS = -g -DDEBUG $(INCLUDE)
AM_CFLAGS = -DVERSION='"$(VERSION)"'

LPD_DIR = $(ROOT_DIR)/lpd

COMMON_OBJS = ../files.o ../lp_util.o ../indicator_ses.o \
	      ../indicator_rtas.o ../indicator.o ../servicelog.o

LED_TOOL_OBJS =  ledtool.o
SES_TOOL_OBJS = sesdevices.o

LED_TOOL_LIBS = -lrtas -lrtasevent -lservicelog -lsqlite3

LICENSE = COPYRIGHT

all: $(CMDS)

ledtool: $(LED_TOOL_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CC) $(CFLAGS) $(COMMON_OBJS) -o $@ $^ $(LED_TOOL_LIBS)

sesdevices: $(SES_TOOL_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	$(CC) $(CFLAGS) $(COMMON_OBJS) -o $@ $^ $(LED_TOOL_LIBS)

install: all
	@$(call install_sbin,$(CMDS),$(DESTDIR))

uninstall:
	@$(call uninstall_sbin,$(CMDS),$(DESTDIR))

clean:
	@echo "Cleaning up lpd test files..."
	@rm -rf $(SES_TOOL_OBJS) $(LED_TOOL_OBJS) $(CMDS)
