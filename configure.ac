#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([sumocpp], [0.1.0])
AM_INIT_AUTOMAKE([gnu dist-zip tar-ustar])
AC_CONFIG_SRCDIR([src/tracitestclient_main.cpp])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_PROG_RANLIB

AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

AC_DEFINE([SUMOReal], [double], [defines the precision of floats])
ac_enabled="$ac_enabled DoublePrecision"
AC_DEFINE([POSITION_EPS], [(SUMOReal)0.1], [defines the epsilon to use on position comparison])
AC_DEFINE([NUMERICAL_EPS], [(SUMOReal)0.001], [defines the epsilon to use on general floating point comparison])
AC_DEFINE([OUTPUT_ACCURACY], [2], [defines the number of digits after the comma in output])
AC_DEFINE([GEO_OUTPUT_ACCURACY], [6], [defines the number of digits after the comma in geo-coordinates output])
AC_DEFINE([SUMOLong], [long long int], [defines the type for very long integers])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([floor gethostbyname gettimeofday memset pow select socket sqrt strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile
		 src/foreign/Makefile
		 src/foreign/tcpip/Makefile
                 src/utils/Makefile
                 src/utils/common/Makefile
                 src/utils/traci/Makefile])
AC_OUTPUT
