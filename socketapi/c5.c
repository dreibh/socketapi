/* 
 * A simple client which reads from stdin and sends it to the peer. Messages received
 * over the network are written to stdout. No connections are aacepted.
 * Usage:  client local_port remote_addr remote_port\n"
 */  
   
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ext_socket.h>

#define HEARTBEAT_TIMEOUT 1000
#define PATH_MAX_RTX      2

void
print_addresses(struct sockaddr *addrs, int num)
{
	char buf[INET6_ADDRSTRLEN];
    struct sockaddr *sa;
    int i, fam, incr = 0;
    struct sockaddr_in *lad4;
    struct sockaddr_in6 *lad6;

	sa = (struct sockaddr *)addrs;
	for(i=0; i<num; i++){
		fam = sa->sa_family;
		switch (fam) {
			case AF_INET:
				lad4 = (struct sockaddr_in *)sa;
				inet_ntop(fam, &lad4->sin_addr , buf, sizeof(buf));
				incr = sizeof(struct sockaddr_in);
				printf("IPv4 Address: %s:%d\n", buf, ntohs(lad4->sin_port));
				break;
			case AF_INET6:
				lad6 = (struct sockaddr_in6 *)sa;
				inet_ntop(fam, &lad6->sin6_addr, buf, sizeof(buf));
				incr = sizeof(struct sockaddr_in6);
				printf("IPv6 Address: %s:%d\n", buf, ntohs(lad6->sin6_port));
				break;
			default:
				printf("Unknown family %d\n", fam);
				break;
		}
		sa = (struct sockaddr *)((unsigned int)sa + incr);
	}
}

void
set_path_parameters(int fd, sctp_assoc_t assoc_id, struct sockaddr *addrs, int num, uint32_t hbinterval, uint16_t pathmaxrxt)
{
    struct sockaddr *sa;
    int i, incr = 0;
    struct sctp_paddrparams peer_params;
    size_t size;
    
    peer_params.spp_assoc_id   = assoc_id;
    peer_params.spp_hbinterval = hbinterval;
    peer_params.spp_pathmaxrxt = pathmaxrxt;
    size = sizeof(peer_params);

	sa = (struct sockaddr *)addrs;
	for(i=0; i<num; i++){
		switch (sa->sa_family) {
			case AF_INET:
				incr = sizeof(struct sockaddr_in);
				break;
			case AF_INET6:
                incr = sizeof(struct sockaddr_in6);
				break;
			default:
				printf("Unknown family %d\n", sa->sa_family);
				break;
		}
        memcpy((void *) &(peer_params.spp_address), (const void *) sa, incr);
        if (sctp_opt_info(fd, 0, SCTP_PEER_ADDR_PARAMS, (void *)&peer_params, (socklen_t *)&size) < 0)
            perror("sctp_opt_info");
		sa = (struct sockaddr *)((unsigned int)sa + incr);
	}
}

void
process_notification(int fd, char *notify_buf)
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
    struct sockaddr *sal, *sar;
    int num_loc, num_rem;

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
		
		if((sac->sac_state == SCTP_COMM_UP) ||
           (sac->sac_state == SCTP_RESTART)) {
 
        	num_rem = sctp_getpaddrs(fd, sac->sac_assoc_id, &sar);
            printf("There are %d remote addresses and they are:\n", num_rem);
            print_addresses(sar, num_rem);
            set_path_parameters(fd, 0, sar, num_rem, HEARTBEAT_TIMEOUT, PATH_MAX_RTX);
 	        sctp_freepaddrs(sar);

            num_loc = sctp_getladdrs(fd,sac->sac_assoc_id, &sal);
            printf("There are %d local addresses and they are:\n", num_loc);
            print_addresses(sal, num_loc);
            sctp_freeladdrs(sal);
        }
		break;
	case SCTP_PEER_ADDR_CHANGE:
		spc = &snp->sn_paddr_change;
		switch(spc->spc_state) {
		case SCTP_ADDR_REACHABLE:
			str = "ADDRESS REACHABLE";
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
	int fd, n, addr_len, len, msg_flags, i;
	size_t buffer_size;
	fd_set rset;
	char buffer[1000];
	struct sctp_event_subscribe evnts;
	struct sctp_sndrcvinfo sri;
	struct sockaddr_in local_addr, remote_addr;

	i = 0;
	if (argc < 4) {
		printf("Usage: client local_port remote_addr remote_port [autoclose]\n");
		exit(-1);
	}
	
	if ((fd = ext_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
		perror("socket");
		
    /* enable all events */
	bzero(&evnts, sizeof(evnts));
	evnts.sctp_data_io_event =          1;
	evnts.sctp_association_event =      1;
	evnts.sctp_address_event =          1;
	evnts.sctp_send_failure_event =     1;
	evnts.sctp_peer_error_event =       1;
	evnts.sctp_shutdown_event =         1;
	evnts.sctp_partial_delivery_event = 1;
	evnts.sctp_adaption_layer_event =   1;

    /* alternative code: 
    memset((void *) &evnts, 1, sizeof(struct sctp_event_subscribe));
    */

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
			   
    /* Initialize remote address using the command line options */
	remote_addr.sin_family      = AF_INET;
	remote_addr.sin_port        = htons(atoi(argv[3]));
#ifdef HAVE_SIN_LEN
	remote_addr.sin_len         = sizeof(struct sockaddr_in);
#endif
	remote_addr.sin_addr.s_addr = inet_addr(argv[2]);

    if (ext_connect(fd, (const struct sockaddr *) &remote_addr, sizeof(struct sockaddr_in)) < 0)
        perror("connect");
        
	FD_ZERO(&rset);
	
	while (1) {
		FD_SET(fd, &rset);
		FD_SET(0,  &rset);
		
		n = ext_select(fd + 1, &rset, NULL, NULL, NULL);
		
		if (n == 0) {
			printf("Timer was runnig off.\n");
		}
		
		if (FD_ISSET(0, &rset)) {
#ifdef DEBUG
			printf("Reading from stdin.\n");
#endif
			len = ext_read(0, (void *) buffer, sizeof(buffer));
			if (len == 0) 
				break;
			if (ext_sendto(fd, (const void *)buffer, len, 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr)) != len)
				perror("sendto");
			else
				printf("Message of length %d sent to %s:%u: %.*s", len, inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port), len, buffer);

		}
				
		if (FD_ISSET(fd, &rset)) {
#ifdef DEBUG
			printf("Reading from network.\n");
#endif
			addr_len = sizeof(struct sockaddr_in);
			buffer_size = sizeof(buffer);
			if ((len = sctp_recvmsg(fd, (void *) buffer, buffer_size , (struct sockaddr *)&remote_addr, &addr_len, &sri, &msg_flags)) < 0)
				perror("sctp_recvmsg");
			else {
			    printf("sctp_recvmsg returned %d.\n", len);
				if(msg_flags & MSG_NOTIFICATION) {
					process_notification(fd, buffer);
					continue;
				} else {
				    if (len > 0)
    					printf("Message of length %d received from %s:%u: %.*s", len, inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port), len, buffer);
    				else
    				    break;
				}
			}
		}
	}
	if (ext_close(fd) < 0)
		perror("close");
#ifndef HAVE_KERNEL_SCTP
	sleep(1);
#endif
	return 0;
}
