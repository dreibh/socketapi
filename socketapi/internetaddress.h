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
 * Purpose: Internet Address Implementation
 *
 */

#ifndef INTERNETADDRESS_H
#define INTERNETADDRESS_H


#include "tdsystem.h"
#include "strings.h"
#include "socketaddress.h"
#include "portableaddress.h"


#include <netinet/in.h>
#include <resolv.h>



/**
  * This class manages an internet address.
  *
  * @short   Socket Address
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  */
class InternetAddress : virtual public SocketAddress
{
   // ====== Constructors/Destructor ========================================
   public:
   /**
     * Constructor for an empty internet address.
     */
   InternetAddress();

   /**
     * Constructor for an internet address from an internet address.
     *
     * @param address Internet address.
     */
   InternetAddress(const InternetAddress& address);

   /**
     * Constructor for a internet address given by a string.
     * Examples: "gaffel:7500", "12.34.56.78:7500", "3ffe:4711::0!7500", "odin:7500", "ipv6-odin:7500".
     *
     * @param address Address string.
     */
   InternetAddress(const String& address);

   /**
     * Constructor for a internet address given by host name and port.
     *
     * @param hostName Host name.
     * @param port Port number.
     */
   InternetAddress(const String& hostName,
                   const card16 port);

   /**
     * Constructor for INADDR_ANY address with given port.
     *
     * @param port Port number.
     */
   InternetAddress(const card16 port);

   /**
     * Constructor for a internet address from the system's sockaddr structure. The
     * sockaddr structure may be sockaddr_in (IPv4) or sockaddr_in6 (IPv6).
     *
     * @param address sockaddr.
     * @param length Length of sockaddr (sizeof(sockaddr_in) or sizeof(sockaddr_in6)).
     */
   InternetAddress(const sockaddr* address, const socklen_t length);

   /**
     * Destructor.
     */
   ~InternetAddress();


   // ====== Initialization =================================================
   /**
     * Reset internet address.
     */
   void reset();

   /**
     * duplicate() implementation of SocketAddress.
     *
     * @see SocketAddress#duplicate
     */
   SocketAddress* duplicate() const;

   /**
     * Initialize internet address from internet address.
     */
   void init(const InternetAddress& address);

   /**
     * Initialize internet address with given host name and port.
     *
     * @param hostName Host name.
     * @param port Port number.
     */
   void init(const String& hostName, const card16 port);

   /**
     * Initialize internet address with INADDR_ANY and given port.
     *
     * @param port Port number.
     */
   void init(const card16 port);

   /**
     * Initialize internet address from the system's sockaddr structure. The
     * sockaddr structure may be sockaddr_in (IPv4) or sockaddr_in6 (IPv6).
     *
     * @param address sockaddr.
     * @param length Length of sockaddr (sizeof(sockaddr_in) or sizeof(sockaddr_in6)).
     */
   void init(const sockaddr* address, const socklen_t length);


   // ====== Operators ======================================================
   /**
     * Implementation of = operator.
     */
   inline InternetAddress& operator=(const InternetAddress& source);


   // ====== Address manipulation ===========================================
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
     * @see SocketAddress#getAddressString
     */
   String getAddressString(const cardinal format = PF_Default) const;

   /**
     * Check, if internet address is an IPv4 or IPv4-mapped address (a.b.c.d or ::ffff:a.b.c.d).
     *
     * @return true, if address is an IPv4 or IPv4-mapped address; false otherwise.
     */
   inline bool isIPv4() const;

   /**
     * Check, if internet address is an IPv4-compatible IPv6 address (::a.b.c.d and NOT ::ffff:a.b.c.d).
     *
     * @return true, if address is an IPv4-compatible IPv6 address; false otherwise.
     */
   inline bool isIPv4compatible() const;

   /**
     * Check, if internet address is a real (not IPv4-mapped) IPv6 address.
     * Addresses which return true here can be used with labeled flows by
     * class Socket.
     *
     * @return true, if address is real IPv6; false otherwise.
     */
   inline bool isIPv6() const;

   /**
     * Check, if the address is null address (0.0.0.0 for IPv4 or :: for IPv6) and
     * port number 0. To skip port number check, use isUnspecified().
     *
     * @return true, if the address is null; false otherwise.
     *
     * @see isUnspecified
     */
   inline bool isNull() const;

   /**
     * Check, if the address is null address (0.0.0.0 for IPv4 or :: for IPv6).
     * This function does not check the port number.
     * To also check the port number, use isNull().
     *
     * @return true, if the address is null; false otherwise.
     *
     * @see isNull
     */
   inline bool isUnspecified() const;

   /**
     * Check, if the address is loopback address (127.x.y.z for IPv4 or ::1 for IPv6).
     *
     * @return true, if the address is loopback address; false otherwise.
     */
   inline bool isLoopback() const;

   /**
     * Check, if internet address is link local (IPv6) or 127.x.y.z (IPv4).
     *
     * @return true or false.
     */
   inline bool isLinkLocal() const;

   /**
     * Check, if internet address is site local (IPv6) or 127.x.y.z,
     * 192.168.x.y or 10.x.y.z or within {172.16.0.0 to 127.31.255.255} (IPv4).
     *
     * @return true or false.
     */
   inline bool isSiteLocal() const;

   /**
     * Check, if internet address is global, that is !(isLinkLocal() || isSiteLocal()).
     *
     * @return true or false.
     */
   inline bool isGlobal() const;

   /**
     * Check, if internet address is a multicast address.
     *
     * @return true or false.
     */
   inline bool isMulticast() const;

   /**
     * Check, if internet address broadcast address.
     *
     * @return true or false.
     */
   inline bool isBroadcast() const;

