lib_LTLIBRARIES = libcppsctpsocket.la

libcppsctpsocketincludedir      = $(prefix)/include/cppsctpsocket
libcppsctpsocketinclude_HEADERS = breakdetector.h multitimerthread.h packetaddress.h range.h ringbuffer.h streamsrcdest.h tdsocket.h timedthread.h trafficclassvalues.h condition.h internetaddress.h internetflow.h portableaddress.h randomizer.h socketaddress.h synchronizable.h tdmessage.h tdstrings.h tdsystem.h thread.h tools.h unixaddress.h condition.icc internetaddress.icc internetflow.icc multitimerthread.icc packetaddress.icc portableaddress.icc randomizer.icc range.icc ringbuffer.icc socketaddress.icc synchronizable.icc tdmessage.icc tdsocket.icc tdstrings.icc thread.icc timedthread.icc tools.icc trafficclassvalues.icc unixaddress.icc multitimerthread.cc

libcppsctpsocket_la_SOURCES = packetaddress.cc tdsocket.cc \
                              breakdetector.cc \
                              timedthread.cc trafficclassvalues.cc \
                              streamsrcdest.cc multitimerthread.cc ringbuffer.cc

libcppsctpsocket_la_LDFLAGS = \
   -version-info $(SOCKETAPI_CURRENT):$(SOCKETAPI_REVISION):$(SOCKETAPI_AGE) \
   -release $(SOCKETAPI_RELEASE)


INCLUDES = -I../sctp -I../socketapi


noinst_PROGRAMS =
#t1

#t1_SOURCES =  t1.cc
#t1_LDADD = -lcppsctpsocket -lsctpsocket -lsctp @glib_LIBS@ @thread_LIBS@
