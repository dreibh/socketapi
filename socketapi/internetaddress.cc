/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2015 by Thomas Dreibholz
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


#include "tdsystem.h"
#include "internetaddress.h"
#include "ext_socket.h"


#include <netdb.h>
#include <netinet/in.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <arpa/nameser.h>
#include <ctype.h>


// Print note, if IPv6 is not available.
// #ifdef PRINT_NOIPV6_NOTE


// Check, if IPv6 is available on this host.
bool InternetAddress::UseIPv6 = checkIPv6();



// ###### Internet address constructor ######################################
InternetAddress::InternetAddress()
{
   reset();
   Valid = false;
}


// ###### Internet address constructor ######################################
// ## address: <Name>:port
// ##          <Address>:port
// ##          <Name>            => implies port = 0
// ##          <Address>         => implies port = 0
InternetAddress::InternetAddress(const String& address)
{
   if(address.isNull()) {
      reset();
   }
   else {
      // ====== Handle addresses with additional hostname:port ==============
      // Skip hostname:port, only use (address:port) within brackets
      String host = address;
      String port = "0";
      integer p1 = host.index('(');
      if(p1 > 0) {
         host = host.mid(p1 + 1);
         host = host.left(host.length());
      }

      // ====== Handle RFC 2732 compliant address ===========================
      if(host[0] == '[') {
         p1 = host.index(']');
         if(p1 > 0) {
            if((host[p1 + 1] == ':') || (host[p1 + 1] == '!')) {
               port = host.mid(p1 + 2);
            }
            host = host.mid(1,p1 - 1);
            host = host.left(host.length());
         }
         else {
            Valid = false;
            return;
         }
      }

      // ====== Handle standard address:port ================================
      else {
         p1 = address.rindex(':');
         if(p1 < 0) p1 = address.rindex('!');
         if(p1 > 0) {
            host = address.left(p1);
            port = address.mid(p1 + 1);
         }
      }

      // ====== Initialize address ==========================================
      int portNumber;
      if((sscanf(port.getData(),"%d",&portNumber) == 1) &&
         (portNumber >= 0) &&
         (portNumber <= 65535)) {
         init(host.getData(),portNumber);
      }
      else {
         portNumber = getServiceByName(port.getData());
         if(portNumber != 0) {
            init(host.getData(),portNumber);
         }
         else {
            Valid = false;
         }
      }
   }
}


// ###### Internet address constructor ######################################
InternetAddress::InternetAddress(const InternetAddress& address)
{
   init(address);
}


// ###### Internet address constructor ######################################
InternetAddress::InternetAddress(const String& hostName,
                                 const card16  port)
{
   if(hostName.isNull()) {
      init(port);
   }
   else {
      init(hostName,port);
   }
}


// ###### Internet address constructor ######################################
InternetAddress::InternetAddress(const PortableAddress& address)
{
   for(cardinal i = 0;i < 8;i++) {
      AddrSpec.Host16[i] = address.Host[i];
   }
   Port    = address.Port;
   ScopeID = 0;
   Valid   = true;
   setPrintFormat(PF_Default);
}


// ###### Internet address constructor ######################################
InternetAddress::~InternetAddress()
{
   Valid = false;
}


// ###### Internet address constructor ######################################
InternetAddress::InternetAddress(const card16 port)
{
   init(port);
}


// ###### Internet address constructor ######################################
InternetAddress::InternetAddress(const sockaddr* address, const socklen_t length)
{
   init(address,length);
}


// ###### Get port ##########################################################
card16 InternetAddress::getPort() const
{
   return(ntohs(Port));
}


// ###### Set port ##########################################################
void InternetAddress::setPort(const card16 port)
{
   Port = htons(port);
}


// ###### Reset #############################################################
void InternetAddress::reset()
{
   for(cardinal i = 0;i < 8;i++) {
      AddrSpec.Host16[i] = 0x0000;
   }
   ScopeID = 0;
   Valid   = true;
   setPort(0);
   setPrintFormat(PF_Default);
}


// ###### Create duplicate ##################################################
SocketAddress* InternetAddress::duplicate() const
{
   return(new InternetAddress(*this));
}


// ###### Check, if address is valid ########################################
bool InternetAddress::isValid() const
{
   return(Valid);
}


// ###### Get address family ################################################
integer InternetAddress::getFamily() const
{
   if(isIPv6()) {
      return(AF_INET6);
   }
   return((UseIPv6 == true) ? AF_INET6 : AF_INET);
}


