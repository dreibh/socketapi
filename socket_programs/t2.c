#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <ext_socket.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


/* ###### Set blocking mode ############################################## */
int setNonBlocking(int fd)
{
   int flags = ext_fcntl(fd,F_GETFL,0);
   if(flags != -1) {
      flags |= O_NONBLOCK;
      if(ext_fcntl(fd,F_SETFL, flags) == 0) {
         return(1);
      }
   }
   return(0);
}


int main()
{
   struct sockaddr_in a[10];
   int fd;
   char message[32];
   int i;

   inet_aton("0.0.0.0", &a[0].sin_addr);
   inet_aton("10.1.9.151", &a[1].sin_addr);
   inet_aton("10.2.9.151", &a[2].sin_addr);
   inet_aton("10.1.9.151", &a[3].sin_addr);
   inet_aton("10.2.9.151", &a[4].sin_addr);
   a[0].sin_family = AF_INET;
   a[0].sin_port = htons(7466);
   a[1].sin_family = AF_INET;
   a[1].sin_port = htons(7);
   a[2].sin_family = AF_INET;
   a[2].sin_port = htons(7);
   a[3].sin_family = AF_INET;
   a[3].sin_port = htons(9);
   a[4].sin_family = AF_INET;
   a[4].sin_port = htons(9);

   if ((fd = ext_socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
      perror("socket");

   if(ext_bind(fd, (struct sockaddr*)&a[0], sizeof(a[0])) < 0)
      perror("bind");


   if(!setNonBlocking(fd)) {
      puts("Setting Non-Blocking-Mode failed!\n");
      exit(1);
   }



   puts("---- Send1...");
   strcpy((char*)&message, "Das ist ein Test #1\n");
/*
   puts("**** CONNECT");
   if(sctp_connectx(fd, (const struct sockaddr *)&a[1], 2) < 0) {
      perror("Connect1");
   }
*/
   puts("\n\n");
   for(i = 0;i < 10;i++) {
      puts("**** SENDTO1");
      if(ext_sendto(fd,
                  (const void *)message, strlen(message),
                  0,
                  (const struct sockaddr *)&a[1], sizeof(a[1])) < 0) {
         perror("Send1");
      }
   }
   puts("---- SUCCESS!");


   puts("---- Send2...");
   strcpy((char*)&message, "Das ist ein Test #2\n");

   if(sctp_connectx(fd, (const struct sockaddr *)&a[3], 2) < 0) {
      perror("Connect1");
   }

   for(i = 0;i < 10;i++) {
      puts("**** SENDTO2");
      if(ext_sendto(fd, (const void *)message, strlen(message),
                  0,
                  (const struct sockaddr *)&a[4], sizeof(a[4])) < 0) {
         perror("Send2");
      }
   }
   puts("---- SUCCESS!");


   strcpy((char*)&message, "Test für alle!\n");
   if(ext_sendto(fd, (const void *)message, strlen(message),
                 MSG_SEND_TO_ALL,
                 NULL, 0) < 0) {
      perror("Send3");
   }
   puts("---- SUCCESS!");


   usleep(2000000);
}
