/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 2005-2009 by Thomas Dreibholz
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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: discussion@sctp.de
 *          dreibh@iem.uni-due.de
 *          tuexen@fh-muenster.de
 *
 * Purpose: Socket Implementation
 *
 */


#ifndef TDSOCKET_H
#define TDSOCKET_H


#include "tdsystem.h"
#include "internetaddress.h"
#include "internetflow.h"
#include "ext_socket.h"


#include <fcntl.h>



/**
  * UDP header size.
  */
const cardinal UDPHeaderSize = 8;

/**
  * IPv4 header size.
  */
const cardinal IPv4HeaderSize = 20;

/**
  * IPv6 header size.
  */
const cardinal IPv6HeaderSize = 40;


/**
  * This class manages a socket. IPv6 support is automatically available,
  * when supported by the system.
  *
  * @short   Socket
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  */
class Socket
{
   // ====== Definitions ====================================================
   public:
   enum SocketCommunicationDomain {
      UndefinedSocketCommunicationDomain = -1,
      IP                                 = 255,
      IPv4                               = AF_INET,  // Do not use IPv4/IPv6,
      IPv6                               = AF_INET6, // use IP instead!
      Unix                               = AF_UNIX
   };
   enum SocketType {
      UndefinedSocketType = -1,
      UDP                 = SOCK_DGRAM,
      Datagram            = SOCK_DGRAM,
      TCP                 = SOCK_STREAM,
      Stream              = SOCK_STREAM,
      Raw                 = SOCK_RAW,
      RDM                 = SOCK_RDM,
      SeqPacket           = SOCK_SEQPACKET
   };
   enum SocketProtocol {
      UndefinedSocketProtocol = -1,
      Default                 = 0,
      ICMPv4                  = IPPROTO_ICMP,
      ICMPv6                  = IPPROTO_ICMPV6,
      SCTP                    = IPPROTO_SCTP
   };
   enum GetSocketAddressFlags {
      GLAF_HideLoopback       = (1 << 0),
      GLAF_HideLinkLocal      = (1 << 1),
      GLAF_HideSiteLocal      = (1 << 2),
      GLAF_HideLocal          = GLAF_HideLoopback|GLAF_HideLinkLocal|GLAF_HideSiteLocal,
      GLAF_HideAnycast        = (1 << 3),
      GLAF_HideMulticast      = (1 << 4),
      GLAF_HideBroadcast      = (1 << 5),
      GLAF_HideReserved       = (1 << 6),
      GLAF_Default            = GLAF_HideLoopback|GLAF_HideLinkLocal|Socket::GLAF_HideSiteLocal|GLAF_HideBroadcast|GLAF_HideMulticast|GLAF_HideAnycast
   };


   // ====== Constructor/Destructor =========================================
   /**
     * Constructor.
     */
   Socket();

   /**
     * Constructor for a new socket. For automatic usage of IPv6 when available,
     * set communication domain to IP. Use IPv4/IPv6 only if a special
     * protocol version is necessary!
     * The creation success can be checked using ready() method.
     *
     * @param communicationDomain Communication domain (e.g. IP).
     * @param socketType Socket type (e.g. TCP, UDP).
     * @param socketProtocol Socket protocol (e.g. Default).
     *
     * @see ready
     */
   Socket(const integer communicationDomain,
          const integer socketType,
          const integer socketProtocol = Default);

   /**
     * Destructor.
     */
   ~Socket();


   // ====== Create/close socket ============================================
   /**
     * Close existing socket and create new socket. For automatic usage of
     * IPv6 when available, set communication domain to IP.
     * Use IPv4/IPv6 only if a special protocol version is necessary!
     *
     * @param communicationDomain Communication domain (e.g. IP).
     * @param socketType Socket type (e.g. TCP, UDP).
     * @param socketProtocol Socket protocol (e.g. Default).
     * @return true, if creation was sucessful; false otherwise.
     */
   bool create(const integer communicationDomain = IP,
               const integer socketType          = TCP,
               const integer socketProtocol      = Default);

   /**
     * Close socket.
     */
   void close();

   /**
     * Shutdown full-duplex connection partial or completely.
     * SHUT_RD - further receives will be disallowed.
     * SHUT_WR - further sends will be disallowed.
     * SHUT_RDWR - further sends and receives will be disallowed.
     *
     * @param shutdownLevel SHUT_RD, SHUT_WR, SHUT_RDWR.
     */
   void shutdown(const cardinal shutdownLevel);


