AC_INIT(
  [Fam], 
  [0.0.1], 
  [mihai-radu-niculae.popescu11@alumni.imperial.ac.uk])

AC_CONFIG_SRCDIR([prog/fam.c])

# Before AM_INIT_AUTOMAKE to avoid errors
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC([gcc cl cc])

# config.h is generated in the build directory as configure variations
# may potentially generate different versions
AC_CONFIG_HEADERS([artifacts/config.h:config.h.in])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
