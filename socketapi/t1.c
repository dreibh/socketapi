#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ext_socket.h>

int main(int argc, char **argv) 
{
   int fd, n, addr_len, len;
   fd_set rset;
   char buffer[1000];
   struct sctp_event_subscribe evnts;
   struct sockaddr_in local_addr, remote_addr;
   

   if (argc != 4) {
      printf("Usage: client local_port remote_addr remote_port\n");
      exit(-1);
   }
   
   if ((fd = ext_socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
      perror("socket");
      
   bzero(&evnts, sizeof(evnts));
   if (ext_setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &evnts, sizeof(evnts)) < 0)
      perror("setsockopt");
      
   bzero(&local_addr, sizeof(struct sockaddr_in));
   local_addr.sin_family      = AF_INET;
#ifdef HAVE_SIN_LEN
   local_addr.sin_len         = sizeof(struct sockaddr_in);
#endif
   local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   local_addr.sin_port        = htons(atoi(argv[1]));
   if (ext_bind(fd, (struct sockaddr *) &local_addr, sizeof(local_addr)) != 0)
      perror("bind");

   if (ext_listen(fd, 1) != 0)
      perror("listen");
      
   remote_addr.sin_family      = AF_INET;
   remote_addr.sin_port        = htons(atoi(argv[3]));
#ifdef HAVE_SIN_LEN
   remote_addr.sin_len         = sizeof(struct sockaddr_in);
#endif
   remote_addr.sin_addr.s_addr = inet_addr(argv[2]);

   FD_ZERO(&rset);
   
   while (1) {
      FD_SET(fd, &rset);
      FD_SET(0,  &rset);

      puts("\n\n\n\nSELECT...\n");
      n = ext_select(fd + 1, &rset, NULL, NULL, NULL);
      
      if (n == 0) {
         printf("Timer was runnig off.\n");
      }
      
      if (FD_ISSET(0, &rset)) {
         printf("Reading from stdin.\n");
         len = ext_read(0, (void *) buffer, sizeof(buffer));
         if (len == 0) 
            break;
         if (ext_sendto(fd, (const void *)buffer, len, 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr)) != len)
            perror("sendto");
      }
            
      if (FD_ISSET(fd, &rset)) {
         printf("Reading from network.\n");
         addr_len = sizeof(struct sockaddr_in);
         if ((len = ext_recvfrom(fd, (void *) buffer, sizeof(buffer),0,(struct sockaddr *) &remote_addr, &addr_len)) < 0)
            {perror("recvfrom");
/*           puts("STOP!------------------");
             exit(1);
*/
            }
         else
            printf("Message of length %d received from %s:%u: %.*s", len, inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port), len, buffer);

      }
   }
   if (ext_close(fd) < 0)
      perror("close");
   sleep(2);
   return 0;
}
