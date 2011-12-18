/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 2005-2012 by Thomas Dreibholz
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


#include "tdsystem.h"
#include "tdsocket.h"
#include "randomizer.h"
#include "tdmessage.h"


#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>


#if (SYSTEM == OS_Linux)
#define LINUX_PROC_IPV6_FILE "/proc/net/if_inet6"
#endif
#if (SYSTEM == OS_SOLARIS)
#include <sys/sockio.h>
#endif
#if (SYSTEM == OS_FreeBSD) || (SYSTEM == OS_Darwin)
#define USES_BSD_4_4_SOCKET
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#if (SYSTEM != OS_Darwin)
#include <netinet6/in6_var.h>
#endif
#include <machine/param.h>
#endif


#ifndef IPV6_JOIN_GROUP
#define IPV6_JOIN_GROUP IPV6_ADD_MEMBERSHIP
#endif
#ifndef IPV6_LEAVE_GROUP
#define IPV6_LEAVE_GROUP IPV6_DROP_MEMBERSHIP
#endif



// ###### Socket constructor ################################################
Socket::Socket()
{
   init();
}


// ###### Socket constructor ################################################
Socket::Socket(const integer communicationDomain,
               const integer socketType,
               const integer socketProtocol)
{
   init();
   create(communicationDomain,socketType,socketProtocol);
}



// ###### Socket destructor #################################################
Socket::~Socket()
{
   close();
}


// ###### Pack sockaddr_storage array to sockaddr_in/in6 blocks #############
void Socket::packSocketAddressArray(const sockaddr_storage* addrArray,
                                    const size_t            addrs,
                                    sockaddr*               packedArray)
{
   sockaddr* a = packedArray;
   for(size_t i = 0;i < addrs;i++) {
      switch(((sockaddr*)&addrArray[i])->sa_family) {
         case AF_INET:
            memcpy((void*)a, (void*)&addrArray[i], sizeof(struct sockaddr_in));
            a = (sockaddr*)((long)a + (long)sizeof(sockaddr_in));
          break;
         case AF_INET6:
            memcpy((void*)a, (void*)&addrArray[i], sizeof(struct sockaddr_in6));
            a = (sockaddr*)((long)a + (long)sizeof(sockaddr_in6));
          break;
         default:
            std::cerr << "ERROR: pack_sockaddr_storage() - Unknown address type #" << ((sockaddr*)&addrArray[i])->sa_family << "!" << std::endl;
            std::cerr << "IMPORTANT NOTE:" << std::endl
                      << "The standardizers have changed the socket API; the sockaddr_storage array has been replaced by a variable-sized sockaddr_in/in6 blocks. Do not blame us for this change, send your complaints to the standardizers at sctp-impl@external.cisco.com!" << std::endl;
            ::abort();
          break;
      }
   }
}


// ###### Initialize socket #################################################
void Socket::init()
{
   SocketDescriptor = -1;
   Family           = UndefinedSocketFamily;
   Type             = UndefinedSocketType;
   Protocol         = UndefinedSocketProtocol;
   Destination      = NULL;
   SendFlow         = 0;
   ReceivedFlow     = 0;
   LastError        = 0;
   Backlog          = 0;
}


// ###### Create socket #####################################################
bool Socket::create(const integer communicationDomain,
                    const integer socketType,
                    const integer socketProtocol)
{
   close();
   Family   = communicationDomain;
   Type     = socketType;
   Protocol = socketProtocol;
   if(Family == IP) {
      if(InternetAddress::UseIPv6) {
         Family = IPv6;
      }
      else
         Family = IPv4;
   }

   // ====== Create socket ==================================================
   SocketDescriptor = ext_socket(Family,socketType,socketProtocol);
   if(SocketDescriptor < 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: Socket::Socket() - Unable to create socket!" << std::endl;
#endif
      return(false);
   }

   // ====== Set options ====================================================
   // Send and receive IPv6 flow labels.
#if (SYSTEM == OS_Linux)
   int opt1 = 1;
   setsockopt(SocketDescriptor,SOL_IPV6,IPV6_FLOWINFO,&opt1,sizeof(opt1));
   setsockopt(SocketDescriptor,SOL_IPV6,IPV6_FLOWINFO_SEND,&opt1,sizeof(opt1));
#endif

   // Receive IPv4 TOS field.
#if (SYSTEM == OS_Linux)
   bool opt2 = true;
   setsockopt(SocketDescriptor,SOL_IP,IP_RECVTOS,&opt2,sizeof(opt2));
#endif
   return(true);
}


// ###### Close socket ######################################################
void Socket::close()
{
   if(SocketDescriptor != -1) {
      ext_close(SocketDescriptor);
      SocketDescriptor = -1;
   }
   if(Destination != NULL) {
      delete [] Destination;
      Destination = NULL;
   }
}


// ###### Shutdown connection ###############################################
void Socket::shutdown(const cardinal shutdownLevel)
{
   ext_shutdown(SocketDescriptor,shutdownLevel);
}


// ###### Bind socket to socket address #####################################
// ## address: null address               => implies any address, any port
// ##          InternetAddress(name)      => implies any port
// ##          InternetAddress(name,port) => use name and port
bool Socket::bind(const SocketAddress& address)
{
   // ====== Get address ====================================================
   char          socketAddressBuffer[SocketAddress::MaxSockLen];
   sockaddr_in6* socketAddress       = (sockaddr_in6*)&socketAddressBuffer;
   socklen_t     socketAddressLength =
      address.getSystemAddress((sockaddr*)socketAddress,SocketAddress::MaxSockLen,
                               Family);
   if(socketAddressLength == 0) {
      LastError = ENAMETOOLONG;
      return(false);
   }

   // ====== Bind socket ====================================================
   int result;
   if(((socketAddress->sin6_family == AF_INET6) ||
       (socketAddress->sin6_family == AF_INET)) &&
       (socketAddress->sin6_port == 0)) {
      Randomizer random;
      for(cardinal i = 0;i < 4 * (MaxAutoSelectPort - MinAutoSelectPort);i++) {
         const cardinal port = random.random32() % (MaxAutoSelectPort - MinAutoSelectPort);
         socketAddress->sin6_port = (card16)htons(port + MinAutoSelectPort);
         result = ext_bind(SocketDescriptor,(sockaddr*)socketAddress,
                          socketAddressLength);
         if(result == 0) {
            break;
         }
         else {
            LastError = errno;
            if(LastError == EPROTONOSUPPORT) {
               return(false);
            }
         }
      }
      if(result != 0) {
         for(cardinal i = Socket::MinAutoSelectPort;i < Socket::MaxAutoSelectPort;i += 2) {
            socketAddress->sin6_port = (card16)htons(i);
            result = ext_bind(SocketDescriptor,(sockaddr*)socketAddress,
                             socketAddressLength);
            if(result == 0) {
               break;
            }
            else {
               LastError = errno;
               if(LastError == EPROTONOSUPPORT) {
                  return(false);
               }
            }
         }
      }
   }
   else {
      result = ext_bind(SocketDescriptor,(sockaddr*)socketAddress,
                       socketAddressLength);
      if(result < 0) {
         LastError = errno;
      }
   }
   return(result == 0);
}


