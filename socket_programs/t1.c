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

   memset((void *) &local_addr, 0, sizeof(remote_addr));
   local_addr.sin_family = AF_INET;
   if(ext_bind(fd, (struct sockaddr*)&local_addr, sizeof(local_addr) < 0))
      perror("bind");

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
