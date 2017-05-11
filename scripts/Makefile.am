PPC64_DIAG_SCRIPT = scripts/ppc64_diag_setup \
		     scripts/ppc64_diag_mkrsrc \
		     scripts/ppc64_diag_notify \
		     scripts/ppc64_diag_migrate \
		     scripts/servevent_parse.pl

CONFIG_FILE = scripts/ppc64-diag.config

INIT_FILES = scripts/rtas_errd scripts/opal_errd

SERVICE_SCRIPT = scripts/rtas_errd.service scripts/opal_errd.service

install-exec-hook-scripts:
	install -d --mode=755 $(DESTDIR)/etc/ppc64-diag/
	install -D --mode=744 $(PPC64_DIAG_SCRIPT) $(DESTDIR)/etc/ppc64-diag/
	install -D --mode=644 $(CONFIG_FILE) $(DESTDIR)/etc/ppc64-diag
	install -d --mode=755 $(DESTDIR)/usr/libexec/ppc64-diag/
	install -D --mode=755 $(INIT_FILES) $(DESTDIR)/usr/libexec/ppc64-diag/
	install -d --mode=755 $(DESTDIR)/usr/lib/systemd/system/
	install -D --mode=644 $(SERVICE_SCRIPT) $(DESTDIR)/usr/lib/systemd/system/

uninstall-hook-scripts:
	rm -f $(DESTDIR)/etc/ppc64-diag/ppc64_diag_setup
	rm -f $(DESTDIR)/etc/ppc64-diag/ppc64_diag_mkrsrc
	rm -f $(DESTDIR)/etc/ppc64-diag/ppc64_diag_notify
	rm -f $(DESTDIR)/etc/ppc64-diag/ppc64_diag_migrate
	rm -f $(DESTDIR)/etc/ppc64-diag/servevent_parse.pl
	rm -f $(DESTDIR)/etc/ppc64-diag/ppc64-diag.config
	rm -f $(DESTDIR)/usr/libexec/ppc64-diag/rtas_errd
	rm -f $(DESTDIR)/usr/libexec/ppc64-diag/opal_errd
	rm -f $(DESTDIR)/usr/lib/systemd/system/rtas_errd.service
	rm -f $(DESTDIR)/usr/lib/systemd/system/opal_errd.service

INSTALL_EXEC_HOOKS += install-exec-hook-scripts
UNINSTALL_HOOKS += uninstall-hook-scripts

EXTRA_DIST += $(PPC64_DIAG_SCRIPT) $(CONFIG_FILE) \
	      $(INIT_FILES) $(SERVICE_SCRIPT) \
	      scripts/ppc64_diag_servagent
