noinst_PROGRAMS = sctpmultiserver sctpterminal sctptftp sctpportscanner
#t1 t2


sctpmultiserver_SOURCES =  sctpmultiserver.cc sctpinfoprinter.cc  sctpinfoprinter.h sctptftp.h ansicolor.h
sctpmultiserver_LDADD = @cppsocketapi_LIBS@ @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@

sctpterminal_SOURCES =  sctpterminal.cc sctpinfoprinter.cc sctpinfoprinter.h ansicolor.h
sctpterminal_LDADD = @cppsocketapi_LIBS@ @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@

sctptftp_SOURCES =  sctptftp.cc sctptftp.h
sctptftp_LDADD = @cppsocketapi_LIBS@ @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@

sctpportscanner_SOURCES =  sctpportscanner.cc ansicolor.h
sctpportscanner_LDADD = @cppsocketapi_LIBS@ @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@


#t1_SOURCES = t1.cc
#t1_LDADD   = @cppsocketapi_LIBS@ @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@

#t2_SOURCES = t2.cc
#t2_LDADD   = @cppsocketapi_LIBS@ @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@


INCLUDES = -I../socketapi -I../cppsocketapi
