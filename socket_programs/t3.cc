#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ext_socket.h>
#include <string.h>

int main()
{
   struct sockaddr_in6 addr;
   int sd;

   unsigned short port = 0;


   for(size_t i = 0;i < 100000;i++) {
      sd = ext_socket(AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);
      if(sd < 0) { perror("SOCKET!"); exit(1); }

      memset((char*)&addr, 0, sizeof(addr));


      addr.sin6_family = AF_INET6;
      addr.sin6_port   = htons(port);
      if(ext_bind(sd, (sockaddr*)&addr, sizeof(addr)) < 0) {
         printf("port=%d\n", port);
         puts("Bind!");
         usleep(100000);
      }
      else {
          //port = 10000 + (random() % 10000);
          //printf("port=%d\n", port);
         //printf("#%u erfolgreich!\n", i);
         ext_close(sd);
      }

   }
   return(0);
}
