EXTRA_DIST = Makefile.am TCP_combined_server.c UDP_combined_server.c idexampletcp.c idexampleudp.c inet_ntop.c newexampletcp.c

noinst_PROGRAMS = TCP_combined_server UDP_combined_server idexampletcp idexampleudp newexampletcp
#t1

UDP_combined_server_SOURCES =  UDP_combined_server.c
UDP_combined_server_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++

TCP_combined_server_SOURCES =  TCP_combined_server.c
TCP_combined_server_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++

idexampletcp_SOURCES = idexampletcp.c
idexampletcp_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++

idexampleudp_SOURCES = idexampleudp.c
idexampleudp_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++

newexampletcp_SOURCES = newexampletcp.c
newexampletcp_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++

#t1_SOURCES = t1.c
#t1_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++


INCLUDES = -I../socketapi
