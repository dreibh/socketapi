/*
 *  $Id: ext_socket.h,v 1.4 2003/06/01 22:45:45 dreibh Exp $
 *
 * SCTP implementation according to RFC 2960.
 * Copyright (C) 1999-2002 by Thomas Dreibholz
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
 * The authors alone are responsible for the contents.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de which should be
 * used for any discussion related to this implementation.
 *
 * Contact: discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Extended Socket API
 *
 */


#ifndef EXTSOCKET_H
#define EXTSOCKET_H


#ifndef HAVE_KERNEL_SCTP


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <inttypes.h>
#include <netinet/in.h>


#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif


#define SOCKETAPI_MAJOR_VERSION  1
#define SOCKETAPI_MINOR_VERSION  2100


#if defined(__APPLE__)
 typedef int socklen_t;
#endif



#define MSG_UNORDERED    (1 << 31)
#define MSG_UNBUNDLED    (1 << 30)
#define MSG_ADDR_OVER    (1 << 29)
#ifndef MSG_NOTIFICATION
#define MSG_NOTIFICATION (1 << 28)
#endif
#define MSG_ABORT        (1 << 27)
#ifndef MSG_EOF
#define MSG_EOF          (1 << 26)
#endif
#define MSG_SHUTDOWN     MSG_EOF



typedef unsigned int   sctp_assoc_t;
typedef unsigned short sctp_stream_t;


#define SCTP_UNDEFINED 0

#define SCTP_INIT 1
struct sctp_initmsg {
   uint16_t sinit_num_ostreams;
   uint16_t sinit_max_instreams;
   uint16_t sinit_max_attempts;
   uint16_t sinit_max_init_timeo;
};

#define SCTP_SNDRCV 2
struct sctp_sndrcvinfo
{
   uint16_t     sinfo_stream;
   uint16_t     sinfo_ssn;
   /* !!! 32 bits instead of 16 bits !!! */
   uint32_t     sinfo_flags;
   uint32_t     sinfo_ppid;
   uint32_t     sinfo_context;
   uint32_t     sinfo_timetolive;
   uint32_t     sinfo_tsn;
   sctp_assoc_t sinfo_assoc_id;
};
#define SCTP_INFINITE_TTL (uint32_t)0xffffffff


#define SCTP_ASSOC_CHANGE 1
struct sctp_assoc_change
{
   uint16_t     sac_type;
   uint16_t     sac_flags;
   uint32_t     sac_length;
   uint16_t     sac_state;
   uint16_t     sac_error;
   uint16_t     sac_outbound_streams;
   uint16_t     sac_inbound_streams;
   sctp_assoc_t sac_assoc_id;
};
#define SCTP_COMM_UP        11
#define SCTP_COMM_LOST      12
#define SCTP_RESTART        13
#define SCTP_SHUTDOWN_COMP  14
#define SCTP_CANT_STR_ASSOC 15

#define SCTP_PEER_ADDR_CHANGE 2
struct sctp_paddr_change
{
    uint16_t                spc_type;
    uint16_t                spc_flags;
    uint32_t                spc_length;
    struct sockaddr_storage spc_aaddr;
    int                     spc_state;
    int                     spc_error;
    sctp_assoc_t            spc_assoc_id;
};
#define SCTP_ADDR_AVAILABLE  21
#define SCTP_ADDR_UNREACHABL 22
#define SCTP_ADDR_REMOVED    23
#define SCTP_ADDR_ADDED      24
#define SCTP_ADDR_MADE_PRIM  25


#define SCTP_REMOTE_ERROR 3
struct sctp_remote_error
{
   uint16_t     sre_type;
   uint16_t     sre_flags;
   uint32_t     sre_length;
   uint16_t     sre_error;
   sctp_assoc_t sre_assoc_id;
   uint8_t      sre_data[0];
};

#define SCTP_SEND_FAILED 4
struct sctp_send_failed
{
   uint16_t               ssf_type;
   uint16_t               ssf_flags;
   uint32_t               ssf_length;
   uint32_t               ssf_error;
   struct sctp_sndrcvinfo ssf_info;
   sctp_assoc_t           ssf_assoc_id;
   uint8_t                ssf_data[0];
};
#define SCTP_DATA_UNSENT 41
#define SCTP_DATA_SENT   42


#define SCTP_SHUTDOWN_EVENT 5
struct sctp_shutdown_event
{
   uint16_t     sse_type;
   uint16_t     sse_flags;
   uint32_t     sse_length;
   sctp_assoc_t sse_assoc_id;
};


#define SCTP_ADAPTION_INDICATION 6
struct sctp_adaption_event
{
   uint16_t     sai_type;
   uint16_t     sai_flags;
   uint32_t     sai_length;
   uint32_t     sai_adaptation_bits;
   sctp_assoc_t sai_assoc_id;
};


