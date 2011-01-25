noinst_PROGRAMS = sctpmultiserver sctpterminal sctptftp sctpportscanner


sctpmultiserver_SOURCES =  sctpmultiserver.cc sctpinfoprinter.cc  sctpinfoprinter.h sctptftp.h ansicolor.h
sctpmultiserver_CXXFLAGS =  -I../socketapi -I../cppsocketapi
sctpmultiserver_LDADD = ../cppsocketapi/libcppsocketapi.la ../socketapi/libsctpsocket.la @sctplib_LIBS@ @glib_LIBS@ -lstdc++

sctpterminal_SOURCES =  sctpterminal.cc sctpinfoprinter.cc sctpinfoprinter.h ansicolor.h
sctpterminal_CXXFLAGS =  -I../socketapi -I../cppsocketapi
sctpterminal_LDADD = ../cppsocketapi/libcppsocketapi.la ../socketapi/libsctpsocket.la @sctplib_LIBS@ @glib_LIBS@ -lstdc++

sctptftp_SOURCES =  sctptftp.cc sctptftp.h
sctptftp_CXXFLAGS =  -I../socketapi -I../cppsocketapi
sctptftp_LDADD = ../cppsocketapi/libcppsocketapi.la ../socketapi/libsctpsocket.la @sctplib_LIBS@ @glib_LIBS@ -lstdc++

sctpportscanner_SOURCES =  sctpportscanner.cc ansicolor.h
sctpportscanner_CXXFLAGS =  -I../socketapi -I../cppsocketapi
sctpportscanner_LDADD = ../cppsocketapi/libcppsocketapi.la ../socketapi/libsctpsocket.la @sctplib_LIBS@ @glib_LIBS@ -lstdc++
