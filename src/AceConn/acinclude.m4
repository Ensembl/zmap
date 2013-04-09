#####
#
# Local macros for acedb and zmap packages.
#


##### 
#
# SYNOPSIS
#
#   AC_STRICT_COMPILE_CHECK_SIZEOF(TYPE, SIZE, [, HEADERS])
#
# DESCRIPTION
#
#   This macro verfies that a given type has a given size.
#   You can supply extra HEADERS to look into.
#   If a match is found, it will #define SIZEOF_`TYPE' to
#   that value. Otherwise it will emit a configure time error
#   indicating the size of the type could not be determined.
#
#   The trick is that C will not allow duplicate case labels. While
#   this is valid C code:
#
#        switch (0) case 0: case 1:;
#
#   The following is not:
#
#        switch (0) case 0: case 0:;
#
#   Thus, the AC_TRY_COMPILE will fail if the currently tried size does
#   not match.
#
#   Here is an example skeleton configure.in script, demonstrating the
#   macro's usage:
#
#        AC_PROG_CC
#        AC_CHECK_HEADERS(stddef.h unistd.h)
#        AC_TYPE_SIZE_T
#        AC_CHECK_TYPE(ssize_t, int)
#
#        headers='#ifdef HAVE_STDDEF_H
#        #include <stddef.h>
#        #endif
#        #ifdef HAVE_UNISTD_H
#        #include <unistd.h>
#        #endif
#        '
#
#        AC_STRICT_COMPILE_CHECK_SIZEOF(char, 1)
#        AC_STRICT_COMPILE_CHECK_SIZEOF(int, 4)
#        AC_STRICT_COMPILE_CHECK_SIZEOF(unsigned char *, 8)
#        AC_STRICT_COMPILE_CHECK_SIZEOF(size_t, 4, $headers)
#
# COPYLEFT
#
#   Copyright (c) 2007: Genome Research Ltd.
#
#   This program is free software: you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation, either version 3 of the
#   License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program. If not, see
#   <http://www.gnu.org/licenses/>.
#

AC_DEFUN([AC_STRICT_COMPILE_CHECK_SIZEOF],
[changequote(<<, >>)dnl
dnl The name to #define.
define(<<AC_TYPE_NAME>>, translit(sizeof_$1, [a-z *], [A-Z_P]))dnl
dnl The cache variable name.
define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_$1, [ *], [_p]))dnl
changequote([, ])dnl
AC_MSG_CHECKING(size of $1)
AC_CACHE_VAL(AC_CV_NAME,
[AC_TRY_COMPILE([#include "confdefs.h"
#include <sys/types.h>
$3
], [switch (0) case 0: case (sizeof ($1) == $2):;], AC_CV_NAME=$2)
  if test x$AC_CV_NAME != x ; then break; fi
])
if test x$AC_CV_NAME = x ; then
  AC_MSG_ERROR([package requires size of $1 to be 4 bytes but it is $2])
fi
AC_MSG_RESULT($AC_CV_NAME)
AC_DEFINE_UNQUOTED(AC_TYPE_NAME, $AC_CV_NAME, [Checks number of bytes in type $1])
undefine([AC_TYPE_NAME])dnl
undefine([AC_CV_NAME])dnl
])

