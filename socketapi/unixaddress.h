/*
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2025 by Thomas Dreibholz
 *
 * Realized in co-operation between
 * - Siemens AG
 * - University of Duisburg-Essen, Institute for Experimental Mathematics
 * - Münster University of Applied Sciences, Burgsteinfurt
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
 *          thomas.dreibholz@gmail.com
 *          tuexen@fh-muenster.de
 *
 * Purpose: Unix address implementation
 *
 */


#ifndef UNIXADDRESS_H
#define UNIXADDRESS_H


#include "tdsystem.h"
#include "strings.h"
#include "socketaddress.h"


#include <sys/socket.h>
#include <sys/un.h>



/**
  * This class manages an unix socket address.
  *
  * @short   Socket Address
  * @author  Thomas Dreibholz (thomas.dreibholz@gmail.com)
  * @version 1.0
  */
class UnixAddress : virtual public SocketAddress
{
   // ====== Constructors/Destructor ========================================
   public:
   /**
     * Constructor for an empty unix address.
     */
   UnixAddress();

   /**
     * Constructor for an unix address from an unix address.
     *
     * @param address Unix address.
     */
   UnixAddress(const UnixAddress& address);

   /**
     * Constructor for a unix address given by a string.
     * Examples: "/tmp/test.socket".
     *
     * @param name Address string.
     */
   UnixAddress(const String& name);

   /**
     * Constructor for a unix address from the system's sockaddr structure.
     *
     * @param address sockaddr.
     * @param length Length of sockaddr.
     */
   UnixAddress(const sockaddr* address, const cardinal length);

   /**
     * Destructor.
     */
   ~UnixAddress();


   // ====== Initialization =================================================
   /**
     * Reset unix address.
     */
   void reset();

   /**
     * duplicate() implementation of SocketAddress.
     *
     * @see SocketAddress#duplicate
     */
   SocketAddress* duplicate() const;

   /**
     * Initialize unix address from unix address.
     */
   void init(const UnixAddress& address);

   /**
     * Initialize unix address from socket name.
     */
   void init(const String& name);

   /**
     * Implementation of = operator.
     */
   inline UnixAddress& operator=(const UnixAddress& source);


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
   bool setSystemAddress(const sockaddr* address, const socklen_t length);


    // ====== Comparision operators =========================================
   /**
     * Implementation of == operator.
     */
   int operator==(const UnixAddress& address) const;

   /**
     * Implementation of != operator.
     */
   inline int operator!=(const UnixAddress& address) const;

   /**
     * Implementation of < operator.
     */
   int operator<(const UnixAddress& address) const;

   /**
     * Implementation of <= operator.
     */
   inline int operator<=(const UnixAddress& address) const;

   /**
     * Implementation of > operator.
     */
   int operator>(const UnixAddress& address) const;

   /**
     * Implementation of >= operator.
     */
   inline int operator>=(const UnixAddress& address) const;


   // ====== Private data ===================================================
   private:
   static const cardinal MaxNameLength = 108 - 1;

   char Name[MaxNameLength + 1];
};


#include "unixaddress.icc"


#endif
