#
# Makefile for ppc64-diag/diags
#

include ../rules.mk

INCLUDE = -I$(COMMON_DIR)

PROGS = diag_encl encl_led

DIAG_ENCL_OBJS = diag_encl.o 7031_D24_T24.o bluehawk.o encl_util.o \
		 encl_common.o homerun.o
ENCL_LED_OBJS = encl_led.o encl_util.o bluehawk_led.o homerun_led.o
COMMON_UTILS_OBJ = $(addprefix $(COMMON_DIR)/, platform.o utils.o)

DIAG_ENCL_LIBS = -lservicelog -lrtasevent

SUBDIRS = man

DOXYGEN_CFG = doxygen.config

all: $(PROGS)
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d;)

diag_encl: $(DIAG_ENCL_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	@$(CC) $(CFLAGS) $(INCLUDE) $(COMMON_UTILS_OBJ) -o $@ $^ $(DIAG_ENCL_LIBS)

encl_led: $(ENCL_LED_OBJS)
	@echo "LD $(WORK_DIR)/$@"
	@$(CC) $(CFLAGS) $(INCLUDE) $(COMMON_UTILS_OBJ) -o $@ $^

diag_encl.o: encl_util.h diag_encl.h

7031_D24_T24.o: diag_encl.h

bluehawk.o: encl_util.h bluehawk.h diag_encl.h

encl_util.o: encl_util.h

encl_led.o: encl_util.h bluehawk.h

install: all
	@$(call install_sbin,diag_encl,$(DESTDIR))
	@$(call install_sbin,encl_led,$(DESTDIR))
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d install)

uninstall:
	@$(call uninstall_sbin,diag_encl,$(DESTDIR))
	@$(call uninstall_sbin,encl_led,$(DESTDIR))
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d uninstall)

doc: $(DOXYGEN_CFG)
	@echo "Creating doxygen docs for diag_encl"
	@$(DOXYGEN) $(DOXYGEN_CFG)

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -rf $(PROGS) *.o