   /**
     * Check, if internet address is an unicast address (not broadcast or multicast).
     *
     * @return true or false.
     */
   inline bool isUnicast() const;

   /**
     * Check, if internet address is an reserved address.
     *
     * @return true or false.
     */
   inline bool isReserved() const;

   /**
     * Check, if internet address is a node local IPv6 multicast address.
     *
     * @return true or false.
     */
   inline bool isNodeLocalMulticast() const;

   /**
     * Check, if internet address is a link local IPv6 multicast address.
     *
     * @return true or false.
     */
   inline bool isLinkLocalMulticast() const;

   /**
     * Check, if internet address is a site local IPv6 multicast address.
     *
     * @return true or false.
     */
   inline bool isSiteLocalMulticast() const;

   /**
     * Check, if internet address is a organization local IPv6 multicast address.
     *
     * @return true or false.
     */
   inline bool isOrgLocalMulticast() const;

   /**
     * Check, if internet address is a global IPv6 multicast address.
     *
     * @return true or false.
     */
   inline bool isGlobalMulticast() const;


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


   // ====== IPv6 support functions ========================================
   /**
     * Wrapper for system's gethostbyname() function. This version does
     * support IPv6 addresses even if the system itself does not support IPv6.
     * IPv6 addresses are then converted to IPv4 if possible (IPv4-mapped IPv6).
     *
     * @param name Host name.
     * @param myadr Storage space to save a IPv6 address (16 bytes).
     * @param length Length of the address saved in myaddr or 0 in case of failure.
     */
   static cardinal getHostByName(const String& name, card16* myadr);

   /**
     * Get port number for given service (e.g. http).
     *
     * @param name Service name (e.g. "http" or "telnet").
     * @return Port number of 0 if unknown.
     */
   static card16 getServiceByName(const char* name);

   /**
     * Get the computer's full locat name (format: name.domain).
     *
     * @param str Buffer to store name to.
     * @param size Size of buffer.
     * @return true for success; false otherwise.
     */
   static bool getFullHostName(char* str, const size_t size);

   /**
     * Check, if IPv6 support is available.
     *
     * @return true, if IPv6 support is available; false otherwise.
     */
   static inline bool hasIPv6();

    /**
      * Static variable which shows the availability of IPv6. Setting this
      * variable to false on an IPv6 system simulates an IPv4-only system.
      *
      * Do *not* change this variable if Socket or InternetAddress objects
      * are already in use!!!
      */
   static bool UseIPv6;


   // ====== Get local address for a connection =============================
   /**
     * Get the local host address. The parameter peer gives the address of
     * the other host.
     *
     * @param peer Address of peer.
     * @return Local internet address.
     *
     * Examples:
     * localhost => localhost address (127.0.0.1 or ::1).
     * ethernet-host => ethernet interface address.
     * internet-address => dynamic-ip address set by pppd.
     */
   static InternetAddress getLocalAddress(const InternetAddress& peer);


    // ====== Comparision operators =========================================
   /**
     * Implementation of == operator.
     */
   int operator==(const InternetAddress& address) const;

   /**
     * Implementation of != operator.
     */
   inline int operator!=(const InternetAddress& address) const;

   /**
     * Implementation of < operator.
     */
   int operator<(const InternetAddress& address) const;

   /**
     * Implementation of <= operator.
     */
   inline int operator<=(const InternetAddress& address) const;

   /**
     * Implementation of > operator.
     */
   int operator>(const InternetAddress& address) const;

   /**
     * Implementation of >= operator.
     */
   inline int operator>=(const InternetAddress& address) const;


   // ====== Conversion from and to PortableAddress =========================
   /**
     * Get PortableAddress from InternetAddress.
     *
     * @return PortableAddress.
     */
   inline PortableAddress getPortableAddress() const;

   /**
     * Constructor for InternetAddress from PortableAddress.
     *
     * @param address PortableAddress.
     */
   InternetAddress(const PortableAddress& address);

   /**
     * Initialize InternetAddress from PortableAddress.
     *
     * @param address PortableAddress.
     */
   void init(const PortableAddress& address);


   // ====== System functions ===============================================
   /**
     * Set in_addr structure from InternetAddress (IPv4 only!).
     *
     * @param address InternetAddress.
     * @param ipv4Address Pointer to in_addr to write address to.
     * @return true for success; false otherwise (IPv6 address).
     */
   static bool setIPv4Address(const InternetAddress& address,
                              in_addr*               ipv4Address);

   /**
     * Get IPv4 InternetAddress from in_addr structure.
     *
     * @param ipv4Address in_addr to get address from.
     * @return InternetAddress.
     */
   static InternetAddress getIPv4Address(const in_addr& ipv4Address);

   /**
     * Calculate internet checksum.
     *
     * @param buffer Buffer to calculate checksum from.
     * @param bytes Number of bytes.
     * @param sum Checksum value to add.
     * @return Checksum.
     */
   static card32 calculateChecksum(card8*         buffer,
                                   const cardinal bytes,
                                   card32         sum);

   /**
     * Prepare checksum for writing into header: Wrap sum
     * and convert byte order.
     *
     * @param sum Checksum.
     * @return Checksum.
     */
   static card32 wrapChecksum(card32 sum);


   // ====== Private data ===================================================
   private:
   static bool checkIPv6();


   private:
   /**
     * Host address in network byte order. IPv4 addresses are converted to
     * IPv4-mapped IPv6 addresses.
     */
   union {
      card16   Host16[8];
      card32   Host32[4];
      in6_addr Address;
   } AddrSpec;

   /**
     * Port number.
     */
   card16 Port;

   /**
     * Is address valid?
     */
   bool Valid;
};


#include "internetaddress.icc"


#endif
