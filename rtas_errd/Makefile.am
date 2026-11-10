rtas_errd_h_files = rtas_errd/config.h \
		    rtas_errd/dchrp_frus.h \
		    rtas_errd/dchrp.h \
		    rtas_errd/ela_msg.h \
		    rtas_errd/fru_prev6.h \
		    rtas_errd/rtas_errd.h

rtas_errd_common_source = common/platform.c

sbin_PROGRAMS += rtas_errd/convert_dt_node_props \
		 rtas_errd/extract_platdump \
		 rtas_errd/rtas_errd

rtas_errd_convert_dt_node_props_SOURCES = rtas_errd/convert_dt_node_props.c \
					  $(rtas_errd_common_source)

rtas_errd_extract_platdump_SOURCES = \
		rtas_errd/extract_platdump.c \
		rtas_errd/config.c \
		rtas_errd/rtas_errd.h \
		$(rtas_errd_common_source)
rtas_errd_extract_platdump_LDADD = -lrtas

rtas_errd_rtas_errd_SOURCES = \
		rtas_errd/rtas_errd.c \
		rtas_errd/epow.c \
		rtas_errd/dump.c \
		rtas_errd/guard.c \
		rtas_errd/eeh.c \
		rtas_errd/update.c \
		rtas_errd/files.c \
		rtas_errd/config.c \
		rtas_errd/diag_support.c \
		rtas_errd/ela.c \
		rtas_errd/v6ela.c \
		rtas_errd/servicelog.c \
		rtas_errd/signal.c \
		rtas_errd/prrn.c \
		rtas_errd/hotplug.c \
		common/utils.c \
		$(rtas_errd_common_source) \
		$(rtas_errd_h_files)
rtas_errd_rtas_errd_LDADD = -lrtas -lrtasevent -lservicelog

rtas_scripts = rtas_errd/rc.powerfail
dist_man_MANS += rtas_errd/man/rtas_errd.8 rtas_errd/man/extract_platdump.8 \
								 rtas_errd/man/convert_dt_node_props.8

install-exec-hook-rtas-errd:
	install -d --mode=755 $(DESTDIR)/etc/
	install -D --mode=744 $(rtas_scripts) $(DESTDIR)/etc/

uninstall-hook-rtas-errd:
	rm -f $(DESTDIR)/etc/rc.powerfail

INSTALL_EXEC_HOOKS += install-exec-hook-rtas-errd

UNINSTALL_HOOKS += uninstall-hook-rtas-errd

EXTRA_DIST += $(rtas_scripts)