// ###### Bind socket to socket addresses ###################################
bool Socket::bindx(const SocketAddress** addressArray,
                   const cardinal        addresses,
                   const integer         flags)
{
   // ====== Bind to ANY address ============================================
   if(addresses == 0) {
      return(bind());
   }

   // ====== Get system socket addresses ====================================
   sockaddr_storage storage[addresses];
   for(cardinal i = 0;i < addresses;i++) {
      if(addressArray[i]->getSystemAddress((sockaddr*)&storage[i],
                                           sizeof(sockaddr_storage),
                                           AF_UNSPEC) == false) {
#ifndef DISABLE_WARNINGS
         std::cerr << "ERROR: Socket::bindx() - Unable to get system socket address for "
                   << *(addressArray[i]) << "!" << std::endl;
#endif
         return(false);
      }
   }

   // ====== Bind socket ====================================================
   sockaddr_in6* socketAddress = (sockaddr_in6*)&storage[0];
   int result;
   if(((socketAddress->sin6_family == AF_INET6) ||
       (socketAddress->sin6_family == AF_INET)) &&
       (socketAddress->sin6_port == 0)) {
      Randomizer random;
      for(cardinal i = 0;i < 4 * (MaxAutoSelectPort - MinAutoSelectPort);i++) {
         const cardinal port = random.random32() % (MaxAutoSelectPort - MinAutoSelectPort);
         socketAddress->sin6_port = (card16)htons(port + MinAutoSelectPort);
         for(cardinal n = 1;n < addresses;n++) {
            sockaddr_in6* address2 = (sockaddr_in6*)&storage[n];
            if((address2->sin6_family == AF_INET6) ||
               (address2->sin6_family == AF_INET)) {
               address2->sin6_port = socketAddress->sin6_port;
            }
         }
         sockaddr_storage packedSocketAddressArray[addresses];
         packSocketAddressArray(storage, addresses, (sockaddr*)&packedSocketAddressArray);
         result = sctp_bindx(SocketDescriptor,
                             (sockaddr*)&packedSocketAddressArray,
                             addresses,
                             (int)flags);
         if(result == 0) {
            break;
         }
         else {
            LastError = errno;
            if(LastError == EPROTONOSUPPORT) {
               return(false);
            }
         }
      }
      if(result != 0) {
         for(cardinal i = Socket::MinAutoSelectPort;i < Socket::MaxAutoSelectPort;i += 2) {
            socketAddress->sin6_port = (card16)htons(i);
            sockaddr_storage packedSocketAddressArray[addresses];
            packSocketAddressArray(storage, addresses, (sockaddr*)&packedSocketAddressArray);
            result = sctp_bindx(SocketDescriptor,
                                (sockaddr*)&packedSocketAddressArray,
                                addresses,
                                (int)flags);
            for(cardinal n = 1;n < addresses;n++) {
               sockaddr_in6* address2 = (sockaddr_in6*)&storage[n];
               if((address2->sin6_family == AF_INET6) ||
                  (address2->sin6_family == AF_INET)) {
                  address2->sin6_port = socketAddress->sin6_port;
               }
            }
            if(result == 0) {
               break;
            }
            else {
               LastError = errno;
               if(LastError == EPROTONOSUPPORT) {
                  return(false);
               }
            }
         }
      }
   }
   else {
      sockaddr_storage packedSocketAddressArray[addresses];
      packSocketAddressArray(storage, addresses, (sockaddr*)&packedSocketAddressArray);
      result = sctp_bindx(SocketDescriptor,
                          (sockaddr*)&packedSocketAddressArray,
                          addresses,
                          (int)flags);
      if(result < 0) {
         LastError = errno;
      }
   }

   return(result == 0);
}


// ###### Bind socket pair to address, port x and x + 1 #####################
bool Socket::bindSocketPair(Socket&              socket1,
                            Socket&              socket2,
                            const SocketAddress& address)
{
   SocketAddress* ba1    = address.duplicate();
   SocketAddress* ba2    = address.duplicate();
   bool           result = false;

   if((ba1 != NULL) && (ba2 != NULL)) {
      if(ba1->getPort() != 0) {
         ba2->setPort(ba1->getPort() + 1);
         if((socket1.bind(*ba1) &&
            socket2.bind(*ba2))) {
            result = true;
            goto end;
         }
         goto end;
      }

      Randomizer random;
      for(cardinal i = 0;i < 4 * (Socket::MaxAutoSelectPort - Socket::MinAutoSelectPort);i++) {
         const cardinal port = random.random32() % (Socket::MaxAutoSelectPort - Socket::MinAutoSelectPort - 1);
         ba1->setPort(port);
         ba2->setPort(port + 1);
         if(socket1.bind(*ba1) &&
            socket2.bind(*ba2)) {
            result = true;
            goto end;
         }
         else if(socket1.LastError == EPROTONOSUPPORT) {
            goto end;
         }
      }
      for(cardinal i = Socket::MinAutoSelectPort;i < Socket::MaxAutoSelectPort;i += 2) {
         ba1->setPort(i);
         ba2->setPort(i + 1);
         if(socket1.bind(*ba1) &&
            socket2.bind(*ba2)) {
            result = true;
            goto end;
         }
      }
   }

end:
   if(ba1 != NULL) {
      delete ba1;
   }
   if(ba2 != NULL) {
      delete ba2;
   }
   return(result);
}


