/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2003 by Thomas Dreibholz
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
 *          tuexen@fh-muenster.de
 *
 * Purpose: Chunk Arrival Queue
 *
 */



#include "tdsystem.h"
#include "sctpnotificationqueue.h"
#include "sctp.h"



// ###### Constructor #######################################################
SCTPNotificationQueue::SCTPNotificationQueue()
{
   UpdateCondition.setName("SCTPNotificationQueue::UpdateCondition");
   First = NULL;
   Last  = NULL;
   Count = 0;
}


// ###### Destructor ########################################################
SCTPNotificationQueue::~SCTPNotificationQueue()
{
   flush();
}


// ###### Add notification to end of queue ##################################
bool SCTPNotificationQueue::addNotification(const SCTPNotification& notification)
{
   SCTPNotification* newNotification = new SCTPNotification;
   if(newNotification != NULL) {
      *newNotification = notification;
      newNotification->NextNotification = NULL;

      if(Last != NULL) {
         Last->NextNotification = newNotification;
      }
      Last = newNotification;
      if(First == NULL) {
         First = newNotification;
      }
      Count++;

      signal();
      return(true);
   }
#ifndef DISABLE_WARNINGS
   cerr << "ERROR: SCTPNotificationQueue::addNotification() - Out of memory!" << endl;
#endif
   return(false);
}


// ###### Get and remove chunk from queue ##################################
bool SCTPNotificationQueue::getNotification(SCTPNotification& notification)
{
   if(First != NULL) {
      notification = *First;
      return(true);
   }
   return(false);
}


// ###### Add notification to top of queue ##################################
void SCTPNotificationQueue::updateNotification(const SCTPNotification& notification)
{
   if(First != NULL) {
      SCTPNotification* next = First->NextNotification;
      *First = notification;
      First->NextNotification = next;
   }
   else {
#ifndef DISABLE_WARNINGS
      cerr << "ERROR: SCTPNotificationQueue::updateHeadNotification() - Queue is empty!" << endl;
#endif
   }
}


// ###### Drop first notification ###########################################
void SCTPNotificationQueue::dropNotification()
{
   if(First != NULL) {
      SCTPNotification* next = First->NextNotification;
      if(Last == First) {
         Last = next;
      }
      delete First;
      First = next;
      Count--;
   }
}


// ###### Flush all chunks #################################################
void SCTPNotificationQueue::flush()
{
   SCTPNotification* notification = First;
   while(notification != NULL) {
      SCTPNotification* next = notification->NextNotification;
      delete notification;
      notification = next;
   }
   First = NULL;
   Last  = NULL;
   Count = 0;
}


// ###### Check, if queue has data to read for given flags ##################
bool SCTPNotificationQueue::hasData(const unsigned int notificationFlags)
{
   SCTPNotification* notification = First;
   while(notification != NULL) {
      if( ((notification->Content.sn_header.sn_type == SCTP_DATA_ARRIVE)                                                      ||
          ((notification->Content.sn_header.sn_type == SCTP_ASSOC_CHANGE)     && (notificationFlags & SCTP_RECVASSOCEVNT))    ||
          ((notification->Content.sn_header.sn_type == SCTP_PEER_ADDR_CHANGE) && (notificationFlags & SCTP_RECVPADDREVNT))    ||
          ((notification->Content.sn_header.sn_type == SCTP_REMOTE_ERROR)     && (notificationFlags & SCTP_RECVPEERERR))      ||
          ((notification->Content.sn_header.sn_type == SCTP_SEND_FAILED)      && (notificationFlags & SCTP_RECVSENDFAILEVNT)) ||
          ((notification->Content.sn_header.sn_type == SCTP_SHUTDOWN_EVENT)   && (notificationFlags & SCTP_RECVSHUTDOWNEVNT)))) {
         return(true);
      }
      notification = notification->NextNotification;
   }
   return(false);
}
