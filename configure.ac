#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([ppc64_diag_version], 2.7.4)

AC_PREREQ([2.69])
AC_INIT([ppc64-diag], ppc64_diag_version, [hegdevasant@linux.vnet.ibm.com, kumar.ankit008@in.ibm.com])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST(DATE, [`date`])

AM_INIT_AUTOMAKE([-Wall subdir-objects -Werror foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS([config/config.h])
LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_AWK
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_CHECK_FUNCS([memset strchr strcasecmp strdup strerror strrchr strstr strtol \
		dup2 localtime_r mkdir munmap regcomp select strpbrk])

# Checks for header files.
AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS([stdlib.h string.h unistd.h limits.h arpa/inet.h fcntl.h malloc.h \
		  stddef.h sys/ioctl.h syslog.h sys/time.h])

AC_CHECK_HEADER([curses.h],,[AC_MSG_ERROR([ncurses header files are required for building ppc64-diag])])
AC_CHECK_HEADER([libudev.h],,[AC_MSG_ERROR([libudev header files are required for building ppc64-diag])])


# check for librtas
AC_ARG_WITH([librtas],
    [AS_HELP_STRING([--without-librtas],
        [disable building utilities that require librtas])],
    [],
    [with_librtas=yes]
)

AS_IF([test "x$with_librtas" != "xno"],
	[AC_CHECK_HEADER([librtas.h],
        [
	    with_libtras=yes
            AC_DEFINE(WITH_LIBRTAS, [], ["with librtas"])
	],
        [AC_MSG_FAILURE(
                [librtas test failed (--without-librtas to disable)])]
    )]
)

AM_CONDITIONAL([WITH_LIBRTAS], [test "x$with_librtas" = "xyes"])

AC_COMPILE_IFELSE(
		  [AC_LANG_PROGRAM([int i;])],
		  [],
		  [AC_MSG_ERROR([C compiler is missing])],
		  )

# yacc/flex check
#  - In SLES: bison provides yacc interface
#  - In RHEL: byacc provides yacc interface
AC_CHECK_PROG(YACC, yacc, yacc)
AS_IF([test -z "$YACC"], [AC_MSG_ERROR([yacc command is required for ppc64-diag])])
AC_CHECK_PROG(FLEX, flex, flex)
AS_IF([test -z "$FLEX"], [AC_MSG_ERROR([flex command is required for ppc64-diag])])


# Checks for libraries.
AC_CHECK_LIB(udev, udev_device_get_devpath, [echo "udev library found: yes"], [
			echo "udev library is required for ppc64-diag"
			exit 1])

AC_DEFUN([LOCAL_CHECK_FLAGS],[
  AC_REQUIRE([AX_CHECK_LINK_FLAG])
  AC_REQUIRE([AX_APPEND_COMPILE_FLAGS])
  AC_LANG_PUSH([C])
  AX_APPEND_COMPILE_FLAGS([-Wall])
  AX_APPEND_COMPILE_FLAGS([-D_FORTIFY_SOURCE=2 -fstack-protector-all])
  AX_APPEND_COMPILE_FLAGS([-fwrapv -fPIE -Wstack-protector])
  AX_APPEND_COMPILE_FLAGS([--param=ssp-buffer-size=1])
  AX_CHECK_LINK_FLAG([-z relro -z now])
  AX_CHECK_LINK_FLAG([-pie])
  AC_LANG_POP
])
LOCAL_CHECK_FLAGS

AC_CONFIG_FILES([Makefile ppc64-diag.spec])
AC_OUTPUT