// ###### Get address array for bindxSocketPair() ###########################
SocketAddress** getAddressArray(const SocketAddress** addressArray,
                                cardinal              addresses)
{
   SocketAddress** array;
   if(addresses > 0) {
      array = SocketAddress::newAddressList(addresses);
      if(array != NULL) {
         for(cardinal i = 0;i < addresses;i++) {
            array[i] = addressArray[i]->duplicate();
            if(array[0] == NULL) {
               SocketAddress::deleteAddressList(array);
               array = NULL;
               break;
            }
         }
      }
   }
   else {
      array = SocketAddress::newAddressList(1);
      if(array != NULL) {
         array[0] = new InternetAddress(0);
         if(array[0] == NULL) {
            SocketAddress::deleteAddressList(array);
            array = NULL;
         }
      }
   }
   return(array);
}


// ###### Set port number in address array for bindxSocketPair() ############
void setAddressArrayPort(SocketAddress** addressArray,
                         cardinal        addresses,
                         const card16    port)
{
   for(cardinal i = 0;i < addresses;i++) {
      addressArray[i]->setPort(port);
   }
}


// ###### Bind socket pair to address, port x and x + 1 #####################
bool Socket::bindxSocketPair(Socket&               socket1,
                             Socket&               socket2,
                             const SocketAddress** addressArray,
                             const cardinal        addresses,
                             const integer         flags)
{
   SocketAddress** ba1    = getAddressArray(addressArray,addresses);
   SocketAddress** ba2    = getAddressArray(addressArray,addresses);
   bool            result = false;

   if((ba1 != NULL) && (ba2 != NULL)) {
      if(ba1[0]->getPort() != 0) {
         setAddressArrayPort(ba2, addresses, ba1[0]->getPort() + 1);
         if(socket1.bindx((const SocketAddress**)ba1, addresses, flags) &&
            socket2.bindx((const SocketAddress**)ba2, addresses, flags)) {
            result = true;
            goto end;
         }
         goto end;
      }

      Randomizer random;
      for(cardinal i = 0;i < 4 * (Socket::MaxAutoSelectPort - Socket::MinAutoSelectPort);i++) {
         const cardinal port = random.random32() % (Socket::MaxAutoSelectPort - Socket::MinAutoSelectPort - 1);
         setAddressArrayPort(ba1, addresses, port);
         setAddressArrayPort(ba2, addresses, port + 1);
         if(socket1.bindx((const SocketAddress**)ba1, addresses, flags) &&
            socket2.bindx((const SocketAddress**)ba2, addresses, flags)) {
            result = true;
            goto end;
         }
         else if(socket1.LastError == EPROTONOSUPPORT) {
            goto end;
         }
      }
      for(cardinal i = Socket::MinAutoSelectPort;i < Socket::MaxAutoSelectPort;i += 2) {
         setAddressArrayPort(ba1, addresses, i);
         setAddressArrayPort(ba2, addresses, i + 1);
         if(socket1.bindx((const SocketAddress**)ba1, addresses, flags) &&
            socket2.bindx((const SocketAddress**)ba2, addresses, flags)) {
            result = true;
            goto end;
         }
      }
   }

end:
   if(ba1 != NULL) {
      SocketAddress::deleteAddressList(ba1);
   }
   if(ba2 != NULL) {
      SocketAddress::deleteAddressList(ba2);
   }
   return(result);
}


// ###### Set socket listing for connections ################################
bool Socket::listen(const cardinal backlog)
{
   int result = ext_listen(SocketDescriptor,backlog);
   if(result < 0)
      return(false);

   Backlog = backlog;
   return(true);
}


// ###### Accept connection #################################################
Socket* Socket::accept(SocketAddress** address)
{
   if(address != NULL) {
      *address = NULL;
   }
   char      socketAddressBuffer[SocketAddress::MaxSockLen];
   socklen_t socketAddressLength = SocketAddress::MaxSockLen;
   int result = ext_accept(SocketDescriptor,(sockaddr*)&socketAddressBuffer,
                          &socketAddressLength);

   if(result < 0) {
      return(NULL);
   }

   Socket* accepted = new Socket;
   if(accepted != NULL) {
      accepted->SocketDescriptor    = result;
      accepted->Family = Family;
      accepted->Type                = Type;
      accepted->Protocol            = Protocol;
      if(address != NULL) {
         *address = SocketAddress::createSocketAddress(
                       0,(sockaddr*)&socketAddressBuffer,socketAddressLength);
      }
      return(accepted);
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: Socket::accept() - Out of memory!" << std::endl;
#endif
      ext_close(result);
   }
   return(NULL);
}


// ###### Connect to socket address #########################################
bool Socket::connect(const SocketAddress& address, const card8 trafficClass)
{
   // ====== Get address ====================================================
   char          socketAddressBuffer[SocketAddress::MaxSockLen];
   sockaddr_in6* socketAddress       = (sockaddr_in6*)&socketAddressBuffer;
   socklen_t     socketAddressLength =
      address.getSystemAddress((sockaddr*)socketAddress,SocketAddress::MaxSockLen,
                               Family);
   if(socketAddressLength == 0) {
      return(false);
   }

   // ====== Set traffic class ==============================================
   SendFlow = 0;
   if(trafficClass != 0x00) {
      if((socketAddress->sin6_family == AF_INET6) || (socketAddress->sin6_family == AF_INET)) {
         SendFlow = (card32)trafficClass << 20;
         if(!setTypeOfService(trafficClass)) {
            SendFlow = 0;
         }
      }
   }

   // ====== Set flow label =================================================
   if(socketAddress->sin6_family == AF_INET6) {
      socketAddress->sin6_flowinfo = htonl(ntohl(socketAddress->sin6_flowinfo) | SendFlow);
      SendFlow = ntohl(socketAddress->sin6_flowinfo);
   }

   // ====== Copy destination address =======================================
   char* dest = new char[socketAddressLength];
   if(dest != NULL) {
      memcpy((void*)dest,(void*)socketAddress,socketAddressLength);
      Destination = (sockaddr*)dest;
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: Socket::connect() - Out of memory!" << std::endl;
#endif
      return(false);
   }

   // ====== Connect ========================================================
   int result = ext_connect(SocketDescriptor,(sockaddr*)socketAddress,socketAddressLength);
   if(result != 0) {
      LastError = errno;
      if(LastError != EINPROGRESS) {
         SendFlow = 0;
      }
      return(false);
   }
   return(true);
}


