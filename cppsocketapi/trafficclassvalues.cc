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
 * Purpose: Traffic Class Values
 *
 */


#include "tdsystem.h"
#include "trafficclassvalues.h"



// ###### Traffic class values ##############################################
const card8 TrafficClassValues::TCValues[TrafficClassValues::MaxValues] = {
   46,10,12,14,18,20,22,26,28,30,34,36,38,40,42,0
};


// ###### Traffic class names ###############################################
const char* TrafficClassValues::TCNames[TrafficClassValues::MaxValues] = {
   "EF",
   "AF11","AF12","AF13",
   "AF21","AF22","AF23",
   "AF31","AF32","AF33",
   "AF41","AF42","AF43",
   "TD1", "TD2",
   "BE"
};


// ###### Get index of traffic class ########################################
cardinal TrafficClassValues::getIndexForTrafficClass(const card8 trafficClass)
{
   for(cardinal i = 0;i < TrafficClassValues::MaxValues;i++) {
      if(TCValues[i] == trafficClass) {
         return(i);
      }
   }
   return(MaxValues - 1);
}


// ###### Get name of traffic class #########################################
const char* TrafficClassValues::getNameForTrafficClass(const card8 trafficClass)
{
   for(cardinal i = 0;i < TrafficClassValues::MaxValues;i++) {
      if(TCValues[i] == trafficClass) {
         return(TCNames[i]);
      }
   }
   return(NULL);
}


// ###### Get traffic class for name ########################################
const card16 TrafficClassValues::getTrafficClassForName(const char* name)
{
   for(cardinal i = 0;i < TrafficClassValues::MaxValues;i++) {
      if(!(strcasecmp(TCNames[i],name))) {
         return((card16)TCValues[i]);
      }
   }
   return(0xffff);
}
