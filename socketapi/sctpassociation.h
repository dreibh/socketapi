/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2013 by Thomas Dreibholz
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
 * Purpose: SCTP Association
 *
 */

#ifndef SCTPASSOCIATION_H
#define SCTPASSOCIATION_H


#include "tdsystem.h"
#include "condition.h"
#include "internetaddress.h"
#include "sctpsocket.h"
#include "sctpnotificationqueue.h"

#include <sctp.h>


class SCTPSocket;


/**
  * This structure contains defaults for an association.
  */
struct AssocIODefaults
{
   /**
     * Default stream ID.
     */
   unsigned short StreamID;

   /**
     * Default protocol ID.
     */
   unsigned int ProtoID;

   /**
     * Default time to live.
     */
   unsigned int TimeToLive;

   /**
     * Default conext.
     */
   unsigned int Context;
};



/**
  * This class manages a SCTP assocation. Note: The constructor is protected,
  * a SCTP assocation can be created from an SCTP socket using associate() or
  * accept().
  *
  * @short   SCTP Association
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  *
  * @see SCTPSocket
  * @see SCTPSocket#associate
  * @see SCTPSocket#accept
  */
class SCTPAssociation
{
   // ====== Friend classes =================================================
   friend class SCTPSocket;
   friend class SCTPConnectionlessSocket;
   friend class SCTPSocketMaster;


   // ====== Destructor =====================================================
   public:
   /**
     * Destructor.
     */
   ~SCTPAssociation();


   // ====== Association functions ==========================================
   /**
     * Get internal association ID.
     *
     * @return Association ID.
     */
   inline unsigned int getID() const;

   /**
     * Get local addresses.
     *
     * @param addressArray Reference to store NULL-terminated array of local addresses. The addresses are allocated automatically and have to be freed using deleteAddressList().
     * @return true, if addressEntries are sufficient; false otherwise.
     *
     * @see SocketAddress#deleteAddressList
     */
   bool getLocalAddresses(SocketAddress**& addressArray);

   /**
     * Get remote addresses.
     *
     * @param addressArray Reference to store NULL-terminated array of local addresses. The addresses are allocated automatically and have to be freed using deleteAddressList().
     * @return true, if addressEntries are sufficient; false otherwise.
     *
     * @see SocketAddress#deleteAddressList
     */
   bool getRemoteAddresses(SocketAddress**& addressArray);


   /**
     * Receive data.
     *
     * @param buffer Buffer to store data to.
     * @param bufferSize Size of data buffer; this will be overwritten with actual size of data content.
     * @param flags Flags; this will be overwritten with actual reception flags.
     * @param streamID Variable to store stream ID to.
     * @param protoID Variable to store protocol ID to.
     * @param ssn Variable to store SSN to.
     * @param tsn Variable to store TSN to.
     * @return error code (0 for success).
     */
   int receive(char*           buffer,
               size_t&         bufferSize,
               int&            flags,
               unsigned short& streamID,
               unsigned int&   protoID,
               uint16_t&       ssn,
               uint32_t&       tsn);

   /**
     * Receive data.
     *
     * @param buffer Buffer to store data to.
     * @param bufferSize Size of data buffer; this will be overwritten with actual size of data content.
     * @param flags Flags; this will be overwritten with actual reception flags.
     * @param streamID Variable to store stream ID to.
     * @param protoID Variable to store protocol ID to.
     * @param ssn Variable to store SSN to.
     * @param tsn Variable to store TSN to.
     * @param address Reference to store the destination addresses to. The address is allocated automatically and has to be freed using delete operator. Set NULL to skip creation of the address.
     * @return error code (0 for success).
     */
   int receiveFrom(char*           buffer,
                   size_t&         bufferSize,
                   int&            flags,
                   unsigned short& streamID,
                   unsigned int&   protoID,
                   uint16_t&       ssn,
                   uint32_t&       tsn,
                   SocketAddress** address);


