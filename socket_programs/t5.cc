/*
 * A SCTP terminal using the socket API.
 * Copyright (C) 2005-2006 by Thomas Dreibholz, dreibh@exp-math.uni-essen.de
 *
 * $Id: terminal.c 1045 2006-03-27 13:47:32Z dreibh $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contact: rsplib-discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/socket.h>
#include <ext_socket.h>


/* ###### Find first occurrence of character in string ################### */
char* strindex(char* string, const char character)
{
   if(string != NULL) {
      while(*string != character) {
         if(*string == 0x00) {
            return(NULL);
         }
         string++;
      }
      return(string);
   }
   return(NULL);
}


/* ###### Find last occurrence of character in string #################### */
char* strrindex(char* string, const char character)
{
   const char* original = string;

   if(original != NULL) {
      string = (char*)&string[strlen(string)];
      while(*string != character) {
         if(string == original) {
            return(NULL);
         }
         string--;
      }
      return(string);
   }
   return(NULL);
}


/* ###### Convert string to address ###################################### */
int string2address(const char* string, struct sockaddr_storage* address)
{
   char                 host[128];
   char                 port[128];
   struct sockaddr_in*  ipv4address = (struct sockaddr_in*)address;
   struct sockaddr_in6* ipv6address = (struct sockaddr_in6*)address;
   char*                p1;
   int                  portNumber;

   struct addrinfo  hints;
   struct addrinfo* res;
   int isNumeric;
   int isIPv6;
   size_t hostLength;
   size_t i;

   if(strlen(string) > sizeof(host)) {
      fputs("ERROR: Address string is too long!\n", stderr);
      return(0);
   }
   strcpy((char*)&host,string);
   strcpy((char*)&port, "0");

   /* ====== Handle RFC2732-compliant addresses ========================== */
   if(string[0] == '[') {
      p1 = strindex(host,']');
      if(p1 != NULL) {
         if((p1[1] == ':') || (p1[1] == '!')) {
            strcpy((char*)&port, &p1[2]);
         }
         memmove((char*)&host, (char*)&host[1], (long)p1 - (long)host - 1);
         host[(long)p1 - (long)host - 1] = 0x00;
      }
   }

   /* ====== Handle standard address:port ================================ */
   else {
      p1 = strrindex(host,':');
      if(p1 == NULL) {
         p1 = strrindex(host,'!');
      }
      if(p1 != NULL) {
         p1[0] = 0x00;
         strcpy((char*)&port, &p1[1]);
      }
   }

   /* ====== Check port number =========================================== */
   if((sscanf(port, "%d", &portNumber) == 1) &&
      (portNumber < 0) &&
      (portNumber > 65535)) {
      return(0);
   }

   /* ====== Create address structure ==================================== */

   /* ====== Get information for host ==================================== */
   res        = NULL;
   isNumeric  = 1;
   isIPv6     = 0;
   hostLength = strlen(host);

   memset((char*)&hints,0,sizeof(hints));
   hints.ai_socktype = SOCK_DGRAM;
   hints.ai_family   = AF_UNSPEC;

   for(i = 0;i < hostLength;i++) {
      if(host[i] == ':') {
         isIPv6 = 1;
         break;
      }
   }
   if(!isIPv6) {
      for(i = 0;i < hostLength;i++) {
         if(!(isdigit(host[i]) || (host[i] == '.'))) {
            isNumeric = 0;
            break;
         }
       }
   }
   if(isNumeric) {
      hints.ai_flags = AI_NUMERICHOST;
   }

   if(getaddrinfo(host, NULL, &hints, &res) != 0) {
      return(0);
   }

   memset((char*)address,0,sizeof(struct sockaddr_storage));
   memcpy((char*)address,res->ai_addr,res->ai_addrlen);

   switch(ipv4address->sin_family) {
      case AF_INET:
         ipv4address->sin_port = htons(portNumber);
#ifdef HAVE_SIN_LEN
         ipv4address->sin_len  = sizeof(struct sockaddr_in);
#endif
       break;
      case AF_INET6:
         ipv6address->sin6_port = htons(portNumber);
#ifdef HAVE_SIN6_LEN
         ipv6address->sin6_len  = sizeof(struct sockaddr_in6);
#endif
       break;
      default:
         fprintf(stderr, "Unsupported address family #%d\n",
                 ((struct sockaddr*)address)->sa_family);
         return(0);
       break;
   }

   freeaddrinfo(res);
   return(1);
}


