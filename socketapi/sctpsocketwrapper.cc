/*
 *  $Id: sctpsocketwrapper.cc,v 1.11 2003/06/18 15:21:25 dreibh Exp $
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
 * Purpose: SCTP Socket Wrapper
 *
 */

#include "tdsystem.h"
#include "sctpsocketwrapper.h"
#include "extsocketdescriptor.h"
#include "tdin6.h"

#include <stdarg.h>
#include <sys/ioctl.h>


// Random port selection for INADDR_ANY. Necessary for localhost testing!
#define RANDOM_PORTSELECTION

// Print note, if no SCTP is available in kernel.
// #define PRINT_NOSCTP_NOTE
// #define PRINT_SELECT
// #define PRINT_RANDOM_PORTSELECTION



#ifndef HAVE_KERNEL_SCTP


#include "tdmessage.h"
#include "internetaddress.h"
#include "sctpsocketmaster.h"
#include "sctpsocket.h"
#include "sctpassociation.h"


#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>


// Set errno variable and return.
inline static int getErrnoResult(const int result)
{
   if(result < 0) {
      errno = -result;
      return(-1);
   }
   errno = 0;
   return(result);
}
#define errno_return(x) return(getErrnoResult(x))


// FD_ISSET rewriting with NULL check
inline static int SAFE_FD_ISSET(int fd, fd_set* fdset)
{
   if(fdset == NULL) {
      return(0);
   }
   return(FD_ISSET(fd,fdset));
}


// FD_ZERO rewriting with NULL check
inline static void SAFE_FD_ZERO(fd_set* fdset)
{
   if(fdset != NULL) {
      FD_ZERO(fdset);
   }
}


// ###### Constructor #######################################################
ExtSocketDescriptorMaster::ExtSocketDescriptorMaster()
{
   for(unsigned int i = 0;i < MaxSockets;i++) {
      Sockets[i].Type = ExtSocketDescriptor::ESDT_Invalid;
   }
   if(STDIN_FILENO < MaxSockets) {
      Sockets[STDIN_FILENO].Type                   = ExtSocketDescriptor::ESDT_System;
      Sockets[STDIN_FILENO].Socket.SystemSocketID  = STDIN_FILENO;
   }
   if(STDOUT_FILENO < MaxSockets) {
      Sockets[STDOUT_FILENO].Type                  = ExtSocketDescriptor::ESDT_System;
      Sockets[STDOUT_FILENO].Socket.SystemSocketID = STDOUT_FILENO;
   }
   if(STDERR_FILENO < MaxSockets) {
      Sockets[STDERR_FILENO].Type                  = ExtSocketDescriptor::ESDT_System;
      Sockets[STDERR_FILENO].Socket.SystemSocketID = STDERR_FILENO;
   }
}


// ###### Destructor ########################################################
ExtSocketDescriptorMaster::~ExtSocketDescriptorMaster()
{
   for(unsigned int i = 0;i < MaxSockets;i++) {
      if((Sockets[i].Type != ExtSocketDescriptor::ESDT_Invalid) &&
         (i != STDIN_FILENO)  &&
         (i != STDOUT_FILENO) &&
         (i != STDERR_FILENO)) {
         ext_close(i);
      }
   }
}


// ###### Get ExtSocketDescriptor for given ID #########################################
inline ExtSocketDescriptor* ExtSocketDescriptorMaster::getSocket(const int id)
{
   if((id < (int)MaxSockets) && (id >= 0)) {
      return(&Sockets[id]);
   }
   return(NULL);
}


// ###### Set ExtSocketDescriptor of given ID ##########################################
int ExtSocketDescriptorMaster::setSocket(const ExtSocketDescriptor& newSocket)
{
   for(int i = (int)(MaxSockets - 1);i >= 0;i--) {
      if(Sockets[i].Type == ExtSocketDescriptor::ESDT_Invalid) {
         Sockets[i] = newSocket;
         return(i);
      }
   }
   return(-ENOMEM);
}


// ###### Get paddrinfo #####################################################
static int getPrimaryAddressInfo(ExtSocketDescriptor* tdSocket,
                                 unsigned int         assocID,
                                 sctp_paddrinfo&      paddrinfo)
{
   int             result = -EBADF;
   SCTP_PathStatus parameters;

   SCTPSocketMaster::MasterInstance.lock();

   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->
         getPathParameters(NULL,parameters)) {
         assocID = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getID();
         result = 0;
      }
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->
         getPathParameters(assocID,NULL,parameters)) {
         result = 0;
      }
   }

   if(result == 0) {
      SocketAddress* address = SocketAddress::createSocketAddress(0,
                                  (char*)&parameters.destinationAddress);
      if(address != NULL) {
         if(address->getSystemAddress((sockaddr*)&paddrinfo.spinfo_address,
                                      sizeof(paddrinfo.spinfo_address),
                                      tdSocket->Socket.SCTPSocketDesc.Domain) > 0) {
            paddrinfo.spinfo_state    = parameters.state;
            paddrinfo.spinfo_cwnd     = parameters.cwnd;
            paddrinfo.spinfo_srtt     = parameters.srtt;
            paddrinfo.spinfo_rto      = parameters.rto;
            paddrinfo.spinfo_mtu      = parameters.mtu;
            paddrinfo.spinfo_assoc_id = assocID;
         }
         else {
            result = -EIO;
         }
         delete address;
      }
      else {
         result = -EIO;
      }
   }

   SCTPSocketMaster::MasterInstance.unlock();

   return(result);
}


// ###### Get association parameters ########################################
static int getAssocParams(ExtSocketDescriptor*     tdSocket,
                          unsigned int&            assocID,
                          SCTP_Association_Status& parameters)
{
   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->
            getAssocStatus(parameters)) {
         assocID = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getID();
         return(0);
      }
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->
            getAssocStatus(assocID, parameters)) {
         return(0);
      }
   }
   return(-1);
}


// ###### Set association parameters ########################################
static int setAssocParams(ExtSocketDescriptor*           tdSocket,
                          const unsigned int             assocID,
                          const SCTP_Association_Status& parameters)
{
   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->
            setAssocStatus(parameters)) {
         return(0);
      }
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->
            setAssocStatus(assocID, parameters)) {
         return(0);
      }
   }
   return(-1);
}


// ###### Get association parameters ########################################
static int getAddressParams(ExtSocketDescriptor* tdSocket,
                            unsigned int&        assocID,
                            sockaddr*            sockadr,
                            const socklen_t      socklen,
                            SCTP_PathStatus&     parameters)
{
   SocketAddress* address = SocketAddress::createSocketAddress(0,sockadr,socklen);
   if(address == NULL) {
      return(-EINVAL);
   }

   // ====== Get path status =============================================
   bool ok = false;
   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      ok = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getPathParameters(address,parameters);
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      ok = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getPathParameters(assocID,address,parameters);
   }

   return((ok == false) ? -EIO : 0);
}


// ###### Set association parameters ########################################
static int setAddressParams(ExtSocketDescriptor*    tdSocket,
                            unsigned int            assocID,
                            sockaddr*               sockadr,
                            const socklen_t         socklen,
                            const SCTP_PathStatus&  newParameters)
{
   SocketAddress* address = SocketAddress::createSocketAddress(0,sockadr,socklen);
   if(address == NULL) {
      return(-EINVAL);
   }

   // ====== Get path status =============================================
   bool ok = false;
   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      ok = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->setPathParameters(address,newParameters);
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      ok = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setPathParameters(assocID,address,newParameters);
   }

   return((ok == false) ? -EIO : 0);
}


// ###### socket() wrapper ##################################################
int ext_socket(int domain, int type, int protocol)
{
   ExtSocketDescriptor tdSocket;
   if(protocol == IPPROTO_SCTP) {
      // ====== Is SCTP available? ==========================================
      if(!sctp_isavailable()) {
         errno_return(-ENOPROTOOPT);
      }

      // ====== Check parameters ============================================
      cardinal flags;
      if(type == SOCK_STREAM) {
         tdSocket.Socket.SCTPSocketDesc.ConnectionOriented = true;
         flags = 0;
      }
      else if((type == SOCK_DGRAM) || (type == SOCK_SEQPACKET)) {
         tdSocket.Socket.SCTPSocketDesc.ConnectionOriented = false;
         flags = SCTPSocket::SSF_GlobalQueue|SCTPSocket::SSF_AutoConnect;
      }
      else {
         errno_return(-EINVAL);
      }

      // ====== Create SCTP socket ==========================================
      tdSocket.Type = ExtSocketDescriptor::ESDT_SCTP;
      tdSocket.Socket.SCTPSocketDesc.Domain             = domain;
      tdSocket.Socket.SCTPSocketDesc.Type               = type;
      tdSocket.Socket.SCTPSocketDesc.Flags              = 0;
      tdSocket.Socket.SCTPSocketDesc.Parent             = 0;
      tdSocket.Socket.SCTPSocketDesc.Linger.l_onoff     = 1;
      tdSocket.Socket.SCTPSocketDesc.Linger.l_linger    = 10;
      tdSocket.Socket.SCTPSocketDesc.SCTPAssociationPtr = NULL;
      tdSocket.Socket.SCTPSocketDesc.InitMsg.sinit_num_ostreams   = 1;
      tdSocket.Socket.SCTPSocketDesc.InitMsg.sinit_max_instreams  = 1;
      tdSocket.Socket.SCTPSocketDesc.InitMsg.sinit_max_attempts   = 8;
      tdSocket.Socket.SCTPSocketDesc.InitMsg.sinit_max_init_timeo = 60000;
      tdSocket.Socket.SCTPSocketDesc.SCTPSocketPtr = new SCTPSocket(flags);
      if(tdSocket.Socket.SCTPSocketDesc.SCTPSocketPtr == NULL) {
         errno_return(-ENOMEM);
      }

      // Set default notifications for UDP-like socket: no notification messages
      if(tdSocket.Socket.SCTPSocketDesc.ConnectionOriented == false) {
         tdSocket.Socket.SCTPSocketDesc.SCTPSocketPtr->setNotificationFlags(
            SCTP_RECVDATAIOEVNT );
      }

      int result = ExtSocketDescriptorMaster::setSocket(tdSocket);
      if(result < 0) {
         delete tdSocket.Socket.SCTPSocketDesc.SCTPSocketPtr;
         tdSocket.Socket.SCTPSocketDesc.SCTPSocketPtr = NULL;
      }
      errno_return(result);
   }
   else {
      // ====== Create system socket ========================================
      tdSocket.Type = ExtSocketDescriptor::ESDT_System;
      tdSocket.Socket.SystemSocketID = socket(domain,type,protocol);
      if(tdSocket.Socket.SystemSocketID >= 0) {
         int result = ExtSocketDescriptorMaster::setSocket(tdSocket);
         if(result < 0) {
            delete tdSocket.Socket.SCTPSocketDesc.SCTPSocketPtr;
            tdSocket.Socket.SCTPSocketDesc.SCTPSocketPtr = NULL;
         }
         errno_return(result);
      }
      errno_return(tdSocket.Socket.SystemSocketID);
   }
}


// ###### open() wrapper ####################################################
int ext_open(const char* pathname, int flags, mode_t mode)
{
   ExtSocketDescriptor tdSocket;
   tdSocket.Type = ExtSocketDescriptor::ESDT_System;
   tdSocket.Socket.SystemSocketID = open(pathname,flags,mode);
   if(tdSocket.Socket.SystemSocketID >= 0) {
      int result = ExtSocketDescriptorMaster::setSocket(tdSocket);
      if(result < 0) {
         close(tdSocket.Socket.SystemSocketID);
      }
      errno_return(result);
   }
   errno_return(tdSocket.Socket.SystemSocketID);
}


// ###### creat() wrapper ###################################################
int ext_creat(const char* pathname, mode_t mode)
{
   ExtSocketDescriptor tdSocket;
   tdSocket.Type = ExtSocketDescriptor::ESDT_System;
   tdSocket.Socket.SystemSocketID = creat(pathname,mode);
   if(tdSocket.Socket.SystemSocketID >= 0) {
      int result = ExtSocketDescriptorMaster::setSocket(tdSocket);
      if(result < 0) {
         close(tdSocket.Socket.SystemSocketID);
      }
      errno_return(result);
   }
   errno_return(tdSocket.Socket.SystemSocketID);
}


