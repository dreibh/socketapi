#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <unistd.h> 
#include <sys/uio.h> 
#include <ext_socket.h>
#include <string.h>

#define BUFLEN  100

static void
handle_event(void *buf)
{
  struct sctp_assoc_change *sac;
  struct sctp_send_failed *ssf;
  struct sctp_paddr_change *spc;
  struct sctp_remote_error *sre;
  union sctp_notification *snp;
  char addrbuf[INET6_ADDRSTRLEN];
  const char *ap;
  struct sockaddr_in *sin;
  struct sockaddr_in6 *sin6;

  snp = buf;

  switch (snp->sn_header.sn_type) {
    case SCTP_ASSOC_CHANGE:
      sac = &snp->sn_assoc_change;
      printf("^^^ assoc_change: state=%hu, error=%hu, instr=%hu, outstr=%hu\n",
              sac->sac_state, sac->sac_error, sac->sac_inbound_streams, sac->sac_outbound_streams);
      break;
    case SCTP_SEND_FAILED:
      ssf = &snp->sn_send_failed;
      printf("^^^ sendfailed: len=%hu err=%d\n", ssf->ssf_length, ssf->ssf_error);
      break;
    case SCTP_PEER_ADDR_CHANGE:
      spc = &snp->sn_paddr_change; 
      if (((struct sockaddr_in *)&spc->spc_aaddr)->sin_family == AF_INET) {
        sin = (struct sockaddr_in *)&spc->spc_aaddr; 
        ap = inet_ntop(AF_INET, &sin->sin_addr, addrbuf, INET6_ADDRSTRLEN);
      } else {
        sin6 = (struct sockaddr_in6 *)&spc->spc_aaddr; 
        ap = inet_ntop(AF_INET6, &sin6->sin6_addr, addrbuf, INET6_ADDRSTRLEN);
      }
      printf("^^^ intf_change: %s state=%d, error=%d\n", ap, spc->spc_state, spc->spc_error);
      break;
    case SCTP_REMOTE_ERROR:
      sre = &snp->sn_remote_error;
      printf("^^^ remote_error: err=%hu\n", ntohs(sre->sre_error));
      break;
    case SCTP_SHUTDOWN_EVENT:
      printf("^^^ shutdown event\n");
      break;
    default:
      printf("unknown type: %hu\n", snp->sn_header.sn_type);
      break;
  }
}

static void *
my_recvmsg(int fd, struct msghdr *msg, void *buf, size_t *buflen, ssize_t *nrp, size_t cmsglen)
{
  ssize_t nr = 0;
  struct iovec iov[1];

  *nrp = 0;
  iov->iov_base = buf;
  msg->msg_iov = iov;
  msg->msg_iovlen = 1;

  for (;;) {
    msg->msg_flags = 0; 
    msg->msg_iov->iov_len = *buflen;
    msg->msg_controllen = cmsglen;

    nr += ext_recvmsg(fd, msg, 0);
    if (nr <= 0) {
      /* EOF or error */
      *nrp = nr;
      return (NULL);
    }

    if ((msg->msg_flags & MSG_EOR) != 0) {
      *nrp = nr;
      return (buf);
    }

    /* Realloc the buffer? */
    if (*buflen == (size_t)nr) {
      buf = realloc(buf, *buflen * 2);
      if (buf == 0) {
        fprintf(stderr, "out of memory\n");
        exit(1);
      }
      *buflen *= 2;
    }

    /* Set the next read offset */
    iov->iov_base = (char *)buf + nr;
    iov->iov_len = *buflen - nr;
  }
}

static void
echo(int fd, int socketModeUDP)
{
  ssize_t nr;
  struct sctp_sndrcvinfo *sri;
  struct msghdr msg[1];
  struct cmsghdr *cmsg;
  char cbuf[sizeof (*cmsg) + sizeof (*sri)];
  char *buf;
  size_t buflen;
  struct iovec iov[1];
  size_t cmsglen = sizeof (*cmsg) + sizeof (*sri);

  /* Allocate the initial data buffer */
  buflen = BUFLEN;
  if (!(buf = malloc(BUFLEN))) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }

  /* Set up the msghdr structure for receiving */
  memset(msg, 0, sizeof (*msg));
  msg->msg_control = cbuf;
  msg->msg_controllen = cmsglen;
  msg->msg_flags = 0;
  cmsg = (struct cmsghdr *)cbuf;
  sri = (struct sctp_sndrcvinfo *)(cmsg + 1);

  /* Wait for something to echo */
  while ((buf = my_recvmsg(fd, msg, buf, &buflen, &nr, cmsglen))) {

    /* Intercept notifications here */
    if (msg->msg_flags & MSG_NOTIFICATION) {
      handle_event(buf);
      continue;
    }
  
    iov->iov_base = buf;
    iov->iov_len = nr;
    msg->msg_iov = iov;
    msg->msg_iovlen = 1;
  
    printf("got %u bytes on stream %hu:\n", nr, sri->sinfo_stream);
    write(0, buf, nr);
  
    /* Echo it back */
    msg->msg_flags = 0; 
    if (ext_sendmsg(fd, msg, 0) < 0) {
      perror("sendmsg");
      exit(1);
    }
  }

  if (nr < 0) {
    perror("recvmsg");
  }
	if(socketModeUDP == 0)
	  ext_close(fd);
}

int main()
{
  int fd;
  int idleTime = 2;
  struct sctp_event_subscribe events;
  struct sockaddr_in sin[1];

  if ((fd = ext_socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) == -1) {
    perror("socket");
    exit(1);
  }

  sin->sin_family = AF_INET;
  sin->sin_port = htons(7);
  sin->sin_addr.s_addr = INADDR_ANY;
  if (ext_bind(fd, (struct sockaddr *)sin, sizeof (*sin)) == -1) {
    perror("bind");
    exit(1);
  }

  /* Enable ancillary data and notifications */
  memset((char*)&events,1,sizeof(events));
  if (ext_setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events)) < 0) {
   perror("setsockopt SCTP_EVENTS");
   exit(1);
  }

  /* Set associations to auto-close in 2 seconds of inactivity */
  if (ext_setsockopt(fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &idleTime, 4) < 0) {
    perror("setsockopt SCTP_AUTOCLOSE");
    exit(1);
  }

  /* Allow new associations to be accepted */
  if (ext_listen(fd, 0) < 0) {
    perror("listen");
    exit(1);
  }

  /* Wait for new associations */
  while(1){
	  /* Echo back any and all data */
	  echo(fd,1);
  }
}
