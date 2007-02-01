/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2003 by Thomas Dreibholz
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
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de which should be
 * used for any discussion related to this implementation.
 *
 * Contact: discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *          tuexen@fh-muenster.de
 *
 * Purpose: Stream Source and Destination
 *
 */


#include "tdsystem.h"
#include "streamsrcdest.h"
#include "tools.h"



// ###### Constructor #######################################################
StreamSrcDest::StreamSrcDest()
{
   reset();
}


// ###### Reset #############################################################
void StreamSrcDest::reset()
{
   Source.reset();
   Destination.reset();
   FlowLabel    = 0;
   TrafficClass = 0;
   IsValid      = false;
   pad          = 0;
}


// ###### Translate byte order ##############################################
void StreamSrcDest::translate()
{
   FlowLabel = htonl(FlowLabel);
}


// ###### Comparision operator ##############################################
int StreamSrcDest::operator==(const StreamSrcDest& ssd) const
{
   return((IsValid     == ssd.IsValid)     &&
          (Source      == ssd.Source)      &&
          (Destination == ssd.Destination) &&
          (FlowLabel   == ssd.FlowLabel));
}


// ###### Comparision operator ##############################################
int StreamSrcDest::operator!=(const StreamSrcDest& ssd) const
{
   return((IsValid     != ssd.IsValid)     ||
          (Source      != ssd.Source)      ||
          (Destination != ssd.Destination) ||
          (FlowLabel   != ssd.FlowLabel));
}


// ###### Output operator ###################################################
std::ostream& operator<<(std::ostream& os, const StreamSrcDest& ssd)
{
   if(ssd.IsValid) {
      os << "   Source              = " << InternetAddress(ssd.Source) << std::endl;
      os << "   Destination         = " << InternetAddress(ssd.Destination) << std::endl;
      char str[64];
      snprintf((char*)&str,sizeof(str),"$%02x",ssd.TrafficClass);
      os << "   Traffic Class       = " << str << std::endl;
      snprintf((char*)&str,sizeof(str),"$%05x",ssd.FlowLabel);
      os << "   Flow Label          = " << str << std::endl;
   }
   else {
      os << "   (not valid)" << std::endl;
   }
   return(os);
}
