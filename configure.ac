## Copyright 2013 Mo McRoberts.
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.

m4_pattern_allow([^PKG_CONFIG(_PATH)?$])dnl

AC_INIT([containerware],[master])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

AC_CONFIG_HEADER([config.h])

AC_PROG_CC
AC_PROG_CC_C99

LT_INIT

AC_MSG_CHECKING([whether to enable compiler warnings])
if test x"$GCC" = x"yes" ; then
   AC_MSG_RESULT([yes, -W -Wall])
   AM_CPPFLAGS="$_CPPFLAGS -W -Wall"
else
   AC_MSG_RESULT([no])
fi
AC_SUBST([AM_CPPFLAGS])

AC_CHECK_HEADERS([unistd.h dirent.h uuid/uuid.h uuid.h sys/types.h sys/stat.h fcntl.h])

INCLUDES="$INCLUDES -I\${top_srcdir}/iniparser/src"
AC_SUBST([INCLUDES])

BT_REQUIRE_LIBDL
BT_REQUIRE_LIBUUID
BT_REQUIRE_LIBURI_INCLUDED
BT_REQUIRE_LIBJSONDATA_INCLUDED
BT_REQUIRE_PTHREAD

AC_ARG_WITH([fcgi],[AS_HELP_STRING(--with-fcgi=PREFIX)],[fcgi_prefix="$withval"],[fcgi_prefix="no"])
if test x"$fcgi_prefix" = x"yes" ; then
	fcgi_prefix=""
fi
if ! test x"$fcgi_prefix" = x"no" ; then
	if ! test x"$fcgi_prefix" = x"" ; then
		fcgi_CFLAGS="-I$fcgi_prefix/include"
		fcgi_LDFLAGS="-L$fcgi_prefix/lib"
	fi
	fcgi_LIBS="-lfcgi"
	WITH_FASTCGI=1
	AC_DEFINE_UNQUOTED([WITH_FASTCGI],[1],[Define if FastCGI is available])
fi
AC_SUBST([fcgi_LIBS])
AC_SUBST([fcgi_CFLAGS])
AC_SUBST([fcgi_LDFLAGS])
AC_SUBST([fcgi_SUBDIRS])
AM_CONDITIONAL([WITH_FASTCGI],[test x"$WITH_FASTCGI" = x"1"])

buildroot="`pwd`"
AC_SUBST([buildroot])

BT_DEFINE_PREFIX

dnl Flags for building modules inside the tree

LIBCONTAINER_CPPFLAGS="-I\${top_srcdir}/libcontainer -I\${top_builddir}/libcontainer $LIBURI_CPPFLAGS $LIBJSONDATA_CPPFLAGS"
LIBCONTAINER_LDFLAGS="$LIBURI_LDFLAGS $LIBJSONDATA_LDFLAGS"
LIBCONTAINER_LIBS="$LIBURI_LIBS $LIBJSONDATA_LIBS"
LIBCONTAINER_LOCAL_LIBS="\${top_builddir}/libcontainer/libcontainer.la $LIBURI_LOCAL_LIBS $LIBJSONDATA_LOCAL_LIBS"

AC_SUBST([LIBCONTAINER_CPPFLAGS])
AC_SUBST([LIBCONTAINER_LDFLAGS])
AC_SUBST([LIBCONTAINER_LIBS])
AC_SUBST([LIBCONTAINER_LOCAL_LIBS])

LIBSHIP_CPPFLAGS="-I\${top_srcdir}/libship -I\${top_builddir}/libship $LIBSHIP_CPPFLAGS"
LIBSHIP_LDFLAGS=""
LIBSHIP_LIBS=""
LIBSHIP_LOCAL_LIBS="\${top_builddir}/ship/libship.la"

AC_SUBST([LIBSHIP_CPPFLAGS])
AC_SUBST([LIBSHIP_LDFLAGS])
AC_SUBST([LIBSHIP_LIBS])
AC_SUBST([LIBSHIP_LOCAL_LIBS])

AC_CONFIG_FILES([
Makefile
libcontainer/Makefile
libship/Makefile
endpoints/Makefile
endpoints/dummy/Makefile
endpoints/fcgi/Makefile
containers/Makefile
containers/info/Makefile
m4/Makefile
])

AC_OUTPUT
