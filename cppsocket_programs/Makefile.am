noinst_PROGRAMS = sctpmultiserver sctpterminal sctptftp sctpportscanner
#t1 t2


sctpmultiserver_SOURCES =  sctpmultiserver.cc sctpinfoprinter.cc  sctpinfoprinter.h sctptftp.h ansicolor.h
sctpmultiserver_CXXFLAGS =  -I../socketapi -I../cppsocketapi
sctpmultiserver_LDADD = ../cppsocketapi/libcppsctpsocket.la

sctpterminal_SOURCES =  sctpterminal.cc sctpinfoprinter.cc sctpinfoprinter.h ansicolor.h
sctpterminal_CXXFLAGS =  -I../socketapi -I../cppsocketapi
sctpterminal_LDADD = ../cppsocketapi/libcppsctpsocket.la

sctptftp_SOURCES =  sctptftp.cc sctptftp.h
sctptftp_CXXFLAGS =  -I../socketapi -I../cppsocketapi
sctptftp_LDADD = ../cppsocketapi/libcppsctpsocket.la

sctpportscanner_SOURCES =  sctpportscanner.cc ansicolor.h
sctpportscanner_CXXFLAGS =  -I../socketapi -I../cppsocketapi
sctpportscanner_LDADD = ../cppsocketapi/libcppsctpsocket.la


#t1_SOURCES = t1.cc
#t1_CXXFLAGS =  -I../socketapi -I../cppsocketapi
#t1_LDFLAGS =  -L@sctplib_LIBS@ -L../socketapi -L../cppsocketapi
#t1_LDADD   = -lcppsctpsocket -lsctpsocket -lsctp @glib_LIBS@ @thread_LIBS@

#t2_SOURCES = t2.cc
#t2_CXXFLAGS =  -I../socketapi -I../cppsocketapi
#t2_LDFLAGS =  -L@sctplib_LIBS@ -L../socketapi -L../cppsocketapi
#t2_LDADD   = -lcppsctpsocket -lsctpsocket -lsctp @glib_LIBS@ @thread_LIBS@