   // ====== Socket properties ==============================================
   /**
     * Get socket's type.
     *
     * @return Socket type.
     */
   inline integer getType() const;

   /**
     * Get socket's communication domain.
     *
     * @return Socket communication domain.
     */
   inline integer getCommunicationDomain() const;

   /**
     * Get socket's protocol.
     *
     * @return Socket protocol.
     */
   inline integer getProtocol() const;


   // ====== Statistics functions ===========================================
   /**
     * Get number of bytes sent.
     *
     * @return Number of bytes sent.
     */
   inline card64 getBytesSent() const;

   /**
     * Get number of bytes received.
     *
     * @return Number of bytes received.
     */
   inline card64 getBytesReceived() const;

   /**
     * Reset number of bytes sent.
     */
   inline void resetBytesSent();

   /**
     * Reset number of bytes received.
     */
   inline void resetBytesReceived();


   // ====== Socket control functions =======================================
   /**
     * Check, if socket is ready.
     *
     * @return true, if socket is ready; false otherwise.
     */
   inline bool ready() const;

   /**
     * Bind socket to given address. If address is null address, then
     * INADDR_ANY and an automatically selected port will be used.
     *
     * @param address Socket address.
     * @return true on success; false otherwise.
     */
   bool bind(const SocketAddress& address = InternetAddress());

   /**
O     * Bind socket to one or more given addresses. If no addresses are given,
     * INADDR_ANY and an automatically selected port will be used.
     *
     * @param addressArray Array of socket addresses.
     * @param addresses Number of addresses.
     * @param flags Flags.
     * @return true on success; false otherwise.
     */
   bool bindx(const SocketAddress** addressArray = NULL,
              const cardinal        addresses    = 0,
              const integer         flags        = 0);

   /**
     * Set socket to listen mode with given backlog (queue length for sockets
     * waiting for acception).
     *
     * @param backlog Backlog.
     * @return true on success; false otherwise.
     */
   bool listen(const cardinal backlog = 5);

   /**
     * Accept a connection.
     *
     * @param address Reference to store SocketAddress object to with peer's address to (NULL to skip).
     * @return New socket.
     */
   Socket* accept(SocketAddress** address = NULL);

   /**
     * Connect socket to given address. A value for traffic class is supported
     * if the connection is an IPv6 connection; otherwise it is ignored.
     *
     * @param address Address.
     * @param trafficClass Traffic class of the connection (IPv6 only!)
     * @return true on success; false otherwise.
     */
   bool connect(const SocketAddress& address, const card8 trafficClass = 0);


   /**
     * Connect socket to destination given by list of addresses. A value for
     * traffic class is supported if the connection is an IPv6 connection;
     * otherwise it is ignored.
     *
     * @param address Address.
     * @param trafficClass Traffic class of the connection (IPv6 only!)
     * @return true on success; false otherwise.
     */
   bool connectx(const SocketAddress** addressArray,
                 const size_t          addresses);

   // ====== Error code =====================================================
   /**
     * Get last error code. It will be reset to 0 after copying.
     *
     * @return Last error code.
     */
   inline integer getLastError();


   // ====== Socket options =================================================
   /**
     * Get socket option (wrapper for getsockopt());
     *
     * @param level Level (e.g. SOL_SOCKET).
     * @param optionNumber Option (e.g. SO_REUSEADDR).
     * @param optionValue Memory to store option got from getsockopt().
     * @param optionLength Memory with size of option memory.
     * @return Result from getsockopt().
     */
   inline integer getSocketOption(const cardinal level,
                                  const cardinal optionNumber,
                                  void*          optionValue,
                                  socklen_t*     optionLength);

   /**
     * Get SO_LINGER option of socket.
     *
     * @return SO_LINGER value.
     */
   cardinal getSoLinger();

   /**
     * Get SO_REUSEADDR option of socket.
     *
     * @return SO_REUSEADDR value.
     */
   bool getSoReuseAddress();

   /**
     * Get SO_BROADCAST option of socket.
     *
     * @return SO_BROADCAST value.
     */
   bool getSoBroadcast();

   /**
     * Get TCP_NODELAY option of socket.
     *
     * @return TCP_NODELAY value.
     */
   bool getTCPNoDelay();

