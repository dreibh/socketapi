lib_LTLIBRARIES = libsctpsocket.la

libsctpsocketincludedir      = $(prefix)/include
libsctpsocketinclude_HEADERS = ext_socket.h
libsctpsocket_la_CXXFLAGS = @sctplib_CXXFLAGS@
libsctpsocket_la_SOURCES = thread.cc tdstrings.cc synchronizable.cc \
                           sctpsocketwrapper.cc sctpsocketmaster.cc sctpsocket.cc \
                           sctpnotificationqueue.cc sctpassociation.cc randomizer.cc \
                           internetaddress.cc condition.cc tools.cc socketaddress.cc staticglobals.cc \
                           internetflow.cc unixaddress.cc \
                           condition.h randomizer.h socketaddress.h thread.h \
                           sctpassociation.h synchronizable.h tools.h \
                           extsocketdescriptor.h sctpnotificationqueue.h tdin6.h unixaddress.h \
                           internetaddress.h sctpsocket.h tdmessage.h \
                           internetflow.h sctpsocketmaster.h tdstrings.h \
                           portableaddress.h sctpsocketwrapper.h tdsystem.h \
                           condition.icc randomizer.icc sctpsocketmaster.icc tdstrings.icc \
                           internetaddress.icc sctpassociation.icc socketaddress.icc thread.icc \
                           internetflow.icc sctpnotificationqueue.icc synchronizable.icc tools.icc \
                           portableaddress.icc sctpsocket.icc tdmessage.icc unixaddress.icc

libsctpsocket_la_LIBADD = @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++
libsctpsocket_la_LDFLAGS = \
   -version-info $(SOCKETAPI_CURRENT):$(SOCKETAPI_REVISION):$(SOCKETAPI_AGE) \
   -release $(SOCKETAPI_RELEASE)

noinst_PROGRAMS = t1 t2 c1 c2 c3 c4 c5 p1

t1_SOURCES = t1.c
t1_LDADD = libsctpsocket.la

t2_SOURCES = t2.c
t2_LDADD = libsctpsocket.la

c1_SOURCES = c1.c
c1_LDADD = libsctpsocket.la

c2_SOURCES = c2.c
c2_LDADD = libsctpsocket.la

c3_SOURCES = c3.c
c3_LDADD = libsctpsocket.la

c4_SOURCES = c4.c
c4_LDADD = libsctpsocket.la

c5_SOURCES = p1.c
c5_LDADD = libsctpsocket.la

p1_SOURCES = p1.c
p1_LDADD = libsctpsocket.la