// ###### Delete all childs of a parent socket ##############################
static void ext_closeChilds(const int parent)
{
   for(unsigned int i = 1;i <= ExtSocketDescriptorMaster::MaxSockets;i++) {
      ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(i);
      if((tdSocket != NULL) &&
         (tdSocket->Type == ExtSocketDescriptor::ESDT_SCTP) &&
         (tdSocket->Socket.SCTPSocketDesc.Parent == parent)) {
         ext_close(i);
      }
   }
}


// ###### close() wrapper ###################################################
int ext_close(int sockfd)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            ext_closeChilds(sockfd);
            if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
               if(tdSocket->Socket.SCTPSocketDesc.Linger.l_onoff == 1) {
                  if(tdSocket->Socket.SCTPSocketDesc.Linger.l_linger > 0) {
                     tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->shutdown();
                  }
                  else {
                     tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->abort();
                  }
               }
               delete tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr;
               tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr = NULL;
            }
            if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
               if(tdSocket->Socket.SCTPSocketDesc.Linger.l_onoff == 1) {
                  if(tdSocket->Socket.SCTPSocketDesc.Linger.l_linger > 0) {
                     tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->unbind(false);
                  }
                  else {
                     tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->unbind(true);
                  }
               }
               delete tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr;
               tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr = NULL;
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            close(tdSocket->Socket.SystemSocketID);
            tdSocket->Socket.SystemSocketID = 0;
          break;
         default:
            errno_return(-ENXIO);
          break;
      }
      tdSocket->Type = ExtSocketDescriptor::ESDT_Invalid;
      errno_return(0);
   }
   errno_return(-EBADF);
}


// ###### bind to any address ###############################################
static int bindToAny(struct ExtSocketDescriptor* tdSocket)
{
   int result = 0;
   if((tdSocket->Type == ExtSocketDescriptor::ESDT_SCTP)      &&
      (tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) &&
      (tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getID() == 0)) {
      InternetAddress anyAddress;
      anyAddress.reset();

      SocketAddress* addressArray[2];
      addressArray[0] = (SocketAddress*)&anyAddress;
      addressArray[1] = NULL;

#ifdef RANDOM_PORTSELECTION
      card16 port;
      for(cardinal i = 0;i < 50000;i++) {
         SCTPSocketMaster::MasterInstance.lock();
         port = (card16)(16384 + SCTPSocketMaster::Random.random32() % (61000 - 16384));
         SCTPSocketMaster::MasterInstance.unlock();
         addressArray[0]->setPort(port),
#ifdef PRINT_RANDOM_PORTSELECTION
         cout << "trying " << *(addressArray[0]) << "..." << endl;
#endif
#endif

         result = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->bind(
            addressArray[0]->getPort(),
            tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_max_instreams,
            tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_num_ostreams,
            (const SocketAddress**)&addressArray);

#ifdef RANDOM_PORTSELECTION
         if(result >= 0) {
            break;
         }
      }
#endif

   }
   errno_return(result);
}


// ###### bind() wrapper ####################################################
int ext_bind(int sockfd, struct sockaddr* my_addr, socklen_t addrlen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
             return(ext_bindx(sockfd,(sockaddr_storage*)my_addr,1,SCTP_BINDX_ADD_ADDR));
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(bind(tdSocket->Socket.SystemSocketID,(sockaddr*)my_addr,addrlen));
          break;
         default:
            errno_return(-ENXIO);
          break;
      }
   }
   errno_return(-EBADF);
}


// ###### ext_bind() wrapper ################################################
int ext_bindx(int                      sockfd,
              struct sockaddr_storage* addrs,
              int                      addrcnt,
              int                      flags)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP: {
               // ====== Check parameters and descriptor ====================
               if((addrcnt < 1) || (addrcnt > SCTP_MAX_NUM_ADDRESSES)) {
                  errno_return(-EINVAL);
               }
               else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr == NULL) {
                  errno_return(-EBADF);
               }

               // ====== Get addresses ======================================
               SocketAddress* addressArray[addrcnt + 1];
               sockaddr_storage* a = addrs;
               for(int i = 0;i < addrcnt;i++) {
                  addressArray[i] = SocketAddress::createSocketAddress(
                                       0,(sockaddr*)a,sizeof(sockaddr_storage));
                  if(addressArray[i] == NULL) {
                     for(int j = 0;j < i;j++) {
                        delete addressArray[j];
                     }
                     errno_return(-ENOMEM);
                  }
                  a = (sockaddr_storage*)((long)a + (long)sizeof(sockaddr_storage));
               }
               addressArray[addrcnt] = NULL;

               // ====== Bind socket ========================================
               int result = -EINVAL;
               if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getID() == 0) {
                  if(flags == SCTP_BINDX_ADD_ADDR) {
                     result = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->bind(
                                 addressArray[0]->getPort(),
                                 tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_max_instreams,
                                 tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_num_ostreams,
                                 (const SocketAddress**)&addressArray);
                  }
               }
               else {
                  if(flags == SCTP_BINDX_ADD_ADDR) {
                     result = 0;
                     if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                        for(int i = 0;i < addrcnt;i++) {
                           const bool ok = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->
                                              addAddress(*(addressArray[i]));
                           if(!ok) {
                              result = -EINVAL;
                              break;
                           }
                        }
                     }
                     else {
                        for(int i = 0;i < addrcnt;i++) {
                           const bool ok = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->
                                              addAddress(0,*(addressArray[i]));
                           if(!ok) {
                              result = -EINVAL;
                              break;
                           }
                        }
                     }
                  }
                  else if(flags == SCTP_BINDX_REM_ADDR) {
                     result = 0;
                     if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                        for(int i = 0;i < addrcnt;i++) {
                           const bool ok = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->
                                              deleteAddress(*(addressArray[i]));
                           if(!ok) {
                              result = -EINVAL;
                              break;
                           }
                        }
                     }
                     else {
                        for(int i = 0;i < addrcnt;i++) {
                           const bool ok = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->
                                              deleteAddress(0,*(addressArray[i]));
                           if(!ok) {
                              result = -EINVAL;
                              break;
                           }
                        }
                     }
                  }
               }

               // ====== Remove addresses ===================================
               for(int i = 0;i < addrcnt;i++) {
                  delete addressArray[i];
               }
               errno_return(result);
            }
           break;
         case ExtSocketDescriptor::ESDT_System:
            return(bind(tdSocket->Socket.SystemSocketID,(sockaddr*)addrs,sizeof(sockaddr_storage)));
           break;
         default:
            errno_return(-ENXIO);
          break;
      }
   }
   errno_return(-EBADF);
}


// ###### listen() wrapper ####################################################
int ext_listen(int sockfd, int backlog)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
               tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->listen(backlog);
               errno_return(0);
            }
            errno_return(-EOPNOTSUPP);
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(listen(tdSocket->Socket.SystemSocketID,backlog));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### accept() wrapper ####################################################
int ext_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                  SocketAddress** remoteAddressArray = NULL;
                  SCTPAssociation* association =
                     tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->accept(
                        &remoteAddressArray,
                        !(tdSocket->Socket.SCTPSocketDesc.Flags & O_NONBLOCK));
                  if(association != NULL) {
                     if((remoteAddressArray != NULL)    &&
                        (remoteAddressArray[0] != NULL) &&
                        (addr != NULL) && (addrlen != NULL)) {
                        *addrlen = remoteAddressArray[0]->getSystemAddress(
                                      addr, *addrlen,
                                      tdSocket->Socket.SCTPSocketDesc.Domain);
                     }
                     else {
                        addrlen = 0;
                     }

                     ExtSocketDescriptor newExtSocketDescriptor = *tdSocket;
                     newExtSocketDescriptor.Socket.SCTPSocketDesc.ConnectionOriented = true;
                     newExtSocketDescriptor.Socket.SCTPSocketDesc.SCTPSocketPtr      = NULL;
                     newExtSocketDescriptor.Socket.SCTPSocketDesc.SCTPAssociationPtr = association;
                     newExtSocketDescriptor.Socket.SCTPSocketDesc.Parent             = sockfd;
                     const int newFD = ExtSocketDescriptorMaster::setSocket(newExtSocketDescriptor);
                     SocketAddress::deleteAddressList(remoteAddressArray);
                     if(newFD < 0) {
                        delete newExtSocketDescriptor.Socket.SCTPSocketDesc.SCTPAssociationPtr;
                        newExtSocketDescriptor.Socket.SCTPSocketDesc.SCTPAssociationPtr = NULL;
                     }

                     // New socket may not inherit flags from parent socket!
                     newExtSocketDescriptor.Socket.SCTPSocketDesc.SCTPAssociationPtr->setNotificationFlags(
                        SCTP_RECVDATAIOEVNT
                     );

                     errno_return(newFD);
                  }
                  else {
                     errno_return(-EWOULDBLOCK);
                  }
               }
               errno_return(-EOPNOTSUPP);
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            {
               ExtSocketDescriptor newExtSocketDescriptor = *tdSocket;
               newExtSocketDescriptor.Socket.SystemSocketID = accept(tdSocket->Socket.SystemSocketID,addr,addrlen);
               if(newExtSocketDescriptor.Socket.SystemSocketID >= 0) {
                  int result = ExtSocketDescriptorMaster::setSocket(newExtSocketDescriptor);
                  if(result < 0) {
                     close(newExtSocketDescriptor.Socket.SystemSocketID);
                  }
                  errno_return(result);
               }
               errno_return(newExtSocketDescriptor.Socket.SystemSocketID);
            }
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### getsockname() wrapper #############################################
int ext_getsockname(int sockfd, struct sockaddr* name, socklen_t* namelen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               int result = -ENXIO;
               SocketAddress** localAddressArray = NULL;
               if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
                  tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getLocalAddresses(localAddressArray);
               }
               else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                  tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getLocalAddresses(localAddressArray);
               }
               else {
                  result = -EBADF;
               }

               if((localAddressArray != NULL)    &&
                  (localAddressArray[0] != NULL) &&
                  (name != NULL) && (namelen != NULL)) {
                  if(localAddressArray[0]->getSystemAddress(name,*namelen,
                                              tdSocket->Socket.SCTPSocketDesc.Domain)) {
                     result = 0;
                  }
                  else {
                     result = -ENAMETOOLONG;
                  }
               }

               SocketAddress::deleteAddressList(localAddressArray);
               errno_return(result);
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(getsockname(tdSocket->Socket.SystemSocketID,name,namelen));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### getpeername() wrapper #############################################
int ext_getpeername(int sockfd, struct sockaddr* name, socklen_t* namelen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               int result = -ENXIO;
               SocketAddress** remoteAddressArray = NULL;
               if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                  tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getRemoteAddresses(remoteAddressArray);
               }
               else {
                  result = -EBADF;
               }

               if((remoteAddressArray != NULL)    &&
                  (remoteAddressArray[0] != NULL) &&
                  (name != NULL) && (namelen != NULL)) {
                  if(remoteAddressArray[0]->getSystemAddress(name,*namelen,tdSocket->Socket.SCTPSocketDesc.Domain) > 0) {
                     result = 0;
                  }
                  else {
                     result = -ENAMETOOLONG;
                  }
               }

               SocketAddress::deleteAddressList(remoteAddressArray);
               errno_return(result);
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(getpeername(tdSocket->Socket.SystemSocketID,name,namelen));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### fcntl() wrapper ###################################################
int ext_fcntl(int sockfd, int cmd, ...)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      va_list va;
      unsigned long int arg;
      va_start (va, cmd);
      arg = va_arg (va, unsigned long int);
      va_end (va);
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            switch(cmd) {
               case F_GETFL:
                  errno_return(tdSocket->Socket.SCTPSocketDesc.Flags);
                 break;
               case F_SETFL:
                   tdSocket->Socket.SCTPSocketDesc.Flags = arg;
                   errno_return(0);
                 break;
               default:
                  errno_return(-EOPNOTSUPP);
                break;
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(fcntl(tdSocket->Socket.SystemSocketID,cmd,arg));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### ioctl() wrapper ###################################################
int ext_ioctl(int sockfd, int request, const void* argp)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            errno_return(-EOPNOTSUPP);
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(ioctl(tdSocket->Socket.SystemSocketID,request,argp));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### Get option ########################################################
static int getFlag(ExtSocketDescriptor* tdSocket,
                   void* optval, socklen_t* optlen, const int flag)
{
   if((*optlen < (int)sizeof(int)) || (optval == NULL)) {
      errno_return(-EINVAL);
   }
   *optlen = sizeof(int);
   if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
      *((int*)optval) = (tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getNotificationFlags() & flag);
      errno_return(0);
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      *((int*)optval) = (tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getNotificationFlags() & flag);
      errno_return(0);
   }
   errno_return(-EBADF);
}


