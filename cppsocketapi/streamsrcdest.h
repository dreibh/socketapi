/*
 *  $Id: streamsrcdest.h,v 1.1 2003/05/15 11:35:50 dreibh Exp $
 *
 * SCTP implementation according to RFC 2960.
 * Copyright (C) 1999-2001 by Thomas Dreibholz
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at www.sctp.de which should be used for
 * any discussion related to this implementation.
 *
 * Contact: discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *
 * Purpose: Stream Source and Destination
 *
 */


#ifndef STREAMSRCDEST_H
#define STREAMSRCDEST_H


#include "tdsystem.h"
#include "internetaddress.h"



/**
  * This class is contains source and destination of a stream.
  *
  * @short   Stream Source and Destination
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
  * @version 1.0
  */
class StreamSrcDest
{
   // ====== Constructor ====================================================
   public:
   /**
     * Constructor.
     */
   StreamSrcDest();


   // ====== Byte order translation =========================================
   /**
     * Translate byte order.
     */
   void translate();


   // ====== Reset ==========================================================
   /**
     * Reset.
     */
   void reset();


   // ====== Comparision operators ==========================================
   /**
     * == operator.
     */
   int operator==(const StreamSrcDest& ssd) const;

   /**
     * != operator.
     */
   int operator!=(const StreamSrcDest& ssd) const;


   // ====== Values =========================================================
   public:
   /**
     * Source address of the stream in portable address format.
     */           
   PortableAddress Source;

   /**
     * Destination address of the stream in portable address format.
     */
   PortableAddress Destination;

   /**
     * Flow label for IPv6 support.
     */            
   card32 FlowLabel;

   /**
     * Traffic class.
     */
   card8 TrafficClass;
   
   /**
     * Is this StreamSrcDest valid?
     */
   bool IsValid;
   
   /**
     * Padding for alignment.
     */
   card16 pad;  
};


/**
  * << operator.
  */
ostream& operator<<(ostream& os, const StreamSrcDest& ssd);


#endif

