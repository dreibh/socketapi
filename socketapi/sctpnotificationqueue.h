/*
 *  $Id: sctpnotificationqueue.h,v 1.1 2003/05/15 11:35:50 dreibh Exp $
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
 * Purpose: Chunk Arrival Queue
 *
 */


#ifndef SCTPNOTIFICATIONQUEUE_H
#define SCTPNOTIFICATIONQUEUE_H


#include "tdsystem.h"
#include "condition.h"
#include "ext_socket.h"
#include "sctp.h"



/**
  * SCTP Notification
  *
  * @short   SCTP Notification
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
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
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
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
