EXTRA_DIST = Makefile.am combined_server_1to1.c combined_server_1toN.c idexampletcp.c idexampleudp.c inet_ntop.c newexampletcp.c

noinst_PROGRAMS = combined_server_1to1 combined_server_1toN idexampletcp idexampleudp newexampletcp

combined_server_1to1_SOURCES =  combined_server_1to1.c
combined_server_1to1_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++

combined_server_1toN_SOURCES =  combined_server_1toN.c
combined_server_1toN_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++

idexampletcp_SOURCES = idexampletcp.c
idexampletcp_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++

idexampleudp_SOURCES = idexampleudp.c
idexampleudp_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++

newexampletcp_SOURCES = newexampletcp.c
newexampletcp_LDADD = @socketapi_LIBS@ @sctplib_LIBS@ @glib_LIBS@ @thread_LIBS@ -lstdc++


INCLUDES = -I../socketapi
