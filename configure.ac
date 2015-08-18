#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(sumocpp, 0.1.0)
AC_CONFIG_SRCDIR(src)
AC_CONFIG_HEADERS(src/config.h)

dnl enable automake-support
AM_INIT_AUTOMAKE([gnu dist-zip tar-ustar])

AC_LANG([C++])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

AC_DEFINE([SUMOLong], [long long int], [defines a long])

AC_DEFINE([POSITION_EPS], [(SUMOReal)0.1], [defines the epsilon to use on position comparison])
AC_DEFINE([NUMERICAL_EPS], [(SUMOReal)0.001], [defines the epsilon to use on general floating point comparison])
AC_DEFINE([OUTPUT_ACCURACY], [2], [defines the number of digits after the comma in output])
AC_DEFINE([GEO_OUTPUT_ACCURACY], [6], [defines the number of digits after the comma in geo-coordinates output])
AC_DEFINE([SUMOLong], [long long int], [defines the type for very long integers])


# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([floor gethostbyname gettimeofday memset pow select socket sqrt strerror])

AC_CONFIG_FILES([src/Makefile
                 src/utils/Makefile
                 src/utils/common/Makefile
                 src/utils/distribution/Makefile
                 src/utils/emissions/Makefile
                 src/utils/foxtools/Makefile
                 src/utils/geom/Makefile
                 src/utils/importio/Makefile
                 src/utils/iodevices/Makefile
                 src/utils/options/Makefile
                 src/utils/shapes/Makefile
                 src/utils/traci/Makefile
                 src/utils/vehicle/Makefile
                 src/utils/xml/Makefile])
AC_OUTPUT([Makefile])
