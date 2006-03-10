/*
 *  $Id: portableaddress.h
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
 * Purpose: This file implements a portable address
 *
 */

#ifndef PORTABLEADDRESS_H
#define PORTABLEADDRESS_H


#include "tdsystem.h"



/**
  * Binary representation for a socket address for sending the address over
  * a network. The difference between InternetAddress is that PortableAddress
  * does not contain hidden information on virtual function management, which
  * make network transfer of InternetAddress objects problematic.
  *
  * @short   Portable Internet Address
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
  * @version 1.0
  */            
class PortableAddress
{
   // ====== Comparision operators ==========================================
   public:
   /**
     * Implementation of == operator.
     */
   int operator==(const PortableAddress& address) const;

   /**
     * Implementation of == operator.
     */
   int operator!=(const PortableAddress& address) const;

   /**
     * Implementation of < operator.
     */
   int operator<(const PortableAddress& address) const;

   /**
     * Implementation of <= operator.
     */
   int operator<=(const PortableAddress& address) const;

   /**
     * Implementation of > operator.
     */
   int operator>(const PortableAddress& address) const;

   /**
     * Implementation of >= operator.
     */
   int operator>=(const PortableAddress& address) const;


   // ====== Reset ==========================================================
   /**
     * Reset portable address.
     */
   inline void reset();


   // ====== Address data ===================================================
   public:
   /**
     * Host address in network byte order. IPv4 addresses are converted to
     * IPv4-mapped IPv6 addresses.
     */
   card16 Host[8];

   /**
     * Port number.
     */
   card16 Port;
};


#include "portableaddress.icc"


#endif
