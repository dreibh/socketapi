/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2011 by Thomas Dreibholz
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
 * Purpose: Socket Adress
 *
 */


#ifndef SOCKETADDRESS_H
#define SOCKETADDRESS_H


#include "tdsystem.h"


#include <sys/socket.h>
#include <sys/un.h>



/**
  * This class is an interface for a socket address.
  *
  * @short   Socket Address
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  */
class SocketAddress
{
   // ====== Destructor =====================================================
   public:
   /**
     * Destructor.
     */
   virtual ~SocketAddress();


   // ====== Duplication ====================================================
   /**
     * Create a duplicate of the SocketAddress object.
     *
     * @return Copy of SocketAddress object.
     */
   virtual SocketAddress* duplicate() const = 0;


   // ====== Reset ==========================================================
   /**
     * Reset address.
     */
   virtual void reset() = 0;


   // ====== Check, if address is valid =====================================
   /**
     * Check, if address is valid.
     *
     * @return true, if address is valid; false otherwise.
     */
   virtual bool isValid() const = 0;


   // ====== Print format ===================================================
   /**
     * setPrintFormat() printing formats.
     */
   enum PrintFormat {
      /**
        * Print address.
        */
      PF_Address  = (1 << 0),

      /**
        * Print hostname, if possible. Otherwise, print address.
        */
      PF_Hostname = (1 << 1),

      /**
        * Print both, address and hostname (if resolvable).
        */
      PF_Full = (PF_Address | PF_Hostname),

      /**
        * Hide port number.
        */
      PF_HidePort = (1 << 15),

      /**
        * Legacy mode: Do *not* print IPv4 addresses on IPv6-capable hosts as
        * IPv4-mapped address. That is, 1.2.3.4 instead of ::ffff:1.2.3.4.
        */
      PF_Legacy = (1 << 16),

      /**
        * Default print format.
        */
      PF_Default = (PF_Address | PF_Legacy)
   };


   /**
     * Get printing format.
     *
     * @return Print format.
     */
   inline cardinal getPrintFormat() const;

   /**
     * Set printing format.
     *
     * @param format Print format.
     */
   inline void setPrintFormat(const cardinal format);


   // ====== Address management =============================================
   /**
     * Get port of address.
     *
     * @return Port.
     */
   virtual card16 getPort() const = 0;

   /**
     * Set port of address.
     */
   virtual void setPort(const card16 port) = 0;

   /**
     * Get family of address.
     *
     * @return Address family (e.g. AF_INET or AF_UNIX).
     */
   virtual integer getFamily() const = 0;


   // ====== Get local address for a connection =============================
   /**
     * Get the local host address. The parameter peer gives the address of
     * the other host.
     *
     * @param peer Address of peer.
     * @return Local SocketAddress or NULL in case of an error.
     *
     * Examples:
     * localhost => localhost address (127.0.0.1 or ::1).
     * ethernet-host => ethernet interface address.
     * internet-address => dynamic-ip address set by pppd.
     */
   static SocketAddress* getLocalAddress(const SocketAddress& peer);


   // ====== Get address string =============================================
   /**
     * Get address string.
     *
     * @return Address string.
     */
   virtual String getAddressString(const cardinal format = PF_Default) const = 0;


   // ====== SocketAddress creation =========================================
   /**
     * Create SocketAddress object for given address family.
     *
     * @param family Address family (e.g. AF_INET or AF_UNIX).
     * @return SocketAddress object or NULL in case of failure.
     */
   static SocketAddress* createSocketAddress(const integer family);

   /**
     * Create SocketAddress object from address string.
     *
     * @param flags flags
     * @param string Address string.
     * @return SocketAddress object or NULL in case of failure.
     */
   static SocketAddress* createSocketAddress(const cardinal flags, const String& name);

   /**
     * Create SocketAddress object from address string and port number.
     *
     * @param flags flags
     * @param string Address string.
     * @param port Port number.
     * @return SocketAddress object or NULL in case of failure.
     */
   static SocketAddress* createSocketAddress(const cardinal flags,
                                             const String&  name,
                                             const card16   port);

   /**
     * Create SocketAddress object from system's sockaddr structure.
     *
     * @param flags flags.
     * @param address sockaddr.
     * @param length Length of sockaddr.
     * @return SocketAddress object or NULL in case of failure.
     */
   static SocketAddress* createSocketAddress(const cardinal  flags,
                                             sockaddr*       address,
                                             const socklen_t length);


   // ====== Get/set system sockaddr structure ==============================
   /**
     * Get system's sockaddr structure for the address.
     *
     * @param buffer Buffer to write sockaddr to.
     * @param length Length of buffer.
     * @param type Socket address type, e.g. AF_INET or AF_INET6.
     * @return Length of written sockaddr structure.
     */
   virtual cardinal getSystemAddress(sockaddr*       buffer,
                                     const socklen_t length,
                                     const cardinal  type = AF_UNSPEC) const = 0;

   /**
     * Initialize the socket address from the system's sockaddr structure.
     *
     * @param address sockaddr.
     * @param length Length of sockaddr.
     */
   virtual bool setSystemAddress(sockaddr* address, const socklen_t length) = 0;


   // ====== SocketAddress lists ============================================
   /**
     * Allocate memory for NULL-terminated SocketAddress list with given number of entries.
     *
     * @param entries Number of entries.
     * @return Address list or NULL, if out of memory.
     */
   static SocketAddress** newAddressList(const cardinal entries);

   /**
     * Deallocate NULL-terminated list of SocketAddress objects.
     *
     * @param addressArray Address list.
     *
     * @see getLocalAddresses
     * @see accept
     */
   static void deleteAddressList(SocketAddress**& addressArray);


   // ====== Constants ======================================================
   /**
     * Maximum sockaddr length in bytes.
     */
   static const cardinal MaxSockLen =
      (sizeof(sockaddr_un) > sizeof(sockaddr_storage)) ? sizeof(sockaddr_un) : sizeof(sockaddr_storage);


   // ====== Protected data =================================================
   protected:
   /**
     * Print format.
     */
   cardinal Format;


   friend inline std::ostream& operator<<(std::ostream& os, const SocketAddress& sa);
};


/**
  * Output operator.
  */
inline std::ostream& operator<<(std::ostream& os, const SocketAddress& sa);


#include "socketaddress.icc"


#endif
