/*
 *  $Id: internetflow.h,v 1.1 2003/05/15 11:35:50 dreibh Exp $
 *
 * SCTP implementation according to RFC 2960.
 * Copyright (C) 1999-2002 by Thomas Dreibholz
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
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
 *
 * Purpose: Internet Flow Implementation
 *
 */



#ifndef INTERNETFLOW_H
#define INTERNETFLOW_H


#include "tdsystem.h"
#include "internetaddress.h"



/**
  * This class inherits InternetAddress and contains an additional flow label
  * for IPv6 support.
  *
  * @short   Internet Flow
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
  * @version 1.0
  */            
class InternetFlow : public InternetAddress
{
   // ====== Constructors/Destructor ========================================
   public:
   /**
     * Constructor for a new InternetFlow.
     *
     */
   InternetFlow();

   /**
     * Constructor for a new InternetFlow.
     *
     * @param flow InternetFlow to be copied.
     */
   InternetFlow(const InternetFlow& flow);

   /**
     * Constructor for a new InternetFlow.
     *
     * @param address InternetAddress.
     * @param flowLabel Flow label (20 bits).
     * @param trafficClass Traffic class (8 bits).
     */
   InternetFlow(const InternetAddress& address,
                const card32           flowLabel,
                const card8            trafficClass);


   // ====== Initialization =================================================
   /**
     * Reset flow info.
     */
   void reset();

   /**
     * duplicate() implementation of SocketAddress.
     *
     * @see SocketAddress#duplicate
     */
   SocketAddress* duplicate() const;


   // ====== Address functions ==============================================
   /**
     * getAddressString() implementation of SocketAddress.
     *
     * @see SocketAddress#getAddressString
     */
   String getAddressString(const cardinal format = PF_Default) const;


   // ====== Get/set system sockaddr structure ==============================
   /**
     * getSystemAddress() implementation of SocketAddressInterface.
     *
     * @see SocketAddressInterface#getSystemAddress
     */            
   cardinal getSystemAddress(sockaddr*       buffer,
                             const socklen_t length,
                             const cardinal  type) const;

   /**
     * setSystemAddress() implementation of SocketAddressInterface.
     *
     * @see SocketAddressInterface#setSystemAddress
     */
   bool setSystemAddress(sockaddr* address, socklen_t length);


   // ====== Status functions ===============================================
   /**
     * Get IPv6 flow info: (flowLabel | (trafficClass << 20)).
     *
     * @return Flow info.
     */
   inline card32 getFlowInfo() const;


   /**
     * Get flow label.
     *
     * @return Flow label.
     */
   inline card32 getFlowLabel() const;

   /**
     * Set flow label.
     *
     * @param flowLabel Flow label.
     */
   inline void setFlowLabel(const card32 flowLabel);


   /**
     * Get traffic class.
     *
     * @return Traffic class.
     */
   inline card8 getTrafficClass() const;

   /**
     * Set traffic class.
     *
     * @param trafficClass New traffic class.
     */
   inline void setTrafficClass(const card8 trafficClass);


   // ====== Private data ===================================================
   private:
   card32 FlowInfo;
};


#include "internetflow.icc"


#endif