// ###### Internet address constructor ######################################
void InternetAddress::init(const InternetAddress& address)
{
   Port = address.Port;
   for(cardinal i = 0;i < 8;i++) {
      AddrSpec.Host16[i] = address.AddrSpec.Host16[i];
   }
   ScopeID = address.ScopeID;
   Valid   = address.Valid;
   setPrintFormat(address.getPrintFormat());
}


// ###### Internet address initializer ######################################
void InternetAddress::init(const card16 port)
{
   for(cardinal i = 0;i < 8;i++) {
      AddrSpec.Host16[i] = 0x0000;
   }
   ScopeID = 0;
   Valid   = true;
   setPort(port);
   setPrintFormat(PF_Default);
}


// ###### Initialize internet address with hostname and port ################
void InternetAddress::init(const String& hostName, const card16 port)
{
   card16   address[8];
   card16   scopeID;
   cardinal length = getHostByName(hostName.getData(),(card16*)&address,&scopeID);

   Valid = true;
   setPort(port);
   setPrintFormat(PF_Default);
   ScopeID = scopeID;
   switch(length) {
      case 4:
         for(cardinal i = 0;i < 5;i++) {
            AddrSpec.Host16[i] = 0x0000;
         }
         AddrSpec.Host16[5] = 0xffff;
         memcpy((char*)&AddrSpec.Host16[6],&address,4);
       break;
      case 16:
         memcpy((char*)&AddrSpec.Host16,&address,16);
       break;
      default:
        reset();
        Valid = false;
       break;
   }
}


// ###### Internet address initializer ######################################
void InternetAddress::init(const PortableAddress& address)
{
   for(cardinal i = 0;i < 8;i++) {
      AddrSpec.Host16[i] = address.Host[i];
   }
   Port    = address.Port;
   ScopeID = 0;
   Valid   = true;
   setPrintFormat(PF_Default);
}


// ###### Initialize internet address from system's sockaddr structure ######
void InternetAddress::init(const sockaddr* address, const socklen_t length)
{
   setSystemAddress(address,length);
   setPrintFormat(PF_Default);
}


// ###### Get local address #################################################
InternetAddress InternetAddress::getLocalAddress(const InternetAddress& peer)
{
   InternetAddress address;

   int sd = ext_socket((UseIPv6 == true) ? AF_INET6 : AF_INET,SOCK_DGRAM,IPPROTO_UDP);
   if(socket >= 0) {
      sockaddr_storage socketAddress;
      socklen_t        socketAddressLength =
                          peer.getSystemAddress((sockaddr*)&socketAddress,SocketAddress::MaxSockLen,
                                                (UseIPv6 == true) ? AF_INET6 : AF_INET);
      if(socketAddressLength > 0) {
         if(ext_connect(sd,(sockaddr*)&socketAddress,socketAddressLength) == 0) {
            if(ext_getsockname(sd,(sockaddr*)&socketAddress,&socketAddressLength) == 0) {
               address.setSystemAddress((sockaddr*)&socketAddress,socketAddressLength);
               address.setPort(0);
            }
         }
      }
      ext_close(sd);
   }

   return(address);
}


