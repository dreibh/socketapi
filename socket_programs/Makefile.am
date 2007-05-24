EXTRA_DIST = Makefile.am combined_server_1to1.c combined_server_1toM.c idexample1to1.c idexample1toM.c newexample1to1.c

noinst_PROGRAMS = combined_server_1to1 combined_server_1toM idexample1to1 idexample1toM newexample1to1 terminal

combined_server_1to1_SOURCES =  combined_server_1to1.c
combined_server_1to1_CFLAGS =  -I../socketapi
combined_server_1to1_LDADD = ../socketapi/libsctpsocket.la @sctplib_LIBS@ -lstdc++

combined_server_1toM_SOURCES =  combined_server_1toM.c
combined_server_1toM_CFLAGS =  -I../socketapi
combined_server_1toM_LDADD = ../socketapi/libsctpsocket.la @sctplib_LIBS@ -lstdc++

idexample1to1_SOURCES = idexample1to1.c
idexample1to1_CFLAGS =  -I../socketapi
idexample1to1_LDADD = ../socketapi/libsctpsocket.la @sctplib_LIBS@ -lstdc++

idexample1toM_SOURCES = idexample1toM.c
idexample1toM_CFLAGS =  -I../socketapi
idexample1toM_LDADD = ../socketapi/libsctpsocket.la @sctplib_LIBS@ -lstdc++

newexample1to1_SOURCES = newexample1to1.c
newexample1to1_CFLAGS =  -I../socketapi
newexample1to1_LDADD = ../socketapi/libsctpsocket.la @sctplib_LIBS@ -lstdc++

terminal_SOURCES = terminal.c
terminal_CFLAGS =  -I../socketapi
terminal_LDADD = ../socketapi/libsctpsocket.la @sctplib_LIBS@ -lstdc++

#t5_SOURCES = t5.cc
#t5_CFLAGS =  -I../socketapi
#t5_LDADD = ../socketapi/libsctpsocket.la @sctplib_LIBS@ -lstdc++
