lpd_common_h_files = lpd/indicator.h \
		     lpd/indicator_marvell.h \
		     lpd/indicator_ses.h \
		     lpd/lp_util.h

lpd_h_files = $(lpd_common_h_files) lpd/servicelog.h

lpd_common_files = lpd/files.c \
		   lpd/lp_util.c \
		   lpd/indicator.c \
		   lpd/indicator_ses.c \
		   lpd/indicator_opal.c \
		   lpd/indicator_rtas.c \
		   lpd/indicator_marvell.c \
		   common/utils.c \
		   common/platform.c

lpd_common_lib = -lrtas -lrtasevent -lservicelog
lp_diag_lib = -lncurses -lmenu $(lpd_common_lib)

sbin_PROGRAMS += lpd/lp_diag lpd/usysident

lpd_lp_diag_SOURCES = lpd/lp_diag.c \
		      lpd/servicelog.c \
		      $(lpd_common_files) \
		      $(lpd_h_files)
lpd_lp_diag_LDADD = $(lp_diag_lib)

lpd_usysident_SOURCES = lpd/usysident.c \
			$(lpd_common_files)\
			$(lpd_common_h_files)
lpd_usysident_LDADD = $(lpd_common_lib)

dist_man_MANS += lpd/man/lp_diag.8 \
		 lpd/man/usysattn.8 \
		 lpd/man/usysident.8 \
		 lpd/man/usysfault.8

LPD_SCRIPT = lpd/scripts/lp_diag_setup \
	     lpd/scripts/lp_diag_notify

all-local-lpd:
	ln -sf usysident lpd/usysattn
	ln -sf usysident lpd/usysfault

AM_LOCALS += all-local-lpd

install-exec-hook-lpd:
	install -d --mode=755 $(DESTDIR)/etc/ppc64-diag
	install -D --mode=744 $(LPD_SCRIPT) $(DESTDIR)/etc/ppc64-diag/
	(cd $(DESTDIR)/$(sbindir) && $(LN_S) -f usysident usysattn)
	(cd $(DESTDIR)/$(sbindir) && $(LN_S) -f usysident usysfault)

INSTALL_EXEC_HOOKS += install-exec-hook-lpd

uninstall-hook-lpd:
	rm -f $(DESTDIR)/etc/ppc64-diag/lp_diag_setup
	rm -f $(DESTDIR)/etc/ppc64-diag/lp_diag_notify
	rm -f $(sbindir)/usysattn
	rm -f $(sbindir)/usysfault

UNINSTALL_HOOKS += uninstall-hook-lpd

clean-local-lpd:
	rm -f lpd/usysattn
	rm -f lpd/usysfault

CLEAN_LOCALS += clean-local-lpd

lpd_test_common_files = lpd/servicelog.c \
			$(lpd_common_files)

check_PROGRAMS += lpd/test/ledtool \
		  lpd/test/sesdevices

lpd_test_ledtool_SOURCES = lpd/test/ledtool.c \
			   $(lpd_test_common_files)
lpd_test_ledtool_LDADD = $(lpd_common_lib)

lpd_test_sesdevices_SOURCES = lpd/test/sesdevices.c \
			      $(lpd_test_common_files)
lpd_test_sesdevices_LDADD = $(lpd_common_lib)

EXTRA_DIST += $(LPD_SCRIPT) \
	      lpd/test/e1000e_message \
	      lpd/test/e1000e_message2 \
	      lpd/test/lpd_ela_test.sh