// ###### Get address string ################################################
String InternetAddress::getAddressString(const cardinal format) const
{
   // ====== Check for invalid address ======================================
   if(!Valid) {
      return(String("(invalid)"));
   }

   // ====== Initialize =====================================================
   char hostString[NI_MAXHOST + NI_MAXSERV + 16];
   char addressString[256];
   hostString[0]    = 0x00;
   addressString[0] = 0x00;
   if(!((format & PF_Hostname) || (format & PF_Address))) {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: InternetAddress::getAddressString() - "
                   "Set PF_Hostname or PF_Address before printing!" << std::endl;
#endif
      return(String("(check print format)"));
   }

   // ====== Generate hostname string =======================================
   if(format & PF_Hostname) {
#if (SYSTEM != OS_Darwin)
      char             cname[NI_MAXHOST];
      char             sname[NI_MAXSERV];
      sockaddr_storage socketAddress;
      const cardinal len = getSystemAddress((sockaddr*)&socketAddress,
                                               sizeof(sockaddr_in6),
                                               AF_UNSPEC);
      const int error = getnameinfo((sockaddr*)&socketAddress,len,
                                       (char*)&cname,MAXDNAME,
                                       (char*)&sname,sizeof(sname),
                                       NI_NAMEREQD);
      if(error == 0) {
         if(!(format & PF_HidePort)) {
            snprintf((char*)&hostString,sizeof(hostString),"%s:%s",cname,sname);
         }
         else {
            snprintf((char*)&hostString,sizeof(hostString),"%s",cname);
         }
      }
#else
#warning DNS reverse lookup unter Darwin ist noch nicht implementiert!
#endif
   }

   // ====== Generate address string ========================================
   if((format & PF_Address) || ((format & PF_Hostname) && (hostString[0] == 0x00))) {
      if((UseIPv6 && !(format & PF_Legacy)) || (!isIPv4())) {
         char str[32];
         bool compressed = false;

         // ====== If IPv6 address has an embedded IPv4 address
         // print ::ffff:a.b.c.d format =====================================
         integer length = isIPv6() ? 8 : 6;

         // ====== Use RFC 2732 compilant notation ==========================
         if(!(format & PF_HidePort)) {
            strcat((char*)&addressString,"[");
         }

         // ====== Print IPv6 hex notation ==================================
         const cardinal l0 = strlen(addressString);
         for(cardinal i = 0;i < (cardinal)length;i++) {
            const card16 value = ntohs(AddrSpec.Host16[i]);
            if((value != 0) || (compressed == true) || (i == 7)) {
               snprintf((char*)&str,sizeof(str),"%x",value);
            }
            else {
               cardinal j;
               for(j = i + 1;j < 8;j++) {
                  if(AddrSpec.Host16[j] != 0) {
                     break;
                  }
               }
               if(j == i + 1) {
                  snprintf((char*)&str,sizeof(str),"%x",value);
               }
               else {
                  if((i == 0) || (j == 8)) {
                     strcpy((char*)&str,":");
                  }
                  else {
                     strcpy((char*)&str,"");
                  }
                  compressed = true;
                  i = j - 1;
               }
            }
            strcat((char*)&addressString,(char*)&str);
            if(i < 7) {
               strcat((char*)&addressString,":");
            }
         }
         if(addressString[l0 + 1] == 0x00) { // Any-Adress "::"
            strcat((char*)&addressString,":");
         }

         // ====== Print embedded IPv4 address in IPv4 notation =============
         if(length == 6) {
            card32 a = ntohl(AddrSpec.Host32[3]);
            snprintf((char*)&str,sizeof(str),
                     "%d.%d.%d.%d",(a & 0xff000000) >> 24,
                                   (a & 0x00ff0000) >> 16,
                                   (a & 0x0000ff00) >> 8,
                                   (a & 0x000000ff));
            strcat((char*)&addressString,(char*)&str);
         }

         // ====== Interface for link-local address =========================
         if((isIPv6()) && (isLinkLocal())) {
            strcat((char*)&addressString,"%");
            char        ifnamebuffer[IFNAMSIZ];
            const char* ifname = if_indextoname(ScopeID, (char*)&ifnamebuffer);
            if(ifname != NULL) {
               strcat((char*)&addressString,ifname);
            }
            else {
               strcat((char*)&addressString,"(BAD!)");
            }
         }

         // ====== Add port number ==========================================
         if(!(format & PF_HidePort)) {
            snprintf((char*)&str,sizeof(str),"]:%d",ntohs(Port));
            strcat((char*)&addressString,(char*)&str);
         }
      }
      else {
         card32 a = ntohl(AddrSpec.Host32[3]);
         if(!(format & PF_HidePort)) {
            snprintf((char*)&addressString,sizeof(addressString),
                     "%d.%d.%d.%d:%d",(a & 0xff000000) >> 24,
                                      (a & 0x00ff0000) >> 16,
                                      (a & 0x0000ff00) >> 8,
                                      (a & 0x000000ff),
                                      ntohs(Port));
         }
         else {
            snprintf((char*)&addressString,sizeof(addressString),
                     "%d.%d.%d.%d",(a & 0xff000000) >> 24,
                                   (a & 0x00ff0000) >> 16,
                                   (a & 0x0000ff00) >> 8,
                                   (a & 0x000000ff));
         }
      }
   }

   if((hostString[0] != 0x00) && (addressString[0] != 0x00))
      return(String(hostString) + " (" + String(addressString) + ")");

   return(String(hostString) + String(addressString));
}


