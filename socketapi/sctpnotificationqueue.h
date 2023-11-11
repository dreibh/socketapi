/*
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2024 by Thomas Dreibholz
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
 * Purpose: Chunk Arrival Queue
 *
 */


#ifndef SCTPNOTIFICATIONQUEUE_H
#define SCTPNOTIFICATIONQUEUE_H


#include "tdsystem.h"
#include "condition.h"
#include "ext_socket.h"
#include <sctp.h>



#define SCTP_RECVDATAIOEVNT           (1 << 0)
#define SCTP_RECVASSOCEVNT            (1 << 1)
#define SCTP_RECVPADDREVNT            (1 << 2)
#define SCTP_RECVPEERERR              (1 << 3)
#define SCTP_RECVSENDFAILEVNT         (1 << 4)
#define SCTP_RECVSHUTDOWNEVNT         (1 << 5)
#define SCTP_RECVADAPTATIONINDICATION (1 << 6)
#define SCTP_RECVPDEVNT               (1 << 7)



/**
  * SCTP Notification
  *
  * @short   SCTP Notification
  * @author  Thomas Dreibholz (thomas.dreibholz@gmail.com)
  * @version 1.0
  */
struct SCTPNotification
{
   /**
     * Pointer to next notification.
     */
   SCTPNotification* NextNotification;

   /**
     * Remote port.
     */
   unsigned short RemotePort;

   /**
     * Number of remote addresses.
     */
   unsigned short RemoteAddresses;

   /**
     * Remote addresses.
     */
   char RemoteAddress[SCTP_MAX_NUM_ADDRESSES][SCTP_MAX_IP_LEN];

   /**
     * SCTP notification.
     */
   sctp_notification Content;

   /**
     * Position within content to allow piecewise reading.
     */
   cardinal ContentPosition;
};



/**
  * Update condition types.
  */
enum UpdateConditionType {
   UCT_Read   = 0,
   UCT_Write  = 1,
   UCT_Except = 2
};



/**
  * This class is a queue for SCTP notifications.
  *
  * @short   SCTP Notification Queue
  * @author  Thomas Dreibholz (thomas.dreibholz@gmail.com)
  * @version 1.0
  */
class SCTPNotificationQueue
{
   // ====== Constructor/Destructor =========================================
   public:
   /**
     * Constructor.
     */
   SCTPNotificationQueue();

   /**
     * Destructor.
     */
   ~SCTPNotificationQueue();

   // ====== Queuing functions ==============================================
   /**
     * Add notification to tail of queue.
     *
     * @param notification Notification.
     */
   bool addNotification(const SCTPNotification& notification);

   /**
     * Get and remove notification from top of queue.
     *
     * @param notification Reference to store notification to.
     * @return true, if there was a notification; false, if the queue is empty.
     */
   bool getNotification(SCTPNotification& notification);

   /**
     * Update notification on head of queue.
     *
     * @param notification Notification.
     */
   void updateNotification(const SCTPNotification& notification);

   /**
     * Drop notification on head of queue.
     */
   void dropNotification();

   /**
     * Check, if queue has data to read for given flags.
     *
     * @param notificationFlags Flags (SCTP_RECVxxxx).
     * @return true if queue has data; false otherwise.
     */
   bool hasData(const unsigned int notificationFlags);


   // ====== Waiting functions ==============================================
   /**
     * Wait for new notification.
     *
     * @param timeout Timeout in microseconds.
     * @return true, if new notification arrived; false otherwise.
     */
   inline bool waitForChunk(const card64 timeout);

   /**
     * Signalize, that new notification has arrived.
     */
   inline void signal();


   // ====== Queue maintenance ==============================================
   /**
     * Flush queue.
     */
   void flush();

   /**
     * Get number of chunks in queue.
     *
     * @return Number of chunks.
     */
   inline cardinal count() const;

   /**
     * Get pointer to update condition.
     *
     * @return Update condition.
     */
   inline Condition* getUpdateCondition();


   // ====== Private data ===================================================
   private:
   cardinal          Count;
   SCTPNotification* First;
   SCTPNotification* Last;
   Condition         UpdateCondition;
};


#include "sctpnotificationqueue.icc"


#endif
