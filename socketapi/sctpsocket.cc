/*
 *  $Id: sctpsocket.cc,v 1.16 2003/07/07 16:12:44 dreibh Exp $
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



#include "tdsystem.h"
#include "sctpsocket.h"
#include "sctpsocketmaster.h"


/*
#define PRINT_BIND
#define PRINT_UNBIND
#define PRINT_ADDIP
#define PRINT_ACCEPT
#define PRINT_ASSOCIATE
#define PRINT_NEW_ASSOCIATIONS
#define PRINT_SHUTDOWNS
#define PRINT_PRSCTP
#define PRINT_NOTIFICATION_SKIP
#define PRINT_DATA
#define PRINT_RECVSTATUS
#define PRINT_SETPRIMARY
*/

/*
#define PRINT_AUTOCLOSE_TIMEOUT
#define PRINT_AUTOCLOSE_CHECK

#define PRINT_RECVWAIT
#define PRINT_ISSHUTDOWN
#define PRINT_PATHFORINDEX
#define PRINT_ASSOCSEARCH
#define PRINT_ASSOC_USECOUNT
#define PRINT_RTO
*/


// ###### Constructor #######################################################
SCTPSocket::SCTPSocket(const cardinal flags)
{
   CorrelationID       = 0;
   AutoCloseTimeout    = 30000000;
   InstanceName        = 0;
   ConnectionRequests  = NULL;
   Flags               = flags;
   NotificationFlags   = 0;
   DefaultTrafficClass = 0x00;
   ReadReady           = false;
   WriteReady          = false;
   HasException        = false;

   EstablishCondition.setName("SCTPSocket::EstablishCondition");
   ReadUpdateCondition.setName("SCTPSocket::ReadUpdateCondition");
   WriteUpdateCondition.setName("SCTPSocket::WriteUpdateCondition");
   ExceptUpdateCondition.setName("SCTPSocket::ExceptUpdateCondition");
   GlobalQueue.getUpdateCondition()->setName("SCTPSocket::GlobalQueueCondition");

   EstablishCondition.addParent(&ReadUpdateCondition);
   GlobalQueue.getUpdateCondition()->addParent(&ReadUpdateCondition);
}


// ###### Destructor ########################################################
SCTPSocket::~SCTPSocket()
{
   unbind();
}


// ###### Get association for association ID ################################
SCTPAssociation* SCTPSocket::getAssociationForAssociationID(const unsigned int assocID,
                                                            const bool activeOnly)
{
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      AssociationList.find(assocID);
   if(iterator != AssociationList.end()) {
      if(!((iterator->second->IsShuttingDown) && (activeOnly))) {
         return(iterator->second);
      }
   }
   return(NULL);
}