// ###### Get sockaddr structure from internet address ######################
cardinal InternetAddress::getSystemAddress(sockaddr*       buffer,
                                           const socklen_t length,
                                           const cardinal  type) const
{
   cardinal newType = type;
   if(newType == AF_UNSPEC) {
      newType = (UseIPv6 == true) ? AF_INET6 : AF_INET;
   }
   switch(newType) {
      case AF_INET6: {
         sockaddr_in6* address = (sockaddr_in6*)buffer;
         if(sizeof(sockaddr_in6) <= (size_t)length) {
            address->sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
            address->sin6_len = sizeof(sockaddr_in6);
#endif
            address->sin6_flowinfo = 0;
            address->sin6_port     = Port;
            address->sin6_scope_id = ScopeID;
#if (SYSTEM == OS_Linux)
            memcpy((char*)&address->sin6_addr.s6_addr16[0],(char*)&AddrSpec.Host16,16);
#elif (SYSTEM == OS_SOLARIS)
            memcpy((char*)&address->sin6_addr._S6_un._S6_u8[0],(char*)&AddrSpec.Host16,16);
#else
            memcpy((char*)&address->sin6_addr.__u6_addr.__u6_addr16[0],(char*)&AddrSpec.Host16,16);
#endif
            return(sizeof(sockaddr_in6));
         }
         else {
#ifndef DISABLE_WARNINGS
            std::cerr << "WARNING: InternetAddress::getSystemInternetAddress() - "
                         "Buffer size too low for AF_INET6!" << std::endl;
#endif
         }
        }
       break;
      case AF_INET: {
         sockaddr_in* address = (sockaddr_in*)buffer;
         if(sizeof(sockaddr_in) <= (size_t)length) {
            address->sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
            address->sin_len = sizeof(sockaddr_in);
#endif
            if(isIPv4()) {
               address->sin_port = Port;
               memcpy((char*)&address->sin_addr.s_addr,(char*)&AddrSpec.Host16[6],4);
               return(sizeof(sockaddr_in));
            }
         }
         else {
#ifndef DISABLE_WARNINGS
            std::cerr << "WARNING: InternetAddress::getSystemInternetAddress() - "
                         "Buffer size too low for AF_INET!" << std::endl;
#endif
         }
        }
       break;
      default:
#ifndef DISABLE_WARNINGS
         std::cerr << "WARNING: InternetAddress::getSystemInternetAddress() - Unknown type "
                   << newType << "!" << std::endl;
#endif
       break;
   }
   return(0);
}


// ###### Initialize internet address from sockaddr structure ###############
bool InternetAddress::setSystemAddress(const sockaddr* address, const socklen_t length)
{
   const sockaddr_in* address4 = (const sockaddr_in*)address;
   Port = address4->sin_port;

   switch(address4->sin_family) {
      case AF_INET:
         for(cardinal i = 0;i < 5;i++) {
            AddrSpec.Host16[i] = 0x0000;
         }
         AddrSpec.Host16[5] = 0xffff;
         memcpy((char*)&AddrSpec.Host16[6],(const char*)&address4->sin_addr.s_addr,4);
         Valid = true;
         return(true);
       break;
      case AF_INET6: {
         const sockaddr_in6* address6 = (const sockaddr_in6*)address;
#if (SYSTEM == OS_Linux)
         memcpy((char*)&AddrSpec.Host16,(const char*)&address6->sin6_addr.s6_addr16[0],16);
#elif (SYSTEM == OS_SOLARIS)
         memcpy((char*)&AddrSpec.Host16,(const char*)&address6->sin6_addr._S6_un._S6_u8[0],16);
#else
         memcpy((char*)&AddrSpec.Host16,(const char*)&address6->sin6_addr.__u6_addr.__u6_addr8[0],16);
#endif
         ScopeID = address6->sin6_scope_id;
         Valid   = true;
         return(true);
        }
       break;
      default:
         reset();
         Valid = true;
        break;
   }
   return(false);
}


// ###### Check, if IPv6 is available #######################################
bool InternetAddress::checkIPv6()
{
   int result = socket(AF_INET6,SOCK_DGRAM,0);
   if(result != -1) {
      close(result);
      return(true);
   }
   else {
#ifdef PRINT_NOIPV6_NOTE
      std::cerr << "IPv6 is unsupported on this host!" << std::endl;
#endif
      return(false);
   }
}