// ###### Get association status ############################################
static int getAssocStatus(ExtSocketDescriptor* tdSocket,
                          void* optval, socklen_t* optlen)
{
   if((*optlen < (int)sizeof(sctp_status)) || (optval == NULL)) {
      errno_return(-EINVAL);
   }
   sctp_status*            status  = (sctp_status*)optval;
   unsigned int            assocID = status->sstat_assoc_id;
   SCTP_Association_Status parameters;

   SCTPSocketMaster::MasterInstance.lock();
   int result = getAssocParams(tdSocket,assocID,parameters);
   if(result == 0) {
      status->sstat_state     = parameters.state;
      status->sstat_rwnd      = parameters.currentReceiverWindowSize;
      status->sstat_unackdata = parameters.noOfChunksInRetransmissionQueue;
      status->sstat_penddata  = parameters.noOfChunksInReceptionQueue;
      status->sstat_instrms   = parameters.inStreams;
      status->sstat_outstrms  = parameters.outStreams;
      status->sstat_fragmentation_point = 0;
      status->sstat_assoc_id            = assocID;
      result = getPrimaryAddressInfo(tdSocket,assocID,status->sstat_primary);
      *optlen = sizeof(sctp_status);
   }
   SCTPSocketMaster::MasterInstance.unlock();
   errno_return(result);
}


// ###### Get RTO info ######################################################
static int getRTOInfo(ExtSocketDescriptor* tdSocket,
                      void* optval, socklen_t* optlen)
{
   if((optval == NULL) || (*optlen < sizeof(sctp_rtoinfo))) {
      errno_return(-EINVAL);
   }
   sctp_rtoinfo*           rtoinfo = (sctp_rtoinfo*)optval;
   unsigned int            assocID = rtoinfo->srto_assoc_id;
   SCTP_Association_Status parameters;

   int result = getAssocParams(tdSocket,assocID,parameters);
   if(result == 0) {
      rtoinfo->srto_initial  = parameters.rtoInitial;
      rtoinfo->srto_max      = parameters.rtoMax;
      rtoinfo->srto_min      = parameters.rtoMin;
      rtoinfo->srto_assoc_id = assocID;
      *optlen = sizeof(sctp_rtoinfo);
   }
   errno_return(result);
}


// ###### Get RTO info ######################################################
static int getAssocInfo(ExtSocketDescriptor* tdSocket,
                          void* optval, socklen_t* optlen)
{
   if((optval == NULL) || (*optlen < sizeof(sctp_assocparams))) {
      errno_return(-EINVAL);
   }
   sctp_assocparams*       assocparams = (sctp_assocparams*)optval;
   unsigned int            assocID     = assocparams->sasoc_assoc_id;
   SCTP_Association_Status parameters;

   int result = getAssocParams(tdSocket,assocID,parameters);
   if(result == 0) {
      assocparams->sasoc_assoc_id                 = assocID;
      assocparams->sasoc_asocmaxrxt               = parameters.assocMaxRetransmits;
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
      assocparams->sasoc_number_peer_destinations = parameters.numberOfAddresses;
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
      assocparams->sasoc_number_peer_destinations = parameters.numberOfDestinationPaths;
#else
#error Wrong sctplib version!
#endif
      assocparams->sasoc_peer_rwnd                = 0;
      assocparams->sasoc_local_rwnd               = parameters.currentReceiverWindowSize;
      assocparams->sasoc_cookie_life              = parameters.validCookieLife;
      *optlen = sizeof(sctp_assocparams);
   }
   errno_return(result);
}


// ###### Get address information ###########################################
static int getPeerAddressInfo(ExtSocketDescriptor* tdSocket,
                              void* optval, socklen_t* optlen)
{
   if((optval == NULL) || (*optlen < sizeof(sctp_paddrinfo))) {
      errno_return(-EINVAL);
   }
   int              result;
   sctp_paddrinfo*  paddrinfo = (sctp_paddrinfo*)optval;
   unsigned int     assocID   = paddrinfo->spinfo_assoc_id;
   SCTP_PathStatus  parameters;

   result = getAddressParams(tdSocket,assocID,
                             (sockaddr*)&paddrinfo->spinfo_address,
                             sizeof(paddrinfo->spinfo_address),
                             parameters);
   if(result == 0) {
      switch(parameters.state) {
         case 1:
            paddrinfo->spinfo_state = SCTP_INACTIVE;
          break;
         default:
            paddrinfo->spinfo_state = SCTP_ACTIVE;
          break;
      }
      paddrinfo->spinfo_cwnd     = parameters.cwnd;
      paddrinfo->spinfo_srtt     = parameters.srtt;
      paddrinfo->spinfo_rto      = parameters.rto;
      paddrinfo->spinfo_mtu      = parameters.mtu;
      paddrinfo->spinfo_assoc_id = assocID;
      *optlen = sizeof(sctp_paddrinfo);
   }
   errno_return(result);
}


// ###### Get address parameters ############################################
static int getPeerAddressParams(ExtSocketDescriptor* tdSocket,
                                void* optval, socklen_t* optlen)
{
   if((optval == NULL) || (*optlen < sizeof(sctp_paddrparams))) {
      errno_return(-EINVAL);
   }
   int               result;
   sctp_paddrparams* paddrparams = (sctp_paddrparams*)optval;
   unsigned int      assocID     = paddrparams->spp_assoc_id;
   SCTP_PathStatus   parameters;

   result = getAddressParams(tdSocket,
                             assocID,
                             (sockaddr*)&paddrparams->spp_address,
                             sizeof(paddrparams->spp_address),
                             parameters);
   if(result == 0) {
      paddrparams->spp_hbinterval = parameters.heartbeatIntervall;
      paddrparams->spp_pathmaxrxt = 0;
      paddrparams->spp_assoc_id = assocID;
      *optlen = sizeof(sctp_paddrparams);
   }
   errno_return(result);
}


// ###### getsockopt() wrapper ##############################################
int ext_getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               switch(level) {
#if (SYSTEM == OS_Linux)
                  // ====== IPv6 ============================================
                  case SOL_IPV6:
                      switch(optname) {
                         case IPV6_FLOWINFO:
                            errno_return(0);
                          break;
                         case IPV6_FLOWINFO_SEND:
                            errno_return(0);
                          break;
                         default:
                            errno_return(-EOPNOTSUPP);
                          break;
                      }
                   break;

                  // ====== IPv4 ============================================
                  case SOL_IP:
                      switch(optname) {
                         case IP_TOS:
                            if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                               errno_return(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getTrafficClass());
                            }
                            errno_return(0);
                          break;
                         case IP_RECVTOS:
                            errno_return(0);
                          break;
                         default:
                            errno_return(-EOPNOTSUPP);
                          break;
                      }
                   break;
#endif

                  // ====== SCTP ============================================
                  case IPPROTO_SCTP:
                      switch(optname) {
                         case SCTP_STATUS:
                            return(getAssocStatus(tdSocket,optval,optlen));
                          break;
                         case SCTP_INITMSG: {
                               if((optval == NULL) || (*optlen < sizeof(sctp_initmsg))) {
                                  errno_return(-EINVAL);
                               }
                               sctp_initmsg* initmsg = (sctp_initmsg*)optval;
                               *initmsg = tdSocket->Socket.SCTPSocketDesc.InitMsg;
                               *optlen = sizeof(sctp_initmsg);
                               errno_return(0);
                            }
                          break;
                         case SCTP_GET_PEER_ADDR_INFO:
                             return(getPeerAddressInfo(tdSocket,optval,optlen));
                          break;
                         case SCTP_GET_PEER_ADDR_PARAMS:
                             return(getPeerAddressParams(tdSocket,optval,optlen));
                          break;
                         case SCTP_RTOINFO: {
                               return(getRTOInfo(tdSocket,optval,optlen));
                            }
                          break;
                         case SCTP_ASSOCINFO:
                            return(getAssocInfo(tdSocket,optval,optlen));
                          break;

                         /* ---- Start of deprecated block ------------------------------ */
                         case SCTP_RECVDATAIOEVNT:
                            return(getFlag(tdSocket,optval,optlen,SCTP_RECVDATAIOEVNT));
                          break;
                         case SCTP_RECVASSOCEVNT:
                            return(getFlag(tdSocket,optval,optlen,SCTP_RECVASSOCEVNT));
                          break;
                         case SCTP_RECVPADDREVNT:
                            return(getFlag(tdSocket,optval,optlen,SCTP_RECVPADDREVNT));
                          break;
                         case SCTP_RECVSENDFAILEVNT:
                            return(getFlag(tdSocket,optval,optlen,SCTP_RECVSENDFAILEVNT));
                          break;
                         case SCTP_RECVPEERERR:
                            return(getFlag(tdSocket,optval,optlen,SCTP_RECVPEERERR));
                          break;
                         case SCTP_RECVSHUTDOWNEVNT:
                            return(getFlag(tdSocket,optval,optlen,SCTP_RECVSHUTDOWNEVNT));
                          break;
                          /* ---- End of deprecated block ------------------------------ */

                         case SCTP_AUTOCLOSE:
                            if((optval == NULL) || (*optlen < sizeof(unsigned int))) {
                               errno_return(-EINVAL);
                            }
                            if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                               *((unsigned int*)optval) =
                                  (unsigned int)(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getAutoClose() /
                                                    1000000);
                               *optlen = sizeof(unsigned int);
                               errno_return(0);
                            }
                            errno_return(-EBADF);
                          break;
                      }
                   break;

                  // ====== Socket ==========================================
                  case SOL_SOCKET:
                      switch(optname) {
                         case SO_SNDBUF:
                            if((optval == NULL) || (*optlen < sizeof(unsigned int))) {
                               errno_return(-EINVAL);
                            }
                            if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                               *((int*)optval) = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getSendBuffer();
                               *optlen = sizeof(unsigned int);
                               errno_return((*((int*)optval) >= 0) ? 0 : -EIO);
                            }
                            errno_return(-EBADF);
                          break;
                         case SO_RCVBUF:
                            if((optval == NULL) || (*optlen < sizeof(unsigned int))) {
                               errno_return(-EINVAL);
                            }
                            if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                               *((int*)optval) = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getReceiveBuffer();
                               *optlen = sizeof(unsigned int);
                               errno_return((*((int*)optval) >= 0) ? 0 : -EIO);
                            }
                            errno_return(-EBADF);
                          break;
                         case SO_LINGER:
                            if((optval == NULL) || (*optlen < sizeof(linger))) {
                               errno_return(-EINVAL);
                            }
                            *((linger*)optval) = tdSocket->Socket.SCTPSocketDesc.Linger;
                            *optlen = sizeof(linger);
                            errno_return(0);
                          break;
                      }
                   break;

                  // ====== Default =========================================
                  default:
                     errno_return(-EOPNOTSUPP);
                   break;
               }
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(getsockopt(tdSocket->Socket.SystemSocketID,level,optname,optval,optlen));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### Set option ########################################################
static int setFlag(ExtSocketDescriptor* tdSocket,
                   const void* optval, const socklen_t optlen, const int flag)
{
   if((optlen != sizeof(int)) || (optval == NULL)) {
      errno_return(-EINVAL);
   }
   const bool on = (*((int*)optval) != 0);
   if(on) {
      if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
         tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->setNotificationFlags(
            tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getNotificationFlags() |
            flag);
         errno_return(0);
      }
      else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
         tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setNotificationFlags(
            tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getNotificationFlags() |
            flag);
         errno_return(0);
      }
   }
   else {
      if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
         tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->setNotificationFlags(
            tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getNotificationFlags() &
            ~flag);
         errno_return(0);
      }
      else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
         tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setNotificationFlags(
            tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getNotificationFlags() &
            ~flag);
         errno_return(0);
      }
   }
   errno_return(-EBADF);
}


