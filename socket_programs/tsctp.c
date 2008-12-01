/*
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2003 by Michael Tuexen
 * Copyright (C) 2008-2008 by Thomas Dreibholz
 *
 * $Id: idexample1toM.c 2005 2008-11-13 18:07:57Z dreibh $
 *
 * Realized in co-operation between
 * - Siemens AG
 * - University of Essen, Institute of Computer Networking Technology
 * - University of Applied Sciences, Muenster
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium fuer Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Foerderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de which should be
 * used for any discussion related to this implementation.
 *
 * Contact: discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *          tuexen@fh-muenster.de
 *
 * Purpose: SCTP Test
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifdef LINUX
#include <getopt.h>
#endif
#include <ext_socket.h>


#ifndef timersub
#define timersub(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)
#endif

struct info {
   int  fd;
   struct sockaddr_in   addr;
};



const char* Usage = "\
Usage: tsctp\n\
Options:\n\
        -v      verbose\n\
        -V      very verbose\n\
        -L      local address\n\
        -p      local port\n\
        -l      size of send/receive buffer\n\
        -n      number of messages sent (0 means infinite)/received\n\
        -D      turns Nagle off\n\
";

#define DEFAULT_LENGTH             1024
#define DEFAULT_NUMBER_OF_MESSAGES 1024
#define DEFAULT_PORT               5001
#define BUFFERSIZE                  (1<<16)
#define LINGERTIME                 1000

static void* handle_connection(void *arg)
{
   ssize_t n;
   unsigned long sum = 0;
   char *buf;
   pthread_t tid;
   int fdc, length;
   struct timeval start_time, now, diff_time;
   double seconds;
   unsigned long messages=0;
   struct info* info;

   buf = malloc(BUFFERSIZE);
   info = (struct info*)arg;
   fdc = info->fd;
   tid = pthread_self();
   pthread_detach(tid);
   gettimeofday(&start_time, NULL);

   n = ext_recv(fdc, (void*)buf, BUFFERSIZE, 0);
   length = n;
   while (n>0) {
      sum += n;
      messages++;
      n = ext_recv(fdc, (void*)buf, BUFFERSIZE, 0);
   }
   gettimeofday(&now, NULL);
   timersub(&now, &start_time, &diff_time);
   seconds = diff_time.tv_sec + (double)diff_time.tv_usec/1000000;
   fprintf(stdout, "%u, %ld, %f, %f, %f, %f\n",
          length, (long)messages, start_time.tv_sec + (double)start_time.tv_usec/1000000, now.tv_sec+(double)now.tv_usec/1000000, seconds, sum / seconds / 1024.0);
   fflush(stdout);
   ext_close(fdc);
   return NULL;

}


int main(int argc, char **argv)
{
   int fd, *cfdptr;
   char c, *buffer;
   socklen_t addr_len;
   struct sockaddr_in local_addr, remote_addr;
   struct timeval start_time, now, diff_time;
   int length, verbose, very_verbose, client;
   short local_port, remote_port, port;
   double seconds;
   double throughput;
   int one = 1;
   int nodelay = 0;
   unsigned long i, number_of_messages;

   pthread_t tid;
   const int on = 1;
   struct info* server_info;

   struct linger    linger;

   length             = DEFAULT_LENGTH;
   number_of_messages = DEFAULT_NUMBER_OF_MESSAGES;
   port               = DEFAULT_PORT;
   verbose            = 0;
   very_verbose       = 0;

   memset((void *) &local_addr,  0, sizeof(local_addr));
   memset((void *) &remote_addr, 0, sizeof(remote_addr));
   local_addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
   local_addr.sin_len         = sizeof(struct sockaddr_in);
#endif
   local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

   server_info = malloc(sizeof(struct info));

   while ((c = getopt(argc, argv, "p:l:L:n:vVD")) != -1) {
      switch(c) {
         case 'l':
            length = atoi(optarg);
          break;
         case 'L':
            local_addr.sin_addr.s_addr = inet_addr(optarg);
          break;
         case 'n':
            number_of_messages = atoi(optarg);
          break;
         case 'p':
            port = atoi(optarg);
          break;
         case 'v':
            verbose = 1;
          break;
         case 'V':
            verbose = 1;
            very_verbose = 1;
          break;
         case 'D':
            nodelay = 1;
          break;
         default:
            fputs(Usage, stderr);
            exit(1);
          break;
      }
   }

   if (optind == argc) {
      client      = 0;
      local_port  = port;
      remote_port = 0;
   } else {
      client       = 1;
      local_port  = 0;
      remote_port = port;
   }
   local_addr.sin_port = htons(local_port);

   if ((fd = ext_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
      perror("socket");

   if (!client)
      ext_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, (socklen_t)sizeof(on));

   if (ext_bind(fd, (struct sockaddr *) &local_addr, sizeof(local_addr)) != 0)
      perror("bind");

   if (!client) {
      if (ext_listen(fd, 1) < 0)
         perror("listen");
      while (1) {
         memset(&remote_addr, 0, sizeof(remote_addr));
         addr_len = sizeof(struct sockaddr_in);
         cfdptr = malloc(sizeof(int));
         if ((*cfdptr = ext_accept(fd, (struct sockaddr *)&remote_addr, &addr_len)) < 0)
            perror("accept");
         if (verbose)
            fprintf(stdout,"Connection accepted from %s:%d\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
         server_info->addr = remote_addr;
         server_info->fd = *cfdptr;
         pthread_create(&tid, NULL, &handle_connection, (void*) server_info);
      }
      close(fd);
   } else {
      remote_addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
      remote_addr.sin_len         = sizeof(struct sockaddr_in);
#endif

      remote_addr.sin_addr.s_addr = inet_addr(argv[optind]);
      remote_addr.sin_port        = htons(remote_port);

      if (ext_connect(fd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) < 0)
         perror("connect");

      if (nodelay == 1) {
         if(ext_setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, (char *)&one, sizeof(one)) < 0)
            perror("setsockopt: nodelay");
      }
      buffer = malloc(length);
      gettimeofday(&start_time, NULL);
      if (verbose) {
         printf("Start sending %ld messages...", (long)number_of_messages);
         fflush(stdout);
      }

      i = 0;
      while ((number_of_messages == 0) || (i < number_of_messages)) {
         i++;
         if (very_verbose)
            printf("Sending message number %lu.\n", i);
         ext_send(fd, buffer, length, 0);
      }
      if (verbose)
         printf("done.\n");
      linger.l_onoff = 1;
      linger.l_linger = LINGERTIME;
      if (ext_setsockopt(fd, SOL_SOCKET, SO_LINGER,(char*)&linger, sizeof(struct linger))<0)
         perror("setsockopt");
      close(fd);
      gettimeofday(&now, NULL);
      timersub(&now, &start_time, &diff_time);
      seconds = diff_time.tv_sec + (double)diff_time.tv_usec/1000000;
      fprintf(stdout, "%s of %ld messages of length %u took %f seconds.\n",
             "Sending", (long)number_of_messages, length, seconds);
      throughput = (double)(number_of_messages * length) / seconds / 1024.0;
      fprintf(stdout, "Throughput was %f KB/sec.\n", throughput);
   }
   return 0;
}