// ###### Connect to socket address list ####################################
bool Socket::connectx(const SocketAddress** addressArray,
                      const size_t          addresses)
{
   // ====== Get address ====================================================
   sockaddr_storage socketAddressArray[addresses];
   // socklen_t        socketAddressLength[addresses];

   for(cardinal i = 0;i < addresses;i++) {
      // socketAddressLength[i] =
         addressArray[i]->getSystemAddress((sockaddr*)&socketAddressArray[i],
                                           sizeof(socketAddressArray[addresses]),
                                           Family);
   }
   Destination = NULL;

   // ====== Connect ========================================================
   sockaddr_storage packedSocketAddressArray[addresses];
   packSocketAddressArray(socketAddressArray, addresses, (sockaddr*)&packedSocketAddressArray);
   int result = sctp_connectx(SocketDescriptor,
                              (sockaddr*)&packedSocketAddressArray,
                              addresses, NULL);
   if(result != 0) {
      LastError = errno;
      if(LastError != EINPROGRESS) {
         SendFlow = 0;
      }
      return(false);
   }
   return(true);
}


// ###### Receive message ###################################################
ssize_t Socket::receiveMsg(struct msghdr* msg,
                           const integer  flags,
                           const bool     internalCall)
{
#ifdef SOCKETAPI_MAJOR_VERSION
   const int cc = ext_recvmsg2(SocketDescriptor,msg,flags,(internalCall == true) ? 0 : 1);
#else
   const int cc = ext_recvmsg(SocketDescriptor,msg,flags);
#endif
   if(cc < 0) {
      LastError = errno;
      return(-LastError);
   }

   ReceivedFlow = 0;
   for(cmsghdr* c = CFirstHeader(msg);c;c = CNextHeader(msg,c)) {
      switch(c->cmsg_level) {
#if (SYSTEM == OS_Linux)
         case SOL_IPV6:
            if(((struct sockaddr_in6*)msg->msg_name)->sin6_family == AF_INET6) {
               switch(c->cmsg_type) {
                   case IPV6_FLOWINFO:
                      ((struct sockaddr_in6*)msg->msg_name)->sin6_flowinfo = *(__u32*)CData(c);
                      ReceivedFlow = ntohl(*(__u32*)CData(c));
                    break;
               }
            }
          break;
         case SOL_IP:
             switch(c->cmsg_type) {
                case IP_TOS:
                   ReceivedFlow = (card32)(*(__u8*)CData(c)) << 20;
                 break;
             }
          break;
#endif
      }
   }
   return(cc);
}


// ###### recvfrom() impl. with support for getting flow info and TOS #######
ssize_t Socket::recvFrom(int              fd,
                         void*            buf,
                         const size_t     len,
                         integer&         flags,
                         struct sockaddr* addr,
                         socklen_t*       addrlen)
{
   int           cc;
   char          cbuf[1024];
   struct iovec  iov = { (char*)buf, len };
   struct msghdr msg = {
#if (SYSTEM == OS_Darwin)
      (char*)addr,
#else
      addr,
#endif
      *addrlen,
      &iov, 1,
      cbuf, sizeof(cbuf),
      flags
   };

   cc = receiveMsg(&msg,flags,true);
   if(cc < 0) {
      return(cc);
   }

   flags = msg.msg_flags;
   *addrlen = msg.msg_namelen;
   return(cc);
}


// ###### Receive data from sender ##########################################
ssize_t Socket::receiveFrom(void*          buffer,
                            const size_t   length,
                            SocketAddress& sender,
                            integer&       flags)
{
   char      socketAddressBuffer[SocketAddress::MaxSockLen];
   socklen_t socketAddressLength = SocketAddress::MaxSockLen;

   const ssize_t result = recvFrom(SocketDescriptor, buffer,
                                   length, flags,
                                   (sockaddr*)&socketAddressBuffer,
                                   &socketAddressLength);
   if(result > 0) {
      sender.setSystemAddress((sockaddr*)&socketAddressBuffer,
                              socketAddressLength);
   }
   return(result);
}


// ###### Send data #########################################################
ssize_t Socket::send(const void*   buffer,
                     const size_t  length,
                     const integer flags,
                     const card8   trafficClass)
{
   // ====== Check, if traffic class has to be set ==========================
   if((trafficClass != 0) && (Destination != NULL)) {

      // ====== IPv6 address -> Set traffic class ===========================
      sockaddr_in6* socketAddress = (sockaddr_in6*)Destination;
      if((socketAddress->sin6_family == AF_INET6) &&
         !IN6_IS_ADDR_V4MAPPED(&socketAddress->sin6_addr)) {
         sockaddr_in6 newAddress  = *socketAddress;
         newAddress.sin6_flowinfo = htonl((ntohl(newAddress.sin6_flowinfo)
                                           & IPV6_FLOWINFO_FLOWLABEL) |
                                           ((card32)trafficClass << 20));
         ssize_t result = ext_sendto(SocketDescriptor, buffer, length, flags,
                                     (sockaddr*)&newAddress, sizeof(sockaddr_in6));
         if(result < 0) {
            LastError = errno;
            result    = -LastError;
         }
         return(result);
      }

      // ====== IPv4 or IPv4-mapped IPv6 address -> set TOS =================
      else if((socketAddress->sin6_family == AF_INET) ||
              (socketAddress->sin6_family == AF_INET6)) {
         setTypeOfService(trafficClass);
         ssize_t result = ext_send(SocketDescriptor,buffer,length,flags);
         setTypeOfService(SendFlow >> 20);
         if(result < 0) {
            LastError = errno;
            result    = -LastError;
         }
         return(result);
      }
   }

   // ====== Do simple send() without setting traffic class =================
   ssize_t result = ext_send(SocketDescriptor,buffer,length,flags);
   if(result < 0) {
      LastError = errno;
      result    = -LastError;
   }
   return(result);
}


