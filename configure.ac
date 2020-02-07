# ###########################################################################
# socketapi package configuration
# $Id$
# ###########################################################################

AC_INIT([socketapi], [2.2.18], [dreib@iem.uni-due.de])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST
AC_PREFIX_DEFAULT(/usr/local)
if test "x$prefix" = "xNONE"; then
  prefix=$ac_default_prefix
  ac_configure_args="$ac_configure_args --prefix $prefix"
fi
AM_CONFIG_HEADER(config.h)


# ###### Checks for programs ################################################
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AM_PROG_CC_C_O


# ###### Checks for typedefs, structures, and compiler characteristics ######
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)

AC_MSG_CHECKING(for type socklen_t)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>],
[socklen_t x; x = 0;],
[AC_DEFINE(HAVE_SOCKLEN_T, 1, "[Define it socketlen_t is avaiable.])
 AC_MSG_RESULT(yes)],
[AC_MSG_RESULT(no)])

AC_MSG_CHECKING([whether 'struct sockaddr' has sa_len])
AC_TRY_COMPILE([#ifdef HAVE_SYS_TYPES_H
                #include <sys/types.h>
                #endif
                #include <sys/socket.h>],
               [static struct sockaddr sa; int i = sizeof(sa.sa_len);],
               [AC_MSG_RESULT(yes)
                AC_DEFINE(HAVE_SA_LEN,1,"Define this if you have sa_len in sockaddr struct.")],
                AC_MSG_RESULT(no))

AC_MSG_CHECKING([whether 'struct sockaddr_in' has sin_len])
AC_TRY_COMPILE([#ifdef HAVE_SYS_TYPES_H
                #include <sys/types.h>
                #endif
                #include <netinet/in.h>],
               [static struct sockaddr_in in; int i = sizeof(in.sin_len);],
               [AC_MSG_RESULT(yes)
                AC_DEFINE(HAVE_SIN_LEN,1,"Define this if your IPv4 has sin_len in sockaddr_in struct.")],
                AC_MSG_RESULT(no))

AC_MSG_CHECKING([whether 'struct sockaddr_in6' has sin6_len])
AC_TRY_COMPILE([#ifdef HAVE_SYS_TYPES_H
                #include <sys/types.h>
                #endif
                #include <netinet/in.h>],
               [static struct sockaddr_in6 in; int i = sizeof(in.sin6_len);],
               [AC_MSG_RESULT(yes)
                AC_DEFINE(HAVE_SIN6_LEN,1,"Define this if your IPv6 has sin6_len in sockaddr_in6 struct.")],
                AC_MSG_RESULT(no))


# ###### Options ############################################################
AC_ARG_ENABLE([sctp-over-udp],
[  --enable-sctp-over-udp            enable SCTP over UDP mode ],
AC_DEFINE(SCTP_OVER_UDP, 1, "Define to 1 if you want SCTP over UDP"), )

AC_ARG_ENABLE([maintainer-mode],
[  --enable-maintainer-mode            enable maintainer mode ]
[default=no]],enable_maintainer_mode=$enableval,enable_maintainer_mode=no)

AC_ARG_ENABLE(ipv6,
[  --disable-ipv6          turn off IPv6 support ])


# ###### Check for glib #####################################################
AM_PATH_GLIB_2_0(2.0.0, [ glib_is_okay=yes ], [ glib_is_okay=no ])
if test "x$glib_is_okay" = "xno"; then
   AC_MSG_ERROR(GLib distribution not found.)
fi

CFLAGS="$CFLAGS $GLIB_CFLAGS"
CPPFLAGS="$CPPFLAGS $GLIB_CFLAGS"
glib_LIBS="$GLIB_LIBS"
AC_SUBST(glib_LIBS)


# ###### Operating system checks ############################################
case $host_os in
bsdi*)
    CFLAGS="$CFLAGS -DBSDI"
    thread_LIBS="-lpthread"
    CFLAGS="$CFLAGS `glib-config --cflags`"
    LDFLAGS="$LDFLAGS `glib-config --libs`"
    ;;
freebsd*)
    CFLAGS="$CFLAGS -DFreeBSD -D_PTHREADS -pthread -I/usr/local/include"
    CXXFLAGS="$CXXFLAGS -DFreeBSD -D_PTHREADS -pthread -I/usr/local/include"
    LDFLAGS="$LDFLAGS -L/usr/local/lib"
    thread_LIBS=""
    ;;
