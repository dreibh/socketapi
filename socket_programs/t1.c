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

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <ext_socket.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

int main()
{
   struct sockaddr_in remote_addr;
 struct sockaddr_in local_addr;
   int fd;
   char message[10];
   fd_set rset, wset;
   struct timeval tv;

   if ((fd = ext_socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
      perror("socket");

      /*
   memset((void *) &local_addr, 0, sizeof(remote_addr));
   local_addr.sin_family = AF_INET;
   if(ext_bind(fd, (struct sockaddr*)&local_addr, sizeof(local_addr) < 0))
      perror("bind");
*/
   printf("Using fd = %u.\n", fd);

   FD_ZERO(&rset);
   FD_ZERO(&wset);

   FD_SET(fd, &rset);
   FD_SET(fd, &wset);

   tv.tv_sec =5;
   tv.tv_usec=0;

    printf("select returned %d.\n", ext_select(FD_SETSIZE, &rset, &wset, NULL, &tv));
    if (FD_ISSET(fd, &rset))
       printf("readable.\n");
    if (FD_ISSET(fd, &wset))
        printf("writable.\n");

   memset((void *) message, 1, sizeof(message));

   memset((void *) &remote_addr, 0, sizeof(remote_addr));
   remote_addr.sin_family = AF_INET;
//   remote_addr.sin_len    = sizeof(struct sockaddr_in);
   /*
   remote_addr.sin_port   = htons(1234);
   */
   if (ext_sendto(fd, (const void *)message, sizeof(message), 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr)) != sizeof(message))
         perror("sendto");

      printf("Hello Thomas!\n");
      return 0;
}
