lib_LTLIBRARIES = libsctpsocket.la

libsctpsocketincludedir      = $(prefix)/include
libsctpsocketinclude_HEADERS = ext_socket.h

libsctpsocket_la_SOURCES = thread.cc tdstrings.cc synchronizable.cc \
                           sctpsocketwrapper.cc sctpsocketmaster.cc sctpsocket.cc \
                           sctpnotificationqueue.cc sctpassociation.cc randomizer.cc \
                           internetaddress.cc condition.cc tools.cc socketaddress.cc staticglobals.cc \
                           internetflow.cc unixaddress.cc


libsctpsocket_la_LDFLAGS = \
   -version-info $(SOCKETAPI_CURRENT):$(SOCKETAPI_REVISION):$(SOCKETAPI_AGE) \
   -release $(SOCKETAPI_RELEASE)


INCLUDES =


noinst_PROGRAMS = t1 t2 c1 c2 c3 c4
#t1 t2 t3

t1_SOURCES = t1.c
t1_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@

t2_SOURCES = t2.c
t2_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@

#t3_SOURCES = t3.c
#t3_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@

c1_SOURCES = c1.c
c1_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@

c2_SOURCES = c2.c
c2_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@

c3_SOURCES = c3.c
c3_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@

c4_SOURCES = c4.c
c4_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@
