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
 * Purpose: Socket Message
 *
 */



#ifndef TDMESSAGE_H
#define TDMESSAGE_H


#include "tdsystem.h"
#include "socketaddress.h"


#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>



/**
  * This template class manages manages message structures used by
  * sendmsg() and recvmsg(). The template parameter gives the size
  * of the control data block.
  *
  * @short   Socket Message
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  */
template<const size_t size> class SocketMessage
{
   // ====== Constructor ====================================================
   public:
   /**
     * Constructor.
     */
   inline SocketMessage();


   // ====== Structure manipulation =========================================
   /**
     * Clear structure.
     */
   inline void clear();

   /**
     * Get address as SocketAddress object. Note: This address has to be freed
     * using delete operator!
     *
     * @return SocketAddress object.
     */
   inline SocketAddress* getAddress() const;

   /**
     * Set address.
     *
     * @param address SocketAddress object.
     */
   inline void setAddress(const SocketAddress& address);

   /**
     * Set buffer.
     *
     * @param buffer Buffer.
     * @param bufferSize Size of buffer.
     */
   inline void setBuffer(void* buffer, const size_t buffersize);

   /**
     * Set size of control block. Sizes greater than the template parameter
     * are adjusted to the maximum possible value.
     *
     * @param controlsize Size of controlblock.
     */
   inline void setControl(const size_t controlsize);

   /**
     * Add control header of given cmsg level and type. Returns NULL,
     * if there is not enough free space in the control data block.
     *
     * @param payload Size of payload.
     * @param level Level (e.g. IPPROTO_SCTP).
     * @param type Type (e.g. SCTP_INIT).
     * @return Pointer to begin of *payload* area.
     */
   inline char* addHeader(const size_t payload,
                          const int    level,
                          const int    type);

   /**
     * Get first cmsg header in control block.
     *
     * @return First cmsg header or NULL, if there are none.
     */
   inline cmsghdr* getFirstHeader() const;

   /**
     * Get next cmsg header in control block.
     *
     * @param prev Previous cmsg header.
     * @return Next cmsg header or NULL, if there are no more.
     */
   inline cmsghdr* getNextHeader(const cmsghdr* prev) const;

   /**
     * Get flags.
     *
     * @return Flags.
     */
   inline int getFlags() const;

   /**
     * Set flags.
     *
     * @param flags Flags.
     */
   inline void setFlags(const int flags);

   // ====== Message structures =============================================
   /**
     * msghdr structure.
     */
   msghdr Header;

   /**
     * Storage for address.
     */
   sockaddr_storage Address;

   /**
     * iovec structure.
     */
   struct iovec IOVector;

   private:
   cmsghdr* NextMsg;

   /**
     * Control data block, its size is given by the template parameter.
     */
   public:
   char Control[size];
};


/**
  * Wrapper for CMSG_SPACE macro.
  */
#if (SYSTEM == OS_SOLARIS)
#define CSpace(payload) (_CMSG_DATA_ALIGN(sizeof(struct cmsghdr)) + _CMSG_DATA_ALIGN(payload))
#else
#define CSpace(payload) CMSG_SPACE(payload)
#endif

/**
  * Wrapper for CMSG_LEN macro.
  */

#if (SYSTEM == OS_SOLARIS)
#define CLength(l) (_CMSG_DATA_ALIGN(sizeof(struct cmsghdr)) + (l))
#else
#define CLength(l) CMSG_LEN(l)
#endif

/**
  * Wrapper for CMSG_DATA macro.
  */
inline static char* CData(const cmsghdr* cmsg);

/**
  * Wrapper for CMSG_FIRSTHDR macro.
  */
inline static cmsghdr* CFirst(const msghdr* header);

/**
  * Wrapper for CMSG_NXTHDR macro.
  */
inline static cmsghdr* CNext(const msghdr* header, const cmsghdr* cmsg);


#include "tdmessage.icc"


#endif
