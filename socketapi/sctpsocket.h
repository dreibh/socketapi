/*
 *  $Id: sctpsocket.h,v 1.4 2003/07/11 09:45:02 dreibh Exp $
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
 * Purpose: SCTP Socket
 *
 */



#ifndef SCTPSOCKET_H
#define SCTPSOCKET_H


#include "tdsystem.h"
#include "condition.h"
#include "internetaddress.h"
#include "sctpassociation.h"
#include "sctpnotificationqueue.h"


#include <sctp.h>
#include <map>


/**
  * This class manages a SCTP socket (SCTP instance).
  *
  * @short   SCTP Socket
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
  * @version 1.0
  */
class SCTPSocket
{
   // ====== Friend classes =================================================
   friend class SCTPSocketMaster;
   friend class SCTPAssociation;


   // ====== Constructor/Destructor =========================================
   public:
   /**
     * SCTP socket flags.
     */
   enum SCTPSocketFlags {
      SSF_GlobalQueue = (1 << 0),
      SSF_AutoConnect = (1 << 1),
      SSF_Listening   = (1 << 31)
   };

   /**
     * Constructor.
     *
     * @param flags SCTP socket flags.
     */
   SCTPSocket(const cardinal flags = 0);

   /**
     * Destructor.
     */
   ~SCTPSocket();


   // ====== SCTP Socket functions ==========================================
   /**
     * Get internal instance ID.
     *
     * @return Instance ID.
     */
   inline int getID() const;


   /**
     * Bind socket to local port and address(es).
     *
     * @param localPort Local port.
     * @param noOfInStreams Number of incoming streams.
     * @param noOfOutStreams Number of outgoing streams.
     * @param localAddressList NULL-terminated array of local addresses.
     */
   int bind(const unsigned short    localPort,
            const unsigned short    noOfInStreams,
            const unsigned short    noOfOutStreams,
            const SocketAddress**   localAddressList);

   /**
     * Release socket binding.
     *
     * @param sendAbort true to send abort to all UDP-like associations; false for graceful shutdown (default).
     * @see bind
     */
   void unbind(const bool sendAbort = false);

   /**
     * Establish new association.
     *
     * @param noOfOutStreams Number of outgoing streams.
     * @param maxAttempts Maximum number of INIT attempts.
     * @param maxInitTimeout Maximum init timeout.
     * @param destinationAddressList Destination address list.
     * @param blocking true to wait for establishment (default); false otherwise.
     * @return Association or NULL in case of failure.
     */
   SCTPAssociation* associate(const unsigned short  noOfOutStreams,
                              const unsigned short  maxAttempts,
                              const unsigned short  maxInitTimeout,
                              const SocketAddress** destinationAddressList,
                              const bool            blocking = true);

   /**
     * Set socket to listen mode: accept new incoming assocations.
     *
     * @param backlog Maximum number of incoming connections to accept simultaneously.
     */
   void listen(const unsigned int backlog);

   /**
     * Wait for incoming association.
     *
     * @param addressArray Reference to store NULL-terminated array of peer addresses. The addresses are allocated automatically and have to be freed using deleteAddressList(). Set NULL to skip creation of the address array.
     * @param blocking true to wait for new association (default); false otherwise.
     * @return New association or NULL in case of failure.
     *
     * @see SocketAddress#deleteAddressList
     */
   SCTPAssociation* accept(SocketAddress*** addressArray = NULL,
                           const bool       blocking     = true);


   // ====== Association peel-off ===========================================                           
   /**
     * Peel automatically established association in UDP-like mode off.
     *
     * @param assocID Association ID.
     * @return Association peeled of or NULL, if there is no such association.
     */
   SCTPAssociation* peelOff(const unsigned int assocID);

   /**
     * Peel automatically established association in UDP-like mode off.
     *
     * @param assocID Association ID.
     * @return Association peeled of or NULL, if there is no such association.
     */
   SCTPAssociation* peelOff(const SocketAddress& destinationAddress);


   // ====== Get local and remote addresses =================================
   /**
     * Get socket's local addresses.
     *
     * @param addressArray Reference to store NULL-terminated array of local addresses. The addresses are allocated automatically and have to be freed using deleteAddressList().
     * @return true, if addressEntries are sufficient; false otherwise.
     *
     * @see SocketAddress#deleteAddressList
     */
   bool getLocalAddresses(SocketAddress**& addressArray);

   /**
     * Get socket's remote addresses for given association ID.
     *
     * @param addressArray Reference to store NULL-terminated array of local addresses. The addresses are allocated automatically and have to be freed using deleteAddressList().
     * @param assocID Association ID.
     * @return true, if addressEntries are sufficient; false otherwise.
     *
     * @see SocketAddress#deleteAddressList
     */
   bool getRemoteAddresses(SocketAddress**& addressArray,
                           unsigned int     assocID);


