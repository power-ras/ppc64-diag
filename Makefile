#
# Makefile for ppc64-diag
#

include rules.mk

.SILENT:
BUILD_DIR=$(CURDIR)

SUBDIRS = rtas_errd diags scripts ela lpd opal_errd opal-dump-parse

LICENSE = COPYRIGHT

DOXYGEN_CFG = doxygen.config
TARBALL_FILES += $(DOXYGEN_CFG)

all:
	@echo "Building rtas_errd files..."
	@$(MAKE) -C rtas_errd
	@echo "Building diags files..."
	@$(MAKE) -C diags
	@echo "Building ela files..."
	@$(MAKE) -C ela
	@echo "Building lpd files..."
	@$(MAKE) -C lpd
	@echo "Building opal_errd files..."
	@$(MAKE) -C opal_errd
	@echo "Building opal-dump-parse files..."
	@$(MAKE) -C opal-dump-parse


install: all
	@$(call install_files,$(LICENSE),644,$(DESTDIR)/usr/share/doc/packages/ppc64-diag)
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d install;)

uninstall: 
	@$(call uninstall_files,$(LICENSE),644,$(DESTDIR)/usr/share/doc/packages/ppc64-diag)
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d uninstall;)

rpm: all
	@echo "Creating rpm..."
	@export DESTDIR=$(SHIPDIR); $(MAKE) install
	@$(RPM) -bb ppc64-diag.spec
	@rm -rf $(SHIPDIR)

tarball: clean
	@echo "Creating source tarball..."
	@$(BUILD_TARBALL)

doc: $(DOXYGEN_CFG)
	@$(DOXYGEN) $(DOXYGEN_CFG)

clean:
	@$(foreach d,$(SUBDIRS), $(MAKE) -C $d clean;)
	@$(CLEAN) $(SHIPDIR) $(TARBALL) doc
	rm -rf TAGS cscope.*

.PHONY: TAGS
TAGS:
	rm -f $@
	find "$(BUILD_DIR)" -name '*.[hc]' -exec etags --append {} +

cscope:
	rm -f ./cscope.*
	find "$(BUILD_DIR)" -name "*.[chsS]" -print | sed 's,^\./,,' > ./cscope.files
	cscope -b