// ###### Set events ########################################################
static int setDefaultSendParams(ExtSocketDescriptor* tdSocket,
                                const void* optval, const socklen_t optlen)
{
   if((optlen != sizeof(sctp_sndrcvinfo)) || (optval == NULL)) {
      errno_return(-EINVAL);
   }

   sctp_sndrcvinfo*    sndrcvinfo = (sctp_sndrcvinfo*)optval;
   AssocIODefaults defaults;

   defaults.StreamID   = sndrcvinfo->sinfo_stream;
   defaults.ProtoID    = sndrcvinfo->sinfo_ppid;
   defaults.TimeToLive = sndrcvinfo->sinfo_timetolive;
   defaults.Context    = sndrcvinfo->sinfo_context;

   bool result = false;
   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->setAssocIODefaults(defaults);
      result = true;
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      result = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setAssocIODefaults(sndrcvinfo->sinfo_assoc_id,defaults);
   }

   if(result == true) {
      errno_return(0);
   }
   errno_return(-EINVAL);
}


// ###### Set events ########################################################
static int setDefaultStreamTimeouts(ExtSocketDescriptor* tdSocket,
                                    const void* optval, const socklen_t optlen)
{
   if((optlen != sizeof(sctp_setstrm_timeout)) || (optval == NULL)) {
      errno_return(-EINVAL);
   }

   sctp_setstrm_timeout* timeout = (sctp_setstrm_timeout*)optval;

   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->setDefaultStreamTimeouts(
         timeout->ssto_timeout,
         timeout->ssto_streamid_start,
         timeout->ssto_streamid_end);
      return(0);
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setDefaultStreamTimeouts(
         timeout->ssto_assoc_id,
         timeout->ssto_timeout,
         timeout->ssto_streamid_start,
         timeout->ssto_streamid_end)) {
         return(0);
      }
   }
   errno_return(-EINVAL);
}


// ###### Set events ########################################################
static int setEvents(ExtSocketDescriptor* tdSocket,
                     const void* optval, const socklen_t optlen)
{
   if((optlen != sizeof(sctp_event_subscribe)) || (optval == NULL)) {
      errno_return(-EINVAL);
   }

   unsigned int flags           = 0;
   sctp_event_subscribe* events = (sctp_event_subscribe*)optval;

   if(events->sctp_data_io_event) {
      flags |= SCTP_RECVDATAIOEVNT;
   }
   if(events->sctp_association_event) {
      flags |= SCTP_RECVASSOCEVNT;
   }
   if(events->sctp_address_event) {
      flags |= SCTP_RECVPADDREVNT;
   }
   if(events->sctp_send_failure_event) {
      flags |= SCTP_RECVSENDFAILEVNT;
   }
   if(events->sctp_peer_error_event) {
      flags |= SCTP_RECVPEERERR;
   }
   if(events->sctp_shutdown_event) {
      flags |= SCTP_RECVSHUTDOWNEVNT;
   }
   if(events->sctp_partial_delivery_event) {
      flags |= SCTP_RECVPDEVNT;
   }
   if(events->sctp_adaption_layer_event) {
      flags |= SCTP_RECVADAPIONINDICATION;
   }

   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->setNotificationFlags(flags);
      errno_return(0);
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setNotificationFlags(flags);
      errno_return(0);
   }
   return(-EBADF);
}


// ###### Set RTO info ######################################################
static int setRTOInfo(ExtSocketDescriptor* tdSocket,
                      const void* optval, const socklen_t optlen)
{
   if((optval == NULL) || (optlen < sizeof(sctp_rtoinfo))) {
      errno_return(-EINVAL);
   }
   sctp_rtoinfo*           rtoinfo = (sctp_rtoinfo*)optval;
   unsigned int            assocID = rtoinfo->srto_assoc_id;
   SCTP_Association_Status parameters;

   SCTPSocketMaster::MasterInstance.lock();
   int result = getAssocParams(tdSocket,assocID,parameters);
   if(result == 0) {
      if(rtoinfo->srto_initial != 0) {
         parameters.rtoInitial = rtoinfo->srto_initial;
      }
      if(rtoinfo->srto_min != 0) {
         parameters.rtoMin = rtoinfo->srto_min;
      }
      if(rtoinfo->srto_max != 0) {
         parameters.rtoMax = rtoinfo->srto_max;
      }
      result = setAssocParams(tdSocket,assocID,parameters);
   }
   SCTPSocketMaster::MasterInstance.unlock();

   errno_return(result);
}


// ###### Set association info ##############################################
static int setAssocInfo(ExtSocketDescriptor* tdSocket,
                        const void*          optval,
                        const socklen_t      optlen)
{
   if((optval == NULL) || (optlen < sizeof(sctp_assocparams))) {
      errno_return(-EINVAL);
   }
   sctp_assocparams*       assocparams = (sctp_assocparams*)optval;
   unsigned int            assocID     = assocparams->sasoc_assoc_id;
   SCTP_Association_Status parameters;

   SCTPSocketMaster::MasterInstance.lock();
   int result = getAssocParams(tdSocket,assocID,parameters);
   if(result == 0) {
      parameters.assocMaxRetransmits = assocparams->sasoc_asocmaxrxt;
      result = setAssocParams(tdSocket,assocID,parameters);
   }
   SCTPSocketMaster::MasterInstance.unlock();
   errno_return(result);
}


// ###### Set primary address ###############################################
static int setPrimaryAddr(ExtSocketDescriptor* tdSocket,
                          const void* optval, const socklen_t optlen)
{
   if((optval == NULL) || (optlen < sizeof(sctp_setprim))) {
      errno_return(-EINVAL);
   }
   sctp_setprim* setprim = (sctp_setprim*)optval;
   SocketAddress* address = SocketAddress::createSocketAddress(0,
                               (sockaddr*)&setprim->ssp_addr,
                               sizeof(sockaddr_storage));
   if(address == NULL) {
      errno_return(-EINVAL);
   }

   int result = -EBADF;
   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      result = (tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->
                   setPrimary(*address) == true) ? 0 : -EIO;
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      result = (tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->
                   setPrimary(setprim->ssp_assoc_id,*address) == true) ? 0 : -EIO;
   }

   delete address;
   errno_return(result);
}


// ###### Set primary address ###############################################
static int setPeerPrimaryAddr(ExtSocketDescriptor* tdSocket,
                              const void* optval, const socklen_t optlen)
{
   if((optval == NULL) || (optlen < sizeof(sctp_setpeerprim))) {
      errno_return(-EINVAL);
   }
   sctp_setpeerprim* setpeerprim = (sctp_setpeerprim*)optval;
   SocketAddress* address = SocketAddress::createSocketAddress(0,
                               (sockaddr*)&setpeerprim->sspp_addr,
                               sizeof(sockaddr_storage));
   if(address == NULL) {
      errno_return(-EINVAL);
   }

   int result = -EBADF;
   if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
      result = (tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->
                   setPrimary(*address) == true) ? 0 : -EIO;
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      result = (tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->
                   setPrimary(setpeerprim->sspp_assoc_id,*address) == true) ? 0 : -EIO;
   }

   delete address;
   errno_return(result);
}


// ###### Get address parameters ############################################
static int setPeerAddressParams(ExtSocketDescriptor* tdSocket,
                                const void* optval, const socklen_t optlen)
{
   if((optval == NULL) || (optlen < sizeof(sctp_paddrparams))) {
      errno_return(-EINVAL);
   }
   int               result      = -EBADF;
   sctp_paddrparams* paddrparams = (sctp_paddrparams*)optval;
   unsigned int      assocID     = paddrparams->spp_assoc_id;
   SCTP_PathStatus   parameters;

   SCTPSocketMaster::MasterInstance.lock();
   result = getAddressParams(tdSocket,assocID,
                             (sockaddr*)&paddrparams->spp_address,
                             sizeof(paddrparams->spp_address),
                             parameters);
   if(result == 0) {
      parameters.heartbeatIntervall = paddrparams->spp_hbinterval;
      result = setAddressParams(tdSocket,assocID,
                                (sockaddr*)&paddrparams->spp_address,
                                sizeof(paddrparams->spp_address),
                                parameters);
   }
   SCTPSocketMaster::MasterInstance.unlock();

   errno_return(result);
}


// ###### Get address parameters ############################################
static void setInitMsg(SCTPSocket* sctpSocket, struct sctp_initmsg* initmsg)
{
   SCTP_Instance_Parameters parameters;
   bool                     result = false;

   SCTPSocketMaster::MasterInstance.lock();
   if(sctpSocket->getAssocDefaults(parameters)) {
      parameters.outStreams = initmsg->sinit_num_ostreams;
      parameters.inStreams  = initmsg->sinit_max_instreams;
      if(sctpSocket->setAssocDefaults(parameters)) {
         result = true;
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();
}


// ###### setsockopt() wrapper ##############################################
int ext_setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               switch(level) {
#if (SYSTEM == OS_Linux)
                  // ====== IPv6 ============================================
                  case SOL_IPV6:
                      switch(optname) {
                         case IPV6_FLOWINFO:
                            errno_return(0);
                          break;
                         case IPV6_FLOWINFO_SEND:
                            errno_return(0);
                          break;
                         default:
                            errno_return(-EOPNOTSUPP);
                          break;
                      }
                   break;

                  // ====== IPv4 ============================================
                  case SOL_IP:
                      switch(optname) {
                         case IP_TOS:
                            if((optval == NULL) || (optlen < sizeof(int))) {
                               errno_return(-EINVAL);
                            }
                            if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
                               errno_return((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->setTrafficClass(*((int*)optval)) == true) ? 0 : -EIO);
                            }
                            else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                               errno_return((tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setTrafficClass(*((int*)optval)) == true) ? 0 : -EIO);
                            }
                            errno_return(-EOPNOTSUPP);
                          break;
                         case IP_RECVTOS:
                            errno_return(0);
                          break;
                         default:
                            errno_return(-EOPNOTSUPP);
                          break;
                      }
                   break;
#endif

                  // ====== SCTP ============================================
                  case IPPROTO_SCTP:
                      switch(optname) {
                         case SCTP_INITMSG: {
                               if((optval == NULL) || (optlen < sizeof(sctp_initmsg))) {
                                  errno_return(-EINVAL);
                               }
                               sctp_initmsg* initmsg = (sctp_initmsg*)optval;
                               tdSocket->Socket.SCTPSocketDesc.InitMsg = *initmsg;
                               if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr) {
                                  setInitMsg(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr,
                                                initmsg);
                               }
                               errno_return(0);
                            }
                          break;
                         case SCTP_PRIMARY_ADDR:
                            return(setPrimaryAddr(tdSocket,optval,optlen));
                          break;
                         case SCTP_SET_PEER_PRIMARY_ADDR:
                            return(setPeerPrimaryAddr(tdSocket,optval,optlen));
                          break;
                         case SCTP_PEER_ADDR_PARAMS:
                             return(setPeerAddressParams(tdSocket,optval,optlen));
                          break;
                         case SCTP_RTOINFO:
                            return(setRTOInfo(tdSocket,optval,optlen));
                          break;
                         case SCTP_ASSOCINFO:
                            return(setAssocInfo(tdSocket,optval,optlen));
                          break;
                         case SCTP_EVENTS:
                            return(setEvents(tdSocket,optval,optlen));
                          break;
                         case SCTP_SET_DEFAULT_SEND_PARAM:
                            return(setDefaultSendParams(tdSocket,optval,optlen));
                          break;
                         case SCTP_SET_STREAM_TIMEOUTS:
                            return(setDefaultStreamTimeouts(tdSocket,optval,optlen));
                          break;

                         /* ---- Start of deprecated block ------------------------------ */
                         case SCTP_RECVDATAIOEVNT:
                            return(setFlag(tdSocket,optval,optlen,SCTP_RECVDATAIOEVNT));
                          break;
                         case SCTP_RECVASSOCEVNT:
                            return(setFlag(tdSocket,optval,optlen,SCTP_RECVASSOCEVNT));
                          break;
                         case SCTP_RECVPADDREVNT:
                            return(setFlag(tdSocket,optval,optlen,SCTP_RECVPADDREVNT));
                          break;
                         case SCTP_RECVSENDFAILEVNT:
                            return(setFlag(tdSocket,optval,optlen,SCTP_RECVSENDFAILEVNT));
                          break;
                         case SCTP_RECVPEERERR:
                            return(setFlag(tdSocket,optval,optlen,SCTP_RECVPEERERR));
                          break;
                         case SCTP_RECVSHUTDOWNEVNT:
                            return(setFlag(tdSocket,optval,optlen,SCTP_RECVSHUTDOWNEVNT));
                          break;
                         /* ---- Start of deprecated block ------------------------------ */

                         case SCTP_AUTOCLOSE:
                            if((optval == NULL) || (optlen < sizeof(unsigned int))) {
                               errno_return(-EINVAL);
                            }
                            if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr == NULL) {
                               errno_return(-EBADF);
                            }
                            tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setAutoClose(
                               (card64)1000000 * (card64)*((unsigned int*)optval));
                            errno_return(0);
                          break;
                       }
                     break;

                  // ====== Socket ==========================================
                  case SOL_SOCKET:
                      switch(optname) {
                         case SO_SNDBUF:
                            if((optval == NULL) || (optlen < sizeof(unsigned int))) {
                               errno_return(-EINVAL);
                            }
                            if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
                               errno_return((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->setSendBuffer(*((unsigned int*)optval)) == true) ? 0 : -EIO);
                            }
                            else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                               errno_return((tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setSendBuffer(*((unsigned int*)optval)) == true) ? 0 : -EIO);
                            }
                            errno_return(-EBADF);
                          break;
                         case SO_RCVBUF:
                            if((optval == NULL) || (optlen < sizeof(unsigned int))) {
                               errno_return(-EINVAL);
                            }
                            if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
                               errno_return((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->setSendBuffer(*((unsigned int*)optval)) == true) ? 0 : -EIO);
                            }
                            else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                               errno_return((tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->setReceiveBuffer(*((unsigned int*)optval)) == true) ? 0 : -EIO);
                            }
                            errno_return(-EBADF);
                          break;
                         case SO_LINGER:
                            if((optval == NULL) || (optlen < sizeof(linger))) {
                               errno_return(-EINVAL);
                            }
                            if( (((linger*)optval)->l_linger < 0) ||
                                (((linger*)optval)->l_onoff < 0)  ||
                                (((linger*)optval)->l_onoff > 1) ) {
                               errno_return(-EINVAL);
                            }
                            tdSocket->Socket.SCTPSocketDesc.Linger = *((linger*)optval);
                            errno_return(0);
                          break;
                      }
                   break;

                  // ====== Default =========================================
                  default:
                     errno_return(-EOPNOTSUPP);
                   break;
               }
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(setsockopt(tdSocket->Socket.SystemSocketID,level,optname,optval,optlen));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### shutdown() wrapper ################################################
int ext_shutdown(int sockfd, int how)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
               tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->shutdown();
               errno_return(0);
            }
            errno_return(-EOPNOTSUPP);
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(shutdown(tdSocket->Socket.SystemSocketID, how));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### connect() wrapper #################################################
int ext_connect(int sockfd, const struct sockaddr* serv_addr, socklen_t addrlen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      if(tdSocket->Type == ExtSocketDescriptor::ESDT_SCTP) {
         struct sockaddr_storage addressArray[1];
         memcpy((char*)&addressArray[0], serv_addr, min(sizeof(sockaddr_storage), addrlen));
         return(ext_connectx(sockfd, (sockaddr_storage*)&addressArray, 1));
      }
      else {
         return(connect(tdSocket->Socket.SystemSocketID, serv_addr, addrlen));
      }
   }
   errno_return(-EBADF);
}


