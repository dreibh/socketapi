/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2017 by Thomas Dreibholz
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
 * Purpose: Socket Address Implementation
 *
 */


#include "tdsystem.h"
#include "socketaddress.h"
#include "internetaddress.h"
#include "unixaddress.h"
#include "ext_socket.h"

#include <sys/socket.h>



// ###### Socket address destructor #######################################
SocketAddress::~SocketAddress()
{
   Format = PF_Default;
}


// ###### Delete list of addresses ##########################################
SocketAddress** SocketAddress::newAddressList(const cardinal entries)
{
   SocketAddress** list = new SocketAddress*[entries + 1];
   if(list == NULL) {
#ifndef DISABLE_WARNINGS
      std::cerr << "SocketAddress::newAddressList() - Out of memory!" << std::endl;
#endif
      return(NULL);
   }

   for(cardinal i = 0;i < entries + 1;i++) {
      list[i] = NULL;
   }
   return(list);
}


// ###### Delete list of addresses ##########################################
void SocketAddress::deleteAddressList(SocketAddress**& addressArray)
{
   if(addressArray != NULL) {
      cardinal i = 0;
      while(addressArray[i] != NULL) {
         delete addressArray[i];
         addressArray[i] = NULL;
         i++;
      }
      delete [] addressArray;
      addressArray = NULL;
   }
}


// ###### Get local address #################################################
SocketAddress* SocketAddress::getLocalAddress(const SocketAddress& peer)
{
   const int family = peer.getFamily();
   SocketAddress* address = createSocketAddress(family);
   if(address != NULL) {
      int sd = ext_socket(family,SOCK_DGRAM,0);
      if(socket >= 0) {
         sockaddr_storage socketAddress;
         socklen_t        socketAddressLength =
                             peer.getSystemAddress((sockaddr*)&socketAddress,SocketAddress::MaxSockLen,
                                                   family);
         if(socketAddressLength > 0) {
            if(ext_connect(sd,(sockaddr*)&socketAddress,socketAddressLength) == 0) {
               if(ext_getsockname(sd,(sockaddr*)&socketAddress,&socketAddressLength) == 0) {
                  address->setSystemAddress((sockaddr*)&socketAddress,socketAddressLength);
                  address->setPort(0);
               }
            }
         }
         ext_close(sd);
      }
   }
   return(address);
}


// ###### Create SocketAddress object #######################################
SocketAddress* SocketAddress::createSocketAddress(const integer family)
{
   SocketAddress* address = NULL;
   switch(family) {
      case AF_INET:
      case AF_INET6:
         address = new InternetAddress();
       break;
      case AF_UNIX:
         address = new UnixAddress();
       break;
      default:
#ifndef DISABLE_WARNINGS
          std::cerr << "ERROR: SocketAddress::createSocketAddress(family) - "
                       "Unknown address family " << family << "!" << std::endl;
#endif
       break;
   }
   return(address);
}


// ###### Create SocketAddress object #######################################
SocketAddress* SocketAddress::createSocketAddress(const cardinal flags,
                                                  const String&  name)
{
   // ====== Try to create InternetAddress object ===========================
   InternetAddress* internetAddress;
   if(flags & PF_HidePort) {
      internetAddress = new InternetAddress(name,0);
   }
   else {
      internetAddress = new InternetAddress(name);
   }
   if(internetAddress == NULL) {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: SocketAddress::createSocketAddress(name) - Out of memory!" << std::endl;
#endif
   }
   if(internetAddress->isValid()) {
      return(internetAddress);
   }
   delete internetAddress;


   // ====== Try to create InternetAddress object ===========================
   UnixAddress* unixAddress = new UnixAddress(name);
   if(unixAddress == NULL) {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: SocketAddress::createSocketAddress(name) - Out of memory!" << std::endl;
#endif
   }
   if(unixAddress->isValid()) {
      return(unixAddress);
   }
   delete unixAddress;


   return(NULL);
}


// ###### Create SocketAddress object with port number ######################
SocketAddress* SocketAddress::createSocketAddress(const cardinal flags,
                                                  const String&  name,
                                                  const card16   port)
{
   // ====== Try to create InternetAddress object ===========================
   InternetAddress* internetAddress = new InternetAddress(name,port);
   if(internetAddress == NULL) {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: SocketAddress::createSocketAddress(name,port) - Out of memory!" << std::endl;
#endif
   }
   if(internetAddress->isValid()) {
      return(internetAddress);
   }
   delete internetAddress;

   return(NULL);
}


// ###### Create SocketAddress object from system's sockaddr structure ######
SocketAddress* SocketAddress::createSocketAddress(const cardinal  flags,
                                                  sockaddr*       address,
                                                  const socklen_t length)
{
   switch(address->sa_family) {
      case AF_INET:
      case AF_INET6: {
            InternetAddress* internetAddress = new InternetAddress(address,length);
            if(internetAddress == NULL) {
#ifndef DISABLE_WARNINGS
               std::cerr << "ERROR: SocketAddress::createSocketAddress(sockaddr) - Out of memory!" << std::endl;
#endif
            }
            if(internetAddress->isValid()) {
               return(internetAddress);
            }
            delete internetAddress;
         }
       break;
      case AF_UNIX: {
            UnixAddress* unixAddress = new UnixAddress(address,length);
            if(unixAddress == NULL) {
#ifndef DISABLE_WARNINGS
               std::cerr << "ERROR: SocketAddress::createSocketAddress(sockaddr) - Out of memory!" << std::endl;
#endif
            }
            if(unixAddress->isValid()) {
               return(unixAddress);
            }
            delete unixAddress;
         }
       break;
      default:
#ifndef DISABLE_WARNINGS
          std::cerr << "ERROR: SocketAddress::createSocketAddress(sockaddr) - "
                       "Unknown address family " << address->sa_family << "!" << std::endl;
#endif
       break;
   }
   return(NULL);
}
