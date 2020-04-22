ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I $(top_srcdir)/common/ -I $(top_srcdir)/ela/ \
	    -I $(top_srcdir)/diags -I $(top_srcdir)/lpd/ \
	    -I /usr/include/ncurses/ -Wall -g -DDEBUG \
	    -DDEST_DIR='"${exec_prefix}"' -DVERSION='"@VERSION@"'

AM_CXXFLAGS = -I $(top_srcdir)/common/ -I $(top_srcdir)/ela/ \
	      -I $(top_srcdir)/diags -I $(top_srcdir)/lpd/ \
	      -I /usr/include/ncurses/ -Wall -g -DDEBUG \
	      -DDEST_DIR='"${exec_prefix}"' -DVERSION='"@VERSION@"'

AM_LDFLAGS =

dist_man_MANS =
sbin_PROGRAMS =
check_PROGRAMS =
noinst_PROGRAMS =
doc_DATA =
EXTRA_DIST = README.md
INSTALL_EXEC_HOOKS =
UNINSTALL_HOOKS =
AM_LOCALS =
CLEAN_LOCALS =
CHECK_LOCALS =

doc_DATA += COPYING README.md
SPECFILENAME = ppc64-diag.spec

include common/Makefile.am

if WITH_LIBRTAS
include rtas_errd/Makefile.am
include lpd/Makefile.am
include diags/Makefile.am
include diags/test/Makefile.am
include ela/Makefile.am
endif

include opal-dump-parse/Makefile.am
include opal_errd/Makefile.am
include scripts/Makefile.am

EXTRA_DIST += autogen.sh $(SPECFILENAME)

install-exec-hook: $(INSTALL_EXEC_HOOKS)

uninstall-hook: $(UNINSTALL_HOOKS)

am-local: $(AM_LOCALS)

clean-local: $(CLEAN_LOCALS)
