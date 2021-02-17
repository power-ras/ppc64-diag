opal_elog_parse_h_files = \
		opal_errd/opal-elog-parse/libopalevents.h \
		opal_errd/opal-elog-parse/opal-elog.h \
		opal_errd/opal-elog-parse/opal-ch-scn.h \
		opal_errd/opal-elog-parse/opal-datetime.h \
		opal_errd/opal-elog-parse/opal-dh-scn.h \
		opal_errd/opal-elog-parse/opal-ed-scn.h \
		opal_errd/opal-elog-parse/opal-eh-scn.h \
		opal_errd/opal-elog-parse/opal-ei-scn.h \
		opal_errd/opal-elog-parse/opal-ep-scn.h \
		opal_errd/opal-elog-parse/opal-event-data.h \
		opal_errd/opal-elog-parse/opal-event-log.h \
		opal_errd/opal-elog-parse/opal-hm-scn.h \
		opal_errd/opal-elog-parse/opal-ie-scn.h \
		opal_errd/opal-elog-parse/opal-lp-scn.h \
		opal_errd/opal-elog-parse/opal-lr-scn.h \
		opal_errd/opal-elog-parse/opal-mi-scn.h \
		opal_errd/opal-elog-parse/opal-mtms-scn.h \
		opal_errd/opal-elog-parse/opal-mtms-struct.h \
		opal_errd/opal-elog-parse/opal-priv-hdr-scn.h \
		opal_errd/opal-elog-parse/opal-src-fru-scn.h \
		opal_errd/opal-elog-parse/opal-src-scn.h \
		opal_errd/opal-elog-parse/opal-sw-scn.h \
		opal_errd/opal-elog-parse/opal-sw-v1-scn.h \
		opal_errd/opal-elog-parse/opal-sw-v2-scn.h \
		opal_errd/opal-elog-parse/opal-ud-scn.h \
		opal_errd/opal-elog-parse/opal-usr-scn.h \
		opal_errd/opal-elog-parse/opal-v6-hdr.h \
		opal_errd/opal-elog-parse/print-esel-header.h \
		opal_errd/opal-elog-parse/opal-esel-parse.h \
		opal_errd/opal-elog-parse/parse_helpers.h \
		opal_errd/opal-elog-parse/parse-opal-event.h \
		opal_errd/opal-elog-parse/print_helpers.h \
		opal_errd/opal-elog-parse/print-opal-event.h

sbin_PROGRAMS += opal_errd/extract_opal_dump \
		 opal_errd/opal_errd \
		 opal_errd/opal-elog-parse/opal-elog-parse

opal_errd_extract_opal_dump_SOURCES = opal_errd/extract_opal_dump.c

opal_errd_opal_errd_SOURCES = opal_errd/opal_errd.c \
			      opal_errd/opal-elog-parse/opal-event-data.c \
			      opal_errd/opal-elog-parse/opal-esel-parse.c

opal_errd_opal_errd_LDADD = -ludev

opal_errd_opal_elog_parse_opal_elog_parse_SOURCES = \
		opal_errd/opal-elog-parse/parse-opal-event.c \
		opal_errd/opal-elog-parse/opal-elog-parse.c \
		opal_errd/opal-elog-parse/opal-event-log.c \
		opal_errd/opal-elog-parse/print_helpers.c \
		opal_errd/opal-elog-parse/opal-event-data.c \
		opal_errd/opal-elog-parse/print-opal-event.c \
		opal_errd/opal-elog-parse/parse_helpers.c \
		opal_errd/opal-elog-parse/opal-datetime.c \
		opal_errd/opal-elog-parse/opal-v6-hdr.c \
		opal_errd/opal-elog-parse/opal-mtms-scn.c \
		opal_errd/opal-elog-parse/opal-mtms-struct.c \
		opal_errd/opal-elog-parse/opal-priv-hdr-scn.c \
		opal_errd/opal-elog-parse/opal-lr-scn.c \
		opal_errd/opal-elog-parse/opal-eh-scn.c \
		opal_errd/opal-elog-parse/opal-ep-scn.c \
		opal_errd/opal-elog-parse/opal-sw-scn.c \
		opal_errd/opal-elog-parse/opal-ud-scn.c \
		opal_errd/opal-elog-parse/opal-hm-scn.c \
		opal_errd/opal-elog-parse/opal-ch-scn.c \
		opal_errd/opal-elog-parse/opal-lp-scn.c \
		opal_errd/opal-elog-parse/opal-ie-scn.c \
		opal_errd/opal-elog-parse/opal-mi-scn.c \
		opal_errd/opal-elog-parse/opal-ei-scn.c \
		opal_errd/opal-elog-parse/opal-usr-scn.c \
		opal_errd/opal-elog-parse/opal-ed-scn.c \
		opal_errd/opal-elog-parse/opal-dh-scn.c \
		opal_errd/opal-elog-parse/opal-src-scn.c \
		opal_errd/opal-elog-parse/opal-src-fru-scn.c \
		opal_errd/opal-elog-parse/opal-esel-parse.c \
		opal_errd/opal-elog-parse/print-esel-header.c \
		$(opal_elog_parse_h_files)

dist_man_MANS += opal_errd/man/opal-elog-parse.8 opal_errd/man/opal_errd.8

EXTRA_DIST += opal_errd/run_tests \
	      opal_errd/test-extract_opal_dump \
	      opal_errd/test-opal-elog-parse \
	      opal_errd/test-opal_errd \
	      opal_errd/sysfs-test \
	      opal_errd/tests \
	      opal_errd/tests-results

TESTS += opal_errd/run_tests