// ###### Send data to receiver address #####################################
ssize_t Socket::sendTo(const void*          buffer,
                       const size_t         length,
                       const integer        flags,
                       const SocketAddress& receiver,
                       const card8          trafficClass)
{
   // ====== Get address ====================================================
   char          socketAddressBuffer[SocketAddress::MaxSockLen];
   sockaddr_in6* socketAddress       = (sockaddr_in6*)&socketAddressBuffer;
   socklen_t     socketAddressLength =
      receiver.getSystemAddress((sockaddr*)socketAddress,SocketAddress::MaxSockLen,
                               Family);
   if(socketAddressLength == 0) {
      return(-1);
   }

   // ====== Check, if traffic class has to be set ==========================
   if(trafficClass != 0x00) {

      // ====== IPv6 address -> Set traffic class ===========================
      if((socketAddress->sin6_family == AF_INET6) &&
         !IN6_IS_ADDR_V4MAPPED(&socketAddress->sin6_addr)) {
         sockaddr_in6 newAddress = *socketAddress;
         newAddress.sin6_flowinfo = htonl((ntohl(newAddress.sin6_flowinfo)
                                           & IPV6_FLOWINFO_FLOWLABEL) |
                                           ((card32)trafficClass << 20));
         ssize_t result = ext_sendto(SocketDescriptor,buffer,length,flags,
                                      (sockaddr*)&newAddress,sizeof(sockaddr_in6));
         if(result < 0) {
            LastError = errno;
            result    = -LastError;
         }
         return(result);
      }

      // ====== IPv4 or IPv4-mapped IPv6 address -> set TOS =================
      else if((socketAddress->sin6_family == AF_INET) ||
              (socketAddress->sin6_family == AF_INET6)) {
         setTypeOfService(trafficClass);
         ssize_t result = ext_sendto(SocketDescriptor,buffer,length,flags,
                                       (sockaddr*)socketAddress,socketAddressLength);
         setTypeOfService(SendFlow >> 20);
         if(result < 0) {
            LastError = errno;
            result    = -LastError;
         }
         return(result);
      }
   }

   // ====== Do simple send() without setting traffic class =================
   ssize_t result = ext_sendto(SocketDescriptor,buffer,length,flags,
                                 (sockaddr*)socketAddress,socketAddressLength);
   if(result < 0) {
      LastError = errno;
      result    = -LastError;
   }
   return(result);
}


// ###### Send message ######################################################
ssize_t Socket::sendMsg(const struct msghdr* msg,
                        const integer        flags,
                        const card8          trafficClass)
{
   if(trafficClass != 0x00) {
      setTypeOfService(trafficClass);
   }

   ssize_t result = ext_sendmsg(SocketDescriptor,msg,(int)flags);
   if(result < 0) {
      LastError = errno;
      result    = -LastError;
   }

   if(trafficClass != 0x00) {
      setTypeOfService(SendFlow >> 20);
   }
   return(result);
}


// ###### Allocate flow #####################################################
InternetFlow Socket::allocFlow(const InternetAddress& address,
                               const card32           flowLabel,
                               const card8            shareLevel)
{
#if (SYSTEM == OS_Linux)
   // ====== Check, if address is IPv6 ======================================
   if((!InternetAddress::UseIPv6) || (!address.isIPv6())) {
      InternetFlow flow(address,0,0x00);
      return(flow);
   }

   // ====== Get address ====================================================
   sockaddr_in6 socketAddress;
   cardinal     socketAddressLength = address.getSystemAddress(
                   (sockaddr*)&socketAddress,sizeof(sockaddr_in6),
                   AF_INET6);
   if(socketAddressLength == 0) {
      InternetFlow flow(address,0,0x00);
      return(flow);
   }

   // ====== Get flow label =================================================
   in6_flowlabel_req request;
   memcpy((void*)&request.flr_dst,(void*)&socketAddress.sin6_addr,16);
   request.flr_label   = htonl(flowLabel);
   request.flr_action  = IPV6_FL_A_GET;
   request.flr_share   = shareLevel;
   request.flr_flags   = IPV6_FL_F_CREATE;
   request.flr_expires = 10;
   request.flr_linger  = 6;
   request.__flr_pad   = 0;
   integer result = setSocketOption(SOL_IPV6,
                                    IPV6_FLOWLABEL_MGR,&request,
                                    sizeof(in6_flowlabel_req));
   if(result != 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: InternetFlow::allocFlow() - Unable to get flow label!" << std::endl;
#endif
      InternetFlow flow;
      return(flow);
   }

   return(InternetFlow(address,ntohl(request.flr_label),0x00));

#else

   InternetFlow flow(address,0,0x00);
   return(flow);

#endif
}


// ###### Free flow #########################################################
void Socket::freeFlow(InternetFlow& flow)
{
#if (SYSTEM == OS_Linux)
   in6_flowlabel_req request;
   memset((void*)&request.flr_dst,0,16);
   request.flr_label   = htonl(flow.getFlowLabel());
   request.flr_action  = IPV6_FL_A_PUT;
   request.flr_share   = 0;
   request.flr_flags   = 0;
   request.flr_expires = 0;
   request.flr_linger  = 0;
   request.__flr_pad   = 0;
   integer result = setSocketOption(SOL_IPV6,
                                    IPV6_FLOWLABEL_MGR,&request,
                                    sizeof(in6_flowlabel_req));
   if(result != 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: InternetFlow::freeFlow() - Unable to free flow label!" << std::endl;
#endif
   }
#endif
}


// ###### Renew flow label reservation ######################################
bool Socket::renewFlow(InternetFlow&  flow,
                       const cardinal expires,
                       const cardinal linger)
{
#if (SYSTEM == OS_Linux)
   if((InternetAddress::UseIPv6 == false) || ((SendFlow & IPV6_FLOWINFO_FLOWLABEL) == 0)) {
      return(true);
   }

   struct in6_flowlabel_req request;
   memset((void*)&request.flr_dst,0,16);
   request.flr_label   = htonl(flow.getFlowLabel());
   request.flr_action  = IPV6_FL_A_RENEW;
   request.flr_share   = 0;
   request.flr_flags   = 0;
   request.flr_expires = expires;
   request.flr_linger  = linger;
   request.__flr_pad   = 0;
   integer result = setSocketOption(SOL_IPV6,
                                    IPV6_FLOWLABEL_MGR,&request,
                                    sizeof(in6_flowlabel_req));
   if(result != 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: Socket::renew() - Unable to renew flow label!" << std::endl;
#endif
      return(false);
   }
#endif

   return(true);
}


// ###### Renew flow label reservation ######################################
bool Socket::renewFlow(const cardinal expires,
                       const cardinal linger)
{
#if (SYSTEM == OS_Linux)
   if((InternetAddress::UseIPv6 == false) || ((SendFlow & IPV6_FLOWINFO_FLOWLABEL) == 0)) {
      return(true);
   }

   struct in6_flowlabel_req request;
   memset((void*)&request.flr_dst,0,16);
   request.flr_label   = htonl(SendFlow);
   request.flr_action  = IPV6_FL_A_RENEW;
   request.flr_share   = 0;
   request.flr_flags   = 0;
   request.flr_expires = expires;
   request.flr_linger  = linger;
   request.__flr_pad   = 0;
   integer result = setSocketOption(SOL_IPV6,
                                    IPV6_FLOWLABEL_MGR,&request,
                                    sizeof(in6_flowlabel_req));
   if(result != 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: Socket::renew() - Unable to renew flow label!" << std::endl;
#endif
      return(false);
   }
#endif

   return(true);
}