   // ====== SCTP Socket functions ==========================================
   /**
     * Receive data.
     *
     * @param buffer Buffer to store data to.
     * @param bufferSize Size of data buffer; this will be overwritten with actual size of data content.
     * @param flags Flags; this will be overwritten with actual reception flags.
     * @param assocID Variable to store association ID to.
     * @param streamID Variable to store stream ID to.
     * @param protoID Variable to store protocol ID to.
     * @param ssn Variable to store SSN to.
     * @param tsn Variable to store TSN to.
     * @return error code (0 for success).
     */
   int receive(char*           buffer,
               size_t&         bufferSize,
               int&            flags,
               unsigned int&   assocID,
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
     * @param assocID Variable to store association ID to.
     * @param streamID Variable to store stream ID to.
     * @param protoID Variable to store protocol ID to.
     * @param ssn Variable to store SSN to.
     * @param tsn Variable to store TSN to.
     * @param addressArray Reference to store NULL-terminated array of peer addresses. The addresses are allocated automatically and have to be freed using deleteAddressList(). Set NULL to skip creation of the address array.
     * @param status Variable to store SCTPNotification data to.
     * @return error code (0 for success).
     *
     * @see SocketAddress#deleteAddressList
     */
   int receiveFrom(char*             buffer,
                   size_t&           bufferSize,
                   int&              flags,
                   unsigned int&     assocID,
                   unsigned short&   streamID,
                   unsigned int&     protoID,
                   uint16_t&         ssn,
                   uint32_t&         tsn,
                   SocketAddress***  addressArray,
                   SCTPNotification& notification);

   /**
     * Send data.
     *
     * @param buffer Data to be sent.
     * @param length Length of data to be sent.
     * @param flags Flags.
     * @param assocID Association ID (0 to use destinationAddress parameter).
     * @param streamID Stream ID.
     * @param protoID Protocol ID.
     * @param timeToLive Time to live in milliseconds.
     * @param maxAttempts Maximum number of INIT attempts.
     * @param maxInitTimeout Maximum init timeout.
     * @param useDefaults true to use defaults for Stream ID, Protocol ID and TTL; false to use given values.
     * @param destinationAddress Destination address.
     * @param noOfOutgoingStreams For AutoConnect mode: Number of outgoing streams for newly created connections.
     * @return error code (0 for success).
     */
   int sendTo(const char*          buffer,
              const size_t         length,
              const int            flags,
              const unsigned int   assocID,
              const unsigned short streamID,
              const unsigned int   protoID,
              const unsigned int   timeToLive,
              const unsigned short maxAttempts,
              const unsigned short maxInitTimeout,
              const bool           useDefaults,
              const SocketAddress* destinationAddress,
              const cardinal       noOfOutgoingStreams = 1);


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


   // ====== Instance and association parameters ============================
   /**
     * Get assoc defaults.
     *
     * @param assocDefaults Reference to store assoc defaults.
     * @return true, if successful; false otherwise.
     */
   bool getAssocDefaults(SCTP_Instance_Parameters& assocDefaults);

   /**
     * Set assoc defaults.
     *
     * @param assocDefaults assoc defaults.
     * @return true, if successful; false otherwise.
     */
   bool setAssocDefaults(const SCTP_Instance_Parameters& assocDefaults);

   /**
     * Get association defaults.
     *
     * @param assocID Association ID.
     * @param defaults Reference to store association defaults to.
     * @return true, if successful; false otherwise.
     */
   bool getAssocIODefaults(const unsigned int      assocID,
                           struct AssocIODefaults& defaults);

    /**
     * Set association defaults.
     *
     * @param assocID Association ID.
     * @param defaults Association defaults.
     * @return true, if successful; false otherwise.
     */
   bool setAssocIODefaults(const unsigned int            assocID,
                           const struct AssocIODefaults& defaults);

   /**
     * Set stream default timeouts.
     *
     * @param assocID Association ID.
     * @param timeout Timeout in milliseconds.
     * @param start First stream ID to set timeout for.
     * @param start Last stream ID to set timeout for.
     * @return true, if timeout has been updated; false otherwise.
     */
   bool setDefaultStreamTimeouts(const unsigned int   assocID,
                                 const unsigned int   timeout,
                                 const unsigned short start,
                                 const unsigned short end);

   /**
     * Get stream default timeout.
     *
     * @param assocID Association ID.
     * @param streamID Stream ID to get timeout for.
     * @param timeout Reference to store timeout.
     * @return true, if timeout has been found; false otherwise.
     */
   bool getDefaultStreamTimeout(const unsigned int   assocID,
                                const unsigned short streamID,
                                unsigned int&        timeout);

   /**
     * Get association parameters.
     *
     * @param assocID Association ID.
     * @param associationParameters Reference to store association parameters.
     * @return true, if successful; false otherwise.
     */
   bool getAssocStatus(const unsigned int       assocID,
                       SCTP_Association_Status& associationParameters);

   /**
     * Set association parameters.
     *
     * @param assocID Association ID.
     * @param associationParameters Association parameters.
     * @return true, if successful; false otherwise.
     */
   bool setAssocStatus(const unsigned int             assocID,
                       const SCTP_Association_Status& associationParameters);


   // ====== Path parameters ================================================
   /**
     * Get path parameters.
     *
     * @param assocID Path ID.
     * @param address Address to get path parameters for.
     * @param pathParameters Reference to store path parameters.
     * @return true, if successful; false otherwise.
     */
   bool getPathParameters(const unsigned int          assocID,
                          const struct SocketAddress* address,
                          SCTP_PathStatus&            pathParameters);

   /**
     * Set path parameters.
     *
     * @param assocID Path ID.
     * @param address Address to set path parameters for.
     * @param pathParameters Path parameters.
     * @return true, if successful; false otherwise.
     */
   bool setPathParameters(const unsigned int          assocID,
                          const struct SocketAddress* address,
                          const SCTP_PathStatus&      pathParameters);

   /**
     * Get primary address of given association.
     *
     * @param assocID Association ID.
     * @return Primary address. This address has to be deleted after usage.
     */
   SocketAddress* getPrimaryAddress(const unsigned int assocID);

   /**
     * Set primary address of given association.
     *
     * @param assocID Association ID.
     * @param primary Primary address.
     * @return true for success; false otherwise.
     */
   bool setPrimary(const unsigned int   assocID,
                   const SocketAddress& primary);

   /**
     * Set peer primary address of given association.
     *
     * @param assocID Association ID.
     * @param primary Peer primary address.
     * @return true for success; false otherwise.
     */
   bool setPeerPrimary(const unsigned int   assocID,
                       const SocketAddress& primary);

   /**
     * Add address to given association.
     *
     * @param assocID Association ID (0 for all UDP-like associations).
     * @param addAddress Address to be added.
     * @return true for success; false otherwise.
     */
   bool addAddress(const unsigned int   assocID,
                   const SocketAddress& addAddress);

   /**
     * Delete address from given association.
     *
     * @param assocID Association ID (0 for all UDP-like associations).
     * @param delAddress Address to be deleted.
     * @return true for success; false otherwise.
     */
   bool deleteAddress(const unsigned int   assocID,
                      const SocketAddress& delAddress);


   // ====== Other parameters ===============================================
   /**
     * Get AutoClose parameter.
     *
     * @return Timeout in microseconds.
     */
   inline card64 getAutoClose() const;

   /**
     * Set AutoClose parameter.
     *
     * @param timeout Timeout in microseconds.
     */
   inline void setAutoClose(const card64 timeout);

   /**
     * Set send buffer size for all UDP-like associations.
     *
     * @param size Send buffer size.
     */
   bool setSendBuffer(const size_t size);

   /**
     * Set receive buffer size for all UDP-like associations.
     *
     * @param size Receive buffer size.
     */
   bool setReceiveBuffer(const size_t size);

   /**
     * Get default traffic class.
     *
     * @return Default traffic class.
     */
   inline card8 getDefaultTrafficClass() const;

   /**
     * Set traffic class for all UDP-like associations.
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
   SCTPAssociation* getAssociationForAssociationID(const unsigned int assocID,
                                                   const bool activeOnly = true);
   int getErrorCode(const unsigned int assocID);
   int internalReceive(SCTPNotificationQueue& queue,
                       char*                  buffer,
                       size_t&                bufferSize,
                       int&                   flags,
                       unsigned int&          assocID,
                       unsigned short&        streamID,
                       unsigned int&          protoID,
                       uint16_t&              ssn,
                       uint32_t&              tsn,
                       SCTPNotification&      notification,
                       const unsigned int     notificationFlags);
   int internalSend(const char*          buffer,
                    const size_t         length,
                    const int            flags,
                    const unsigned int   assocID,
                    const unsigned short streamID,
                    const unsigned int   protoID,
                    const unsigned int   timeToLive,
                    Condition*           waitCondition,
                    const SocketAddress* pathDestinationAddress);
   static int getPathIndexForAddress(const unsigned int          assocID,
                                     const struct SocketAddress* address,
                                     SCTP_PathStatus&            pathParameters);


   struct IncomingConnection
   {
      IncomingConnection* NextConnection;
      SCTPAssociation*    Association;
      SCTPNotification    Notification;
   };

   SCTPNotificationQueue                    GlobalQueue;
   Condition                                EstablishCondition;
   Condition                                ReadUpdateCondition;
   Condition                                WriteUpdateCondition;
   Condition                                ExceptUpdateCondition;

   IncomingConnection*                      ConnectionRequests;
   multimap<unsigned int, SCTPAssociation*> AssociationList;

   int                                      InstanceName;
   unsigned short                           LocalPort;
   unsigned short                           NoOfInStreams;
   unsigned short                           NoOfOutStreams;

   cardinal                                 Flags;
   unsigned int                             NotificationFlags;
   unsigned int                             CorrelationID;

   card64                                   AutoCloseTimeout;


   // ====== Private data ===================================================
   private:
   void checkAutoConnect();
   void checkAutoClose();


   multimap<unsigned int, SCTPAssociation*> ConnectionlessAssociationList;
   card8                                    DefaultTrafficClass;

   bool                                     WriteReady;
   bool                                     ReadReady;
   bool                                     HasException;

#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19)
   unsigned int                             NoOfLocalAddresses;
   unsigned char                            LocalAddressList[SCTP_MAX_NUM_ADDRESSES][SCTP_MAX_IP_LEN];
#endif
};


#include "sctpsocket.icc"


#endif