#define SCTP_PARTIAL_DELIVERY_EVENT 7
#define SCTP_PARTIAL_DELIVERY_ABORTED 1
struct sctp_rcv_pdapi_event
{
   uint16_t     pdapi_type;
   uint16_t     pdapi_flags;
   uint32_t     pdapi_length;
   uint32_t     pdapi_indication;
   sctp_assoc_t pdapi_assoc_id;
};


/*
   For interal implementation usage only!
 */
#define SCTP_DATA_ARRIVE 8
#define SCTP_ARRIVE_UNORDERED (1 << 0)
struct sctp_data_arrive
{
   uint16_t      sda_type;
   uint16_t      sda_flags;
   uint32_t      sda_length;
   sctp_assoc_t  sda_assoc_id;
   sctp_stream_t sda_stream;
   uint32_t      sda_ppid;
   uint32_t      sda_bytes_arrived;
};


struct sctp_notification_header { /* ????? */
   uint16_t sn_type;
   uint16_t sn_flags;
   uint32_t sn_length;
};

struct sctp_tlv
{
   uint16_t type;
   uint16_t flags;
   uint32_t length;
};

union sctp_notification {
   struct sctp_notification_header h; /* ????? */
   struct sctp_tlv                 sn_type;
   struct sctp_assoc_change        sn_assoc_change;
   struct sctp_paddr_change        sn_paddr_change;
   struct sctp_remote_error        sn_remote_error;
   struct sctp_send_failed         sn_send_failed;
   struct sctp_shutdown_event      sn_shutdown_event;
   struct sctp_adaption_event      sn_adaption_event;
   struct sctp_rcv_pdapi_event     sn_rcv_pdapi_event;

   struct sctp_notification_header sn_notification_header;  /* ????? */
   struct sctp_data_arrive         sn_data_arrive;
};





struct sctp_rtoinfo
{
   sctp_assoc_t srto_assoc_id;
   uint32_t     srto_initial;
   uint32_t     srto_max;
   uint32_t     srto_min;
};


struct sctp_assocparams
{
   sctp_assoc_t sasoc_assoc_id;
   uint16_t     sasoc_asocmaxrxt;
   uint16_t     sasoc_number_peer_destinations;
   uint32_t     sasoc_peer_rwnd;
   int32_t      sasoc_local_rwnd;
   uint32_t     sasoc_cookie_life;
};


struct sctp_setprim
{
   sctp_assoc_t            ssp_assoc_id;
   struct sockaddr_storage ssp_addr;
};


struct sctp_setpeerprim
{
   sctp_assoc_t            sspp_assoc_id;
   struct sockaddr_storage sspp_addr;
};


struct sctp_setstrm_timeout
{
   sctp_assoc_t ssto_assoc_id;
   u_int32_t    ssto_timeout;
   u_int16_t    ssto_streamid_start;
   u_int16_t    ssto_streamid_end;
};


struct sctp_paddrparams {
   sctp_assoc_t            spp_assoc_id;
   struct sockaddr_storage spp_address;
   uint32_t                spp_hbinterval;
   uint16_t                spp_pathmaxrxt;
};


struct sctp_paddrinfo {
   sctp_assoc_t            spinfo_assoc_id;
   struct sockaddr_storage spinfo_address;
   int32_t                 spinfo_state;
   uint32_t                spinfo_cwnd;
   uint32_t                spinfo_srtt;
   uint32_t                spinfo_rto;
   uint32_t                spinfo_mtu;
};

#define SCTP_INACTIVE 0
#define SCTP_ACTIVE   1

struct sctp_status
{
   sctp_assoc_t          sstat_assoc_id;
   int32_t               sstat_state;
   uint32_t              sstat_rwnd;
   uint16_t              sstat_unackdata;
   uint16_t              sstat_penddata;
   uint16_t              sstat_instrms;
   uint16_t              sstat_outstrms;
   uint32_t              sstat_fragmentation_point;
   struct sctp_paddrinfo sstat_primary;
};


struct sctp_event_subscribe
{
   u_int8_t sctp_data_io_event;
   u_int8_t sctp_association_event;
   u_int8_t sctp_address_event;
   u_int8_t sctp_send_failure_event;
   u_int8_t sctp_peer_error_event;
   u_int8_t sctp_shutdown_event;
   u_int8_t sctp_partial_delivery_event;
   u_int8_t sctp_adaption_layer_event;
};