/* ###### Main program ################################################### */
int main(int argc, char** argv)
{
   union sctp_notification*    notification;
   struct sockaddr_storage     remoteAddress;
   struct sctp_initmsg         init;
   struct sctp_event_subscribe event;
   struct sctp_sndrcvinfo      sinfo;
   char                        buffer[65536 + 4];
   struct pollfd               ufds[2];
   ssize_t                     received;
   ssize_t                     sent;
   int                         result;
   int                         flags;
   int                         sd;
   int                         i;
   unsigned int                inStreams  = 128;
   unsigned int                outStreams = 128;
   unsigned int                color;
   size_t                      position;
   unsigned int                streamID;
   uint32_t                    ppid;

   if(argc < 2) {
      fprintf(stderr, "ERROR: Bad argument <%s>!\n", argv[0]);
      exit(1);
   }
   if(!string2address(argv[1], &remoteAddress)) {
      fprintf(stderr, "ERROR: Bad remote address <%s>!\n", argv[i]);
      exit(1);
   }

   puts("SCTP Terminal - Version 1.0");
   puts("===========================\n");
   printf("Remote = %s\n\n", argv[1]);


   /* ====== Create socket =============================================== */
   sd = ext_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
   if(sd < 0) {
      perror("Unable to create SCTP socket");
      exit(1);
   }

   /* ====== Set SCTP_INITMSG ============================================ */
   init.sinit_num_ostreams   = outStreams;
   init.sinit_max_instreams  = inStreams;
   init.sinit_max_attempts   = 0;
   init.sinit_max_init_timeo = 60;
   if(ext_setsockopt(sd, IPPROTO_SCTP, SCTP_INITMSG, &init, sizeof(init)) < 0) {
      perror("Unable to set SCTP_INITMSG parameters");
      exit(1);
   }

   /* ====== Set SCTP_EVENTS ============================================= */
   event.sctp_data_io_event          = 1;
   event.sctp_association_event      = 1;
   event.sctp_address_event          = 1;
   event.sctp_send_failure_event     = 1;
   event.sctp_peer_error_event       = 1;
   event.sctp_shutdown_event         = 1;
   event.sctp_partial_delivery_event = 1;
   event.sctp_adaptation_layer_event   = 1;
   if(ext_setsockopt(sd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) < 0) {
      perror("Unable to set SCTP_EVENTS parameters");
      exit(1);
   }

   /* ====== Connect to server =========================================== */
   printf("Connecting ... ");
   fflush(stdout);
   if(ext_connect(sd, (const struct sockaddr*)&remoteAddress, sizeof(remoteAddress)) != 0) {
      perror("\nUnable to connect to server");
      exit(1);
   }
   printf("okay!\n\n");

int f0;
socklen_t fl = sizeof(f0);
if(ext_getsockopt(sd, SOL_SOCKET,SO_REUSEADDR, &f0, &fl) < 0) {
   perror("GetSockOpt");
}
int f = 1;
if(ext_setsockopt(sd, SOL_SOCKET,SO_REUSEADDR, &f, sizeof(f)) < 0) {
   perror("SetSockOpt");
}

sockaddr_storage ss;
socklen_t l = sizeof(ss);
if(ext_getsockname(sd, (sockaddr*)&ss, &l) != 0) {
   perror("Scheiße");
exit(1);
}
else puts("OK!");

   /* ====== Terminal ==================================================== */
   streamID = 0;
   ppid     = 0;
   for(;;) {
      ufds[0].fd     = STDIN_FILENO;
      ufds[0].events = POLLIN;
      ufds[1].fd     = sd;
      ufds[1].events = POLLIN;
      result = ext_poll((struct pollfd*)&ufds, 2, -1);
      if(result > 0) {
         /* ====== Read line from standard input ========================= */
         if(ufds[0].revents & POLLIN) {
            received = ext_read(STDIN_FILENO, (char*)&buffer, sizeof(buffer));
            if(received > 0) {
               /* ====== Check for stream ID and PPID settings =========== */
               position = 0;
               if(sscanf(buffer, "#%d/$%x:%n", &streamID, &ppid, &position) < 2) {
                  if(sscanf(buffer, "#%d/%x:%n", &streamID, &ppid, &position) < 2) {
                     if(sscanf(buffer, "#%d:%n", &streamID, &position) < 1) {
                        position = 0;
                     }
                  }
               }
               memset((char*)&sinfo, 0, sizeof(sinfo));
               sinfo.sinfo_ppid   = htonl(ppid);
               sinfo.sinfo_stream = streamID;

               /* ====== Send data to server ============================= */
               sent = sctp_send(sd,
                               (const char*)&buffer[position], strlen((const char*)&buffer[position]),
                               &sinfo, 0);
               if(sent < 0) {
                  perror("sctp_send() failed");
               }
            }
            else {
               break;
            }
         }

         /* ====== Read answer from server =============================== */
         if(ufds[1].revents & POLLIN) {
            flags = 0;
            received = sctp_recvmsg(sd, (char*)&buffer, sizeof(buffer),
                                    NULL, NULL,
                                    &sinfo, &flags);
            if(received > 0) {
               if(flags & MSG_NOTIFICATION) {
                  notification = (union sctp_notification*)&buffer;

               }
               else {
                  sctp_assoc_value value;
                  value.assoc_id = sinfo.sinfo_assoc_id;
                  socklen_t l = sizeof(value);
                  if(ext_getsockopt(sd, IPPROTO_SCTP,SCTP_DELAYED_ACK_TIME, &value, &l) < 0) {
                     perror("getSockOpt AckTime1");
                  }
                  printf("AckTime0=%d\n", value.assoc_value);
                  value.assoc_id = 0;
                  l = sizeof(value);
                  if(ext_getsockopt(sd, IPPROTO_SCTP,SCTP_DELAYED_ACK_TIME, &value, &l) < 0) {
                     perror("getSockOpt AckTime1");
                  }
                  printf("AckTime1=%d\n", value.assoc_value);

                  value.assoc_id    = sinfo.sinfo_assoc_id;
                  value.assoc_value = 111;
                  if(ext_setsockopt(sd, IPPROTO_SCTP,SCTP_DELAYED_ACK_TIME, &value, sizeof(value)) < 0) {
                     perror("setSockOpt AckTime0");
                  }
                  printf("NewAckTime0=%d\n", value.assoc_value);
                  value.assoc_id    = 0;
                  value.assoc_value = 1;
                  if(ext_setsockopt(sd, IPPROTO_SCTP,SCTP_DELAYED_ACK_TIME, &value, sizeof(value)) < 0) {
                     perror("setSockOpt AckTime1");
                  }
                  printf("NewAckTime1=%d\n", value.assoc_value);


                  /* ====== Replace non-printable characters ============= */
                  for(i = 0;i < received;i++) {
                     if((unsigned char)buffer[i] < 30) {
                        buffer[i] = '.';
                     }
                  }
                  buffer[i] = 0x00;

                  /* ====== Print server's response ====================== */
                  color = ((sinfo.sinfo_stream + 1) % 15);
                  printf("\x1b[%umS%u,$%08x> %s\x1b[0m\n",
                         30 + (color % 8) + ((color / 8) ? 60 : 0),
                         sinfo.sinfo_stream,
                         ntohl(sinfo.sinfo_ppid),
                         buffer);
                  fflush(stdout);
               }
            }
            else {
               perror("sctp_recvmsg() failed");
            }
         }
      }
   }

   /* ====== Clean up ==================================================== */
   puts("\nTerminated!");
   ext_close(sd);
   return 0;
}