// ###### Get socket's address ##############################################
bool Socket::getSocketAddress(SocketAddress& address) const
{
   char      socketAddress[SocketAddress::MaxSockLen];
   socklen_t socketAddressLength = SocketAddress::MaxSockLen;
   if(ext_getsockname(SocketDescriptor,(sockaddr*)&socketAddress,&socketAddressLength) == 0) {
      address.setSystemAddress((sockaddr*)&socketAddress,socketAddressLength);
      return(true);
   }
   return(false);
}


// ###### Get peer's address ################################################
bool Socket::getPeerAddress(SocketAddress& address) const
{
   char      socketAddress[SocketAddress::MaxSockLen];
   socklen_t socketAddressLength = SocketAddress::MaxSockLen;
   if(ext_getpeername(SocketDescriptor,(sockaddr*)&socketAddress,&socketAddressLength) == 0) {
      address.setSystemAddress((sockaddr*)&socketAddress,socketAddressLength);
      return(true);
   }
   return(false);
}


// ###### Get blocking mode #################################################
bool Socket::getBlockingMode()
{
   const long result = fcntl(F_GETFL,(long)0);
   return(!(result & O_NONBLOCK));
}


// ###### Set blocking mode #################################################
bool Socket::setBlockingMode(const bool on)
{
   long flags = fcntl(F_GETFL,(long)0);
   if(flags != -1) {
      flags = on ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
      return(fcntl(F_SETFL,flags) == 0);
   }
   return(false);
}


// ###### Get SO_LINGER #####################################################
cardinal Socket::getSoLinger()
{
   struct linger arg;
   socklen_t     l = sizeof(arg);
   if(getSocketOption(SOL_SOCKET,SO_LINGER,&arg,&l) != 0)
      return(0);
   return(arg.l_linger);
}


// ###### Set SO_LINGER #####################################################
bool Socket::setSoLinger(const bool on, const cardinal linger)
{
   struct linger arg;
   arg.l_onoff  = (int)on;
   arg.l_linger = linger;
   return(setSocketOption(SOL_SOCKET,SO_LINGER,&arg,sizeof(struct linger)) == 0);
}


// ###### Get SO_REUSEADDR ###################################################
bool Socket::getSoReuseAddress()
{
   int       flags = 0;
   socklen_t l     = sizeof(flags);
   getSocketOption(SOL_SOCKET,SO_REUSEADDR,&flags,&l);
   return(flags != 0);
}


// ###### Set SO_REUSEADDR ###################################################
bool Socket::setSoReuseAddress(const bool on)
{
   int flags = on ? 1 : 0;
   return(setSocketOption(SOL_SOCKET,SO_REUSEADDR,&flags,sizeof(flags)) == 0);
}


// ###### Get SO_BROADCAST ###################################################
bool Socket::getSoBroadcast()
{
   int       flags = 0;
   socklen_t l     = sizeof(flags);
   getSocketOption(SOL_SOCKET,SO_BROADCAST,&flags,&l);
   return(flags != 0);
}


// ###### Set SO_BROADCAST ###################################################
bool Socket::setSoBroadcast(const bool on)
{
   int flags = on ? 1 : 0;
   return(setSocketOption(SOL_SOCKET,SO_BROADCAST,&flags,sizeof(flags)) == 0);
}


// ###### Get TCP_NODELAY ###################################################
bool Socket::getTCPNoDelay()
{
   int       flags = 0;
   socklen_t l     = sizeof(flags);
   getSocketOption(IPPROTO_TCP,TCP_NODELAY,&flags,&l);
   return(flags != 0);
}


// ###### Set TCP_NODELAY ###################################################
bool Socket::setTCPNoDelay(const bool on)
{
   int flags = on ? 1 : 0;
   return(setSocketOption(IPPROTO_TCP,TCP_NODELAY,&flags,sizeof(flags)) == 0);
}


