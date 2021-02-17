spopen_h_files = common/utils.h

check_PROGRAMS += common/tests/spopen

common_tests_spopen_SOURCES = common/tests/spopen.c common/utils.c \
			      $(spopen_h_files)

EXTRA_DIST += common/run_tests \
	      common/tests/test-spopen-001

TESTS += common/run_tests
