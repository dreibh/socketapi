#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ext_socket.h>

void
sctp_print_notification(char *notify_buf)
{
   char buf[256];
   union sctp_notification *snp;
   struct sctp_assoc_change *sac;
   struct sctp_paddr_change *spc;
   struct sctp_remote_error *sre;
   struct sctp_send_failed *ssf;
   struct sctp_shutdown_event *sse;
   struct sctp_pdapi_event *pdapi;
   struct sockaddr_in *msin;
   struct sockaddr_in6 *msin6;
   const char *str;

   snp = (union sctp_notification *)notify_buf;
   switch(snp->sn_header.sn_type) {
   case SCTP_ASSOC_CHANGE:
      sac = &snp->sn_assoc_change;
      switch(sac->sac_state) {
      case SCTP_COMM_UP:
         str = "COMMUNICATION UP";
         break;
      case SCTP_COMM_LOST:
         str = "COMMUNICATION LOST";
         break;
      case SCTP_RESTART:
              str = "RESTART";
         break;
      case SCTP_SHUTDOWN_COMP:
         str = "SHUTDOWN COMPLETE";
         break;
      case SCTP_CANT_STR_ASSOC:
         str = "CANT START ASSOC";
         break;
      default:
         str = "UNKNOWN";
      } /* end switch(sac->sac_state) */
      printf("SCTP_ASSOC_CHANGE: %s, assoc=%xh\n", str, (uint32_t)sac->sac_assoc_id);
      break;
   case SCTP_PEER_ADDR_CHANGE:
      spc = &snp->sn_paddr_change;
      switch(spc->spc_state) {
      case SCTP_ADDR_AVAILABLE:
         str = "ADDRESS AVAILABLE";
         break;
      case SCTP_ADDR_UNREACHABLE:
         str = "ADDRESS UNAVAILABLE";
         break;
      case SCTP_ADDR_REMOVED:
         str = "ADDRESS REMOVED";
         break;
      case SCTP_ADDR_ADDED:
         str = "ADDRESS ADDED";
         break;
      case SCTP_ADDR_MADE_PRIM:
         str = "ADDRESS MADE PRIMARY";
         break;
      case SCTP_ADDR_CONFIRMED:
         str = "ADDRESS CONFIRMED";
         break;
      default:
         str = "UNKNOWN";
      } /* end switch */
      msin6 = (struct sockaddr_in6 *)&spc->spc_aaddr;
      if (msin6->sin6_family == AF_INET6) {
         inet_ntop(AF_INET6, (char*)&msin6->sin6_addr, buf, sizeof(buf));
      } else {
         msin = (struct sockaddr_in *)&spc->spc_aaddr;
         inet_ntop(AF_INET, (char*)&msin->sin_addr, buf, sizeof(buf));
      }
      printf("SCTP_PEER_ADDR_CHANGE: %s (%d), addr=%s, assoc=%xh\n", str, spc->spc_state, buf, (uint32_t)spc->spc_assoc_id);
      break;
   case SCTP_REMOTE_ERROR:
      sre = &snp->sn_remote_error;
      printf("SCTP_REMOTE_ERROR: assoc=%xh\n", (uint32_t)sre->sre_assoc_id);
      break;
   case SCTP_SEND_FAILED:
      ssf = &snp->sn_send_failed;
      printf("SCTP_SEND_FAILED: assoc=%xh\n", (uint32_t)ssf->ssf_assoc_id);
      break;
   case SCTP_ADAPTION_INDICATION:
     {
       struct sctp_adaption_event *ae;
       ae = &snp->sn_adaption_event;
       printf("SCTP_adaption_indication:0x%x\n", (unsigned int)ae->sai_adaption_ind);
     }
     break;
   case SCTP_PARTIAL_DELIVERY_EVENT:
     {
       pdapi = &snp->sn_pdapi_event;
       if(pdapi->pdapi_indication == SCTP_PARTIAL_DELIVERY_ABORTED){
          printf("SCTP_PD-API ABORTED\n");
       } else {
          printf("Unknown SCTP_PD-API EVENT %x\n", pdapi->pdapi_indication);
       }
     }
     break;
   case SCTP_SHUTDOWN_EVENT:
        sse = &snp->sn_shutdown_event;
      printf("SCTP_SHUTDOWN_EVENT: assoc=%xh\n", (uint32_t)sse->sse_assoc_id);
      break;
   default:
      printf("Unknown notification event type=%xh\n",  snp->sn_header.sn_type);
   }
}

int main(int argc, char **argv)
{
   int fd, n, addr_len, len, msg_flags, close_time;
   size_t buffer_size;
   fd_set rset;
   char buffer[1000];
   struct sctp_event_subscribe evnts;
   struct sctp_sndrcvinfo sri;
   struct sockaddr_in local_addr, remote_addr;
   

   if (argc < 4) {
      printf("Usage: peer local_port remote_addr remote_port [autoclose]\n");
      exit(-1);
   }
   
   if ((fd = ext_socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
      perror("socket");
      
   bzero(&evnts, sizeof(evnts));
   evnts.sctp_data_io_event = 1;
   evnts.sctp_association_event = 1;
   evnts.sctp_address_event = 1;
   evnts.sctp_send_failure_event = 1;
   evnts.sctp_peer_error_event = 1;
   evnts.sctp_shutdown_event = 1;
   evnts.sctp_partial_delivery_event = 1;
   evnts.sctp_adaption_layer_event = 1;

   if (ext_setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &evnts, sizeof(evnts)) < 0)
      perror("setsockopt");
      
   if (argc > 4) {
      close_time = atoi(argv[4]);
      if (ext_setsockopt(fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &close_time, sizeof(close_time)) < 0)
         perror("setsockopt");
   }
   
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
         buffer_size = sizeof(buffer);
         if ((len = sctp_recvmsg(fd, (void *) buffer, buffer_size , (struct sockaddr *)&remote_addr, &addr_len, &sri, &msg_flags)) < 0)
            perror("sctp_recvmsg");
         else {
            if(msg_flags & MSG_NOTIFICATION) {
               sctp_print_notification(buffer);
               continue;
            } else {
               printf("Message of length %d received from %s:%u: %.*s", len, inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port), len, buffer);
            }
         }
      }
   }
   if (ext_close(fd) < 0)
      perror("close");
   sleep(2);
   return 0;
}