// ###### connectx() wrapper ################################################
int ext_connectx(int                      sockfd,
                 struct sockaddr_storage* addrs,
                 int                      addrcnt)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19)
               if(addrcnt != 1) {
#ifndef DISABLE_WARNINGS
                  cerr << "ERROR: connectx() with more than one address requires sctplib 1.0.0 or better!" << endl;
#endif
                  errno_return(-EOPNOTSUPP);
               }
#endif
               bindToAny(tdSocket);
               if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                  errno_return(-EISCONN);
               }
               if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                  const SocketAddress* addressArray[addrcnt + 1];
                  for(int i = 0;i < addrcnt;i++) {
                     addressArray[i] = SocketAddress::createSocketAddress(
                                          0, (sockaddr*)&addrs[i], sizeof(sockaddr_storage));
                     if(addressArray[i] == NULL) {
                        for(int j = i - 1;j > 0;j--) {
                           delete addressArray[j];
                           errno_return(-EINVAL);
                        }
                     }
                  }
                  addressArray[addrcnt] = NULL;

                  tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr =
                     tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->associate(
                        tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_num_ostreams,
                        tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_max_attempts,
                        tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_max_init_timeo,
                        (const SocketAddress**)&addressArray,
                        !(tdSocket->Socket.SCTPSocketDesc.Flags & O_NONBLOCK));

                  for(int i = 0;i < addrcnt;i++) {
                     delete addressArray[i];
                     addressArray[i] = NULL;
                  }
                  if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr == NULL) {
                     errno_return(-EIO);
                  }
                  else if(tdSocket->Socket.SCTPSocketDesc.Flags & O_NONBLOCK) {
                     errno_return(-EINPROGRESS);
                  }
                  errno_return(0);
               }
               errno_return(-EBADF);
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            errno_return(-EOPNOTSUPP);
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### recv() wrapper ####################################################
int ext_recv(int sockfd, void* buf, size_t len, int flags)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               socklen_t fromlen = 0;
               return(ext_recvfrom(sockfd,buf,len,flags,NULL,&fromlen));
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(recv(tdSocket->Socket.SystemSocketID,buf,len,flags));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### recvfrom() wrapper ################################################
int ext_recvfrom(int sockfd, void* buf, size_t len, int flags,
                struct sockaddr* from, socklen_t* fromlen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               struct iovec  iov = { (char*)buf, len };
               char          cbuf[1024];
               struct msghdr msg = {
#if (SYSTEM == OS_Darwin)
                  (char*)from,
#else
                  from,
#endif
                  (fromlen != NULL) ? *fromlen : 0,
                  &iov, 1,
                  cbuf, sizeof(cbuf),
                  flags
               };
               int cc = ext_recvmsg2(sockfd,&msg,flags,0);
               if(fromlen != NULL) {
                  *fromlen = msg.msg_namelen;
               }
               return(cc);
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            const int result = recvfrom(tdSocket->Socket.SystemSocketID,buf,len,flags,from,fromlen);
            return(result);
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### recvmsg() wrapper #################################################
static int ext_recvmsg_singlebuffer(int sockfd, struct msghdr* msg, int flags,
                                    const int receiveNotifications)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      unsigned int     assocID            = 0;
      unsigned short   streamID           = 0;
      unsigned int     protoID            = 0;
      uint16_t         ssn                = 0;
      uint32_t         tsn                = 0;
      unsigned int     notificationFlags  = 0;
      SocketAddress**  remoteAddressArray = NULL;
      SCTPNotification notification;
      int result = -EOPNOTSUPP;
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
               if((msg == NULL) || (msg->msg_iov == NULL)) {
                  errno_return(-EINVAL);
               }
               msg->msg_flags |= flags;
#ifndef NON_RECVMSG_NOTIFICATIONS
               if(receiveNotifications) {
                  msg->msg_flags |= MSG_NOTIFICATION;
               }
               else {
                  msg->msg_flags &= ~MSG_NOTIFICATION;
               }
#else
               msg->msg_flags |= MSG_NOTIFICATION;
#endif
               if(tdSocket->Socket.SCTPSocketDesc.Flags & O_NONBLOCK) {
                  msg->msg_flags |= MSG_DONTWAIT;
               }
               if((tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) && (tdSocket->Socket.SCTPSocketDesc.ConnectionOriented)) {
                  const size_t oldSize = msg->msg_iov->iov_len;
                  do {
                     msg->msg_iov->iov_len = oldSize;
                     result = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->receive(
                                 (char*)msg->msg_iov->iov_base,
                                 msg->msg_iov->iov_len,
                                 msg->msg_flags,
                                 streamID, protoID,
                                 ssn, tsn);
                  } while((result == -EAGAIN) && !(msg->msg_flags & MSG_DONTWAIT));
                  notificationFlags = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getNotificationFlags();
                  assocID = tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getID();
               }
               else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                  const size_t oldSize = msg->msg_iov->iov_len;
                  do {
                     if(remoteAddressArray != NULL) {
                        SocketAddress::deleteAddressList(remoteAddressArray);
                     }
                     msg->msg_iov->iov_len = oldSize;
                     result = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->receiveFrom(
                                 (char*)msg->msg_iov->iov_base,
                                 msg->msg_iov->iov_len,
                                 msg->msg_flags,
                                 assocID, streamID, protoID,
                                 ssn, tsn,
                                 &remoteAddressArray,
                                 notification);
                  } while((result == -EAGAIN) && !(msg->msg_flags & MSG_DONTWAIT));
                  notificationFlags = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getNotificationFlags();
               }
               else {
                  result = -EBADF;
               }

               // ====== Set result to length in case of success ============
               if(result >= 0) {
                  result = msg->msg_iov->iov_len;
               }
           break;
         case ExtSocketDescriptor::ESDT_System: {
               const int result = recvmsg(tdSocket->Socket.SystemSocketID,msg,flags);
               return(result);
            }
           break;
         default:
            errno_return(-ENXIO);
          break;
      }

      if(result >= 0) {
         if((msg->msg_name != NULL)      &&
            (remoteAddressArray != NULL) &&
            (remoteAddressArray[0] != NULL)) {
            msg->msg_namelen = remoteAddressArray[0]->getSystemAddress(
               (sockaddr*)msg->msg_name,
               (socklen_t)msg->msg_namelen,
               tdSocket->Socket.SCTPSocketDesc.Domain);
         }
         else {
            msg->msg_namelen = 0;
         }
         if((notificationFlags & SCTP_RECVDATAIOEVNT) &&
            (msg->msg_control != NULL) &&
            (msg->msg_controllen >= CSpace(sizeof(sctp_sndrcvinfo)))) {
            cmsghdr* cmsg = (cmsghdr*)msg->msg_control;
            cmsg->cmsg_len   = CSpace(sizeof(sctp_sndrcvinfo));
            cmsg->cmsg_level = IPPROTO_SCTP;
            cmsg->cmsg_type  = SCTP_SNDRCV;
            sctp_sndrcvinfo* info = (sctp_sndrcvinfo*)((long)cmsg + (long)sizeof(cmsghdr));
            info->sinfo_stream     = streamID;
            info->sinfo_ssn        = ssn;
            info->sinfo_tsn        = tsn;
            info->sinfo_flags      = flags;
            info->sinfo_ppid       = protoID;
            info->sinfo_timetolive = 0;
            info->sinfo_context    = 0;
            info->sinfo_cumtsn     = 0;
            info->sinfo_assoc_id   = assocID;
            msg->msg_controllen = CSpace(sizeof(sctp_sndrcvinfo));
         }
         else {
            msg->msg_control    = NULL;
            msg->msg_controllen = 0;
         }
      }
      else {
         msg->msg_namelen    = 0;
         msg->msg_name       = NULL;
         msg->msg_controllen = 0;
         msg->msg_control    = NULL;
      }

      SocketAddress::deleteAddressList(remoteAddressArray);

      errno_return(result);
   }
   errno_return(-EBADF);
}


// ###### recvmsg() wrapper #################################################
int ext_recvmsg2(int sockfd, struct msghdr* msg, int flags,
                 const int receiveNotifications)
{
   struct iovec* iov   = msg->msg_iov;
   const size_t  count = msg->msg_iovlen;
   int result = 0;
   for(unsigned int i = 0;i < count;i++) {
      msg->msg_iov    = (iovec*)((long)iov + ((long)i * (long)sizeof(iovec)));
      msg->msg_iovlen = 1;
      const int subresult = ext_recvmsg_singlebuffer(sockfd,msg,flags,receiveNotifications);
      if(subresult > 0) {
         result += subresult;
      }
      if((result == 0) && (subresult <= 0)) {
         result = subresult;
         break;
      }
      if((subresult < (int)msg->msg_iov->iov_len) && (msg->msg_flags & MSG_EOR)) {
         break;
      }
   }
   msg->msg_iov    = iov;
   msg->msg_iovlen = count;
   return(result);
}


// ###### recvmsg() wrapper #################################################
int ext_recvmsg(int sockfd, struct msghdr* msg, int flags)
{
   return(ext_recvmsg2(sockfd,msg,flags,1));
}


