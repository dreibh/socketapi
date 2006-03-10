/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2006 by Thomas Dreibholz
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
 *          tuexen@fh-muenster.de
 *
 * Purpose: Internet Flow Implementation
 *
 */

#include "tdsystem.h"
#include "internetaddress.h"
#include "internetflow.h"



// ###### Constructor #######################################################
InternetFlow::InternetFlow()
   : InternetAddress()
{
   FlowInfo = 0;
}


// ###### Constructor #######################################################
InternetFlow::InternetFlow(const InternetFlow& flow)
   : InternetAddress(flow)
{
   FlowInfo = flow.FlowInfo;
}


// ###### Constructor #######################################################
InternetFlow::InternetFlow(const InternetAddress& address,
                           const card32           flowLabel,
                           const card8            trafficClass)
   : InternetAddress(address)
{
   FlowInfo = htonl(flowLabel | ((card32)trafficClass << 20));
}


// ###### Reset #############################################################
void InternetFlow::reset()
{
   InternetAddress::reset();
   FlowInfo = 0;
}


// ###### Create duplicate ##################################################
SocketAddress* InternetFlow::duplicate() const
{
   return(new InternetFlow(*this));
}


// ###### Get system address ################################################
cardinal InternetFlow::getSystemAddress(sockaddr*       buffer,
                                        const socklen_t length,
                                        const cardinal  type) const
{
   const cardinal addressLength = InternetAddress::getSystemAddress(buffer,length,type);
   if((addressLength > 0) && (type == AF_INET6)) {
      sockaddr_in6* address  = (sockaddr_in6*)buffer;
      address->sin6_flowinfo = FlowInfo;
   }
   return(addressLength);
}


// ###### Set system address ################################################
bool InternetFlow::setSystemAddress(sockaddr* address, const socklen_t length)
{
   FlowInfo = 0;
   if(InternetAddress::setSystemAddress(address,length)) {
      sockaddr_in6* address6 = (sockaddr_in6*)address;
      if(address6->sin6_family == AF_INET6) {
         FlowInfo = address6->sin6_flowinfo;
      }
      return(true);
   }
   return(false);
}


// ###### Get address string ################################################
String InternetFlow::getAddressString(const cardinal format) const
{
   String result = InternetAddress::getAddressString(format);
   char str[32];
   snprintf((char*)&str,sizeof(str),
            "/$%05x, $%02x",getFlowLabel(),getTrafficClass());
   return(result + (char*)&str);
}