   /**
     * Check, if blocking mode is on.
     *
     * @return true, if blocking mode is on; false otherwise.
     */
   bool getBlockingMode();


   /**
     * Get socket option (wrapper for getsockopt());
     *
     * @param level Level (e.g. SOL_SOCKET).
     * @param optionNumber Option (e.g. SO_REUSEADDR).
     * @param optionValue Memory with option.
     * @param optionLength Length of option memory.
     * @return Result from setsockopt().
     */
   inline integer setSocketOption(const cardinal  level,
                                  const cardinal  optionNumber,
                                  const void*     optionValue,
                                  const socklen_t optionLength);

   /**
     * Set SO_LINGER option of socket.
     *
     * @param on true to set linger on; false otherwise.
     * @param linger SO_LINGER in seconds.
     * @return true for success; false otherwise.
     */
   bool setSoLinger(const bool on, const cardinal linger);

   /**
     * Set SO_REUSEADDR option of socket.
     *
     * @param on true to set SO_REUSEADDR on; false otherwise.
     * @return true for success; false otherwise.
     */
   bool setSoReuseAddress(const bool on);

   /**
     * Set SO_BROADCAST option of socket.
     *
     * @param on true to set SO_BROADCAST on; false otherwise.
     * @return true for success; false otherwise.
     */
   bool setSoBroadcast(const bool on);

   /**
     * Set TCP_NODELAY option of socket.
     *
     * @param on true to set TCP_NODELAY on; false otherwise.
     * @return true for success; false otherwise.
     */
   bool setTCPNoDelay(const bool on);

   /**
     * Set blocking mode.
     *
     * @param on True to set blocking mode, false to unset.
     * @param true for success; false otherwise.
     * @return true for success; false otherwise.
     */
   bool setBlockingMode(const bool on);


   // ====== Get flow label/traffic class ===================================
   /**
     * Get flow label of the connection.
     *
     * @return Flow label of the connection or 0, if there is no flow label.
     *
     * @see connect
     */
   inline card32 getSendFlowLabel() const;

   /**
     * Get traffic class of the connection.
     *
     * @return Traffic class of the connection or 0, if there is no traffic class.
     *
     * @see connect
     */
   inline card8 getSendTrafficClass() const;

   /**
     * Get last received flow label.
     *
     * @return Last received flow label or 0, if there is no flow label.
     */
   inline card32 getReceivedFlowLabel() const;

   /**
     * Get last received traffic class.
     *
     * @return Last received traffic class or 0, if there is no traffic class.
     */
   inline card8 getReceivedTrafficClass() const;


   // ====== I/O functions ==================================================
   /**
     * Wrapper for send().
     * send() will set the packet's traffic class, if trafficClass is not 0.
     * In this case, the packet will be sent by sendto() to the destination
     * address, the socket is connected to!
     *
     * @param buffer Buffer with data to send.
     * @param length Length of data to send.
     * @param flags Flags for sendto().
     * @param trafficClass Traffic class for packet.
     * @return Bytes sent or error code < 0.
     */
   ssize_t send(const void*    buffer,
                const size_t   length,
                const cardinal flags        = 0,
                const card8    trafficClass = 0x00);

   /**
     * Wrapper for sendto().
     * sendto() will set the packet's traffic class, if trafficClass is not 0.
     *
     * @param buffer Buffer with data to send.
     * @param length Length of data to send.
     * @param flags Flags for sendto().
     * @param receiver Address of receiver.
     * @param trafficClass Traffic class for packet.
     * @return Bytes sent or error code < 0.
     */
   ssize_t sendTo(const void*          buffer,
                  const size_t         length,
                  const cardinal       flags,
                  const SocketAddress& receiver,
                  const card8          trafficClass = 0x00);

   /**
     * Wrapper for sendmsg().
     *
     * @param msg Message.
     * @param flags Flags.
     * @param trafficClass Traffic class for packet.
     * @return Result of sendmsg() call.
     */
   ssize_t sendMsg(const struct msghdr* msg,
                   const cardinal       flags,
                   const card8          trafficClass = 0x00);

   /**
     * Wrapper for write().
     *
     * @param buffer Buffer with data to write
     * @param length Length of data to write
     * @return Bytes sent or error code < 0.
     */
   inline ssize_t write(const void*  buffer,
                        const size_t length);