   /**
     * Send data.
     *
     * @param buffer Data to be sent.
     * @param length Length of data to be sent.
     * @param flags Flags.
     * @param streamID Stream ID.
     * @param protoID Protocol ID.
     * @param timeToLive Time to live in milliseconds.
     * @param useDefaults true to use defaults for Stream ID, Protocol ID and TTL; false to use given values.
     * @return error code (0 for success).
     */
   inline int send(const char*          buffer,
                   const size_t         length,
                   const int            flags,
                   const unsigned short streamID,
                   const unsigned int   protoID,
                   const unsigned int   timeToLive,
                   const bool           useDefaults);

   /**
     * Send data via path given by address.
     *
     * @param buffer Data to be sent.
     * @param length Length of data to be sent.
     * @param flags Flags.
     * @param streamID Stream ID.
     * @param protoID Protocol ID.
     * @param timeToLive Time to live in milliseconds.
     * @param useDefaults true to use defaults for Stream ID, Protocol ID and TTL; false to use given values.
     * @param pathDestinationAddress Destination address of path to send data via.
     * @return error code (0 for success).
     */
   int sendTo(const char*          buffer,
              const size_t         length,
              const int            flags,
              const unsigned short streamID,
              const unsigned int   protoID,
              const unsigned int   timeToLive,
              const bool           useDefaults,
              const SocketAddress* pathDestinationAddress);

   /**
     * Shutdown.
     */
   void shutdown();

   /**
     * Abort.
     */
   void abort();


   // ====== Check, if there is new data to read ============================
   /**
     * Check, if queue has data to read for given flags.
     *
     * @return true if queue has data; false otherwise.
     */
   bool hasData();


   // ====== Notification flags =============================================
   /**
     * Get notification flags.
     *
     * @return Notification flags.
     */
   inline unsigned int getNotificationFlags() const;

   /**
     * Get notification flags.
     *
     * @param notificationFlags Notification flags.
     */
   inline void setNotificationFlags(const unsigned int notificationFlags);


   // ====== Association parameters =========================================
   /**
     * Get association defaults.
     *
     * @param defaults Reference to store association defaults to.
     */
   void getAssocIODefaults(struct AssocIODefaults& defaults);

    /**
     * Set association defaults.
     *
     * @param defaults Association defaults.
     */
   void setAssocIODefaults(const struct AssocIODefaults& defaults);

   /**
     * Set stream default timeouts.
     * @param timeout Timeout in milliseconds.
     * @param start First stream ID to set timeout for.
     * @param start Last stream ID to set timeout for.
     * @return true, if timeout has been updated; false otherwise.
     */
   bool setDefaultStreamTimeouts(const unsigned int   timeout,
                                 const unsigned short start,
                                 const unsigned short end);

   /**
     * Get stream default timeout.
     *
     * @param streamID Stream ID to get timeout for.
     * @param timeout Reference to store timeout.
     * @return true, if timeout has been found; false otherwise.
     */
   bool getDefaultStreamTimeout(const unsigned short streamID, unsigned int& timeout);

   /**
     * Get association parameters.
     *
     * @param assocStatus Reference to store association parameters.
     * @return true, if successful; false otherwise.
     */
   bool getAssocStatus(SCTP_Association_Status& assocStatus);

   /**
     * Set association parameters.
     *
     * @param assocStatus Association parameters.
     * @return true, if successful; false otherwise.
     */
   bool setAssocStatus(const SCTP_Association_Status& assocStatus);


   // ====== Path parameters ================================================
   /**
     * Get path parameters.
     *
     * @param address Address to get path parameters for.
     * @param pathParameters Reference to store path parameters.
     * @return true, if successful; false otherwise.
     */
   bool getPathParameters(const struct SocketAddress* address,
                          SCTP_PathStatus&            pathParameters);

   /**
     * Set path parameters.
     *
     * @param address Address to set path parameters for.
     * @param pathParameters Path parameters.
     * @return true, if successful; false otherwise.
     */
   bool setPathParameters(const struct SocketAddress* address,
                          const SCTP_PathStatus&      pathParameters);