/*
   Already defined in sctp.h.
#define SCTP_CLOSED                     0
#define SCTP_COOKIE_WAIT                1
#define SCTP_COOKIE_ECHOED              2
#define SCTP_ESTABLISHED                3
#define SCTP_SHUTDOWN_PENDING           4
#define SCTP_SHUTDOWN_RECEIVED          5
#define SCTP_SHUTDOWN_SENT              6
#define SCTP_SHUTDOWNACK_SENT           7
*/
/*
#define SCTP_SHUTDOWN_ACK_SENT SCTP_SHUTDOWNACK_SENT
#define SCTP_BOUND             108
#define SCTP_LISTEN            109
*/



#define SCTP_INITMSG                1000
#define SCTP_AUTOCLOSE              1001

#define SCTP_RTOINFO                1010
#define SCTP_ASSOCINFO              1011
#define SCTP_SET_PRIMARY_ADDR       1012
#define SCTP_SET_PEER_PRIMARY_ADDR  1013
#define SCTP_SET_STREAM_TIMEOUTS    1014
#define SCTP_SET_PEER_ADDR_PARAMS   1015
#define SCTP_STATUS                 1016
#define SCTP_GET_PEER_ADDR_INFO     1017

#define SCTP_NODELAY                1030
#define SCTP_DISABLE_FRAGMENTS      1031
#define SCTP_SET_DEFAULT_SEND_PARAM 1032
#define SCTP_SET_EVENTS             1033

#define SCTP_GET_PEER_ADDR_PARAMS   1050


/* Interal usage only */
#define SCTP_RECVDATAIOEVNT        (1 << 31)
#define SCTP_RECVASSOCEVNT         (1 << 30)
#define SCTP_RECVPADDREVNT         (1 << 29)
#define SCTP_RECVPEERERR           (1 << 28)
#define SCTP_RECVSENDFAILEVNT      (1 << 27)
#define SCTP_RECVSHUTDOWNEVNT      (1 << 26)
#define SCTP_RECVADAPIONINDICATION (1 << 25)
#define SCTP_RECVPDEVNT            (1 << 24)



#ifdef __cplusplus
extern "C" {
#endif

int ext_socket(int domain, int type, int protocol);
int ext_open(const char* pathname, int flags, mode_t mode);
int ext_creat(const char* pathname, mode_t mode);
int ext_bind(int sockfd, struct sockaddr* my_addr, socklen_t addrlen);
int ext_connect(int sockfd, const struct sockaddr* serv_addr,
               socklen_t addrlen);
int ext_listen(int s, int backlog);
int ext_accept(int s,  struct  sockaddr * addr,  socklen_t* addrlen);
int ext_shutdown(int s, int how);
int ext_close(int fd);
int ext_getsockname(int sockfd, struct sockaddr* name, socklen_t* namelen);
int ext_getpeername(int sockfd, struct sockaddr* name, socklen_t* namelen);
int ext_fcntl(int fd, int cmd, ...);
int ext_ioctl(int d, int request, const void* argp);
int ext_getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
int ext_setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
int ext_recv(int s, void* buf, size_t len, int flags);
int ext_recvfrom(int  s,  void * buf,  size_t len, int flags,
                struct sockaddr* from, socklen_t* fromlen);
int ext_recvmsg(int s, struct msghdr* msg, int flags);
int ext_send(int s, const void* msg, size_t len, int flags);
int ext_sendto(int s, const void* msg, size_t len, int flags,
              const struct sockaddr* to, socklen_t tolen);
int ext_sendmsg(int s, const struct msghdr* msg, int flags);
ssize_t ext_read(int fd, void* buf, size_t count);
ssize_t ext_write(int fd, const void* buf, size_t count);
int ext_select(int             n,
               fd_set*         readfds,
               fd_set*         writefds,
               fd_set*         exceptfds,
               struct timeval* timeout);

#if defined(__APPLE__)
#define POLLIN  0x001
#define POLLPRI 0x002
#define POLLOUT 0x004
#define POLLERR 0x008
#define POLLHUP 0x010

struct pollfd {
   int       fd;
   short int events;
   short int revents;
};
#else
#include <sys/poll.h>
#endif

int ext_poll(struct pollfd* fdlist, long unsigned int count, int time);


/* For internal usage only! */
int ext_recvmsg2(int sockfd, struct msghdr* msg, int flags,
                 const int receiveNotifications);


#define SCTP_BINDX_ADD_ADDR 1
#define SCTP_BINDX_REM_ADDR 2

int ext_bindx(int                     sockfd,
             struct sockaddr_storage* addrs,
             int                      addrcnt,
             int                      flags);
#define sctp_bindx ext_bindx

int ext_connectx(int                      sockfd,
                 struct sockaddr_storage* addrs,
                 int                      addrcnt,
                 int                      flags);
#define sctp_connectx ext_connectx

int sctp_peeloff(int sockfd, sctp_assoc_t* id, struct sockaddr* addr, socklen_t* addrlen);

int sctp_getpaddrs(int sockfd, sctp_assoc_t id, struct sockaddr_storage** addrs);
void sctp_freepaddrs(struct sockaddr_storage* addrs);

int sctp_getladdrs(int sockfd, sctp_assoc_t id, struct sockaddr_storage** addrs);
void sctp_freeladdrs(struct sockaddr_storage* addrs);

int sctp_opt_info(int sd, sctp_assoc_t assocID, int opt, void* arg, socklen_t* size);


/**
  * Check, if SCTP support is available.
  *
  * @return true, if SCTP is available; false otherwise.
  */
int sctp_isavailable();

/**
  * Enable or disable OOTB handling.
  *
  * @param enable 0 to disable, <>0 to enable OOTB handling.
  * @return 0 for success, error code in case of error.
  */
int sctp_enableOOTBHandling(const unsigned int enable);

/**
  * Enable or disable CRC32 checksum.
  *
  * @param enable 0 to disable (use Adler32), <>0 to enable CRC32.
  * @return 0 for success, error code in case of error.
  */
int sctp_enableCRC32(const unsigned int enable);


#ifdef __cplusplus
}
#endif