   /**
     * Wrapper for recv().
     *
     * @param buffer Buffer to read data to.
     * @param length Maximum length of data to be received.
     * @param flags Flags for recv().
     * @return Bytes read or error code < 0.
     */
   inline ssize_t receive(void*          buffer,
                          const size_t   length,
                          const cardinal flags = 0);

   /**
     * Wrapper for recvfrom().
     *
     * @param buffer Buffer to receive data to.
     * @param length Maximum length of data to be received.
     * @param sender Address to store sender's address.
     * @param flags Flags for recvfrom().
     * @return Bytes received or error code < 0.
     */
   ssize_t receiveFrom(void*          buffer,
                       const size_t   length,
                       SocketAddress& sender,
                       const cardinal flags = 0);

   /**
     * Wrapper for recvmsg().
     *
     * @param msg Message.
     * @param flags Flags.
     * @param internalCall Internal usage only; set to false.
     * @return Result of recvmsg() call.
     */
   ssize_t receiveMsg(struct msghdr* msg,
                      const cardinal flags,
                      const bool     internalCall = false);

   /**
     * Wrapper for read().
     *
     * @param buffer Buffer to read data to.
     * @param length Maximum length of data to be received.
     * @return Bytes read or error code < 0.
     */
   inline ssize_t read(void*        buffer,
                       const size_t length);

   /**
     * Wrapper for fcntl().
     *
     * @param cmd Command.
     * @param arg Argument.
     * @return Result of fcntl() call.
     */
   inline integer fcntl(const integer cmd, long arg);

   /**
     * Wrapper for fcntl().
     *
     * @param cmd Command.
     * @param lock Lock.
     * @return Result of fcntl() call.
     */
   inline integer fcntl(const integer cmd, struct flock* lock);

   /**
     * Wrapper for ioctl().
     *
     * @param request Request.
     * @param argp Argument.
     * @return Result of ioctl() call.
     */
   inline integer ioctl(const integer request, const void* argp);


   // ====== Get address ====================================================
   /**
     * Get the socket's address. Note: A socket has to be bound to an address
     * and port or connected to a peer first to let the socket have an address!
     *
     * @param address Reference to SocketAddress to write address to.
     * @return true, if call was successful; false otherwise.
     *
     * @see bind
     * @see connect
     * @see getPeerAddress
     */
   bool getSocketAddress(SocketAddress& address) const;

   /**
     * Get the peer's address. Note: A socket has to be connected to a peer
     * first to get a peer address!
     *
     * @param address Reference to SocketAddress to write address to.
     * @return true, if call was successful; false otherwise.
     *
     * @see bind
     * @see connect
     * @see getSocketAddress
     */
   bool getPeerAddress(SocketAddress& address) const;


   // ====== Multicast functions ============================================
   /**
     * Add multicast membership.
     *
     * @param address Multicast address.
     * @param interface Interface name.
     * @return true for success; false otherwise.
     */
   inline bool addMulticastMembership(const SocketAddress& address,
                                      const char* interface = NULL);

   /**
     * Drop multicast membership.
     *
     * @param address Multicast address.
     * @param interface Interface name.
     * @return true for success; false otherwise.
     */
   inline bool dropMulticastMembership(const SocketAddress& address,
                                       const char* interface = NULL);

   /**
     * Get multicast loop mode.
     *
     * @return true if multicast loop is enabled, false otherwise.
     */
   bool getMulticastLoop();

   /**
     * Set multicast loop mode.
     *
     * @param on true to enable, false to disable.
     * @return true for success; false otherwise.
     */
   bool setMulticastLoop(const bool on);

   /**
     * Get multicast TTL.
     *
     * @return Multicast TTL.
     */
   card8 getMulticastTTL();

   /**
     * Set multicast TTL.
     *
     * @param ttl TTL.
     * @return true for success; false otherwise.
     */
   bool setMulticastTTL(const card8 ttl);


   // ====== IPv6 flow functions ============================================
   /**
     * Allocate a new flow to a given destination. A InternetFlow object is
     * returned, the value flow.getFlowLabel() will not be 0, if the allocFlow()
     * call was successful.
     *
     * @param address Address of the destination.
     * @param flowLabel Flowlabel; 0 for random value.
     * @param shareLevel Share level for flow label.
     * @return InternetFlow.
     */
   InternetFlow allocFlow(const InternetAddress& address,
                          const card32           flowLabel  = 0,
                          const card8            shareLevel = 2);