   /**
     * Get primary address.
     *
     * @return Primary address. This address has to be deleted after usage.
     */
   SocketAddress* getPrimaryAddress();

   /**
     * Set primary address of given association.
     *
     * @param primary Primary address.
     * @return true for success; false otherwise.
     */
   bool setPrimary(const SocketAddress& primary);

   /**
     * Set peer primary address of given association.
     *
     * @param primary Peer primary address.
     * @return true for success; false otherwise.
     */
   bool setPeerPrimary(const SocketAddress& primary);

   /**
     * Add address to given association.
     *
     * @param addAddress Address to be added.
     * @return true for success; false otherwise.
     */
   bool addAddress(const SocketAddress& addAddress);

   /**
     * Delete address from given association.
     *
     * @param delAddress Address to be deleted.
     * @return true for success; false otherwise.
     */
   bool deleteAddress(const SocketAddress& delAddress);


   // ====== Other parameters ===============================================
   /**
     * Get send buffer size.
     *
     * @param Send buffer size (-1 in case of error).
     */
   ssize_t getSendBuffer();

   /**
     * Set send buffer size.
     *
     * @param size Send buffer size.
     */
   bool setSendBuffer(const size_t size);

   /**
     * Get receive buffer size.
     *
     * @param Receive buffer size (-1 in case of error).
     */
   ssize_t getReceiveBuffer();

   /**
     * Set receive buffer size.
     *
     * @param size Receive buffer size.
     */
   bool setReceiveBuffer(const size_t size);

   /**
     * Get traffic class.
     *
     * @param streamID Stream ID (default: 0).
     * @return Traffic class (-1 in case of error).
     */
   int getTrafficClass(const int streamID = 0);

   /**
     * Set traffic class.
     *
     * @param trafficClass Traffic class.
     * @param streamID Stream ID (-1 for all streams, default).
     */
   bool setTrafficClass(const card8 trafficClass,
                        const int   streamID = -1);

   /**
     * Get pointer to update condition.
     *
     * @param type Update condition type.
     * @return Update condition.
     */
   inline Condition* getUpdateCondition(const UpdateConditionType type);


   // ====== Protected data =================================================
   protected:
   SCTPAssociation(SCTPSocket*        socket,
                   const unsigned int associationID,
                   const unsigned int notificationFlags,
                   const bool         udpLike);


   // ====== Private data ===================================================
   private:
   bool sendPreEstablishmentPackets();

   SCTPSocket*           Socket;
   SCTPNotificationQueue InQueue;
   Condition             EstablishCondition;
   Condition             ShutdownCompleteCondition;
   Condition             ReadyForTransmit;
   Condition             ReadUpdateCondition;
   Condition             WriteUpdateCondition;
   Condition             ExceptUpdateCondition;

   card64                LastUsage;
   cardinal              UseCount;

   unsigned int          AssociationID;
   unsigned int          NotificationFlags;
   AssocIODefaults       Defaults;

   struct StreamDefaultTimeout {
      bool         Valid;
      unsigned int Timeout;
   };
   StreamDefaultTimeout*   StreamDefaultTimeoutArray;
   unsigned int            StreamDefaultTimeoutCount;

   bool                    CommunicationUpNotification;
   bool                    CommunicationLostNotification;
   bool                    ShutdownCompleteNotification;
   bool                    IsShuttingDown;

   bool                    ReadReady;
   bool                    WriteReady;
   bool                    HasException;

   bool                    RTOMaxIsInitTimeout;
   unsigned int            InitTimeout;
   unsigned int            RTOMax;

   struct PreEstablishmentPacket {
      PreEstablishmentPacket* Next;
      unsigned int            Flags;
      uint32_t                ProtoID;
      uint16_t                StreamID;
      unsigned int            TimeToLive;
      size_t                  Length;
      char*                   Data;
   };
   PreEstablishmentPacket* FirstPreEstablishmentPacket;
   PreEstablishmentPacket* LastPreEstablishmentPacket;
   SocketAddress**         PreEstablishmentAddressList;

   bool                    PeeledOff;
};


#include "sctpassociation.icc"


#endif