// ###### Get host address by name ##########################################
cardinal InternetAddress::getHostByName(const String& hostName, card16* myadr, card16* myscope)
{
   // ====== Check for null address =========================================
   if(myscope) {
      *myscope = 0;
   }
   if(hostName.isNull()) {
      for(cardinal i = 0;i < 8;i++) myadr[i] = 0;
      if(UseIPv6) {
         return(16);
      }
      else {
         return(4);
      }
   }

   // ====== Get information for host =======================================
   addrinfo  hints;
   addrinfo* res = NULL;
   memset((char*)&hints,0,sizeof(hints));
   hints.ai_socktype = SOCK_DGRAM;
   if(UseIPv6 == true) {
      hints.ai_family = AF_UNSPEC;
      hints.ai_flags  = AI_ADDRCONFIG;
   }
   else {
      hints.ai_family = AF_INET;
   }
   const char* name = hostName.getData();

   // Avoid DNS lookups for IP addresses
   bool isNumeric = true;
   bool isIPv6    = false;
   const cardinal nameLength = strlen(name);
   for(cardinal i = 0;i < nameLength;i++) {
      if(name[i] == ':') {
         isIPv6 = true;
         break;
      }
   }
   if(!isIPv6) {
      for(cardinal i = 0;i < nameLength;i++) {
         if(!(isdigit(name[i]) || (name[i] == '.'))) {
            isNumeric = false;
            break;
         }
       }
   }
   if(isNumeric) {
      hints.ai_flags = AI_NUMERICHOST;
   }

   if(getaddrinfo(name,NULL,&hints,&res) != 0) {
      return(0);
   }

   // ====== Copy address ===================================================
   cardinal result;
   switch(res->ai_addr->sa_family) {
      case AF_INET: {
             const sockaddr_in* adr = (const sockaddr_in*)res->ai_addr;
             memcpy((char*)myadr,(const char*)&adr->sin_addr,4);
             result = 4;
          }
       break;
      case AF_INET6: {
             const sockaddr_in6* adr = (const sockaddr_in6*)res->ai_addr;
             if((IN6_IS_ADDR_LINKLOCAL(&adr->sin6_addr)) &&
                (adr->sin6_scope_id == 0)) {
                // Link-local without scope!
                result = 0;
             }
             else {
                memcpy((char*)myadr,(const char*)&adr->sin6_addr,16);
                if(myscope) {
                   *myscope = adr->sin6_scope_id;
                }
                result = 16;
             }
          }
       break;
      default:
         result = 0;
       break;
   }

   // ====== Free host information structure ================================
   freeaddrinfo(res);

   return(result);
}


// ###### Get service by name ###############################################
card16 InternetAddress::getServiceByName(const char* name)
{
   addrinfo  hints;
   addrinfo* res;
   memset((char*)&hints,0,sizeof(hints));
   hints.ai_family = AF_INET;
   int error = getaddrinfo(NULL,name,&hints,&res);
   if(error == 0) {
      sockaddr_in* adr = (sockaddr_in*)res->ai_addr;
      const card16 port = ntohs(adr->sin_port);
      freeaddrinfo(res);
      return(port);
   }
   return(0);
}


// ###### Get full hostname #################################################
bool InternetAddress::getFullHostName(char* str, const size_t size)
{
   struct utsname uts;
   if(uname(&uts) == 0) {
      const InternetAddress address(uts.nodename);
      snprintf(str,size,"%s",address.getAddressString(SocketAddress::PF_Hostname|SocketAddress::PF_HidePort).getData());
      return(true);
   }
   str[0] = 0x00;
   return(false);
}


// ###### Set in_addr structure from InternetAddress ########################
bool InternetAddress::setIPv4Address(const InternetAddress& address,
                                     in_addr*               ipv4Address)
{
   sockaddr_in sa;
   if(address.getSystemAddress((sockaddr*)&sa,sizeof(sa),AF_INET) > 0) {
      memcpy((char*)ipv4Address,(char*)&sa.sin_addr,sizeof(in_addr));
      return(true);
   }
   return(false);
}


// ###### Get InternetAddress from in_addr structure ########################
InternetAddress InternetAddress::getIPv4Address(const in_addr& ipv4Address)
{
   sockaddr_in sa;
   sa.sin_family = AF_INET;
   sa.sin_port   = 0;
   memcpy((char*)&sa.sin_addr,(char*)&ipv4Address,sizeof(in_addr));

   InternetAddress address((sockaddr*)&sa,sizeof(sa));
   return(address);
}


// ###### Internet checksum calculation #####################################
card32 InternetAddress::calculateChecksum(card8*         buf,
                                          const cardinal nbytes,
                                          card32         sum)
{
   // Checksum all the pairs of bytes first...
   cardinal i;
   for(i = 0; i < (nbytes & ~1U);i += 2) {
      sum += (card16)ntohs(*((card16*)(buf + i)));
      // Add carry
      if(sum > 0xffff) {
         sum -= 0xffff;
      }
   }

   // If there's a single byte left over, checksum it, too.
   if (i < nbytes) {
      // sum += buf[i] << 8;
      sum += htons((card16)buf[i]);
      // Add carry
      if(sum > 0xffff) {
         sum -= 0xffff;
      }
   }

   return(sum);
}


// ###### Internet checksum calculation #####################################
card32 InternetAddress::wrapChecksum(card32 sum)
{
   sum = ~sum & 0xFFFF;
   return(htons(sum));
}
