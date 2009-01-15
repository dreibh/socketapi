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
 * Purpose: Packet address implementation
 *
 */

#include "tdsystem.h"
#include "strings.h"
#include "packetaddress.h"


#if (SYSTEM == OS_Linux)



// ###### Packet address constructor ##########################################
PacketAddress::PacketAddress()
{
   reset();
}


// ###### Packet address constructor ##########################################
PacketAddress::PacketAddress(const String& name)
{
   init(name);
}


// ###### Packet address constructor ######################################
PacketAddress::PacketAddress(const PacketAddress& address)
{
   init(address);
}


// ###### Packet address constructor ######################################
PacketAddress::~PacketAddress()
{
}


// ###### Packet address constructor ######################################
PacketAddress::PacketAddress(sockaddr* address, const cardinal length)
{
   setSystemAddress(address,length);
}


// ###### Packet address constructor ######################################
void PacketAddress::init(const PacketAddress& address)
{
   init((char*)&address.Name);
}


// ###### Create duplicate ##################################################
SocketAddress* PacketAddress::duplicate() const
{
   return(new PacketAddress(*this));
}


// ###### Initialize ####################################################
void PacketAddress::init(const String& name)
{
   Name[0] = 0x00;
   const cardinal length = name.length();
   if(length < MaxNameLength) {
      if(name.left(7) == "packet:") {
         strncpy((char*)&Name,name.mid(7).getData(),MaxNameLength);
         Name[MaxNameLength] = 0x00;
         return;
      }
      else {
         strncpy((char*)&Name,name.getData(),MaxNameLength);
         Name[MaxNameLength] = 0x00;
         return;
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: PacketAddress::init() - Name too long!" <<  std::endl;
#endif
   }
}


// ###### Get port ##########################################################
card16 PacketAddress::getPort() const
{
   // PacketAddresses do not use ports...
   return(0);
}


// ###### Set port ##########################################################
void PacketAddress::setPort(const card16 port)
{
   // PacketAddresses do not use ports...
}


// ###### Reset #############################################################
void PacketAddress::reset()
{
   Name[0] = 0x00;
}


// ###### Check, if address is valid ########################################
bool PacketAddress::isValid() const
{
   return(!isNull());
}


// ###### Get address family ################################################
integer PacketAddress::getFamily() const
{
   return(AF_PACKET);
}


// ###### Get address string ################################################
String PacketAddress::getAddressString(const cardinal format) const
{
   if(Name[0] == 0x00) {
      return(String("(invalid)"));
   }
   return("packet:" + String((char*)&Name));
}


// ###### Get sockaddr structure from internet address ######################
cardinal PacketAddress::getSystemAddress(sockaddr*       address,
                                         const socklen_t length,
                                         const cardinal  type) const
{
   switch(type) {
      case AF_UNSPEC:
      case AF_PACKET: {
         if(sizeof(sockaddr) <= length) {
            ifreq* ifr = (ifreq*)&address->sa_data;
            memset((char*)ifr,0,sizeof(ifr));
            strncpy((char*)&ifr->ifr_ifrn.ifrn_name,Name,MaxNameLength);
            address->sa_family = AF_PACKET;
            return(sizeof(sockaddr));
         }
         else {
#ifndef DISABLE_WARNINGS
            std::cerr << "WARNING: PacketAddress::getSystemPacketAddress() - "
                         "Buffer size too low for AF_PACKET!" <<  std::endl;
#endif
         }
        }
       break;
      default:
#ifndef DISABLE_WARNINGS
         std::cerr << "WARNING: PacketAddress::getSystemPacketAddress() - Unknown type "
                   << type << "!" <<  std::endl;
#endif
       break;
   }
   return(0);
}


// ###### Initialize internet address from sockaddr structure ###############
bool PacketAddress::setSystemAddress(sockaddr* address, const socklen_t length)
{
   sockaddr* packetAddress = (sockaddr*)address;
   switch(packetAddress->sa_family) {
      case AF_PACKET: {
            const ifreq* ifr = (ifreq*)&address->sa_data;
            strncpy((char*)&Name,ifr->ifr_ifrn.ifrn_name,MaxNameLength);
            Name[MaxNameLength] = 0x00;
            return(true);
         }
       break;
      default:
         reset();
        break;
   }
   return(false);
}


#endif