// ###### send() wrapper ####################################################
int ext_send(int sockfd, const void* msg, size_t len, int flags)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            return(ext_sendto(sockfd,msg,len,flags,NULL,0));
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(send(tdSocket->Socket.SystemSocketID,msg,len,flags));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### sendto() wrapper ####################################################
int ext_sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* to, socklen_t tolen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               struct iovec  iov = { (char*)buf, len };
               struct msghdr msg = {
#if (SYSTEM == OS_Darwin)
                  (char*)to,
#else
                  (void*)to,
#endif
                  tolen,
                  &iov, 1,
                  NULL, 0,
                  flags
               };
               return(ext_sendmsg(sockfd,&msg,flags));
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(sendto(tdSocket->Socket.SystemSocketID,buf,len,flags,to,tolen));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### sendmsg() wrapper #################################################
static int ext_sendmsg_singlebuffer(int sockfd, const struct msghdr* msg, int flags)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               if(msg == NULL) {
                  return(-EINVAL);
               }
               bindToAny(tdSocket);
               bool             useDefaults = true;
               sctp_sndrcvinfo* info        = NULL;
               for(cmsghdr* cmsg = CFirst(msg);
                  cmsg != NULL;
                  cmsg = CNext(msg,cmsg)) {
                  if(cmsg->cmsg_level == IPPROTO_SCTP) {
                     if(cmsg->cmsg_type == SCTP_SNDRCV) {
                        if(cmsg->cmsg_len >= sizeof(sctp_sndrcvinfo)) {
                           info        = (sctp_sndrcvinfo*)CData(cmsg);
                           useDefaults = false;
                        }
                        else {
                           errno_return(-EINVAL);
                        }
                     }
                     else if(cmsg->cmsg_type == SCTP_INIT) {
                        const sctp_initmsg* initmsg = (sctp_initmsg*)CData(cmsg);
                        tdSocket->Socket.SCTPSocketDesc.InitMsg = *initmsg;
                        if((tdSocket->Socket.SCTPSocketDesc.ConnectionOriented) && (msg->msg_name != NULL)) {
                           if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                              errno_return(-EBUSY);
                           }
                           const SocketAddress* addressArray[2];
                           addressArray[0] = SocketAddress::createSocketAddress(
                                                0, (sockaddr*)msg->msg_name, msg->msg_namelen);
                           addressArray[1] = NULL;
                           if(addressArray[0] != NULL) {
                              tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr =
                                 tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->associate(
                                    initmsg->sinit_num_ostreams,
                                    initmsg->sinit_max_attempts,
                                    initmsg->sinit_max_init_timeo,
                                    (const SocketAddress**)&addressArray,
                                    !(tdSocket->Socket.SCTPSocketDesc.Flags & O_NONBLOCK));
                              delete addressArray[0];
                              addressArray[0] = NULL;
                              if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr == NULL) {
                                 errno_return(-ENOTCONN);
                              }
                           }
                           else {
                              errno_return(-EADDRNOTAVAIL);
                           }
                        }
                     }
                  }
               }

               flags |= msg->msg_flags;
               if(info != NULL) {
                  flags |= info->sinfo_flags;
               }
               if(tdSocket->Socket.SCTPSocketDesc.Flags & O_NONBLOCK) {
                  flags |= MSG_DONTWAIT;
               }
               if(msg->msg_name != NULL) {
                  SocketAddress* destination = SocketAddress::createSocketAddress(
                                                  0,(sockaddr*)msg->msg_name,msg->msg_namelen);
                  if(destination == NULL) {
                     errno_return(-EADDRNOTAVAIL);
                  }
                  int result = -EBADF;
                  if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                     result = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->sendTo(
                                 (char*)msg->msg_iov->iov_base,
                                 msg->msg_iov->iov_len,
                                 flags,
                                 (info != NULL) ? info->sinfo_assoc_id   : 0,
                                 (info != NULL) ? info->sinfo_stream     : 0x0000,
                                 (info != NULL) ? info->sinfo_ppid       : 0x00000000,
                                 ((info != NULL) && (info->sinfo_flags & MSG_PR_SCTP_TTL)) ? info->sinfo_timetolive : SCTP_INFINITE_LIFETIME,
                                 tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_max_attempts,
                                 tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_max_init_timeo,
                                 useDefaults,
                                 destination,
                                 tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_num_ostreams);
                  }
                  delete destination;
                  errno_return(result);
               }
               else {
                  if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                     errno_return(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->send(
                                  (char*)msg->msg_iov->iov_base,
                                  msg->msg_iov->iov_len,
                                  flags,
                                  (info != NULL) ? info->sinfo_stream     : 0x0000,
                                  (info != NULL) ? info->sinfo_ppid       : 0x0000000,
                                  ((info != NULL) && (info->sinfo_flags & MSG_PR_SCTP_TTL)) ? info->sinfo_timetolive : SCTP_INFINITE_LIFETIME,
                                  useDefaults));
                  }
                  else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                     errno_return(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->sendTo(
                                 (char*)msg->msg_iov->iov_base,
                                 msg->msg_iov->iov_len,
                                 flags,
                                 (info != NULL) ? info->sinfo_assoc_id   : 0,
                                 (info != NULL) ? info->sinfo_stream     : 0x0000,
                                 (info != NULL) ? info->sinfo_ppid       : 0x00000000,
                                 ((info != NULL) && (info->sinfo_flags & MSG_PR_SCTP_TTL)) ? info->sinfo_timetolive : SCTP_INFINITE_LIFETIME,
                                 tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_max_attempts,
                                 tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_max_init_timeo,
                                 useDefaults,
                                 NULL,
                                 tdSocket->Socket.SCTPSocketDesc.InitMsg.sinit_num_ostreams));
                  }
                  errno_return(-EBADF);
               }
               errno_return(0);
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            return(sendmsg(tdSocket->Socket.SystemSocketID,msg,flags));
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### sendmsg() wrapper #################################################
int ext_sendmsg(int sockfd, const struct msghdr* msg, int flags)
{
   struct iovec* iov    = msg->msg_iov;
   const size_t  count  = msg->msg_iovlen;

   if(count > 1) {
      size_t required = 0;
      for(unsigned int i = 0;i < count;i++) {
         const iovec* subvec = (iovec*)((long)iov + ((long)i * (long)sizeof(iovec)));
         required += subvec->iov_len;
      }

      char* buffer = new char[required];
      if(buffer == NULL) {
         return(-ENOMEM);
      }
      unsigned int j = 0;
      for(unsigned int i = 0;i < count;i++) {
         const iovec* subvec = (iovec*)((long)iov + ((long)i * (long)sizeof(iovec)));
         const char*  base   = (char*)subvec->iov_base;
         for(unsigned int k = 0;k < subvec->iov_len;k++) {
            buffer[j++] = base[k];
         }
      }

      iovec newvec;
      newvec.iov_len  = required;
      newvec.iov_base = buffer;

      msghdr newmsg;
      newmsg.msg_control    = msg->msg_control;
      newmsg.msg_controllen = msg->msg_controllen;
      newmsg.msg_name       = msg->msg_name;
      newmsg.msg_namelen    = msg->msg_namelen;
      newmsg.msg_iov        = &newvec;
      newmsg.msg_iovlen     = 1;
      newmsg.msg_flags      = msg->msg_flags;
      const int result = ext_sendmsg_singlebuffer(sockfd,&newmsg,flags);

      delete buffer;
      return(result);
   }
   else {
      return(ext_sendmsg_singlebuffer(sockfd,msg,flags));
   }
}


// ###### read() wrapper ####################################################
ssize_t ext_read(int fd, void* buf, size_t count)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(fd);
   if(tdSocket != NULL) {
      if(tdSocket->Type == ExtSocketDescriptor::ESDT_System) {
         return(read(tdSocket->Socket.SystemSocketID,buf,count));
      }
      else {
         return(ext_recv(fd,buf,count,0));
      }
   }
   errno_return(-EBADF);
}


// ###### write() wrapper ####################################################
ssize_t ext_write(int fd, const void* buf, size_t count)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(fd);
   if(tdSocket != NULL) {
      if(tdSocket->Type == ExtSocketDescriptor::ESDT_System) {
         return(write(tdSocket->Socket.SystemSocketID,buf,count));
      }
      else {
         return(ext_send(fd,buf,count,0));
      }
   }
   errno_return(-EBADF);
}


// Internal structure for ext_select().
struct SelectData
{
   cardinal   Conditions;
   int        ConditionFD[FD_SETSIZE];
   cardinal   ConditionType[FD_SETSIZE];
   Condition* ConditionArray[FD_SETSIZE];
   Condition* ParentConditionArray[FD_SETSIZE];
   Condition  GlobalCondition;
   Condition  ReadCondition;
   Condition  WriteCondition;
   Condition  ExceptCondition;
   cardinal   UserCallbacks;
   int        UserCallbackFD[FD_SETSIZE];
   SCTPSocketMaster::UserSocketNotification* UserNotification[FD_SETSIZE];
};


// ###### Add file descriptor to SelectData structure #######################
static int collectSCTP_FDs(SelectData&                 selectData,
                           const int                   fd,
                           struct ExtSocketDescriptor* tdSocket,
                           const UpdateConditionType   type,
                           Condition&                  condition)
{
   selectData.UserCallbackFD[selectData.UserCallbacks] = fd;
   if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
      selectData.ConditionArray[selectData.Conditions] =
         tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getUpdateCondition(type);

#ifdef PRINT_SELECT
      if(selectData.ConditionArray[selectData.Conditions]->peekFired()) {
         cout << "collectSCTP_FDs: condition already fired" << endl;
      }
#endif

      selectData.ParentConditionArray[selectData.Conditions] = &condition;
      selectData.ConditionArray[selectData.Conditions]->addParent(selectData.ParentConditionArray[selectData.Conditions]);
      selectData.ConditionFD[selectData.Conditions]   = fd;
      selectData.ConditionType[selectData.Conditions] = type;
      selectData.Conditions++;
      return(0);
   }
   else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
      selectData.ConditionArray[selectData.Conditions] =
         tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getUpdateCondition(type);
      if((type == UCT_Write) &&
         (tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr == NULL)) {
         selectData.ConditionArray[selectData.Conditions]->signal();
#ifdef PRINT_SELECT
         cout << "collectSCTP_FDs: UDP-like sockets are always writable" << endl;
#endif
      }

#ifdef PRINT_SELECT
      if(selectData.ConditionArray[selectData.Conditions]->peekFired()) {
         cout << "collectSCTP_FDs: condition already fired" << endl;
      }
#endif

      selectData.ParentConditionArray[selectData.Conditions] = &condition;
      selectData.ConditionArray[selectData.Conditions]->addParent(selectData.ParentConditionArray[selectData.Conditions]);
      selectData.ConditionFD[selectData.Conditions]   = fd;
      selectData.ConditionType[selectData.Conditions] = type;
      selectData.Conditions++;
      return(0);
   }
   else {
      return(-EBADF);
   }
}


// ###### Add file descriptor to SelectData structure #######################
static int collectFDs(SelectData&     selectData,
                      const int       fd,
                      const short int eventMask)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(fd);
   if(tdSocket != NULL) {
      if(tdSocket->Type == ExtSocketDescriptor::ESDT_Invalid) {
#ifndef DISABLE_WARNINGS
         cerr << "WARNING: Invalid socket FD given for collectFDs(): " << fd << "!" << endl;
#endif
      }
      else if(tdSocket->Type == ExtSocketDescriptor::ESDT_System) {
#ifdef PRINT_SELECT
         char str[32];
         snprintf((char*)&str,sizeof(str),"$%04x",eventMask);
         cout << "select(" << getpid() << "): adding FD " << tdSocket->Socket.SystemSocketID
              << " (" << fd << ") with mask " << str << endl;
#endif
         selectData.UserCallbackFD[selectData.UserCallbacks] = fd;
         selectData.UserNotification[selectData.UserCallbacks] = new SCTPSocketMaster::UserSocketNotification;
         if(selectData.UserNotification[selectData.UserCallbacks] != NULL) {
            selectData.UserNotification[selectData.UserCallbacks]->FileDescriptor = tdSocket->Socket.SystemSocketID;
            selectData.UserNotification[selectData.UserCallbacks]->EventMask      = eventMask;
#ifdef PRINT_SELECT
            cout << "select(" << getpid() << "): registering user callback for socket "
                 << tdSocket->Socket.SystemSocketID << "..." << endl;
#endif
            if(eventMask & POLLIN) {
               selectData.UserNotification[selectData.UserCallbacks]->UpdateCondition.addParent(&selectData.ReadCondition);
            }
            if(eventMask & POLLOUT) {
               selectData.UserNotification[selectData.UserCallbacks]->UpdateCondition.addParent(&selectData.WriteCondition);
            }
            if(eventMask & POLLERR) {
               selectData.UserNotification[selectData.UserCallbacks]->UpdateCondition.addParent(&selectData.ExceptCondition);
            }
            SCTPSocketMaster::MasterInstance.addUserSocketNotification(selectData.UserNotification[selectData.UserCallbacks]);

            selectData.UserCallbacks++;
            return(0);
         }
      }
      else {
         int result = 0;
         if(eventMask & POLLIN) {
#ifdef PRINT_SELECT
            cout << "select(" << getpid() << "): adding FD " << fd << " for read" << endl;
#endif
            result = collectSCTP_FDs(selectData,fd,tdSocket,UCT_Read,selectData.ReadCondition);
         }
         if(eventMask & POLLOUT) {
#ifdef PRINT_SELECT
            cout << "select(" << getpid() << "): adding FD " << fd << " for write" << endl;
#endif
            result = collectSCTP_FDs(selectData,fd,tdSocket,UCT_Write,selectData.WriteCondition);
         }
         if(eventMask & POLLERR) {
#ifdef PRINT_SELECT
            cout << "select(" << getpid() << "): adding FD " << fd << " for except" << endl;
#endif
            result = collectSCTP_FDs(selectData,fd,tdSocket,UCT_Except,selectData.ExceptCondition);
         }
         errno_return(result);
      }
   }
   errno_return(-EBADF);
}


