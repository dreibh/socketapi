/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2012 by Thomas Dreibholz
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
 * Purpose: Unix address implementation
 *
 */

#include "tdsystem.h"
#include "strings.h"
#include "unixaddress.h"



// ###### Unix address constructor ##########################################
UnixAddress::UnixAddress()
{
   reset();
}


// ###### Unix address constructor ##########################################
UnixAddress::UnixAddress(const String& name)
{
   init(name);
}


// ###### Unix address constructor ######################################
UnixAddress::UnixAddress(const UnixAddress& address)
{
   init(address);
}


// ###### Unix address constructor ######################################
UnixAddress::~UnixAddress()
{
}


// ###### Unix address constructor ######################################
UnixAddress::UnixAddress(const sockaddr* address, const cardinal length)
{
   setSystemAddress(address,length);
}


// ###### Unix address constructor ######################################
void UnixAddress::init(const UnixAddress& address)
{
   init((char*)&address.Name);
}



// ###### Initialize ####################################################
void UnixAddress::init(const String& name)
{
   Name[0] = 0x00;
   const cardinal length = name.length();
   if(length < MaxNameLength) {
      if(name.left(5) == "unix:") {
         strcpy((char*)&Name,name.mid(5).getData());
         return;
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: UnixAddress::init() - Name too long!" << std::endl;
#endif
   }
}


// ###### Get port ##########################################################
card16 UnixAddress::getPort() const
{
   // UnixAddresses do not use ports...
   return(0);
}


// ###### Set port ##########################################################
void UnixAddress::setPort(const card16 port)
{
   // UnixAddresses do not use ports...
}


// ###### Reset #############################################################
void UnixAddress::reset()
{
   Name[0] = 0x00;
}


// ###### Create duplicate ##################################################
SocketAddress* UnixAddress::duplicate() const
{
   return(new UnixAddress(*this));
}


// ###### Check, if address is valid ########################################
bool UnixAddress::isValid() const
{
   return(!isNull());
}


// ###### Get address family ################################################
integer UnixAddress::getFamily() const
{
   return(AF_UNIX);
}


// ###### Get address string ################################################
String UnixAddress::getAddressString(const cardinal format) const
{
   if(Name[0] == 0x00) {
      return(String("(invalid)"));
   }
   return("unix:" + String((char*)&Name));
}


// ###### Get sockaddr structure from internet address ######################
cardinal UnixAddress::getSystemAddress(sockaddr*       buffer,
                                       const socklen_t length,
                                       const cardinal  type) const
{
   switch(type) {
      case AF_UNSPEC:
      case AF_UNIX: {
         sockaddr_un* address = (sockaddr_un*)buffer;
         if(sizeof(sockaddr_un) <= (size_t)length) {
            address->sun_family = AF_UNIX;
            strncpy((char*)&address->sun_path,(char*)&Name,MaxNameLength);
            return(sizeof(sockaddr_un));
         }
         else {
#ifndef DISABLE_WARNINGS
            std::cerr << "WARNING: UnixAddress::getSystemUnixAddress() - "
                         "Buffer size too low for AF_UNIX!" << std::endl;
#endif
         }
        }
       break;
      default:
#ifndef DISABLE_WARNINGS
         std::cerr << "WARNING: UnixAddress::getSystemUnixAddress() - Unknown type "
                   << type << "!" << std::endl;
#endif
       break;
   }
   return(0);
}


// ###### Initialize internet address from sockaddr structure ###############
bool UnixAddress::setSystemAddress(const sockaddr* address, const socklen_t length)
{
   sockaddr_un* unixAddress = (sockaddr_un*)address;
   switch(unixAddress->sun_family) {
      case AF_UNIX:
         strncpy((char*)&Name,(char*)&unixAddress->sun_path,MaxNameLength);
         Name[MaxNameLength] = 0x00;
         return(true);
       break;
      default:
         reset();
        break;
   }
   return(false);
}