   /**
     * Free a flow.
     *
     * @param flow Flow to be freed.
     */
   void freeFlow(InternetFlow& flow);

   /**
     * Renew a flow label allocation with given expires and linger
     * (default 6) values. The expires value gives the seconds to go
     * until the flow label expires, the linger value gives the timeout in
     * seconds the freed flow label cannot be allocated again.
     *
     * @param flow Flow to be renewed.
     * @param expires Seconds until the flow label expires.
     * @param linger Linger (default 6).
     * @return true on success; false otherwise.
     */
   bool renewFlow(InternetFlow&  flow,
                  const cardinal expires,
                  const cardinal linger = 6);

   /**
     * Renew current flow's flow label allocation with given expires and linger
     * (default 6) values.
     *
     * @param expires Seconds until the flow label expires.
     * @param linger Linger (default 6).
     * @return true on success; false otherwise.
     */
   bool renewFlow(const cardinal expires,
                  const cardinal linger = 6);


   // ====== Bind pair of sockets ===========================================
   /**
     * Bind a pair of sockets to a given address and port number
     * x and x + 1. x will be a random number, if given port number is 0.
     *
     * @param socket1 First socket.
     * @param socket2 Second socket.
     * @param address Address (e.g ipv6-localhost:0) or NULL for Any address.
     *
     * @see bind
     */
   static bool bindSocketPair(Socket&              socket1,
                              Socket&              socket2,
                              const SocketAddress& address = InternetAddress());

   /**
     * Bind a pair of sockets to a given address and port number
     * x and x + 1. x will be a random number, if given port number is 0.
     * If no addresses are given, INADDR_ANY and an automatically
     * selected port will be used.
     *
     * @param socket1 First socket.
     * @param socket2 Second socket.
     * @param addressArray Array of socket addresses.
     * @param addresses Number of addresses.
     * @param flags bindx() flags.
     *
     * @see bindx
     */
   static bool bindxSocketPair(Socket&               socket1,
                               Socket&               socket2,
                               const SocketAddress** addressArray = NULL,
                               const cardinal        addresses    = 0,
                               const integer         flags        = 0);


   // ====== Get system's socket descriptor =================================
   /**
     * Get system's socket descriptor.
     * Warning: It is not recommended to manipulate the socket directly.
     * Use Socket's methods instead.
     *
     * @return Socket descriptor.
     */
   inline int getSystemSocketDescriptor() const;


   // ====== Obtaining Local addresses ======================================
   /**
     * Get list of all local addresses (IPv4 and IPv6 are currently supported).
     * The resulting list has to be deallocated using SocketAddress::deleteAddressList().
     *
     * @param addressList Reference to store address list to.
     * @param numberOfNets Reference to store number of addresses to.
     * @param flags Flags.
     * @return true for success; false otherwise.
     *
     * @see SocketAddress#deleteAddressList
     */
   static bool getLocalAddressList(SocketAddress**& addressList,
                                   cardinal&        numberOfNets,
                                   const cardinal   flags = GLAF_Default);


   // ====== Constants ======================================================
   /**
     * Minimum port number for bind()'s automatic port selection.
     *
     * @see bind
     */
   static const cardinal MinAutoSelectPort = 16384;

   /**
     * Maximum port number for bind()'s automatic port selection.
     *
     * @see bind
     */
   static const cardinal MaxAutoSelectPort = 61000;


   // ====== Private data ===================================================
   private:
   friend class TrafficShaper;
   void init();
   bool setTypeOfService(const card8 trafficClass);
   ssize_t recvFrom(int              fd,
                    void*            buf,
                    const size_t     len,
                    const integer    flags,
                    struct sockaddr* addr,
                    socklen_t*       addrlen);
   bool multicastMembership(const SocketAddress& address,
                            const char*          interface,
                            const bool           add);
   void packSocketAddressArray(const sockaddr_storage* addrArray,
                               const size_t            addrs,
                               sockaddr*               packedArray);


   card64    BytesSent;
   card64    BytesReceived;
   card32    SendFlow;
   card32    ReceivedFlow;
   cardinal  Backlog;
   integer   LastError;
   int       SocketDescriptor;
   sockaddr* Destination;
   integer   CommunicationDomain;
   integer   Type;
   integer   Protocol;
};


#include "tdsocket.icc"


#endif
