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


#ifndef PACKETADDRESS_H
#define PACKETADDRESS_H


#include "tdsystem.h"
#include "strings.h"
#include "socketaddress.h"


#if (SYSTEM == OS_Linux)

#include <linux/if.h>



/**
  * This class manages a packet socket address.
  *
  * @short   Packet Address
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  */
class PacketAddress : virtual public SocketAddress
{
   // ====== Constructors/Destructor ========================================
   public:
   /**
     * Constructor for an empty packet address.
     */
   PacketAddress();

   /**
     * Constructor for an packet address from an packet address.
     *
     * @param address Packet address.
     */
   PacketAddress(const PacketAddress& address);

   /**
     * Constructor for a packet address given by a string.
     * Examples: "/tmp/test.socket".
     *
     * @param name Address string.
     */
   PacketAddress(const String& name);

   /**
     * Constructor for a packet address from the system's sockaddr structure.
     *
     * @param address sockaddr.
     * @param length Length of sockaddr.
     */
   PacketAddress(sockaddr* address, cardinal length);

   /**
     * Destructor.
     */
   ~PacketAddress();


   // ====== Initialization =================================================
   /**
     * Reset packet address.
     */
   void reset();

   /**
     * duplicate() implementation of SocketAddress.
     *
     * @see SocketAddress#duplicate
     */
   SocketAddress* duplicate() const;

   /**
     * Initialize packet address from packet address.
     */
   void init(const PacketAddress& address);

   /**
     * Initialize packet address from socket name.
     */
   void init(const String& name);

   /**
     * Implementation of = operator.
     */
   inline PacketAddress& operator=(const PacketAddress& source);


   // ====== Address functions ==============================================
   /**
     * isValid() implementation of SocketAddress.
     *
     * @see SocketAddress#isValid
     */
   bool isValid() const;

   /**
     * getFamily() implementation of SocketAddress.
     *
     * @see SocketAddress#getFamily
     */
   integer getFamily() const;

   /**
     * getAddressString() implementation of SocketAddress.
     *
     * @see SocketAddress#getAddress
     */
   String getAddressString(const cardinal format = PF_Default) const;

   /**
     * Check, if the address is null.
     *
     * @return true, if the address is not null; false otherwise.
     */
   inline bool isNull() const;


   // ====== getPort() dummy ================================================
   /**
     * getPort() implementation of SocketAddress.
     *
     * @see SocketAddress#getPort
     */
   card16 getPort() const;

   /**
     * setPort() implementation of SocketAddress.
     *
     * @see SocketAddress#setPort
     */
   void setPort(const card16 port);


   // ====== Get/set system sockaddr structure ==============================
   /**
     * getSystemAddress() implementation of SocketAddress
     *
     * @see SocketAddress#getSystemAddress
     */
   cardinal getSystemAddress(sockaddr*       buffer,
                             const socklen_t length,
                             const cardinal  type) const;

   /**
     * setSystemAddress() implementation of SocketAddress.
     *
     * @see SocketAddress#setSystemAddress
     */
   bool setSystemAddress(sockaddr* address, const socklen_t length);


    // ====== Comparision operators =========================================
   /**
     * Implementation of == operator.
     */
   int operator==(const PacketAddress& address) const;

   /**
     * Implementation of != operator.
     */
   inline int operator!=(const PacketAddress& address) const;

   /**
     * Implementation of < operator.
     */
   int operator<(const PacketAddress& address) const;

   /**
     * Implementation of <= operator.
     */
   inline int operator<=(const PacketAddress& address) const;

   /**
     * Implementation of > operator.
     */
   int operator>(const PacketAddress& address) const;

   /**
     * Implementation of >= operator.
     */
   inline int operator>=(const PacketAddress& address) const;


   // ====== Private data ===================================================
   private:
   static const cardinal MaxNameLength = IFNAMSIZ - 1;

   char Name[MaxNameLength + 1];
};


#include "packetaddress.icc"


#endif


#endif