hpux*)
    CFLAGS="$CFLAGS -DSNAKE"
    test -z "$GCC" && TRY_CFLAGS="$TRY_CFLAGS -Wp,-H600000"
    thread_LIBS="-lpthread"
   ;;
linux*)
    CFLAGS="$CFLAGS -DLINUX"
    CXXFLAGS="$CXXFLAGS -DLINUX"
    LDFLAGS="$LDFLAGS"
    thread_LIBS="-lpthread"
    ;;
openbsd*)
    LIBS="$LIBS -lcompat"
    thread_LIBS="-lpthread"
    ;;
solaris*)
    CFLAGS="$CFLAGS -D_XPG4_2 -D__EXTENSIONS__ -DSOLARIS -D__sun -DSUN -DSUNOS_5"
    CXXFLAGS="$CXXFLAGS -D_XPG4_2 -D__EXTENSIONS__ -DSOLARIS -D__sun -DSUN -DSUNOS_5"
    thread_LIBS="-lpthread"
    LDFLAGS="$LDFLAGS -lsocket -lnsl -lrt -lresolv"
    ;;
sunos4*)
    CFLAGS="$CFLAGS -DSUN -DSUN4"
    thread_LIBS="-lpthread"
    ;;
darwin7*)
    CFLAGS="$CFLAGS -DDARWIN"
    CXXFLAGS="$CXXFLAGS -DDARWIN"
    thread_LIBS="-lpthread"
    ;;
darwin8*)
    CFLAGS="$CFLAGS -DDARWIN"
    CXXFLAGS="$CXXFLAGS -DDARWIN"
    thread_LIBS="-lpthread"
    ;;
esac
AC_SUBST(thread_LIBS)


# ###### Maintainer mode ####################################################
if test "$enable_maintainer_mode" = "yes"; then
   CFLAGS="$CFLAGS -Wall -Wunreachable-code -g3 -O0 -D_REENTRANT -D_THREAD_SAFE -DSCTP_SOCKET_API"
   CXXFLAGS="$CXXFLAGS -Wall -Wunreachable-code -g3 -O0 -D_REENTRANT -D_THREAD_SAFE -DSCTP_SOCKET_API"
else
   CFLAGS="$CFLAGS -Wall -Wunreachable-code -O3 -g -D_REENTRANT -D_THREAD_SAFE -DSCTP_SOCKET_API"
   CXXFLAGS="$CXXFLAGS -Wall -Wunreachable-code -O3 -g -D_REENTRANT -D_THREAD_SAFE -DSCTP_SOCKET_API"
fi


# ###### Check for sctplib ##################################################
AC_CHECK_HEADERS([sctp.h], [], [
   AC_MSG_ERROR([Unable to find sctplib header file])
])
AC_SEARCH_LIBS([sctp_initLibrary], [sctplib], [], [
   AC_MSG_ERROR([Unable to find sctplib!])
])


# ###### Version information ################################################
SOCKETAPI_CURRENT=2
SOCKETAPI_REVISION=2
SOCKETAPI_AGE=0

AC_SUBST(SOCKETAPI_CURRENT)
AC_SUBST(SOCKETAPI_REVISION)
AC_SUBST(SOCKETAPI_AGE)


# ###### Makefiles ##########################################################
AC_OUTPUT(Makefile socketapi/Makefile socketapi/docs/Makefile cppsocketapi/Makefile socket_programs/Makefile cppsocket_programs/Makefile scripts/Makefile )


# ###### Configuration results ##############################################
echo ""
echo "The socketapi package has been configured with the following options:"
echo ""
echo "   Build with Maintainer Mode : $enable_maintainer_mode"
echo "   Build with SCTP over UDP   : $enable_sctp_over_udp"
echo ""
echo "   glib_LIBS                  : $glib_LIBS"
echo "   Thread Libraries           : $thread_LIBS"
echo ""
echo "   CFLAGS                     : $CFLAGS"
echo "   CXXFLAGS                   : $CXXFLAGS"
echo "   LDFLAGS                    : $LDFLAGS"