// ###### Add or drop multicast membership ##################################
bool Socket::multicastMembership(const SocketAddress& address,
                                 const char*          interface,
                                 const bool           add)
{
   if(Family == AF_INET) {
      sockaddr_in addr;
      if(address.getSystemAddress((sockaddr*)&addr,sizeof(addr),AF_INET) == 0) {
#ifndef DISABLE_WARNINGS
         std::cerr << "ERROR: Socket::multicastMembership() - Bad address type for IPv4 socket!" << std::endl;
#endif
      }
      else {
         ip_mreq mreq;
         mreq.imr_multiaddr = addr.sin_addr;
         if(interface != NULL) {
            ifreq ifr;
            strcpy(ifr.ifr_name,interface);
            if(ioctl(SIOCGIFADDR,&ifr) != 0) {
#ifndef DISABLE_WARNINGS
               std::cerr << "ERROR: Socket::multicastMembership() - Unable to get interface address!" << std::endl;
#endif
               return(false);
            }
            mreq.imr_interface = ((sockaddr_in*)&ifr.ifr_addr)->sin_addr;
         }
         else {
            memset((char*)&mreq.imr_interface,0,sizeof(mreq.imr_interface));
         }
         return(setSocketOption(IPPROTO_IP,
                                add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP,
                                &mreq,sizeof(mreq)));
      }
   }
   else if(Family == AF_INET6) {
      sockaddr_in6 addr;
      if(address.getSystemAddress((sockaddr*)&addr,sizeof(addr),AF_INET6) == 0) {
#ifndef DISABLE_WARNINGS
         std::cerr << "ERROR: Socket::multicastMembership() - Bad address type for IPv6 socket!" << std::endl;
#endif
      }
      else {
         ipv6_mreq mreq;
         memcpy((char*)&mreq.ipv6mr_multiaddr,(char*)&addr.sin6_addr,sizeof(addr.sin6_addr));
         if(interface != NULL) {
             mreq.ipv6mr_interface = if_nametoindex(interface);
         }
         else {
            mreq.ipv6mr_interface = 0;
         }
         return(setSocketOption(IPPROTO_IPV6,
                                add ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP,
                                &mreq,sizeof(mreq)));
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: Socket::multicastMembership() - Multicast is not supported for this socket type!" << std::endl;
#endif
   }
   return(false);
}


// ###### Get multicast loop mode ###########################################
bool Socket::getMulticastLoop()
{
   if(Family == AF_INET) {
      u_char    loop;
      socklen_t size = sizeof(loop);
      if(getSocketOption(IPPROTO_IP,IP_MULTICAST_LOOP,&loop,&size) == 0) {
         return(loop);
      }
   }
   else if(Family == AF_INET6) {
      int       loop;
      socklen_t size = sizeof(loop);
      if(getSocketOption(IPPROTO_IPV6,IPV6_MULTICAST_LOOP,&loop,&size) == 0) {
         return(loop);
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: Socket::getMulticastLoop() - Multicast is not supported for this socket type!" << std::endl;
#endif
   }
   return(false);
}


// ###### Set multicast loop mode ###########################################
bool Socket::setMulticastLoop(const bool on)
{
   if(Family == AF_INET) {
      const u_char value = (on ? 1 : 0);
      return(setSocketOption(IPPROTO_IP,IP_MULTICAST_LOOP,&value,sizeof(value)) == 0);
   }
   else if(Family == AF_INET6) {
      const int value = (on ? 1 : 0);
      return(setSocketOption(IPPROTO_IPV6,IPV6_MULTICAST_LOOP,&value,sizeof(value)) == 0);
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: Socket::setMulticastLoop() - Multicast is not supported for this socket type!" << std::endl;
#endif
   }
   return(false);
}


// ###### Get multicast TTL #################################################
card8 Socket::getMulticastTTL()
{
   if(Family == AF_INET) {
      u_char    ttl;
      socklen_t size = sizeof(ttl);
      if(getSocketOption(IPPROTO_IP,IP_MULTICAST_TTL,&ttl,&size) == 0) {
         return(ttl);
      }
   }
   else if(Family == AF_INET6) {
      int       ttl;
      socklen_t size = sizeof(ttl);
      if(getSocketOption(IPPROTO_IPV6,IPV6_MULTICAST_HOPS,&ttl,&size) == 0) {
         return(ttl);
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: Socket::getMulticastTTL() - Multicast is not supported for this socket type!" << std::endl;
#endif
   }
   return(0);
}


// ###### Set multicast TTL #################################################
bool Socket::setMulticastTTL(const card8 ttl)
{
   if(Family == AF_INET) {
      return(setSocketOption(IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)) == 0);
   }
   else if(Family == AF_INET6) {
      const int value = (int)ttl;
      return(setSocketOption(IPPROTO_IPV6,IPV6_MULTICAST_HOPS,&value,sizeof(value)) == 0);
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: Socket::setMulticastTTL() - Multicast is not supported for this socket type!" << std::endl;
#endif
   }
   return(false);
}


// ###### Set IPv4 type of service field ####################################
bool Socket::setTypeOfService(const card8 trafficClass)
{
#if (SYSTEM == OS_Linux)
   int opt = (int)trafficClass;
   int result = setSocketOption(SOL_IP,IP_TOS,&opt,sizeof(opt));
   if(result != 0) {
      char str[32];
      snprintf((char*)&str,sizeof(str),"$%02x!",trafficClass);
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: Socket::setTypeOfService() - Unable to set TOS to " << str << std::endl;
#endif
      return(false);
   }
#endif
   return(true);
}


// ###### Filter addresses for getLocalAddressList() ########################
bool filterInternetAddress(const InternetAddress* newAddress,
                           const cardinal         flags)
{
   if( ((!InternetAddress::UseIPv6) && (newAddress->isIPv6()))               ||
       ((flags & Socket::GLAF_HideLoopback)  && (newAddress->isLoopback()))  ||
       ((flags & Socket::GLAF_HideLinkLocal) && (newAddress->isLinkLocal())) ||
       ((flags & Socket::GLAF_HideSiteLocal) && (newAddress->isSiteLocal())) ||
       ((flags & Socket::GLAF_HideMulticast) && (newAddress->isMulticast())) ||
       ((flags & Socket::GLAF_HideBroadcast) && (newAddress->isBroadcast())) ||
       ((flags & Socket::GLAF_HideReserved)  && (newAddress->isReserved()))  ||
       (newAddress->isUnspecified()) ) {
      return(false);
   }
   return(true);
}


// ###### Get list of local addresses #######################################
bool Socket::getLocalAddressList(SocketAddress**& addressList,
                                 cardinal&        numberOfNets,
                                 const cardinal   flags)
{
   // ====== Initialize =====================================================
   addressList  = NULL;
   numberOfNets = 0;


   // ====== Get interfaces configuration ===================================
   struct ifconf config;
   char          configBuffer[8192];
   config.ifc_buf = configBuffer;
   config.ifc_len = sizeof(configBuffer);
#if (SYSTEM == OS_SOLARIS)
   // SOLARIS does not like SIOCGIFFLAGS on AF_INET6
   int fd = ::socket(AF_INET,SOCK_DGRAM,0);
#else
   int fd = ::socket((InternetAddress::UseIPv6 == true) ? AF_INET6 : AF_INET,SOCK_DGRAM,0);
#endif
   if(fd < 0) {
      return(false);
   }
   if(::ioctl(fd,SIOCGIFCONF,(char*)&config) == -1) {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: Socket::getLocalAddressList() - SIOCGIFCONF failed!" << std::endl;
#endif
      ::close(fd);
      return(false);
   }


   // ====== Get maximum number of addresses to allocate list entries =======
   struct ifreq* ifrequest;
   struct ifreq* nextif;
   cardinal      numAllocAddr = 0;
   cardinal      pos          = 0;
#ifdef USES_BSD_4_4_SOCKET
   for(pos = 0;pos < (cardinal)config.ifc_len;) {
      ifrequest = (struct ifreq *)&configBuffer[pos];
      pos += (ifrequest->ifr_addr.sa_len + sizeof(ifrequest->ifr_name));
      if(ifrequest->ifr_addr.sa_len == 0) {
         /* if the interface has no address then you must
          * skip at a minium a sockaddr structure
          */
         pos += sizeof(struct sockaddr);
      }
      numAllocAddr++;
   }
#else
   numAllocAddr = config.ifc_len / sizeof(struct ifreq);
   ifrequest = config.ifc_req;
#endif

#if (SYSTEM == OS_Linux)
   cardinal addedNets = 0;
   if(InternetAddress::UseIPv6) {
      FILE* v6list = fopen(LINUX_PROC_IPV6_FILE,"r");
      if(v6list != NULL){
         char addrBuffer[256];
         while(fgets(addrBuffer,sizeof(addrBuffer),v6list) != NULL) {
            addedNets++;
         }
         fclose(v6list);
      }
      numAllocAddr += addedNets;
   }
#endif


   // ====== Allocate address list ==========================================
   addressList = SocketAddress::newAddressList(numAllocAddr);
   if(addressList == NULL) {
      ::close(fd);
      return(false);
   }


   // ====== Get addresses ==================================================
   pos = 0;
#if (SYSTEM == OS_Linux)
   if(InternetAddress::UseIPv6) {
      FILE* v6list = fopen(LINUX_PROC_IPV6_FILE,"r");
      if(v6list != NULL){
         // Linux requires reading address list from /proc file!
         struct sockaddr_in6 sin6;
         char                addrBuffer[256];
         char                addrBuffer2[64];
         memset((char *)&sin6,0,sizeof(sin6));
         sin6.sin6_family = AF_INET6;
         while(fgets(addrBuffer,sizeof(addrBuffer),v6list) != NULL) {
            // ====== Get address ==============================================
            memset(addrBuffer2,0,sizeof(addrBuffer2));
            strncpy(addrBuffer2,addrBuffer,4);
            addrBuffer2[4] = ':';
            strncpy(&addrBuffer2[5],&addrBuffer[4],4);
            addrBuffer2[9] = ':';
            strncpy(&addrBuffer2[10],&addrBuffer[8],4);
            addrBuffer2[14] = ':';
            strncpy(&addrBuffer2[15],&addrBuffer[12],4);
            addrBuffer2[19] = ':';
            strncpy(&addrBuffer2[20],&addrBuffer[16],4);
            addrBuffer2[24] = ':';
            strncpy(&addrBuffer2[25],&addrBuffer[20],4);
            addrBuffer2[29] = ':';
            strncpy(&addrBuffer2[30],&addrBuffer[24],4);
            addrBuffer2[34] = ':';
            strncpy(&addrBuffer2[35],&addrBuffer[28],4);
            if(inet_pton(AF_INET6,addrBuffer2,(void *)&sin6.sin6_addr)){
               InternetAddress* newAddress = new InternetAddress((sockaddr*)&sin6,sizeof(sin6));
               if(newAddress == NULL) {
                 ::close(fd);
                 SocketAddress::deleteAddressList(addressList);
                 return(false);
               }
               if(filterInternetAddress(newAddress,flags)) {
                  addressList[numberOfNets] = newAddress;
                  numberOfNets++;
               }
               else {
                  delete newAddress;
               }
            }
         }
         fclose(v6list);
      }
   }
#endif

   // ====== Read addresses from configuration ==============================
   ifrequest = (struct ifreq *)&configBuffer[pos];
#if (SYSTEM == OS_Linux)
   const cardinal toRead = numAllocAddr - addedNets;
#else
   const cardinal toRead = numAllocAddr;
#endif
   for(cardinal i = 0;i < toRead;i++,ifrequest=nextif) {
      // ====== Get pointer to next address =================================
#ifdef USES_BSD_4_4_SOCKET
      pos += (ifrequest->ifr_addr.sa_len + sizeof(ifrequest->ifr_name));
      if(ifrequest->ifr_addr.sa_len == 0){
         pos += sizeof(struct sockaddr);
      }
      nextif = (struct ifreq *)&configBuffer[pos];
#else
      nextif = ifrequest + 1;
#endif
      sockaddr* currentAddress = &ifrequest->ifr_addr;


      // ====== Get interface flags =========================================
      struct ifreq local;
      memset(&local,0,sizeof(local));
      memcpy(local.ifr_name,ifrequest->ifr_name,IFNAMSIZ);
      if(::ioctl(fd,SIOCGIFFLAGS,(char*)&local) < 0) {
#ifndef DISABLE_WARNINGS
         std::cerr << "ERROR: Socket::getLocalAddressList() - SIOCGIFFLAGS failed!" << std::endl;
#endif
         continue;
      }

      const unsigned int ifflags = local.ifr_flags;
      if(!(ifflags & IFF_UP)) {
         // Device is down.
         continue;
      }

#if (SYSTEM == OS_FreeBSD)
      unsigned int ifflags6 = 0;
      if(currentAddress->sa_family == AF_INET6) {
         struct in6_ifreq local6;
         memset(&local6,0,sizeof(local6));
         memcpy(local6.ifr_name,ifrequest->ifr_name,IFNAMSIZ);
         local6.ifr_addr = *((sockaddr_in6*)currentAddress);
         if(::ioctl(fd,SIOCGIFAFLAG_IN6,(char*)&local6) == 0) {
            ifflags6 = local6.ifr_ifru.ifru_flags6;
         }
      }
#endif


      // ====== Skip loopback device, if requested ==========================
      if(flags & GLAF_HideLoopback) {
         if((ifflags & IFF_LOOPBACK) == IFF_LOOPBACK) {
            continue;
         }
      }
#if (SYSTEM == OS_FreeBSD)
      if(flags & GLAF_HideAnycast) {
         if((ifflags6 & IN6_IFF_ANYCAST) == IN6_IFF_ANYCAST) {
            continue;
         }
      }
#endif


      // ====== Add address to list =========================================
      switch(currentAddress->sa_family) {
         case AF_INET:
         case AF_INET6: {
               InternetAddress* newAddress =
                  new InternetAddress((sockaddr*)currentAddress,
                                      (currentAddress->sa_family == AF_INET6) ?
                                         sizeof(sockaddr_in6) : sizeof(sockaddr_in));
               if(newAddress == NULL) {
                  ::close(fd);
                  SocketAddress::deleteAddressList(addressList);
                  return(false);
               }
               if(filterInternetAddress(newAddress,flags) == false) {
                  delete newAddress;
                  continue;
               }

               addressList[numberOfNets] = newAddress;
            }
           break;
         default:
            continue;
          break;
      }


      // ====== Check for duplicates ========================================
      const String lastAddress = addressList[numberOfNets]->getAddressString();
      for(cardinal j = 0;j < numberOfNets;j++){
         if(addressList[j]->getAddressString() == lastAddress) {
            delete addressList[numberOfNets];
            addressList[numberOfNets] = NULL;
            numberOfNets--;
            break;
         }
      }

      numberOfNets++;
   }

   ::close(fd);
   addressList[numberOfNets] = NULL;
   return(true);
}
