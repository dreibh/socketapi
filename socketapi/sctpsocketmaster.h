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
 * Purpose: SCTP Socket Master
 *
 */

#ifndef SCTPSOCKETMASTER_H
#define SCTPSOCKETMASTER_H


#include "tdsystem.h"
#include "thread.h"
#include "sctpsocket.h"
#include "sctpnotificationqueue.h"
#include "randomizer.h"


#include <sctp.h>
#include <map>
#include <set>
#include <utility>

#define SCTPLIB_VERSION ((SCTP_MAJOR_VERSION << 16) | SCTP_MINOR_VERSION)
#define SCTPLIB_1_0_0_PRE19 19
#define SCTPLIB_1_0_0_PRE20 20
#define SCTPLIB_1_0_0       0x10000
#define SCTPLIB_1_3_0       0x10003



/**
  * This class manages the interaction between the SCTP Socket class and the
  * userland SCTP implementation. It is implemented as a singleton and
  * automatically instantiated at program startup.
  *
  * @short   SCTP Socket Master
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  */
class SCTPSocketMaster : public Thread
{
   // ====== Friend classes =================================================
   friend class SCTPSocket;
   friend class SCTPConnectionlessSocket;
   friend class SCTPAssociation;


   // ====== Constructor/Destructor =========================================
   public:
   /**
     * Constructor.
     */
   SCTPSocketMaster();

   /**
     * Destructor.
     */
   ~SCTPSocketMaster();


   // ====== Lock/unlock ====================================================
   /**
     * Lock the SCTP implementation for exclusive access. This includes
     * Thread's critical section lock.
     */
   inline void lock();

   /**
     * Unlock the SCTP implementation. This includes
     * Thread's critical section lock.
     */
   inline void unlock();


   // ====== User socket notifications ======================================
   struct UserSocketNotification {
      int       FileDescriptor;
      short int EventMask;
      short int Events;
      Condition UpdateCondition;
   };

   /**
     * Add user socket to notification list. The socket is automatically
     * removed from the list when the first notification is received! This
     * is necessary to avoid endless loops of select() -> notification ->
     * select() -> ... calls!
     *
     * @param usn User socket notification description.
     */
   void addUserSocketNotification(UserSocketNotification* usn);

   /**
     * Remove user socket from notification list.
     *
     * @param usn User socket notification description.
     */
   void deleteUserSocketNotification(UserSocketNotification* usn);


   // ====== Options ========================================================
   /**
     * Enable or disable OOTB handling.
     *
     * @param enable false to disable, true to enable OOTB handling.
     * @return true for success; false otherwise.
     */
   static bool enableOOTBHandling(const bool enable);

      /**
     * Enable or disable CRC32 checksum.
     *
     * @param enable false to disable (use Adler32), true to enable CRC32.
     * @return true for success; false otherwise.
     */
   static bool enableCRC32(const bool enable);


   // ====== Public data ====================================================
   public:
   /**
     * Result of sctp_initLibrary() call. If not 0, the SCTP initialization
     * failed.
     */
   static int InitializationResult;


   /**
     * Master instance (singleton).
     */
   static SCTPSocketMaster MasterInstance;

   /**
     * Random number generator (singleton).
     */
   static Randomizer Random;


   // ====== Protected data =================================================
   protected:
   static cardinal                         LockLevel;
   static SCTP_ulpCallbacks                Callbacks;
   static std::multimap<int, SCTPSocket*>  SocketList;
   static std::set<int>                    ClosingSockets;
   static std::multimap<unsigned int, int> ClosingAssociations;
   static card64                           LastGarbageCollection;
   static cardinal                         OldCancelState;
   static int                              BreakPipe[2];
   static int                              GarbageCollectionTimerID;
   static UserSocketNotification           BreakNotification;

   static const card64                     GarbageCollectionInterval = 1000000;


   static SCTPSocket* getSocketForAssociationID(const unsigned int assocID);
   static void delayedDeleteAssociation(const unsigned short instanceID,
                                        const unsigned int assocID);
   static void delayedDeleteSocket(const unsigned short instanceID);


   // ====== Private data ===================================================
   private:
   void run();
   static void lock(void* data);
   static void unlock(void* data);

   static void socketGarbageCollection();
   static bool associationGarbageCollection(const unsigned int assocID,
                                            const bool         sendAbort);

   static void initNotification(SCTPNotification& notification);
   static bool initNotification(SCTPNotification& notification,
                                unsigned int      assocID,
                                unsigned short    streamID);
   static void addNotification(SCTPSocket*             socket,
                               unsigned int            assocID,
                               const SCTPNotification& notification);

   static void dataArriveNotif(unsigned int   assocID,
                               unsigned short streamID,
                               unsigned int   len,
                               unsigned short ssn,
                               unsigned int   tsn,
                               unsigned int   protoID,
                               unsigned int   unordered,
                               void*          ulpDataPtr);

#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
   static void networkStatusChangeNotif(unsigned int   assocID,
                                        short          destAddrIndex,
                                        unsigned short newState,
                                        void*          ulpDataPtr);
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
   static void networkStatusChangeNotif(unsigned int   assocID,
                                        unsigned int   affectedPathID,
                                        int            newState,
                                        void*          ulpDataPtr);
#else
#error Wrong sctplib version!
#endif

   static void* communicationUpNotif(unsigned int   assocID,
                                     int            status,
                                     unsigned int   noOfDestinations,
                                     unsigned short noOfInStreams,
                                     unsigned short noOfOutStreams,
                                     int            supportPRSCTP,
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
                                     int            adaptationLayerIndicationLen,
                                     void*          adaptationLayerIndication,
#endif
                                     void*          dummy);

#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
   static void communicationLostNotif(unsigned int   assocID,
                                      unsigned short status,
                                      void*          ulpDataPtr);
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
   static void communicationLostNotif(unsigned int assocID,
                                      int          status,
                                      void*        ulpDataPtr);
#else
#error Wrong sctplib version!
#endif

#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
   static void communicationErrorNotif(unsigned int   assocID,
                                       unsigned short status,
                                       void*          dummy);
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
   static void communicationErrorNotif(unsigned int assocID,
                                       int          status,
                                       void*        dummy);
#else
#error Wrong sctplib version!
#endif

   static void sendFailureNotif(unsigned int   assocID,
                                unsigned char* unsent_data,
                                unsigned int   dataLength,
                                unsigned int*  context,
                                void*          dummy);
   static void restartNotif(unsigned int assocID,
                            void*        ulpDataPtr);
   static void shutdownReceivedNotif(unsigned int assocID,
                                     void*        ulpDataPtr);
   static void shutdownCompleteNotif(unsigned int assocID,
                                     void*        ulpDataPtr);
   static void queueStatusChangeNotif(unsigned int assocID,
                                      int          queueType,
                                      int          queueIdentifier,
                                      int          queueLength,
                                      void*        ulpData);
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
   static void asconfStatusNotif(unsigned int assocID,
                                 unsigned int correlationID,
                                 int          result,
                                 void*        request,
                                 void*        ulpData);
#endif
   static void userCallback(int        fileDescriptor,
                            short int  eventMask,
                            short int* registeredEvents,
                            void*      userData);
   static void timerCallback(unsigned int tid,
                             void*        param1,
                             void*        param2);
};


#include "sctpsocketmaster.icc"


#endif