// ###### select() wrapper ##################################################
static int select_wrapper(int             n,
                          fd_set*         readfds,
                          fd_set*         writefds,
                          fd_set*         exceptfds,
                          struct timeval* timeout)
{
   fd_set r;
   fd_set w;
   fd_set e;
   FD_ZERO(&r);
   FD_ZERO(&w);
   FD_ZERO(&e);
   int maxFD = 0;
   int reverseMapping[ExtSocketDescriptorMaster::MaxSockets];
   for(unsigned int i = 0;i < min((const unsigned int)n,(const unsigned int)FD_SETSIZE);i++) {
      if(SAFE_FD_ISSET(i,readfds) || SAFE_FD_ISSET(i,writefds) || SAFE_FD_ISSET(i,exceptfds)) {
         ExtSocketDescriptor* socket = ExtSocketDescriptorMaster::getSocket(i);
         if((socket != NULL) && (socket->Type == ExtSocketDescriptor::ESDT_System)) {
            const int fd = socket->Socket.SystemSocketID;
            maxFD = max(maxFD,fd);

            if(SAFE_FD_ISSET(i,readfds)) {
               FD_SET(fd,&r);
            }
            if(SAFE_FD_ISSET(i,writefds)) {
               FD_SET(fd,&w);
            }
            if(SAFE_FD_ISSET(i,exceptfds)) {
               FD_SET(fd,&e);
            }
#ifdef PRINT_SELECT
            cout << "select(" << getpid() << "): added FD " << fd << " (" << i << ")" << endl;
#endif
            reverseMapping[fd] = i;
         }
         else {
#ifndef DISABLE_WARNINGS
            cerr << "ERROR: select_wrapper() - Bad file descriptor " << i << "!" << endl;
#endif
         }
      }
   }

#ifdef PRINT_SELECT
   cout << "select..." << endl;
#endif
   const int result = select(maxFD + 1,&r,&w,&e,timeout);
#ifdef PRINT_SELECT
   cout << "select result " << result << endl;
#endif

   if(result >= 0) {
      SAFE_FD_ZERO(readfds);
      SAFE_FD_ZERO(writefds);
      SAFE_FD_ZERO(exceptfds);
      for(int i = 0;i <= maxFD;i++) {
         if(SAFE_FD_ISSET(i,&r)) {
#ifdef PRINT_SELECT
            cout << "select(" << getpid() << "): read for FD " << reverseMapping[i] << endl;
#endif
            FD_SET(reverseMapping[i],readfds);
         }
         if(SAFE_FD_ISSET(i,&w)) {
#ifdef PRINT_SELECT
            cout << "select(" << getpid() << "): write for FD " << reverseMapping[i] << endl;
#endif
            FD_SET(reverseMapping[i],writefds);
         }
         if(SAFE_FD_ISSET(i,&e)) {
#ifdef PRINT_SELECT
            cout << "select(" << getpid() << "): except for FD " << reverseMapping[i] << endl;
#endif
            FD_SET(reverseMapping[i],exceptfds);
         }
      }
   }

   return(result);
}


// ###### ext_select() implementation #######################################
int ext_select(int             n,
               fd_set*         readfds,
               fd_set*         writefds,
               fd_set*         exceptfds,
               struct timeval* timeout)
{
   if(!SCTPSocketMaster::MasterInstance.running()) {
      // SCTP is not available -> Use select() wrapper for conversion
      // of FDs to system FDs and back.
      return(select_wrapper(n,readfds,writefds,exceptfds,timeout));
   }

   SCTPSocketMaster::MasterInstance.lock();

   SelectData selectData;
   selectData.Conditions    = 0;
   selectData.UserCallbacks = 0;
   selectData.GlobalCondition.setName("ext_select::GlobalCondition");
   selectData.ReadCondition.setName("ext_select::ReadCondition");
   selectData.WriteCondition.setName("ext_select::WriteCondition");
   selectData.ExceptCondition.setName("ext_select::ExceptCondition");
   selectData.ReadCondition.addParent(&selectData.GlobalCondition);
   selectData.WriteCondition.addParent(&selectData.GlobalCondition);
   selectData.ExceptCondition.addParent(&selectData.GlobalCondition);

   int result = 0;
   for(int i = 0;i < min((const int)n,FD_SETSIZE);i++) {
      short int eventMask = 0;
      if(SAFE_FD_ISSET(i,readfds)) {
         eventMask |= POLLIN|POLLPRI;
      }
      if(SAFE_FD_ISSET(i,writefds)) {
         eventMask |= POLLOUT;
      }
      if(SAFE_FD_ISSET(i,exceptfds)) {
         eventMask |= POLLERR;
      }
      if(eventMask) {
         result = collectFDs(selectData,i,eventMask);
         if(result != 0) {
            break;
         }
      }
   }

   if(result == 0) {
      SCTPSocketMaster::MasterInstance.unlock();
      if((selectData.Conditions > 0) || (selectData.UserCallbacks > 0)) {
#ifdef PRINT_SELECT
         cout << "select(" << getpid() << "): waiting..." << endl;
#endif

         if(timeout != NULL) {
            const card64 delay = ((card64)timeout->tv_sec * (card64)1000000) +
                                    (card64)timeout->tv_usec;
            selectData.GlobalCondition.timedWait(delay);
         }
         else {
            selectData.GlobalCondition.wait();
         }

#ifdef PRINT_SELECT
         cout << "select(" << getpid() << "): wake-up" << endl;
#endif
      }
      else {
         select(0,NULL,NULL,NULL,timeout);
      }

      SCTPSocketMaster::MasterInstance.lock();
   }

   if(readfds != NULL) {
      for(cardinal i = 0;i < selectData.Conditions;i++) {
         FD_CLR(selectData.ConditionFD[i],readfds);
      }
   }
   if(writefds != NULL) {
      for(cardinal i = 0;i < selectData.Conditions;i++) {
         FD_CLR(selectData.ConditionFD[i],writefds);
      }
   }
   if(exceptfds != NULL) {
      for(cardinal i = 0;i < selectData.Conditions;i++) {
         FD_CLR(selectData.ConditionFD[i],exceptfds);
      }
   }

   int changes = 0;
   for(cardinal i = 0;i < selectData.Conditions;i++) {
      if(selectData.ConditionArray[i]->fired()) {
         changes++;
         switch(selectData.ConditionType[i]) {
            case UCT_Read:
               if(readfds != NULL) {
#ifdef PRINT_SELECT
                  cout << "select(" << getpid() << "): got read for FD " << selectData.ConditionFD[i] << "." << endl;
#endif
                  FD_SET(selectData.ConditionFD[i],readfds);
               }
             break;
            case UCT_Write:
               if(writefds != NULL) {
#ifdef PRINT_SELECT
                  cout << "select(" << getpid() << "): got write for FD " << selectData.ConditionFD[i] << "." << endl;
#endif
                  FD_SET(selectData.ConditionFD[i],writefds);
               }
             break;
            case UCT_Except:
               if(exceptfds != NULL) {
#ifdef PRINT_SELECT
                  cout << "select(" << getpid() << "): got except for FD " << selectData.ConditionFD[i] << "." << endl;
#endif
                  FD_SET(selectData.ConditionFD[i],exceptfds);
               }
             break;
         }

      }
      selectData.ConditionArray[i]->removeParent(selectData.ParentConditionArray[i]);
   }
   if(readfds != NULL) {
      for(cardinal i = 0;i < selectData.UserCallbacks;i++) {
         FD_CLR(selectData.UserCallbackFD[i],readfds);
      }
   }
   if(writefds != NULL) {
      for(cardinal i = 0;i < selectData.UserCallbacks;i++) {
         FD_CLR(selectData.UserCallbackFD[i],writefds);
      }
   }
   if(exceptfds != NULL) {
      for(cardinal i = 0;i < selectData.UserCallbacks;i++) {
         FD_CLR(selectData.UserCallbackFD[i],exceptfds);
      }
   }
   for(cardinal i = 0;i < selectData.UserCallbacks;i++) {
      SCTPSocketMaster::MasterInstance.deleteUserSocketNotification(selectData.UserNotification[i]);

      if(selectData.UserNotification[i]->Events) {
#ifdef PRINT_SELECT
         char str[32];
         snprintf((char*)&str,sizeof(str),"$%04x",selectData.UserNotification[i]->Events);
         cout << "select(" << getpid() << "): events " << str << " for FD " << selectData.UserNotification[i]->FileDescriptor << " ("
                 << selectData.UserCallbackFD[i] << ")" << endl;
#endif
      }

      bool changed = false;
      if((readfds != NULL) && (selectData.UserNotification[i]->Events & (POLLIN|POLLPRI))) {
#ifdef PRINT_SELECT
         cout << "select(" << getpid() << "): got read for FD " << selectData.UserNotification[i]->FileDescriptor << " ("
              << selectData.UserCallbackFD[i] << ")" << endl;
#endif
         FD_SET(selectData.UserCallbackFD[i],readfds);
         changed = true;
      }
      if((writefds != NULL) && (selectData.UserNotification[i]->Events & POLLOUT)) {
#ifdef PRINT_SELECT
         cout << "select(" << getpid() << "): got write for FD " << selectData.UserNotification[i]->FileDescriptor << " ("
              << selectData.UserCallbackFD[i] << ")" << endl;
#endif
         FD_SET(selectData.UserCallbackFD[i],writefds);
         changed = true;
      }
      if((exceptfds != NULL) && (selectData.UserNotification[i]->Events & (~(POLLIN|POLLPRI|POLLOUT)))) {
#ifdef PRINT_SELECT
         cout << "select(" << getpid() << "): got except for FD " << selectData.UserNotification[i]->FileDescriptor << " ("
              << selectData.UserCallbackFD[i] << ")" << endl;
#endif
         FD_SET(selectData.UserCallbackFD[i],exceptfds);
         changed = true;
      }
      if(changed) {
         changes++;
      }

      delete selectData.UserNotification[i];
   }

   SCTPSocketMaster::MasterInstance.unlock();

   errno_return((result < 0) ? result : changes);
}