// ###### Get local addresses ###############################################
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19)
bool SCTPSocket::getLocalAddresses(SocketAddress**& addressArray)
{
   // ====== Initialize =====================================================
   unsigned int i;
   bool         result = true;
   addressArray = SocketAddress::newAddressList(NoOfLocalAddresses);
   if(addressArray == NULL) {
      return(false);
   }

   // ====== Get local addresses ============================================
   SCTPSocketMaster::MasterInstance.lock();
   for(i = 0;i < NoOfLocalAddresses;i++) {
      addressArray[i] = SocketAddress::createSocketAddress(0,(char*)LocalAddressList[i],LocalPort);
      if(addressArray[i] == NULL) {
#ifndef DISABLE_WARNINGS
         cerr << "WARNING: SCTPSocket::getLocalAddresses() - Bad address "
              << *(LocalAddressList[i]) << ", port " << LocalPort << "!" << endl;
#endif
         SocketAddress::deleteAddressList(addressArray);
         result = false;
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}
#else
bool SCTPSocket::getLocalAddresses(SocketAddress**& addressArray)
{
   SCTP_Instance_Parameters parameters;
   bool                     result = false;

   // ====== Get local addresses ============================================
   SCTPSocketMaster::MasterInstance.lock();
   if(getAssocDefaults(parameters)) {
      const unsigned int localAddresses = parameters.noOfLocalAddresses;
      addressArray = SocketAddress::newAddressList(localAddresses);
      if(addressArray != NULL) {
         for(unsigned int i = 0;i < localAddresses;i++) {
            addressArray[i] = SocketAddress::createSocketAddress(0,(char*)&parameters.localAddressList[i],LocalPort);
            if(addressArray[i] == NULL) {
#ifndef DISABLE_WARNINGS
               cerr << "WARNING: SCTPSocket::getLocalAddresses() - Bad address "
                    << parameters.localAddressList[i] << ", port " << LocalPort << "!" << endl;
#endif
               SocketAddress::deleteAddressList(addressArray);
               addressArray = NULL;
               result       = false;
               break;
            }
         }
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();

   return(result);
}
#endif


// ###### Get remote addresses for given association ########################
bool SCTPSocket::getRemoteAddresses(SocketAddress**& addressArray,
                                    unsigned int     assocID)
{
   SCTPSocketMaster::MasterInstance.lock();

   // ====== Try to find association in association list ====================
   SCTPAssociation* association = getAssociationForAssociationID(assocID,false);
   if(association == NULL) {
      // ====== Try to find association in AutoConnect list =================
      multimap<unsigned int, SCTPAssociation*>::iterator iterator =
         ConnectionlessAssociationList.find(assocID);
      if(iterator != ConnectionlessAssociationList.end()) {
         association = iterator->second;
      }
   }

   bool ok = false;
   if(association != NULL) {
      ok = association->getRemoteAddresses(addressArray);
   }

   SCTPSocketMaster::MasterInstance.unlock();
   return(ok);
}


// ###### Bind socket to local address(es) and port #########################
int SCTPSocket::bind(const unsigned short    localPort,
                     const unsigned short    noOfInStreams,
                     const unsigned short    noOfOutStreams,
                     const SocketAddress**   localAddressList)
{
   if(SCTPSocketMaster::InitializationResult != 0) {
#ifdef PRINT_BIND
      cerr << "WARNING: SCTPSocket::bind() - SCTP is not initialized!" << endl;
#endif
      return(-EPROTONOSUPPORT);
   }

   SCTPSocketMaster::MasterInstance.lock();
   if(!SCTPSocketMaster::MasterInstance.running()) {
      // Start SCTPSocketMaster thread.
      if(SCTPSocketMaster::MasterInstance.start() == false) {
#ifndef DISABLE_WARNINGS
         cerr << "WARNING: SCTPSocket::bind() - Unable to start master thread!" << endl;
#endif
         SCTPSocketMaster::MasterInstance.unlock();
         return(-EPROTONOSUPPORT);
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();

#if (SCTPLIB_VERSION != SCTPLIB_1_0_0_PRE19)
   unsigned int  NoOfLocalAddresses;
   unsigned char LocalAddressList[SCTP_MAX_NUM_ADDRESSES][SCTP_MAX_IP_LEN];
#endif
   NoOfLocalAddresses = 0;
   while(localAddressList[NoOfLocalAddresses] != NULL) {
      NoOfLocalAddresses++;
   }

   SCTPSocketMaster::MasterInstance.lock();

   // ====== Initialize =====================================================
   unbind();
   LocalPort      = localPort;
   NoOfInStreams  = noOfInStreams;
   NoOfOutStreams = noOfOutStreams;
   CorrelationID  = 0;


   // ====== Initialize local addresses =====================================
#ifdef PRINT_BIND
   cout << "Binding to {";
   for(unsigned int i = 0;i < NoOfLocalAddresses;i++) {
      cout << " " << localAddressList[i]->getAddressString(SocketAddress::PF_Address|SocketAddress::PF_HidePort|SocketAddress::PF_Legacy) << ":" << localPort << " ";
   }
   cout << "}." << endl;
#endif
   for(unsigned int i = 0;i < min(NoOfLocalAddresses,(unsigned int)SCTP_MAX_NUM_ADDRESSES);i++) {
      snprintf((char*)&LocalAddressList[i],SCTP_MAX_IP_LEN,"%s",localAddressList[i]->getAddressString(SocketAddress::PF_HidePort|SocketAddress::PF_Address|SocketAddress::PF_Legacy).getData());
   }
   if(NoOfLocalAddresses < 1) {
#ifndef DISABLE_WARNINGS
      cerr << "ERROR: SCTPSocket::bind() - No local addresses!" << endl;
#endif
      SCTPSocketMaster::MasterInstance.unlock();
      return(-EINVAL);
   }


   // ====== Register SCTP instance =========================================
   InstanceName = sctp_registerInstance(LocalPort, NoOfInStreams, NoOfOutStreams,
                                        NoOfLocalAddresses, LocalAddressList,
                                        SCTPSocketMaster::Callbacks);
   if(InstanceName <= 0) {
#ifdef PRINT_BIND
      cerr << "ERROR: SCTPSocket::bind() - sctp_registerInstance() failed!" << endl;
#endif
      SCTPSocketMaster::MasterInstance.unlock();
      return(-EADDRINUSE);
   }


   // ====== Add socket to global list ======================================
   SCTPSocketMaster::SocketList.insert(pair<unsigned short, SCTPSocket*>(InstanceName,this));


   SCTPSocketMaster::MasterInstance.unlock();
   return(0);
}


// ###### Release socket binding ############################################
void SCTPSocket::unbind(bool sendAbort)
{
   if(InstanceName > 0) {
      SCTPSocketMaster::MasterInstance.lock();
#ifdef PRINT_UNBIND
      cout << "Unbinding...";
#endif

      // ====== Delete all associations made by sendTo() ====================
      multimap<unsigned int, SCTPAssociation*>::iterator iterator =
         ConnectionlessAssociationList.begin();
      while(iterator != ConnectionlessAssociationList.end()) {
         SCTPAssociation* association = iterator->second;
         ConnectionlessAssociationList.erase(iterator);
         if(sendAbort) {
            association->abort();
         }
         delete association;
         iterator = ConnectionlessAssociationList.begin();
      }

      // ====== Mark SCTP instance for unregistering ========================
      SCTPSocketMaster::delayedDeleteSocket(InstanceName);

      // ====== Remove socket from global list ==============================
      multimap<int, SCTPSocket*>::iterator socketIterator =
         SCTPSocketMaster::SocketList.find(InstanceName);
      if(socketIterator != SCTPSocketMaster::SocketList.end()) {
         SCTPSocketMaster::SocketList.erase(socketIterator);
      }
      else {
#ifndef DISABLE_WARNINGS
         cerr << "INTERNAL ERROR: SCTPSocket::unbind() - Erase failed for instance "
              << InstanceName << "!" << endl;
         exit(1);
#endif
      }

      // ====== Remove incoming associations ================================
      while(ConnectionRequests != NULL) {
         SCTPAssociation* association   = ConnectionRequests->Association;
         IncomingConnection* oldRequest = ConnectionRequests;
         ConnectionRequests = oldRequest->NextConnection;
         delete association;
         delete oldRequest;
      }

      SCTPSocketMaster::MasterInstance.unlock();

      GlobalQueue.flush();
      InstanceName  = 0;
      CorrelationID = 0;
      Flags &= ~SSF_Listening;

#ifdef PRINT_UNBIND
      cout << "Unbind complete." << endl;
#endif
   }
}


// ###### Set listen mode ###################################################
void SCTPSocket::listen(const unsigned int backlog)
{
   SCTPSocketMaster::MasterInstance.lock();
   if(backlog > 0) {
      Flags |= SSF_Listening;
   }
   else {
      Flags &= ~SSF_Listening;
   }
   SCTPSocketMaster::MasterInstance.unlock();
}


// ###### Accept new association ############################################
SCTPAssociation* SCTPSocket::accept(SocketAddress*** addressArray,
                                    const bool       wait)
{
   if(addressArray != NULL) {
      *addressArray = NULL;
   }
   SCTPSocketMaster::MasterInstance.lock();
   if(!(Flags & SSF_Listening)) {
#ifndef DISABLE_WARNINGS
      cerr << "ERROR: SCTPSocket::accept() - Socket is not in server mode, call listen() first!" << endl;
#endif
      return(NULL);
   }


   // ====== Get new association=============================================
   while(ConnectionRequests == NULL) {
      SCTPSocketMaster::MasterInstance.unlock();

      if(!wait) {
         return(NULL);
      }
      while(EstablishCondition.timedWait(100000) == false) {
         checkAutoConnect();
      }

      SCTPSocketMaster::MasterInstance.lock();
   }
   if(ConnectionRequests == NULL) {
      SCTPSocketMaster::MasterInstance.unlock();
      return(NULL);
   }


   // ====== Initialize address array =======================================
   if(addressArray != NULL) {
      *addressArray = SocketAddress::newAddressList(ConnectionRequests->Notification.RemoteAddresses);
      if(*addressArray == NULL) {
#ifndef DISABLE_WARNINGS
         cerr << "ERROR: SCTPSocket::accept() - Out of memory!" << endl;
#endif
      }
      else {
         unsigned int i;
         for(i = 0;i < ConnectionRequests->Notification.RemoteAddresses;i++) {
            (*addressArray)[i] = SocketAddress::createSocketAddress(
                                    0,
                                    (char*)&ConnectionRequests->Notification.RemoteAddress[i],
                                    ConnectionRequests->Notification.RemotePort);
            if((*addressArray)[i] == NULL) {
#ifndef DISABLE_WARNINGS
               cerr << "WARNING: SCTPSocket::accept() - Bad address "
                    << ConnectionRequests->Notification.RemoteAddress[i] << ", port " << ConnectionRequests->Notification.RemotePort << "!" << endl;
#endif
               SocketAddress::deleteAddressList(*addressArray);
            }
         }
      }
   }

#ifdef PRINT_ACCEPT
   cout << "Accepted association #" << ConnectionRequests->Association->AssociationID << " from {";
   for(unsigned int i = 0;i < ConnectionRequests->Notification.RemoteAddresses;i++) {
      InternetAddress address(String((char*)&ConnectionRequests->Notification.RemoteAddress[i]),ConnectionRequests->Notification.RemotePort);
      cout << " " << address.getAddressString(SocketAddress::PF_Address|SocketAddress::PF_Legacy) << " ";
   }
   cout << "}." << endl;
#endif


   // ====== Remove association from incoming list ==========================
   SCTPAssociation* association   = ConnectionRequests->Association;
   IncomingConnection* oldRequest = ConnectionRequests;
   ConnectionRequests = oldRequest->NextConnection;
   delete oldRequest;

   ReadReady = hasData() || (ConnectionRequests != NULL);

   SCTPSocketMaster::MasterInstance.unlock();
   return(association);
}


// ###### Establish new association #########################################
SCTPAssociation* SCTPSocket::associate(const unsigned short  noOfOutStreams,
                                       const unsigned short  maxAttempts,
                                       const unsigned short  maxInitTimeout,
                                       const SocketAddress** destinationAddressList,
                                       const bool            blocking)
{
   // ====== Establish new association ======================================
   SCTPSocketMaster::MasterInstance.lock();
   SCTP_Instance_Parameters oldParameters;
   SCTP_Instance_Parameters newParameters;
   if(getAssocDefaults(oldParameters)) {
      newParameters = oldParameters;
      newParameters.maxInitRetransmits = maxAttempts;
      if(newParameters.maxInitRetransmits > 0) {
         newParameters.maxInitRetransmits--;
      }
      if(newParameters.maxInitRetransmits <= 0) {
         newParameters.maxInitRetransmits = 1;
      }
      newParameters.rtoMax = maxInitTimeout;
      if(!setAssocDefaults(newParameters)) {
#ifndef DISABLE_WARNINGS
         cerr << "WARNING: SCTPSocket::associate() - Unable to set new instance parameters!" << endl;
#endif
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      cerr << "WARNING: SCTPSocket::associate() - Unable to get instance parameters!" << endl;
#endif
   }

   unsigned int destinationAddresses = 0;
   while(destinationAddressList[destinationAddresses] != NULL) {
      destinationAddresses++;
   }

   unsigned int  assocID = 0;
   unsigned char addressArray[destinationAddresses][SCTP_MAX_IP_LEN];
   if(destinationAddresses > 0) {
      for(unsigned int i = 0;i < destinationAddresses;i++) {
         snprintf((char*)&addressArray[i], SCTP_MAX_IP_LEN, "%s",
                  destinationAddressList[i]->getAddressString(SocketAddress::PF_HidePort|SocketAddress::PF_Address|SocketAddress::PF_Legacy).getData());
      }
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19)
      assocID = sctp_associate(InstanceName,
                               (noOfOutStreams < 1) ? 1 : noOfOutStreams,
                               addressArray[0],
                               destinationAddressList[0]->getPort(),
                               NULL);
#else
      assocID = sctp_associatex(InstanceName,
                                (noOfOutStreams < 1) ? 1 : noOfOutStreams,
                                addressArray,
                                destinationAddresses,
                                SCTP_MAX_NUM_ADDRESSES,
                                destinationAddressList[0]->getPort(),
                                NULL);
#endif
   }
   else {
#ifndef DISABLE_WARNINGS
      cerr << "ERROR: SCTPSocket::associate() - No destination addresses given?!" << endl;
#endif
   }

   if(!setAssocDefaults(oldParameters)) {
#ifndef DISABLE_WARNINGS
      cerr << "WARNING: SCTPSocket::associate() - Unable to restore old instance parameters!" << endl;
#endif
   }

#ifdef PRINT_ASSOCIATE
   cout << "Association to { ";
   for(unsigned int i = 0;i < destinationAddresses;i++) {
      cout << addressArray[i];
      if(i < (destinationAddresses - 1)) {
         cout << ", ";
      }
   }
   cout << " }, port " << destinationAddressList[0]->getPort()
        << " => ID #" << assocID << "." << endl;
#endif


   // ====== Create SCTPAssociation object ==================================
   SCTPAssociation* association = NULL;
   if(assocID != 0) {
      association = new SCTPAssociation(this, assocID, NotificationFlags,
                                        Flags & SCTPSocket::SSF_GlobalQueue);
      if(association == NULL) {
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
         sctp_abort(assocID, 0, NULL);
#else
         sctp_abort(assocID);
#endif
         sctp_deleteAssociation(assocID);
#ifndef DISABLE_WARNINGS
         cerr << "ERROR: SCTPSocket::associate() - Out of memory!" << endl;
#endif
      }
      else {
#ifdef PRINT_ASSOC_USECOUNT
         cout << "Associate: UseCount increment for A" << association->getID() << ": "
              << association->UseCount << " -> ";
#endif
         association->UseCount++;
#ifdef PRINT_ASSOC_USECOUNT
         cout << association->UseCount << endl;
#endif
         association->setTrafficClass(DefaultTrafficClass);

         association->RTOMaxIsInitTimeout = true;
         association->RTOMax              = oldParameters.rtoMax;
         association->InitTimeout         = maxInitTimeout;
#ifdef PRINT_RTO
         cout << "associate() - InitTimeout=" << association->InitTimeout << " SavedMaxRTO=" << association->RTOMax << endl;
#endif
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();

   // ====== Wait for peer's connection up notification =====================
   if(association != NULL) {
      if(blocking) {
#ifdef PRINT_ASSOCIATE
         cout << "Waiting for establishment of association #" << assocID << "..." << endl;
#endif
         while(association->EstablishCondition.timedWait(100000) == false) {
            checkAutoConnect();
         }
         if(!association->CommunicationUpNotification) {
#ifdef PRINT_ASSOCIATE
            cout << "Association #" << assocID << " failed!" << endl;
#endif
            delete association;
            association = NULL;
         }
         else {
            association->setTrafficClass(DefaultTrafficClass);
         }
      }
   }

#ifdef PRINT_ASSOCIATE
   if(association != NULL) {
      cout << "Association #" << assocID << " established." << endl;
   }
#endif

   // ====== Decrement use count ============================================
   SCTPSocketMaster::MasterInstance.lock();
   if(association != NULL) {
#ifdef PRINT_ASSOC_USECOUNT
         cout << "Associate: UseCount decrement for A" << association->getID() << ": "
              << association->UseCount << " -> ";
#endif
      association->UseCount--;
#ifdef PRINT_ASSOC_USECOUNT
      cout << association->UseCount << endl;
#endif
   }
   SCTPSocketMaster::MasterInstance.unlock();

   return(association);
}


// ###### Get error code for given association ID ###########################
int SCTPSocket::getErrorCode(const unsigned int assocID)
{
   SCTPAssociation* association = getAssociationForAssociationID(assocID,false);
   if(association != NULL) {
      if(association->ShutdownCompleteNotification) {
         association->HasException = true;
         return(-ESHUTDOWN);
      }
      else if(association->CommunicationLostNotification) {
         association->HasException = true;
         return(-ECONNABORTED);
      }
   }
   return(0);
}


// ###### Internal receive implementation ###################################
int SCTPSocket::internalReceive(SCTPNotificationQueue& queue,
                                char*                  buffer,
                                size_t&                bufferSize,
                                int&                   flags,
                                unsigned int&          assocID,
                                unsigned short&        streamID,
                                unsigned int&          protoID,
                                uint16_t&              ssn,
                                uint32_t&              tsn,
                                SCTPNotification&      notification,
                                const unsigned int     notificationFlags)
{
   // ====== Check parameters ===============================================
   if(bufferSize == 0) {
#ifndef DISABLE_WARNINGS
      cerr << "WARNING: SCTPSocket::internalReceive() - Data buffer size is zero!" << endl;
#endif
      return(-EINVAL);
   }


   // ====== Get next data or notification from queue =====================
#ifdef PRINT_RECVWAIT
   cout << "Waiting... ";
   cout.flush();
#endif
   SCTPSocketMaster::MasterInstance.lock();
   bool received = queue.getNotification(notification);
   while(received == false) {
      int errorCode = getErrorCode(assocID);
      SCTPSocketMaster::MasterInstance.unlock();

      // ====== No chunk available -> wait for chunk ======================
      if(errorCode != 0) {
         bufferSize = 0;
         if((errorCode == -ESHUTDOWN) && !(queue.hasData(notificationFlags))) {
#ifdef PRINT_ISSHUTDOWN
            cout << "Socket has been shut down -> leaving waiting loop!" << endl;
#endif
            errorCode = 0;
         }
         return(errorCode);
      }
      if(flags & MSG_DONTWAIT) {
         return(-EAGAIN);
      }
      while(queue.waitForChunk(100000) == false) {
         checkAutoConnect();
      }
      SCTPSocketMaster::MasterInstance.lock();
      received = queue.getNotification(notification);
   }
#ifdef PRINT_RECVWAIT
   cout << "Wakeup!" << endl;
#endif


   // ====== Read data ======================================================
   // If MSG_NOTIFICATION is set, notifications are received!
   const bool receiveNotifications = (flags & MSG_NOTIFICATION);
   bool updatedNotification = false;
   int result               = 0;
   if(notification.Content.sn_header.sn_type == SCTP_DATA_ARRIVE) {
      flags &= ~MSG_NOTIFICATION;
      sctp_data_arrive* sda = &notification.Content.sn_data_arrive;
      if(sda->sda_bytes_arrived > 0) {
         assocID  = sda->sda_assoc_id;
         streamID = sda->sda_stream;
         protoID  = sda->sda_ppid;
         if(sda->sda_flags & SCTP_ARRIVE_UNORDERED) {
            flags |= MSG_UNORDERED;
         }
         size_t receivedBytes = min((size_t) sda->sda_bytes_arrived, (size_t) bufferSize);
         const int ok = sctp_receive(assocID, streamID,
                                     (unsigned char*)buffer,
                                     (unsigned int*)&receivedBytes,
                                     &ssn,
                                     &tsn,
                                     (flags & MSG_PEEK) ? SCTP_MSG_PEEK : SCTP_MSG_DEFAULT);
         if(ok == 0) {
            bufferSize = receivedBytes;

#ifdef PRINT_DATA
            cout << "Received " << bufferSize << " bytes user data from association " << assocID << ", stream " << streamID << ":" << endl;
            for(size_t i = 0;i < bufferSize;i++) {
               char str[32];
               snprintf((char*)&str,sizeof(str),"%02x ",((unsigned char*)buffer)[i]);
               cout << str;
            }
            cout << endl;
#endif
            result = (int)bufferSize;

            // ====== Peek mode: Restore chunk arrival information ===================
            if(flags & MSG_PEEK) {
               queue.updateNotification(notification);
               updatedNotification = true;
            }
            else {
               sda->sda_bytes_arrived -= receivedBytes;
               if(sda->sda_bytes_arrived > 0) {
                  queue.updateNotification(notification);
                  updatedNotification = true;
               }
               else {
                  flags |= MSG_EOR;
               }
            }
         }
         else {
#ifndef DISABLE_WARNINGS
            cerr << "WARNING: SCTPSocket::internalReceive() - sctp_receive() failed!" << endl;
#endif
            result = -ECONNABORTED;
         }
      }
      else {
         bufferSize = 0;
      }
   }

   // ====== Handle notification ============================================
   else {
      switch(notification.Content.sn_header.sn_type) {
         case SCTP_ASSOC_CHANGE:
            assocID = notification.Content.sn_assoc_change.sac_assoc_id;
          break;
         case SCTP_REMOTE_ERROR:
            assocID = notification.Content.sn_remote_error.sre_assoc_id;
          break;
         case SCTP_SEND_FAILED:
            assocID = notification.Content.sn_send_failed.ssf_assoc_id;
          break;
         case SCTP_SHUTDOWN_EVENT:
            assocID = notification.Content.sn_shutdown_event.sse_assoc_id;
          break;
      }

      // ====== Copy notification ===========================================
      if((receiveNotifications) &&
         (((notification.Content.sn_header.sn_type == SCTP_ASSOC_CHANGE)     && (notificationFlags & SCTP_RECVASSOCEVNT)) ||
          ((notification.Content.sn_header.sn_type == SCTP_PEER_ADDR_CHANGE) && (notificationFlags & SCTP_RECVPADDREVNT)) ||
          ((notification.Content.sn_header.sn_type == SCTP_REMOTE_ERROR)     && (notificationFlags & SCTP_RECVPEERERR))   ||
          ((notification.Content.sn_header.sn_type == SCTP_SEND_FAILED)      && (notificationFlags & SCTP_RECVSENDFAILEVNT)) ||
          ((notification.Content.sn_header.sn_type == SCTP_SHUTDOWN_EVENT)   && (notificationFlags & SCTP_RECVSHUTDOWNEVNT)))) {
         const cardinal toCopy = min((cardinal)notification.Content.sn_header.sn_length - notification.ContentPosition,(cardinal)bufferSize);
         const char* from = (char*)&notification.Content;
         memcpy(buffer,&from[notification.ContentPosition],toCopy);
         bufferSize = toCopy;
         notification.ContentPosition += toCopy;
         if(notification.ContentPosition < notification.Content.sn_header.sn_length) {
            queue.updateNotification(notification);
            updatedNotification = true;
            flags |= MSG_NOTIFICATION;
         }
         else {
            flags |= (MSG_EOR|MSG_NOTIFICATION);
         }

#ifdef PRINT_DATA
            cout << "Received " << bufferSize << " bytes notification data from association " << assocID << ", stream " << streamID << ":" << endl;
            for(size_t i = 0;i < bufferSize;i++) {
               char str[32];
               snprintf((char*)&str,sizeof(str),"%02x ",((unsigned char*)buffer)[i]);
               cout << str;
            }
            cout << endl;
#endif
         result = (int)bufferSize;
      }
      else {
#ifdef PRINT_NOTIFICATION_SKIP
            cout << "WARNING: Skipping " << notification.Content.sn_header.sn_length << " bytes notification data (type "
                 << notification.Content.sn_header.sn_type << ") from association " << assocID << ", stream " << streamID << ":" << endl;
#endif
#ifdef PRINT_DATA
            for(size_t i = 0;i < notification.Content.sn_header.sn_length;i++) {
               char str[32];
               snprintf((char*)&str,sizeof(str),"%02x ",((unsigned char*)&notification.Content)[i]);
               cout << str;
            }
            cout << endl;
#endif
         result = getErrorCode(assocID);
         if(result == 0) {
            result = -EAGAIN;
         }
      }
   }


   // ====== Drop notification, if not updated ==============================
   if(!updatedNotification) {
      queue.dropNotification();
      SCTPAssociation* association = getAssociationForAssociationID(assocID,false);
      if(association != NULL) {
         association->LastUsage = getMicroTime();
         if(association->UseCount > 0) {
#ifdef PRINT_ASSOC_USECOUNT
            cout << "Receive: UseCount decrement for A" << association->getID() << ": "
                 << association->UseCount << " -> ";
#endif
            association->UseCount--;
#ifdef PRINT_ASSOC_USECOUNT
            cout << association->UseCount << endl;
#endif
         }
#ifndef DISABLE_WARNINGS
         else {
            cerr << "INTERNAL ERROR: SCTPSocket::internalReceive() - Too many association usecount decrements!" << endl;
            exit(1);
         }
#endif
         association->ReadReady = (association->hasData() || (getErrorCode(association->AssociationID) < 0));
#ifdef PRINT_RECVSTATUS
         cout << "Association " << association->AssociationID << ": ReadReady=" << association->ReadReady
                                << " ErrorCode=" << getErrorCode(association->AssociationID) << endl;
#endif
      }
      ReadReady = hasData() || (ConnectionRequests != NULL);
#ifdef PRINT_RECVSTATUS
      cout << "Instance " << InstanceName << ": ReadReady=" << ReadReady << endl;
#endif
   }

   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Internal send implementation ######################################
int SCTPSocket::internalSend(const char*            buffer,
                             const size_t           length,
                             const int              flags,
                             const unsigned int     assocID,
                             const unsigned short   streamID,
                             const unsigned int     protoID,
                             const unsigned int     timeToLive,
                             Condition*             waitCondition)
{
   // ====== Check error code ===============================================
   const int errorCode = getErrorCode(assocID);
   if(errorCode != 0) {
      return(errorCode);
   }

   // ====== Do send ========================================================
   int result = 0;
   do {
      SCTPSocketMaster::MasterInstance.lock();

#ifdef PRINT_DATA
      cout << "Sending data to association " << assocID << ", stream " << streamID << ":" << endl;
      for(size_t i = 0;i < length;i++) {
         char str[32];
         snprintf((char*)&str,sizeof(str),"%02x ",((unsigned char*)buffer)[i]);
         cout << str;
      }
      cout << endl;
#endif

#ifdef PRINT_PRSCTP
      if(timeToLive != SCTP_INFINITE_LIFETIME) {
         cout << "Sending " << length << " bytes with lifetime " << timeToLive << "." << endl;
      }
#endif

#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
      result = sctp_send(
                  assocID, streamID,
                  (unsigned char*)buffer, length,
                  protoID,
                  SCTP_USE_PRIMARY,
                  SCTP_NO_CONTEXT,
                  timeToLive,
                  ((flags & MSG_UNORDERED) ? SCTP_UNORDERED_DELIVERY : SCTP_ORDERED_DELIVERY),
                  ((flags & MSG_UNBUNDLED) ? SCTP_BUNDLING_DISABLED : SCTP_BUNDLING_ENABLED));
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
      result = sctp_send(
                  assocID, streamID,
                  (unsigned char*)buffer, length,
                  protoID,
                  SCTP_USE_PRIMARY,
                  timeToLive,
                  SCTP_NO_CONTEXT,
                  ((flags & MSG_UNORDERED) ? SCTP_UNORDERED_DELIVERY : SCTP_ORDERED_DELIVERY),
                  ((flags & MSG_UNBUNDLED) ? SCTP_BUNDLING_DISABLED : SCTP_BUNDLING_ENABLED));
#else
#error Wrong sctplib version!
#endif

      if((result == SCTP_QUEUE_EXCEEDED) && (!(flags & MSG_DONTWAIT))) {
         if(waitCondition != NULL) {
            SCTPSocketMaster::MasterInstance.unlock();
            waitCondition->timedWait(100000);
            SCTPSocketMaster::MasterInstance.lock();
         }
      }
      SCTPSocketMaster::MasterInstance.unlock();
   }
   while((!(flags & MSG_DONTWAIT)) && (result == SCTP_QUEUE_EXCEEDED));

   if(result == SCTP_QUEUE_EXCEEDED) {
      WriteReady = false;
   }
   else {
      WriteReady = true;
   }

   return((result == 0) ? (int)length : -EIO);
}


// ###### Receive ###########################################################
int SCTPSocket::receive(char*           buffer,
                        size_t&         bufferSize,
                        int&            flags,
                        unsigned int&   assocID,
                        unsigned short& streamID,
                        unsigned int&   protoID,
                        uint16_t&       ssn,
                        uint32_t&       tsn)
{
   SCTPNotification notification;
   return(receiveFrom(buffer,bufferSize,
                      flags,
                      assocID, streamID, protoID,
                      ssn, tsn,
                      NULL,notification));
}


// ###### Check, if socket has data for given flags #########################
bool SCTPSocket::hasData()
{
   bool result = false;

   SCTPSocketMaster::MasterInstance.lock();
   if(Flags & SSF_GlobalQueue) {
      result = GlobalQueue.hasData(NotificationFlags);
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Receive ###########################################################
int SCTPSocket::receiveFrom(char*              buffer,
                            size_t&            bufferSize,
                            int&               flags,
                            unsigned int&      assocID,
                            unsigned short&    streamID,
                            unsigned int&      protoID,
                            uint16_t&          ssn,
                            uint32_t&          tsn,
                            SocketAddress***   addressArray,
                            SCTPNotification&  notification)
{
   // ====== Receive ========================================================
   if(!(Flags & SSF_GlobalQueue)) {
#ifndef DISABLE_WARNINGS
      cerr << "WARNING: SCTPSocket::receiveFrom() - No global queue!" << endl;
#endif
      return(-EBADF);
   }
   assocID = 0;
   const int result = internalReceive(
                         GlobalQueue,
                         buffer, bufferSize,
                         flags,
                         assocID, streamID, protoID,
                         ssn, tsn,
                         notification,
                         NotificationFlags);

   // ====== Extract sender's addresses =====================================
   if((result >= 0) && (addressArray != NULL)) {
      unsigned int i;
      *addressArray = SocketAddress::newAddressList(notification.RemoteAddresses);
      if(*addressArray != NULL) {
         for(i = 0;i < notification.RemoteAddresses;i++) {
            (*addressArray)[i] = SocketAddress::createSocketAddress(
                                    0,
                                    (char*)&notification.RemoteAddress[i],
                                    notification.RemotePort);
            if((*addressArray)[i] == NULL) {
#ifndef DISABLE_WARNINGS
               cerr << "WARNING: SCTPSocket::receiveFrom() - Bad address \""
                    << (char*)&notification.RemoteAddress[i] << "\"!" << endl;
#endif
            }
         }
      }
   }

   // ====== Check, if association has to be closed =========================
   checkAutoConnect();

   return(result);
}


// ###### Send ##############################################################
int SCTPSocket::sendTo(const char*          buffer,
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
                       const cardinal       noOfOutgoingStreams)
{
   SCTPSocketMaster::MasterInstance.lock();

   // ====== Check for already created association ==========================
   SCTPAssociation* association = NULL;
   if(destinationAddress != NULL) {
      if(Flags & SSF_AutoConnect) {
         multimap<unsigned int, SCTPAssociation*>::iterator iterator =
            ConnectionlessAssociationList.begin();
         while(iterator != ConnectionlessAssociationList.end()) {
            SCTP_Association_Status status;
            if(sctp_getAssocStatus(iterator->second->AssociationID,&status) == 0) {
#ifdef PRINT_ASSOCSEARCH
               cout << "CL "
                    << destinationAddress->getAddressString(InternetAddress::PF_HidePort|InternetAddress::PF_Address|InternetAddress::PF_Legacy)
                    << " == "
                    << String((const char*)&status.primaryDestinationAddress)
                    << "?" << endl;
#endif
               if( (!iterator->second->IsShuttingDown)               &&
                   (destinationAddress->getPort() == status.destPort) &&
                   (destinationAddress->getAddressString(InternetAddress::PF_HidePort|InternetAddress::PF_Address|InternetAddress::PF_Legacy) == String((const char*)&status.primaryDestinationAddress)) ) {
                  association = iterator->second;
                  break;
               }
            }
            iterator++;
         }
      }
      if(association == NULL) {
         multimap<unsigned int, SCTPAssociation*>::iterator iterator =
            AssociationList.begin();
         while(iterator != AssociationList.end()) {
            SCTP_Association_Status status;
            if(sctp_getAssocStatus(iterator->second->AssociationID,&status) == 0) {
#ifdef PRINT_ASSOCSEARCH
               cout << "CO "
                    << destinationAddress->getAddressString(InternetAddress::PF_HidePort|InternetAddress::PF_Address|InternetAddress::PF_Legacy)
                    << " == "
                    << String((const char*)&status.primaryDestinationAddress)
                    << "?" << endl;
#endif
               if( (!iterator->second->IsShuttingDown)               &&
                   (destinationAddress->getPort() == status.destPort) &&
                   (destinationAddress->getAddressString(InternetAddress::PF_HidePort|InternetAddress::PF_Address|InternetAddress::PF_Legacy) == String((const char*)&status.primaryDestinationAddress)) ) {
                  association = iterator->second;
                  break;
               }
            }
            iterator++;
         }
      }
   }
   else {
#ifdef PRINT_ASSOCSEARCH
      cout << "AssocIDLookup " << assocID << "... ";
#endif
      multimap<unsigned int, SCTPAssociation*>::iterator iterator =
         AssociationList.find(assocID);
      if(iterator != AssociationList.end()) {
#ifdef PRINT_ASSOCSEARCH
         cout << "ok." << endl;
#endif
         association = iterator->second;
      }
      else {
#ifdef PRINT_ASSOCSEARCH
         cout << "failed!" << endl;
#endif
      }
   }
   if(association != NULL) {
#ifdef PRINT_ASSOC_USECOUNT
      cout << "Send: UseCount increment for A" << association->getID() << ": "
           << association->UseCount << " -> ";
#endif
      association->UseCount++;
#ifdef PRINT_ASSOC_USECOUNT
      cout << association->UseCount << endl;
#endif
   }

   SCTPSocketMaster::MasterInstance.unlock();


   // ====== Create new association =========================================
   if((Flags & SSF_AutoConnect) && (association == NULL) && (destinationAddress != NULL)) {
#ifdef PRINT_NEW_ASSOCIATIONS
      cout << "AutoConnect: New outgoing association to "
           << destinationAddress->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy)
           << "..." << endl;
#endif
      const SocketAddress* destinationAddressList[2];
      destinationAddressList[0] = destinationAddress;
      destinationAddressList[1] = NULL;
      association = associate(noOfOutgoingStreams,
                              maxAttempts, maxInitTimeout,
                              (const SocketAddress**)&destinationAddressList,
                              (flags & MSG_DONTWAIT) ? false : true);
      if(association != NULL) {
#ifdef PRINT_NEW_ASSOCIATIONS
         cout << "AutoConnect: New outgoing association to "
                 << destinationAddress->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy)
                 << ", #" << association->getID() << " established!" << endl;
#endif
         SCTPSocketMaster::MasterInstance.lock();
#ifdef PRINT_ASSOC_USECOUNT
         cout << "AutoConnect: UseCount increment for A" << association->getID() << ": "
              << association->UseCount << " -> ";
#endif
         association->UseCount++;
#ifdef PRINT_ASSOC_USECOUNT
         cout << association->UseCount << endl;
#endif
         ConnectionlessAssociationList.insert(pair<unsigned int, SCTPAssociation*>(association->getID(),association));
         SCTPSocketMaster::MasterInstance.unlock();
      }
#ifdef PRINT_NEW_ASSOCIATIONS
      else {
         cout << "AutoConnect: New outgoing association to "
              << destinationAddress->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy)
              << " failed!" << endl;
      }
#endif
   }


   // ====== Send data ======================================================
   int result;
   if(association != NULL) {
      if((buffer != NULL) && (length > 0)) {
         result = association->send(buffer,length,flags,streamID,protoID,timeToLive,useDefaults);
      }
      else {
         result = 0;
      }

      // ====== Remove association, if SHUTDOWN flag is set =================
      if((flags & MSG_EOF) || (flags & MSG_ABORT)) {
         if(flags & MSG_ABORT) {
            association->abort();
         }
         else if(flags & MSG_EOF) {
            association->shutdown();
         }
         if(Flags & SSF_AutoConnect) {
#ifdef PRINT_SHUTDOWNS
            cout << "AutoConnect: Shutdown of outgoing association ";
            if(destinationAddress != NULL) {
               cout << "to " << *destinationAddress << "..." << endl;
            }
            else {
               cout << "A" << assocID << "..." << endl;
            }
#endif
            SCTPSocketMaster::MasterInstance.lock();
            multimap<unsigned int, SCTPAssociation*>::iterator iterator =
               ConnectionlessAssociationList.find(association->getID());
            if(iterator != ConnectionlessAssociationList.end()) {
               ConnectionlessAssociationList.erase(iterator);
            }
            SCTPSocketMaster::MasterInstance.unlock();
            delete association;
            association = NULL;
#ifdef PRINT_SHUTDOWNS
            cout << "AutoConnect: Shutdown of outgoing association ";
            if(destinationAddress != NULL) {
               cout << "to " << *destinationAddress << endl;
            }
            else {
               cout << "A" << assocID << endl;
            }
            cout << " completed!" << endl;
#endif
         }
         checkAutoConnect();
      }
   }
   else {
      result = -EIO;
   }


   SCTPSocketMaster::MasterInstance.lock();
   if(association != NULL) {
      association->LastUsage = getMicroTime();
      if(association->UseCount > 0) {
#ifdef PRINT_ASSOC_USECOUNT
      cout << "Send: UseCount decrement for A" << association->getID() << ": "
           << association->UseCount << " -> ";
#endif
         association->UseCount--;
#ifdef PRINT_ASSOC_USECOUNT
      cout << association->UseCount << endl;
#endif
      }
#ifndef DISABLE_WARNINGS
      else {
         cerr << "INTERNAL ERROR: SCTPSocket::sendTo() - Too many association usecount decrements!" << endl;
         exit(1);
      }
#endif
   }
   SCTPSocketMaster::MasterInstance.unlock();


   return(result);
}


// ###### Peel UDP-like association off #####################################
SCTPAssociation* SCTPSocket::peelOff(const SocketAddress& destinationAddress)
{
   SCTPAssociation* association = NULL;
   SCTPSocketMaster::MasterInstance.lock();

   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   while(iterator != ConnectionlessAssociationList.end()) {
      SCTP_Association_Status status;
      if(sctp_getAssocStatus(iterator->second->AssociationID,&status) == 0) {
#ifdef PRINT_ASSOCSEARCH
         cout << "CL "
              << destinationAddress.getAddressString(InternetAddress::PF_HidePort|InternetAddress::PF_Address|InternetAddress::PF_Legacy)
              << " == "
              << String((const char*)&status.primaryDestinationAddress)
              << "?" << endl;
#endif
         if( (!iterator->second->IsShuttingDown)               &&
             (destinationAddress.getPort() == status.destPort) &&
             (destinationAddress.getAddressString(InternetAddress::PF_HidePort|InternetAddress::PF_Address|InternetAddress::PF_Legacy) == String((const char*)&status.primaryDestinationAddress)) ) {
            ConnectionlessAssociationList.erase(iterator);
            association = iterator->second;
            break;
         }
      }
      iterator++;
   }

   SCTPSocketMaster::MasterInstance.unlock();
   return(association);
}


// ###### Peel UDP-like association off #####################################
SCTPAssociation* SCTPSocket::peelOff(const unsigned int assocID)
{
   SCTPAssociation* association = NULL;

   SCTPSocketMaster::MasterInstance.lock();
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.find(assocID);
   if( (iterator != ConnectionlessAssociationList.end()) &&
       (!iterator->second->IsShuttingDown) ) {
      association = iterator->second;
      ConnectionlessAssociationList.erase(iterator);
   }
   SCTPSocketMaster::MasterInstance.unlock();

   return(association);
}


// ###### Get instance parameters ###########################################
bool SCTPSocket::getAssocDefaults(SCTP_Instance_Parameters& assocDefaults)
{
   SCTPSocketMaster::MasterInstance.lock();
   const int ok = sctp_getAssocDefaults(InstanceName,&assocDefaults);
   SCTPSocketMaster::MasterInstance.unlock();
   return(ok == 0);
}


// ###### Set instance parameters ###########################################
bool SCTPSocket::setAssocDefaults(const SCTP_Instance_Parameters& assocDefaults)
{
   SCTPSocketMaster::MasterInstance.lock();
   const int ok = sctp_setAssocDefaults(InstanceName,
                                        (SCTP_Instance_Parameters*)&assocDefaults);
   SCTPSocketMaster::MasterInstance.unlock();
   return(ok == 0);
}


// ###### Get association parameters ########################################
bool SCTPSocket::getAssocStatus(const unsigned int       assocID,
                                SCTP_Association_Status& associationParameters)
{
   SCTPSocketMaster::MasterInstance.lock();
   const int ok = sctp_getAssocStatus(assocID, &associationParameters);
   SCTPSocketMaster::MasterInstance.unlock();
   return(ok == 0);
}


// ###### Set association parameters ########################################
bool SCTPSocket::setAssocStatus(const unsigned int             assocID,
                                const SCTP_Association_Status& associationParameters)
{
   SCTPSocketMaster::MasterInstance.lock();
   const int ok = sctp_setAssocStatus(assocID,
                                      (SCTP_Association_Status*)&associationParameters);
   SCTPSocketMaster::MasterInstance.unlock();
   return(ok == 0);
}


// ###### Get path index for address #########################################
int SCTPSocket::getPathIndexForAddress(const unsigned int          assocID,
                                       const struct SocketAddress* address,
                                       SCTP_PathStatus&            pathParameters)
{
   if(address == NULL) {
#ifdef PRINT_PATHFORINDEX
      cout << "pathForIndex - primary" << endl;
#endif
      return(sctp_getPrimary(assocID));
   }

#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
   SCTP_Association_Status status;
   if(sctp_getAssocStatus(assocID,&status) != 0) {
#ifndef DISABLE_WARNINGS
      cerr << "INTERNAL ERROR: SCTPSocket::getPathIndexForAddress() - Unable to get association status!" << endl;
#endif
      return(-1);
   }
#endif

   const String addressString = address->getAddressString(SocketAddress::PF_Address|SocketAddress::PF_HidePort|SocketAddress::PF_Legacy);

   for(unsigned int i = 0;;i++) {
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
      const int index = i;
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
       const int index = status.destinationPathIDs[i];
#else
#error Wrong sctplib version!
#endif

      const int ok = sctp_getPathStatus(assocID, index, &pathParameters);
      if(ok != 0) {
         break;
      }
#ifdef PRINT_PATHFORINDEX
      cout << "pathForIndex: " << index << ": " << addressString << " == " << pathParameters.destinationAddress << "?" << endl;
#endif
      if(addressString == String((char*)&pathParameters.destinationAddress)) {
#ifdef PRINT_PATHFORINDEX
         cout << "   => " << index << endl;
#endif
         return(index);
      }
   }
#ifdef PRINT_PATHFORINDEX
      cout << "pathForIndex - failed" << endl;
#endif
   return(-1);
}


// ###### Get path parameters ###############################################
bool SCTPSocket::getPathParameters(const unsigned int          assocID,
                                   const struct SocketAddress* address,
                                   SCTP_PathStatus&            pathParameters)
{
   SCTPSocketMaster::MasterInstance.lock();
   const int pathIndex = getPathIndexForAddress(assocID,address,pathParameters);
   if(pathIndex >= 0) {
      sctp_getPathStatus(assocID,pathIndex,&pathParameters);
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(pathIndex >= 0);
}


// ###### Set path parameters ###############################################
bool SCTPSocket::setPathParameters(const unsigned int          assocID,
                                   const struct SocketAddress* address,
                                   const SCTP_PathStatus&      pathParameters)
{
   SCTP_PathStatus oldPathParameters;

   SCTPSocketMaster::MasterInstance.lock();
   int pathIndex = getPathIndexForAddress(assocID,address,oldPathParameters);
   if(pathIndex >= 0) {
      if(pathParameters.heartbeatIntervall == (unsigned int)-1) {
         if(sctp_requestHeartbeat(assocID,pathIndex)) {
            pathIndex = -1;
         }
      }
      else {
         if(sctp_changeHeartBeat(assocID,
                                 pathIndex,
                                 (pathParameters.heartbeatIntervall > 0) ? SCTP_HEARTBEAT_ON : SCTP_HEARTBEAT_OFF,
                                 pathParameters.heartbeatIntervall) != 0) {
            pathIndex = -1;
         }
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(pathIndex >= 0);
}


// ###### Get default settings ##############################################
bool SCTPSocket::getAssocIODefaults(const unsigned int          assocID,
                                    struct AssocIODefaults& defaults)
{
   SCTPSocketMaster::MasterInstance.lock();
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   if(iterator != ConnectionlessAssociationList.end()) {
      SCTPAssociation* association = iterator->second;
      association->getAssocIODefaults(defaults);
      return(true);
   }
   return(false);
}


// ###### Set default settings ##############################################
bool SCTPSocket::setAssocIODefaults(const unsigned int                assocID,
                                    const struct AssocIODefaults& defaults)
{
   SCTPSocketMaster::MasterInstance.lock();
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   if(iterator != ConnectionlessAssociationList.end()) {
      SCTPAssociation* association = iterator->second;
      association->setAssocIODefaults(defaults);
      return(true);
   }
   return(false);
}


// ###### Set default timeouts ##############################################
bool SCTPSocket::setDefaultStreamTimeouts(const unsigned int   assocID,
                                          const unsigned int   timeout,
                                          const unsigned short start,
                                          const unsigned short end)
{
   SCTPSocketMaster::MasterInstance.lock();
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   if(iterator != ConnectionlessAssociationList.end()) {
      SCTPAssociation* association = iterator->second;
      association->setDefaultStreamTimeouts(timeout,start,end);
      return(true);
   }
   return(false);
}


// ###### Get default timeout ###############################################
bool SCTPSocket::getDefaultStreamTimeout(const unsigned int   assocID,
                                         const unsigned short streamID,
                                         unsigned int&        timeout)
{
   SCTPSocketMaster::MasterInstance.lock();
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   if(iterator != ConnectionlessAssociationList.end()) {
      SCTPAssociation* association = iterator->second;
      association->getDefaultStreamTimeout(streamID,timeout);
      return(true);
   }
   return(false);
}


// ###### Set send buffer size ##############################################
bool SCTPSocket::setSendBuffer(const size_t size)
{
   bool ok = true;
   SCTPSocketMaster::MasterInstance.lock();
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   if(iterator != ConnectionlessAssociationList.end()) {
      SCTPAssociation* association = iterator->second;
      if(association->setSendBuffer(size) == false) {
         ok = false;
      }
      iterator++;
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(ok);
}


// ###### Set receive buffer size ###########################################
bool SCTPSocket::setReceiveBuffer(const size_t size)
{
   bool ok = true;
   SCTPSocketMaster::MasterInstance.lock();
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   if(iterator != ConnectionlessAssociationList.end()) {
      SCTPAssociation* association = iterator->second;
      if(association->setReceiveBuffer(size) == false) {
         ok = false;
      }
      iterator++;
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(ok);
}


// ###### Set traffic class #################################################
bool SCTPSocket::setTrafficClass(const card8 trafficClass,
                                 const int   streamID)
{
   bool ok = true;
   SCTPSocketMaster::MasterInstance.lock();
   DefaultTrafficClass = trafficClass;
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   if(iterator != ConnectionlessAssociationList.end()) {
      SCTPAssociation* association = iterator->second;
      if(association->setTrafficClass(trafficClass,streamID) == false) {
         ok = false;
      }
      iterator++;
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(ok);
}


// ###### Get primary address ###############################################
SocketAddress* SCTPSocket::getPrimaryAddress(const unsigned int assocID)
{
   SCTPSocketMaster::MasterInstance.lock();

   SocketAddress* address = NULL;
   const int index        = sctp_getPrimary(assocID);
   if(index >= 0) {
      SCTP_Path_Status pathStatus;
      const int result = sctp_getPathStatus(assocID,index,&pathStatus);
      if(result == 0) {
         address = SocketAddress::createSocketAddress(0,(char*)&pathStatus.destinationAddress);
      }
   }

   SCTPSocketMaster::MasterInstance.unlock();
   return(address);
}


// ###### Set primary address ###############################################
bool SCTPSocket::setPrimary(const unsigned int   assocID,
                            const SocketAddress& primary)
{
   SCTP_PathStatus pathParameters;
   int             result = -1;

   SCTPSocketMaster::MasterInstance.lock();
   int index = getPathIndexForAddress(assocID,&primary,pathParameters);
   if(index >= 0) {
#ifdef PRINT_SETPRIMARY
      cout << "setPrimary: Setting primary address to " << primary << endl;
#endif
      result = sctp_setPrimary(assocID,index);
#ifdef PRINT_SETPRIMARY
      if(result != 0) {
         cerr << "WARNING: sctp_setPrimary() failed, error #" << result << endl;
      }
#endif
   }
   SCTPSocketMaster::MasterInstance.unlock();

   return(result == 0);
}


// ###### Set peer primary address ##########################################
bool SCTPSocket::setPeerPrimary(const unsigned int   assocID,
                                const SocketAddress& primary)
{
   SCTPSocketMaster::MasterInstance.lock();
   unsigned char address[SCTP_MAX_IP_LEN];
   snprintf((char*)&address,sizeof(address),"%s",
            primary.getAddressString().getData());
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
   const int result = sctp_setRemotePrimary(assocID,address);
#else
   const int result = -1;
#endif
   SCTPSocketMaster::MasterInstance.unlock();
   return(result == 0);
}


// ###### Add address #######################################################
bool SCTPSocket::addAddress(const unsigned int   assocID,
                            const SocketAddress& addAddress)
{
   if(assocID == 0) {
      bool ok = true;
      SCTPSocketMaster::MasterInstance.lock();
      multimap<unsigned int, SCTPAssociation*>::iterator iterator =
         ConnectionlessAssociationList.begin();
      if(iterator != ConnectionlessAssociationList.end()) {
         SCTPAssociation* association = iterator->second;
         if(association->addAddress(addAddress) == false) {
            ok = false;
         }
         iterator++;
      }
      SCTPSocketMaster::MasterInstance.unlock();
      return(ok);
   }

   SCTPSocketMaster::MasterInstance.lock();
   unsigned char address[SCTP_MAX_IP_LEN];
   snprintf((char*)&address,sizeof(address),"%s",
            addAddress.getAddressString().getData());
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
   cerr << "NOT IMPLEMENTED: sctp_addIPAddress()" << endl;
   const int result = -1;
#else
   const int result = sctp_addIPAddress(assocID,address,&CorrelationID);
#endif
#ifdef PRINT_ADDIP
   cout << "AddIP: " << addAddress << " -> result=" << result << endl;
#endif
   CorrelationID++;
   SCTPSocketMaster::MasterInstance.unlock();
   return(result == 0);
}


// ###### Delete address ####################################################
bool SCTPSocket::deleteAddress(const unsigned int   assocID,
                               const SocketAddress& delAddress)
{
   if(assocID == 0) {
      bool ok = true;
      SCTPSocketMaster::MasterInstance.lock();
      multimap<unsigned int, SCTPAssociation*>::iterator iterator =
         ConnectionlessAssociationList.begin();
      if(iterator != ConnectionlessAssociationList.end()) {
         SCTPAssociation* association = iterator->second;
         if(association->deleteAddress(delAddress) == false) {
            ok = false;
         }
         iterator++;
      }
      SCTPSocketMaster::MasterInstance.unlock();
      return(ok);
   }

   SCTPSocketMaster::MasterInstance.lock();
   unsigned char address[SCTP_MAX_IP_LEN];
   snprintf((char*)&address,sizeof(address),"%s",
            delAddress.getAddressString().getData());
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
   cerr << "NOT IMPLEMENTED: sctp_deleteIPAddress()" << endl;
   const int result = -1;
#else
   const int result = sctp_deleteIPAddress(assocID,address,&CorrelationID);
#endif
#ifdef PRINT_ADDIP
   cout << "DeleteIP: " << delAddress << " -> result=" << result << endl;
#endif
   CorrelationID++;
   SCTPSocketMaster::MasterInstance.unlock();
   return(result == 0);
}


// ###### Check for necessity to auto-close associations ####################
void SCTPSocket::checkAutoClose()
{
   const card64 now = getMicroTime();
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   while(iterator != ConnectionlessAssociationList.end()) {

      SCTPAssociation* association = iterator->second;
#ifdef PRINT_AUTOCLOSE_CHECK
      cout << "AutoConnect: Check for AutoClose:" << endl
           << "   AssocID          = " << association->getID() << endl
           << "   UseCount         = " << association->UseCount << endl
           << "   LastUsage        = " << now - association->LastUsage << endl
           << "   AutoCloseTimeout = " << AutoCloseTimeout << endl;
#endif
      if((association->UseCount == 0) &&
         (AutoCloseTimeout > 0) &&
         (now - association->LastUsage > AutoCloseTimeout)) {
#ifdef PRINT_AUTOCLOSE_TIMEOUT
         const unsigned int assocID = association->getID();
         cout << "AutoConnect: Doing shutdown of association #" << assocID << " due to timeout" << endl;
#endif
         association->shutdown();
         iterator++;
      }
      else if((association->ShutdownCompleteNotification)         ||
              (association->CommunicationLostNotification)) {
#ifdef PRINT_AUTOCLOSE_TIMEOUT
         const unsigned int assocID = association->getID();
         cout << "AutoConnect: Removing association #" << assocID << ": ";
         if(association->ShutdownCompleteNotification) {
            cout << "shutdown complete";
         }
         else if(association->CommunicationLostNotification) {
            cout << "communication lost";
         }
         cout << "..." << endl;
#endif
         delete association;
#ifdef PRINT_AUTOCLOSE_TIMEOUT
         cout << "AutoConnect: AutoClose of association #" << assocID << " completed!" << endl;
#endif
         ConnectionlessAssociationList.erase(iterator);
         iterator = ConnectionlessAssociationList.begin();
      }
      else {
         iterator++;
      }
   }
}


// ###### AutoConnect maintenance ###########################################
void SCTPSocket::checkAutoConnect()
{
   if(Flags & SSF_AutoConnect) {
      SCTPSocketMaster::MasterInstance.lock();

      // ====== Check, if there are new incoming associations ===============
      const cardinal oldFlags = Flags;
      Flags |= SSF_Listening;
      SCTPAssociation* association = accept(NULL,false);
      while(association != NULL) {
#ifdef PRINT_NEW_ASSOCIATIONS
         cout << "AutoConnect: New incoming association #" << association->getID() << "..." << endl;
#endif
         ConnectionlessAssociationList.insert(pair<unsigned int, SCTPAssociation*>(association->getID(),association));
         association = accept(NULL,false);
      }
      Flags = oldFlags;

      SCTPSocketMaster::MasterInstance.unlock();
   }
}
