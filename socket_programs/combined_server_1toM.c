/*
 *
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2003 by Michael Tuexen
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
 * Purpose: Example
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <ext_socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#if defined (SOLARIS)
#include <strings.h>
#endif
#if defined (LINUX)
#include <time.h>
#endif


#define ECHO_PORT    7
#define DISCARD_PORT 9
#define DAYTIME_PORT 13
#define CHARGEN_PORT 19

#define BUFFER_SIZE  3000

int main (int argc, const char * argv[]) {
  int chargen_fd, daytime_fd, discard_fd, echo_fd, backlog, remote_addr_size, length, idle_time, nfds;
  struct sockaddr_in local_addr, remote_addr;
  char buffer[BUFFER_SIZE];
  fd_set rset;
  time_t now;
  char *time_as_string;
  struct sctp_event_subscribe evnts;
  
  /* get sockets */
  if ((chargen_fd = ext_socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
    perror("socket call failed");
  if ((daytime_fd = ext_socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
    perror("socket call failed");
  if ((discard_fd = ext_socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
    perror("socket call failed");
  if ((echo_fd = ext_socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
    perror("socket call failed");

  /* bind the sockets to INADDRANY */
  bzero(&local_addr, sizeof(local_addr));
#if !defined (LINUX) && !defined (SOLARIS)
  local_addr.sin_len    = sizeof(local_addr);
#endif
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = INADDR_ANY;
  local_addr.sin_port   = htons(CHARGEN_PORT);
  if(ext_bind(chargen_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        perror("bind call failed");
  local_addr.sin_port   = htons(DAYTIME_PORT);
  if(ext_bind(daytime_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        perror("bind call failed");
  local_addr.sin_port   = htons(DISCARD_PORT);
  if(ext_bind(discard_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        perror("bind call failed");
  local_addr.sin_port   = htons(ECHO_PORT);
  if(ext_bind(echo_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
        perror("bind call failed");

  /* make the sockets 'active' */
  backlog = 1; 
  if (ext_listen(chargen_fd, backlog) < 0)
    perror("listen call failed");
  if (ext_listen(daytime_fd, backlog) < 0)
    perror("listen call failed");
  if (ext_listen(discard_fd, backlog) < 0)
    perror("listen call failed");
  if (ext_listen(echo_fd, backlog) < 0)
    perror("listen call failed");

  /* disable all event notifications */
  bzero(&evnts, sizeof(evnts));
  evnts.sctp_data_io_event = 1;
  if (ext_setsockopt(chargen_fd,IPPROTO_SCTP, SCTP_EVENTS, &evnts, sizeof(evnts)) != 0)
    perror("setsockopt");

  if (ext_setsockopt(daytime_fd,IPPROTO_SCTP, SCTP_EVENTS, &evnts, sizeof(evnts)) != 0)
    perror("setsockopt");

  if (ext_setsockopt(discard_fd,IPPROTO_SCTP, SCTP_EVENTS, &evnts, sizeof(evnts)) != 0)
    perror("setsockopt");

  if (ext_setsockopt(echo_fd,IPPROTO_SCTP, SCTP_EVENTS, &evnts, sizeof(evnts)) != 0)
    perror("setsockopt");

  /* set autoclose feature: close idle assocs after 5 seconds */
  idle_time = 5;
  if (ext_setsockopt(chargen_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &idle_time, sizeof(idle_time)) < 0)
	  perror("setsockopt SCTP_AUTOCLOSE call failed.");
  idle_time = 5;
  if (ext_setsockopt(daytime_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &idle_time, sizeof(idle_time)) < 0)
	  perror("setsockopt SCTP_AUTOCLOSE call failed.");
  idle_time = 5;
  if (ext_setsockopt(discard_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &idle_time, sizeof(idle_time)) < 0)
	  perror("setsockopt SCTP_AUTOCLOSE call failed.");
  idle_time = 5;
  if (ext_setsockopt(echo_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &idle_time, sizeof(idle_time)) < 0)
	  perror("setsockopt SCTP_AUTOCLOSE call failed.");

  nfds = FD_SETSIZE;
  FD_ZERO(&rset);
  /* Handle now incoming requests */
  while (1) {
    FD_SET(chargen_fd, &rset);
    FD_SET(daytime_fd, &rset);
    FD_SET(discard_fd, &rset);
    FD_SET(echo_fd,    &rset);

    ext_select(nfds, &rset, NULL, NULL, NULL);

    if (FD_ISSET(chargen_fd, &rset)) {
      remote_addr_size = sizeof(remote_addr);
      bzero(&remote_addr, sizeof(remote_addr));
      if ((length =ext_recvfrom(chargen_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote_addr, &remote_addr_size)) < 0)
        perror("recvfrom call failed");
      else {
        length = 1 + (random() % 512);
        memset(buffer, 'A', length);
        buffer[length-1] = '\n';
        if (ext_sendto(chargen_fd, buffer, length, 0, (struct sockaddr *) &remote_addr, remote_addr_size) < 0)
          perror("sendto call failed.\n");
      }
    }

    if (FD_ISSET(daytime_fd, &rset)) {
      remote_addr_size = sizeof(remote_addr);
      bzero(&remote_addr, sizeof(remote_addr));
      if ((length =ext_recvfrom(daytime_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote_addr, &remote_addr_size)) < 0)
        perror("recvfrom call failed");
      else {
        time(&now);
        time_as_string = ctime(&now);
        length = strlen(time_as_string);
        if (ext_sendto(daytime_fd, time_as_string, length, 0, (struct sockaddr *) &remote_addr, remote_addr_size) < 0)
          perror("sendto call failed.\n");
      }
    }

    if (FD_ISSET(discard_fd, &rset)) {
      remote_addr_size = sizeof(remote_addr);
      bzero(&remote_addr, sizeof(remote_addr));
      if ((length =ext_recvfrom(discard_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote_addr, &remote_addr_size)) < 0)
        perror("recvfrom call failed");
    }

    if (FD_ISSET(echo_fd, &rset)) {
      remote_addr_size = sizeof(remote_addr);
      bzero(&remote_addr, sizeof(remote_addr));
      if ((length =ext_recvfrom(echo_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote_addr, &remote_addr_size)) < 0)
        perror("recvfrom call failed");
      else {
        if (ext_sendto(echo_fd, buffer, length, 0, (struct sockaddr *) &remote_addr, remote_addr_size) < 0)
          perror("sendto call failed.\n");
      }
    }
  }
    
  return 0;
}
