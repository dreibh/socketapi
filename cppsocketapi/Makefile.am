lib_LTLIBRARIES = libcppsctpsocket.la

libcppsctpsocketincludedir      = $(prefix)/include/cppsctpsocket
libcppsctpsocketinclude_HEADERS = breakdetector.h multitimerthread.h packetaddress.h range.h ringbuffer.h streamsrcdest.h tdsocket.h timedthread.h trafficclassvalues.h ../socketapi/condition.h ../socketapi/internetaddress.h ../socketapi/internetflow.h ../socketapi/portableaddress.h ../socketapi/randomizer.h ../socketapi/socketaddress.h ../socketapi/synchronizable.h ../socketapi/tdmessage.h ../socketapi/tdstrings.h ../socketapi/tdsystem.h ../socketapi/thread.h ../socketapi/tools.h ../socketapi/unixaddress.h ../socketapi/tdin6.h ../socketapi/condition.icc ../socketapi/internetaddress.icc ../socketapi/internetflow.icc multitimerthread.icc packetaddress.icc ../socketapi/portableaddress.icc ../socketapi/randomizer.icc range.icc ringbuffer.icc ../socketapi/socketaddress.icc ../socketapi/synchronizable.icc ../socketapi/tdmessage.icc tdsocket.icc ../socketapi/tdstrings.icc ../socketapi/thread.icc timedthread.icc ../socketapi/tools.icc trafficclassvalues.icc ../socketapi/unixaddress.icc multitimerthread.icc

libcppsctpsocket_la_SOURCES = packetaddress.cc tdsocket.cc \
                              breakdetector.cc \
                              timedthread.cc trafficclassvalues.cc \
                              streamsrcdest.cc multitimerthread.cc ringbuffer.cc

libcppsctpsocket_la_LDFLAGS = \
   -version-info $(SOCKETAPI_CURRENT):$(SOCKETAPI_REVISION):$(SOCKETAPI_AGE) \
   -release $(SOCKETAPI_RELEASE)


INCLUDES = -I../socketapi 


noinst_PROGRAMS =
#t1

#t1_SOURCES =  t1.cc
#t1_LDADD = -lcppsctpsocket -lsctpsocket -lsctp @glib_LIBS@ @thread_LIBS@
