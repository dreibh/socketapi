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


noinst_PROGRAMS = t1
#t1 t2 t3

t1_SOURCES = t1.c
t1_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@

#t2_SOURCES = t2.c
#t2_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@

#t3_SOURCES = t3.c
#t3_LDADD = libsctpsocket.la -lsctp -lstdc++ @glib_LIBS@ @thread_LIBS@