#else

// #warning You have kernel SCTP! This file probably has to be updated to use it!

#ifdef __cplusplus
#define ext_socket(a,b,c) ::socket(a,b,c)
#define ext_bind(a,b,c) ::bind(a,b,c)
#define ext_bindx(a,b,c,d) ::bindx(a,b,c,d)
#define ext_bindx sctp_bindx
#define ext_connect(a,b,c) ::connect(a,b,c)
#define ext_listen(a,b) ::listen(a,b)
#define ext_accept(a,b,c) ::accept(a,b,c)
#define ext_shutdown(a,b) ::shutdown(a,b)
#define ext_close(a) ::close(a)
#define ext_getsockname(a,b,c) ::getsockname(a,b,c)
#define ext_getpeername(a,b,c) ::getpeername(a,b,c)
#define ext_fcntl(a,b,c) ::fcntl(a,b,c)
#define ext_ioctl(a,b,c) ::ioctl(a,b,c)
#define ext_getsockopt(a,b,c,d,e) ::getsockopt(a,b,c,d,e)
#define ext_setsockopt(a,b,c,d,e) ::setsockopt(a,b,c,d,e)
#define ext_recv(a,b,c,d) ::recv(a,b,c,d)
#define ext_recvfrom(a,b,c,d,e,f) ::recvfrom(a,b,c,d,e,f)
#define ext_recvmsg(a,b,c) ::recvmsg(a,b,c)
#define ext_send(a,b,c,d) ::send(a,b,c,d)
#define ext_sendto(a,b,c,d,e,f) ::sendto(a,b,c,d,e,f)
#define ext_sendmsg(a,b,c) ::sendmsg(a,b,c)
#define ext_read(a,b,c) ::read(a,b,c)
#define ext_write(a,b,c) ::write(a,b,c)
#define ext_select(a,b,c,d,e) ::select(a,b,c,d,e)
#define ext_poll(a,b,c,d,e) ::poll(a,b,c,d,e)
#else
#define ext_socket(a,b,c) socket(a,b,c)
#define ext_bind(a,b,c) bind(a,b,c)
#define ext_bindx sctp_bindx
#define ext_connect(a,b,c) connect(a,b,c)
#define ext_listen(a,b) listen(a,b)
#define ext_accept(a,b,c) accept(a,b,c)
#define ext_shutdown(a,b) shutdown(a,b)
#define ext_close(a) close(a)
#define ext_getsockname(a,b,c) getsockname(a,b,c)
#define ext_getpeername(a,b,c) getpeername(a,b,c)
#define ext_fcntl(a,b,c) fcntl(a,b,c)
#define ext_ioctl(a,b,c) ioctl(a,b,c)
#define ext_getsockopt(a,b,c,d,e) getsockopt(a,b,c,d,e)
#define ext_setsockopt(a,b,c,d,e) setsockopt(a,b,c,d,e)
#define ext_recv(a,b,c,d) recv(a,b,c,d)
#define ext_recvfrom(a,b,c,d,e,f) recvfrom(a,b,c,d,e,f)
#define ext_recvmsg(a,b,c) recvmsg(a,b,c)
#define ext_send(a,b,c,d) send(a,b,c,d)
#define ext_sendto(a,b,c,d,e,f) sendto(a,b,c,d,e,f)
#define ext_sendmsg(a,b,c) sendmsg(a,b,c)
#define ext_read(a,b,c) read(a,b,c)
#define ext_write(a,b,c) write(a,b,c)
#define ext_select(a,b,c,d,e) select(a,b,c,d,e)
#define ext_poll(a,b,c,d,e) poll(a,b,c,d,e)
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_constants.h>

#endif


#endif