// ###### An implementation of poll(), based on ext_select() ################
int ext_poll(struct pollfd* fdlist, long unsigned int count, int time)
{
   // ====== Prepare timeout setting ========================================
   timeval  timeout;
   timeval* to;
   if(time < 0)
      to = NULL;
   else {
      to = &timeout;
      timeout.tv_sec  = time / 1000;
      timeout.tv_usec = (time % 1000) * 1000;
   }

   // ====== Prepare FD settings ============================================
   int    fdcount = 0;
   fd_set readfdset;
   fd_set writefdset;
   fd_set exceptfdset;
   FD_ZERO(&readfdset);
   FD_ZERO(&writefdset);
   FD_ZERO(&exceptfdset);
   for(unsigned int i = 0; i < count; i++) {
      if(fdlist[i].fd < 0) {
         continue;
      }
      if(fdlist[i].events & POLLIN) {
         FD_SET(fdlist[i].fd,&readfdset);
      }
      if(fdlist[i].events & POLLOUT) {
         FD_SET(fdlist[i].fd,&writefdset);
      }
      FD_SET(fdlist[i].fd,&exceptfdset);
      fdcount++;
   }
   if(fdcount == 0) {
      return(0);
   }
   for(unsigned int i = 0;i < count;i++) {
      fdlist[i].revents = 0;
   }

   // ====== Do ext_select() ================================================
   const int tsize  = getdtablesize();
   int       result = ext_select(tsize,&readfdset,&writefdset,&exceptfdset,to);
   if(result < 0) {
      return(result);
   }

   // ====== Set result flags ===============================================
   for(unsigned int i = 0;i < count;i++) {
      if(SAFE_FD_ISSET(fdlist[i].fd,&readfdset) && (fdlist[i].events & POLLIN)) {
         fdlist[i].revents |= POLLIN;
      }
      if(SAFE_FD_ISSET(fdlist[i].fd,&writefdset) && (fdlist[i].events & POLLOUT)) {
         fdlist[i].revents |= POLLOUT;
      }
      if(SAFE_FD_ISSET(fdlist[i].fd,&exceptfdset)) {
         fdlist[i].revents |= POLLERR;
      }
   }
   return(result);
}


// ###### Check, if SCTP is available #######################################
int sctp_isavailable()
{
   return(SCTPSocketMaster::InitializationResult == 0);
}


// ###### Peel association off ##############################################
int sctp_peeloff(int              sockfd,
                 sctp_assoc_t*    id,
                 struct sockaddr* addr,
                 socklen_t*       addrlen)
{
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               SCTPAssociation* association = NULL;
               if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                  if(tdSocket->Socket.SCTPSocketDesc.Type == SOCK_DGRAM) {
                     if(id != NULL) {
                        association = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->peelOff(*id);
                     }
                     else if((addr != NULL) && (*addrlen != NULL)) {
                        SocketAddress* address = SocketAddress::createSocketAddress(
                                                    0,
                                                    (sockaddr*)addr,
                                                    (socklen_t)*addrlen);
                        if(address != NULL) {
                           association = tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->peelOff(*address);
                           delete address;
                        }
                        else {
                           errno_return(-EINVAL);
                        }
                     }
                  }
               }

               if(association != NULL) {
                  ExtSocketDescriptor newExtSocketDescriptor = *tdSocket;
                  newExtSocketDescriptor.Socket.SCTPSocketDesc.SCTPSocketPtr      = NULL;
                  newExtSocketDescriptor.Socket.SCTPSocketDesc.SCTPAssociationPtr = association;
                  newExtSocketDescriptor.Socket.SCTPSocketDesc.Parent             = sockfd;
                  const int newFD = ExtSocketDescriptorMaster::setSocket(newExtSocketDescriptor);
                  if(newFD < 0) {
                     delete newExtSocketDescriptor.Socket.SCTPSocketDesc.SCTPAssociationPtr;
                     newExtSocketDescriptor.Socket.SCTPSocketDesc.SCTPAssociationPtr = NULL;
                  }
                  errno_return(newFD);
               }
               errno_return(-EINVAL);
            }
         default:
            errno_return(-EOPNOTSUPP);
           break;
      }
   }
   return(-EBADF);
}


// ###### sctp_getpaddrs() implementation ###################################
int sctp_getlpaddrs(int                       sockfd,
                    sctp_assoc_t              id,
                    struct sockaddr_storage** addrs,
                    const bool                peerAddresses)
{
   *addrs = NULL;
   ExtSocketDescriptor* tdSocket = ExtSocketDescriptorMaster::getSocket(sockfd);
   if(tdSocket != NULL) {
      switch(tdSocket->Type) {
         case ExtSocketDescriptor::ESDT_SCTP:
            {
               int result = -ENXIO;
               SocketAddress** addressArray = NULL;

               // ====== Get local or peer addresses ========================
               if(peerAddresses) {
                  if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                     if((id != 0) && (tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getID() != id)) {
                        result = -EINVAL;
                     }
                     else {
                        tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getRemoteAddresses(addressArray);
                     }
                  }
                  else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                     tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getRemoteAddresses(addressArray,id);
                  }
                  else {
                     result = -EBADF;
                  }
               }
               else {
                  if(tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr != NULL) {
                     tdSocket->Socket.SCTPSocketDesc.SCTPAssociationPtr->getLocalAddresses(addressArray);
                  }
                  else if(tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr != NULL) {
                     tdSocket->Socket.SCTPSocketDesc.SCTPSocketPtr->getLocalAddresses(addressArray);
                  }
                  else {
                     result = -EBADF;
                  }
               }

               // ====== Copy addresses into sockaddr_storage array =========
               if(addressArray != NULL) {
                  cardinal count = 0;
                  while(addressArray[count] != NULL) {
                     count++;
                  }
                  if(count > 0) {
                     result = (int)count;
                     *addrs = new sockaddr_storage[count];
                     if(*addrs != NULL) {
                        sockaddr* ptr = (sockaddr*)*addrs;
                        for(cardinal i = 0;i < count;i++) {
                           int family = addressArray[i]->getFamily();
                           if(family == AF_INET6) {
                              if(addressArray[i]->getSystemAddress(
                                    ptr,
                                    sizeof(sockaddr_storage),
                                    AF_INET) > 0) {
                                 family = AF_INET;
                              }
                           }
                           if(addressArray[i]->getSystemAddress(
                                 ptr,
                                 sizeof(sockaddr_storage),
                                 family) <= 0) {
                              result = -ENAMETOOLONG;
                              delete *addrs;
                              *addrs = NULL;
                              break;
                           }
                           ptr = (sockaddr*)((long)ptr + (long)sizeof(sockaddr_storage));
                        }
                     }
                     else {
                        result = -ENOMEM;
                     }
                  }
               }

               SocketAddress::deleteAddressList(addressArray);
               errno_return(result);
            }
          break;
         case ExtSocketDescriptor::ESDT_System:
            errno_return(-EOPNOTSUPP);
          break;
      }
      errno_return(-ENXIO);
   }
   errno_return(-EBADF);
}


// ###### sctp_getpaddrs() implementation ###################################
int sctp_getpaddrs(int sockfd, sctp_assoc_t id, struct sockaddr_storage** addrs)
{
   return(sctp_getlpaddrs(sockfd,id,addrs,true));
}


// ###### sctp_freepaddrs() #################################################
void sctp_freepaddrs(struct sockaddr_storage* addrs)
{
   delete [] addrs;
}


// ###### sctp_getladdrs() implementation ###################################
int sctp_getladdrs(int sockfd, sctp_assoc_t id, struct sockaddr_storage** addrs)
{
   return(sctp_getlpaddrs(sockfd,id,addrs,false));
}


// ###### sctp_freeladdrs() #################################################
void sctp_freeladdrs(struct sockaddr_storage* addrs)
{
   delete [] addrs;
}


// ###### sctp_opt_info() implementation ####################################
int sctp_opt_info(int sd, sctp_assoc_t assocID,
                  int opt, void* arg, socklen_t* size)
{
   if((opt == SCTP_RTOINFO)               ||
      (opt == SCTP_ASSOCINFO)             ||
      (opt == SCTP_STATUS)                ||
      (opt == SCTP_GET_PEER_ADDR_INFO)    ||
      (opt == SCTP_GET_PEER_ADDR_PARAMS)) {
         *(sctp_assoc_t *)arg = assocID;
         return(ext_getsockopt(sd,IPPROTO_SCTP,opt,arg,size));
    }
    else if((opt == SCTP_PRIMARY_ADDR)          ||
            (opt == SCTP_SET_PEER_PRIMARY_ADDR) ||
            (opt == SCTP_SET_STREAM_TIMEOUTS)   ||
            (opt == SCTP_PEER_ADDR_PARAMS)) {
       return(ext_setsockopt(sd,IPPROTO_SCTP,opt,arg,*size));
    }
    else {
       errno_return(-EOPNOTSUPP);
    }
}


// ###### sctp_sendmsg() implementation #####################################
int sctp_sendmsg(int              s,
                 void*            data,
                 size_t           len,
                 struct sockaddr* to,
                 socklen_t        tolen,
                 uint32_t         ppid,
                 uint32_t         flags,
                 uint16_t         stream_no,
                 uint32_t         timetolive,
                 uint32_t         context)
{
   sctp_sndrcvinfo* sri;
   struct iovec     iov = { (char*)data, len };
   struct cmsghdr*  cmsg;
   size_t           cmsglen = CMSG_SPACE(sizeof(struct sctp_sndrcvinfo));
   char             cbuf[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
   struct msghdr msg = {
#ifdef __APPLE__
      (char*)to,
#else
      to,
#endif
      tolen,
      &iov, 1,
      cbuf, cmsglen,
      flags
   };

   cmsg = (struct cmsghdr*)CMSG_FIRSTHDR(&msg);
   cmsg->cmsg_len   = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
   cmsg->cmsg_level = IPPROTO_SCTP;
   cmsg->cmsg_type  = SCTP_SNDRCV;

   sri = (struct sctp_sndrcvinfo*)CMSG_DATA(cmsg);
   sri->sinfo_assoc_id   = 0;
   sri->sinfo_stream     = stream_no;
   sri->sinfo_ppid       = ppid;
   sri->sinfo_flags      = flags;
   sri->sinfo_ssn        = 0;
   sri->sinfo_tsn        = 0;
   sri->sinfo_context    = 0;
   sri->sinfo_cumtsn     = 0;
   sri->sinfo_timetolive = timetolive;

   return(ext_sendmsg(s, &msg, 0));
}


// ###### sctp_recvmsg() implementation #####################################
int sctp_recvmsg(int                     s,
                 void*                   data,
                 size_t*                 len,
                 struct sockaddr*        from,
                 socklen_t*              fromlen,
                 struct sctp_sndrcvinfo* sinfo,
                 int*                    msg_flags)
{
   struct iovec    iov = { (char*)data, *len };
   struct cmsghdr* cmsg;
   size_t          cmsglen = CMSG_SPACE(sizeof(struct sctp_sndrcvinfo));
   char            cbuf[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
   struct msghdr msg = {
#ifdef __APPLE__
      (char*)from,
#else
      from,
#endif
      (fromlen != NULL) ? *fromlen : 0,
      &iov, 1,
      cbuf, cmsglen,
      (msg_flags != NULL) ? *msg_flags : 0
   };
   int cc;

   cc = ext_recvmsg(s, &msg, 0);

   *len = iov.iov_len;
   if((cc > 0) && (msg.msg_control != NULL) && (msg.msg_controllen > 0)) {
      cmsg = (struct cmsghdr*)CMSG_FIRSTHDR(&msg);
      if((cmsg != NULL) &&
         (cmsg->cmsg_len   == CMSG_LEN(sizeof(struct sctp_sndrcvinfo))) &&
         (cmsg->cmsg_level == IPPROTO_SCTP)                             &&
         (cmsg->cmsg_type  == SCTP_SNDRCV)) {
         *sinfo = *((struct sctp_sndrcvinfo*)CMSG_DATA(cmsg));
      }
   }
   if(msg_flags != NULL) {
      *msg_flags = msg.msg_flags;
   }
   if(fromlen != NULL) {
      *fromlen = msg.msg_namelen;
   }
   return(cc);
}


#else


// ###### Check, if SCTP is available #######################################
int sctp_isavailable()
{
   int result = socket(AF_INET,SOCK_SEQPACKET,IPPROTO_SCTP);
   if(result != -1) {
      close(result);
      errno_return(1);
   }
   else {
#ifdef PRINT_NOSCTP_NOTE
      cerr << "SCTP is unsupported on this host!" << endl;
#endif
      errno_return(0);
   }
}


#endif


// ###### Change OOTB handling ##############################################
int sctp_enableOOTBHandling(const unsigned int enable)
{
   if(SCTPSocketMaster::enableOOTBHandling(enable != 0) == false) {
      return(-EIO);
   }
   return(0);
}


// ###### Change CRC32 usage ################################################
int sctp_enableCRC32(const unsigned int enable)
{
   if(SCTPSocketMaster::enableCRC32(enable != 0) == false) {
      return(-EIO);
   }
   return(0);
}
