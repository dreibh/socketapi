/*
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2025 by Thomas Dreibholz
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
 * Purpose: SCTP Socket
 *
 */

#include "tdsystem.h"
#include "tools.h"
#include "sctpsocket.h"
#include "sctpsocketmaster.h"


// #define PRINT_BIND
// #define PRINT_UNBIND
// #define PRINT_ADDIP
// #define PRINT_ACCEPT
// #define PRINT_ASSOCIATE
// #define PRINT_NEW_ASSOCIATIONS
// #define PRINT_SEND_TO_ALL
// #define PRINT_SHUTDOWNS
// #define PRINT_PRSCTP
// #define PRINT_NOTIFICATION_SKIP
// #define PRINT_DATA
// #define PRINT_RECVSTATUS
// #define PRINT_SENDSTATUS
// #define PRINT_SETPRIMARY
// #define PRINT_SENDTO
//
//
// #define PRINT_AUTOCLOSE_TIMEOUT
// #define PRINT_AUTOCLOSE_CHECK
//
//
// #define PRINT_RECVWAIT
// #define PRINT_ISSHUTDOWN
// #define PRINT_PATHFORINDEX
// #define PRINT_ASSOCSEARCH
// #define PRINT_ASSOC_USECOUNT
// #define PRINT_RTO


// #define TEST_PARTIAL_DELIVERY
// #define PRINT_PARTIAL_DELIVERY
// #define PARTIAL_DELIVERY_MAXSIZE 67



// ###### Constructor #######################################################
SCTPSocket::SCTPSocket(const int family, const cardinal flags)
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
   Family              = family;

   AutoCloseRecursion  = false;

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
   SCTPAssociation* association = NULL;

   SCTPSocketMaster::MasterInstance.lock();
   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      AssociationList.find(assocID);
   if(iterator != AssociationList.end()) {
      if(!((iterator->second->IsShuttingDown) && (activeOnly))) {
         association = iterator->second;
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();

   return(association);
}


// ###### Get local addresses ###############################################
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19)
bool SCTPSocket::getLocalAddresses(SocketAddress**& addressArray)
{
   // ====== Get local addresses ============================================
   bool         result = true;
   SCTPSocketMaster::MasterInstance.lock();
   if(InstanceName > 0) {
      addressArray = SocketAddress::newAddressList(NoOfLocalAddresses);
      if(addressArray == NULL) {
         for(int i = 0;i < NoOfLocalAddresses;i++) {
            addressArray[i] = SocketAddress::createSocketAddress(0,(char*)LocalAddressList[i],LocalPort);
            if(addressArray[i] == NULL) {
#ifndef DISABLE_WARNINGS
               std::cerr << "WARNING: SCTPSocket::getLocalAddresses() - Bad address "
                  << *(LocalAddressList[i]) << ", port " << LocalPort << "!" << std::endl;
#endif
               SocketAddress::deleteAddressList(addressArray);
               result = false;
            }
         }
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
               std::cerr << "WARNING: SCTPSocket::getLocalAddresses() - Bad address "
                    << parameters.localAddressList[i] << ", port " << LocalPort << "!" << std::endl;
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
      std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
      std::cerr << "WARNING: SCTPSocket::bind() - SCTP is not initialized!" << std::endl;
#endif
      return(-EPROTONOSUPPORT);
   }

   SCTPSocketMaster::MasterInstance.lock();
   if(!SCTPSocketMaster::MasterInstance.running()) {
      // Start SCTPSocketMaster thread.
      if(SCTPSocketMaster::MasterInstance.start() == false) {
#ifndef DISABLE_WARNINGS
         std::cerr << "WARNING: SCTPSocket::bind() - Unable to start master thread!" << std::endl;
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
   for(unsigned int i = 0;i < std::min(NoOfLocalAddresses,(unsigned int)SCTP_MAX_NUM_ADDRESSES);i++) {
      const InternetAddress* localAddress = dynamic_cast<const InternetAddress*>(localAddressList[i]);
      const bool isIPv6 = (localAddress != NULL) ? localAddress->isIPv6() : false;
      if(isIPv6 && (Family == AF_INET6)) {
         snprintf((char*)&LocalAddressList[i],SCTP_MAX_IP_LEN, "%s",
                  localAddressList[i]->getAddressString(
                     SocketAddress::PF_HidePort|SocketAddress::PF_Address).getData());
      }
      else {
         snprintf((char*)&LocalAddressList[i],SCTP_MAX_IP_LEN, "%s",
                  localAddressList[i]->getAddressString(
                     SocketAddress::PF_HidePort|SocketAddress::PF_Address|
                     SocketAddress::PF_Legacy).getData());
      }
   }
#ifdef PRINT_BIND
   std::cout << "Binding to {";
   for(unsigned int i = 0;i < NoOfLocalAddresses;i++) {
      std::cout << " " << LocalAddressList[i] << " ";
   }
   std::cout << "}, port " << LocalPort << "." << std::endl;
#endif
   if(NoOfLocalAddresses < 1) {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: SCTPSocket::bind() - No local addresses!" << std::endl;
#endif
      SCTPSocketMaster::MasterInstance.unlock();
      return(-EINVAL);
   }


   // ====== Register SCTP instance =========================================
   if(LocalPort == 0) {
      for(cardinal i = 0;i < 50000;i++) {
         const card16 port = (card16)(16384 + SCTPSocketMaster::Random.random32() % (61000 - 16384));
         InstanceName = sctp_registerInstance(port, NoOfInStreams, NoOfOutStreams,
                                              NoOfLocalAddresses, LocalAddressList,
                                              SCTPSocketMaster::Callbacks);
         if(InstanceName > 0) {
            LocalPort = port;
#ifdef PRINT_BIND
            std::cout << "Allocated port " << LocalPort << std::endl;
#endif
            break;
         }
      }
   }
   else {
      InstanceName = sctp_registerInstance(LocalPort, NoOfInStreams, NoOfOutStreams,
                                           NoOfLocalAddresses, LocalAddressList,
                                           SCTPSocketMaster::Callbacks);
      if(InstanceName <= 0) {
         /* If the socket has been closed recently, it may not be deleted yet
            (by garbage collector thread). Therefore, we run the garbage collector
            now and try again ... */
         SCTPSocketMaster::socketGarbageCollection();
         InstanceName = sctp_registerInstance(LocalPort, NoOfInStreams, NoOfOutStreams,
                                              NoOfLocalAddresses, LocalAddressList,
                                              SCTPSocketMaster::Callbacks);
      }
   }
   if(InstanceName <= 0) {
#ifdef PRINT_BIND
      std::cerr << "ERROR: SCTPSocket::bind() - sctp_registerInstance() failed!" << std::endl;
#endif
      SCTPSocketMaster::MasterInstance.unlock();
      return(-EADDRINUSE);
   }


   // ====== Add socket to global list ======================================
   SCTPSocketMaster::SocketList.insert(std::pair<unsigned short, SCTPSocket*>(InstanceName,this));


   SCTPSocketMaster::MasterInstance.unlock();
   return(0);
}


// ###### Release socket binding ############################################
void SCTPSocket::unbind(bool sendAbort)
{
   if(InstanceName > 0) {
      SCTPSocketMaster::MasterInstance.lock();
#ifdef PRINT_UNBIND
      std::cout << "Unbinding...";
#endif

      // ====== Delete all associations made by sendTo() ====================
      std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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

      // ====== Delete associations created with associate() or accepted ====
      iterator = AssociationList.begin();
      while(iterator != AssociationList.end()) {
         SCTPAssociation* association = iterator->second;
         AssociationList.erase(iterator);
         if(sendAbort) {
            association->abort();
         }
         delete association;
         iterator = AssociationList.begin();
      }

      // ====== Remove incoming associations ================================
      while(ConnectionRequests != NULL) {
         SCTPAssociation* association   = ConnectionRequests->Association;
         IncomingConnection* oldRequest = ConnectionRequests;
         ConnectionRequests = oldRequest->NextConnection;
         delete association;
         delete oldRequest;
      }

      // ====== Remove socket from global list ==============================
      std::multimap<int, SCTPSocket*>::iterator socketIterator =
         SCTPSocketMaster::SocketList.find(InstanceName);
      if(socketIterator != SCTPSocketMaster::SocketList.end()) {
         SCTPSocketMaster::SocketList.erase(socketIterator);
      }
      else {
#ifndef DISABLE_WARNINGS
         std::cerr << "INTERNAL ERROR: SCTPSocket::unbind() - Erase failed for instance "
                   << InstanceName << "!" << std::endl;
         abort();
#endif
      }

      // ====== Mark SCTP instance for unregistering ========================
      SCTPSocketMaster::delayedDeleteSocket(InstanceName);

      SCTPSocketMaster::MasterInstance.unlock();

      GlobalQueue.flush();
      InstanceName  = 0;
      CorrelationID = 0;
      Flags &= ~SSF_Listening;

#ifdef PRINT_UNBIND
      std::cout << "Unbind complete." << std::endl;
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
      std::cerr << "ERROR: SCTPSocket::accept() - Socket is not in server mode, call listen() first!" << std::endl;
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
         std::cerr << "ERROR: SCTPSocket::accept() - Out of memory!" << std::endl;
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
               std::cerr << "WARNING: SCTPSocket::accept() - Bad address "
                         << ConnectionRequests->Notification.RemoteAddress[i] << ", port " << ConnectionRequests->Notification.RemotePort << "!" << std::endl;
#endif
               SocketAddress::deleteAddressList(*addressArray);
            }
         }
      }
   }

#ifdef PRINT_ACCEPT
   std::cout << "Accepted association #" << ConnectionRequests->Association->AssociationID << " from {";
   for(unsigned int i = 0;i < ConnectionRequests->Notification.RemoteAddresses;i++) {
      InternetAddress address(String((char*)&ConnectionRequests->Notification.RemoteAddress[i]),ConnectionRequests->Notification.RemotePort);
      std::cout << " " << address.getAddressString(SocketAddress::PF_Address|SocketAddress::PF_Legacy) << " ";
   }
   std::cout << "}." << std::endl;
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
         std::cerr << "WARNING: SCTPSocket::associate() - Unable to set new instance parameters!" << std::endl;
#endif
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: SCTPSocket::associate() - Unable to get instance parameters!" << std::endl;
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
         const InternetAddress* destinationAddress = dynamic_cast<const InternetAddress*>(destinationAddressList[i]);
         const bool isIPv6 = (destinationAddress != NULL) ? destinationAddress->isIPv6() : false;
         if(isIPv6 && (Family == AF_INET6)) {
            snprintf((char*)&addressArray[i], SCTP_MAX_IP_LEN, "%s",
                     destinationAddressList[i]->getAddressString(SocketAddress::PF_HidePort|SocketAddress::PF_Address).getData());
         }
         else {
            snprintf((char*)&addressArray[i], SCTP_MAX_IP_LEN, "%s",
                     destinationAddressList[i]->getAddressString(SocketAddress::PF_HidePort|SocketAddress::PF_Address|SocketAddress::PF_Legacy).getData());
         }
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
      std::cerr << "ERROR: SCTPSocket::associate() - No destination addresses given?!" << std::endl;
#endif
   }

   if(!setAssocDefaults(oldParameters)) {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: SCTPSocket::associate() - Unable to restore old instance parameters!" << std::endl;
#endif
   }

#ifdef PRINT_ASSOCIATE
   std::cout << "Association to { ";
   for(unsigned int i = 0;i < destinationAddresses;i++) {
      std::cout << addressArray[i];
      if(i < (destinationAddresses - 1)) {
         std::cout << ", ";
      }
   }
   std::cout << " }, port " << destinationAddressList[0]->getPort()
             << " => ID #" << assocID << "." << std::endl;
#endif


   // ====== Create SCTPAssociation object ==================================
   SCTPAssociation* association = NULL;
   if(assocID != 0) {
      association = new SCTPAssociation(this, assocID, NotificationFlags,
                                        Flags & SCTPSocket::SSF_GlobalQueue);
      if(association == NULL) {
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
         sctp_abort(assocID, 0, NULL);
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
         sctp_abort(assocID);
#else
#error Wrong sctplib version!
#endif
         sctp_deleteAssociation(assocID);
#ifndef DISABLE_WARNINGS
         std::cerr << "ERROR: SCTPSocket::associate() - Out of memory!" << std::endl;
#endif
      }
      else {
#ifdef PRINT_ASSOC_USECOUNT
         std::cout << "Associate: UseCount increment for A" << association->getID() << ": "
                   << association->UseCount << " -> ";
#endif
         association->UseCount++;
#ifdef PRINT_ASSOC_USECOUNT
         std::cout << association->UseCount << std::endl;
#endif
         association->setTrafficClass(DefaultTrafficClass);

         association->RTOMaxIsInitTimeout = true;
         association->RTOMax              = oldParameters.rtoMax;
         association->InitTimeout         = maxInitTimeout;

         association->PreEstablishmentAddressList = SocketAddress::newAddressList(destinationAddresses);
         if(association->PreEstablishmentAddressList != NULL) {
            for(unsigned int i = 0;i < destinationAddresses;i++) {
               association->PreEstablishmentAddressList[i] =
                  destinationAddressList[i]->duplicate();
            }
         }

#ifdef PRINT_RTO
         std::cout << "associate() - InitTimeout=" << association->InitTimeout << " SavedMaxRTO=" << association->RTOMax << std::endl;
#endif
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();

   // ====== Wait for peer's connection up notification =====================
   if(association != NULL) {
      if(blocking) {
#ifdef PRINT_ASSOCIATE
         std::cout << "Waiting for establishment of association #" << assocID << "..." << std::endl;
#endif
         while(association->EstablishCondition.timedWait(100000) == false) {
            checkAutoConnect();
         }
         if(!association->CommunicationUpNotification) {
#ifdef PRINT_ASSOCIATE
            std::cout << "Association #" << assocID << " failed!" << std::endl;
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
      std::cout << "Association #" << assocID << " established." << std::endl;
   }
#endif

   // ====== Decrement use count ============================================
   SCTPSocketMaster::MasterInstance.lock();
   if(association != NULL) {
#ifdef PRINT_ASSOC_USECOUNT
         std::cout << "Associate: UseCount decrement for A" << association->getID() << ": "
                   << association->UseCount << " -> ";
#endif
      association->UseCount--;
#ifdef PRINT_ASSOC_USECOUNT
      std::cout << association->UseCount << std::endl;
#endif
   }
   SCTPSocketMaster::MasterInstance.unlock();

   return(association);
}


// ###### Get error code for given association ID ###########################
int SCTPSocket::getErrorCode(const unsigned int assocID)
{
   SCTPAssociation* association = getAssociationForAssociationID(assocID, false);
   if(association != NULL) {
      if(association->ShutdownCompleteNotification) {
         association->HasException = true;
         return(-ECONNRESET);
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
                                SocketAddress**        address,
                                const unsigned int     notificationFlags)
{
   // ====== Check parameters ===============================================
   if(bufferSize == 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: SCTPSocket::internalReceive() - Data buffer size is zero!" << std::endl;
#endif
      return(-EINVAL);
   }

   // ====== Get next data or notification from queue =====================
#ifdef PRINT_RECVWAIT
   std::cout << "Waiting...";
   std::cout.flush();
#endif
   SCTPSocketMaster::MasterInstance.lock();
   SCTPNotification notification;
   bool received = queue.getNotification(notification);
   while(received == false) {
      int errorCode = getErrorCode(assocID);
      SCTPSocketMaster::MasterInstance.unlock();

      // ====== No chunk available -> wait for chunk ======================
      if(errorCode != 0) {
         bufferSize = 0;
         if((errorCode == -ECONNRESET) && !(queue.hasData(notificationFlags))) {
#ifdef PRINT_ISSHUTDOWN
            std::cout << "Socket has been shut down -> leaving waiting loop!" << std::endl;
#endif
            flags &= ~MSG_NOTIFICATION;
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
   std::cout << "Wakeup!" << std::endl;
#endif


   // ====== Read data ======================================================
   // If MSG_NOTIFICATION is set, notifications are received!
   const bool receiveNotifications = (flags & MSG_NOTIFICATION);
   bool updatedNotification = false;
   int result               = 0;
   if(notification.Content.sn_header.sn_type == SCTP_DATA_ARRIVE) {
      // ====== Some test stuff for the partial delivery API ================
#ifdef TEST_PARTIAL_DELIVERY
#ifdef PRINT_PARTIAL_DELIVERY
      std::cout << "Partial Delivery Test: " << bufferSize << " -> ";
#endif
      bufferSize = MIN(bufferSize, PARTIAL_DELIVERY_MAXSIZE);
#ifdef PRINT_PARTIAL_DELIVERY
      std::cout << bufferSize << std::endl;
#endif
#endif

      flags &= ~MSG_NOTIFICATION;
      sctp_data_arrive* sda = &notification.Content.sn_data_arrive;
      if(sda->sda_bytes_arrived > 0) {
         assocID  = sda->sda_assoc_id;
         streamID = sda->sda_stream;
         protoID  = sda->sda_ppid;
         if(sda->sda_flags & SCTP_ARRIVE_UNORDERED) {
            flags |= MSG_UNORDERED;
         }
         unsigned int receivedBytes = std::min((size_t) sda->sda_bytes_arrived, (size_t) bufferSize);
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0)
         unsigned int pathIndex;
         const int ok = sctp_receivefrom(assocID, streamID,
                                         (unsigned char*)buffer,
                                         (unsigned int*)&receivedBytes,
                                         &ssn,
                                         &tsn,
                                         &pathIndex,
                                         (flags & MSG_PEEK) ? SCTP_MSG_PEEK : SCTP_MSG_DEFAULT);
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
         const int ok = sctp_receive(assocID, streamID,
                                     (unsigned char*)buffer,
                                     (unsigned int*)&receivedBytes,
                                     &ssn,
                                     &tsn,
                                     (flags & MSG_PEEK) ? SCTP_MSG_PEEK : SCTP_MSG_DEFAULT);
         const int pathIndex = sctp_getPrimary(assocID);
#else
#error Wrong sctplib version!
#endif
         if(ok == 0) {
            bufferSize = receivedBytes;

#ifdef PRINT_DATA
            std::cout << "Received " << bufferSize << " bytes user data from association " << assocID << ", stream " << streamID << ":" << std::endl;
            for(size_t i = 0;i < bufferSize;i++) {
               char str[32];
               snprintf((char*)&str,sizeof(str),"%02x ",((unsigned char*)buffer)[i]);
               std::cout << str;
            }
            std::cout << std::endl;
#endif
            result = (int)bufferSize;

            SCTP_PathStatus pathStatus;
            SCTP_AssociationStatus assocStatus;
            if(address) {
               if(sctp_getPathStatus(assocID, pathIndex, &pathStatus) != 0) {
                  std::cerr << "INTERNAL ERROR: SCTPSocket::internalReceiver() - sctp_getPathStatus() failed!" << std::endl;
               }
               else {
                  if(sctp_getAssocStatus(assocID, &assocStatus) != 0) {
                     std::cerr << "INTERNAL ERROR: SCTPSocket::internalReceiver() - sctp_getAssocStatus() failed!" << std::endl;
                  }
                  else {
                     *address = SocketAddress::createSocketAddress(
                                   0, (char*)&pathStatus.destinationAddress, assocStatus.destPort);
                     if(*address == NULL) {
                        std::cerr << "INTERNAL ERROR: SCTPSocket::internalReceiver() - Unable to create destination address object!" << std::endl;
                     }
#ifdef PRINT_DATA
                     else {
                        std::cout << "Received via address " << *(*address) << " (path index " << pathIndex << ")." << std::endl;
                     }
#endif
                  }
               }
            }

            // ====== Peek mode: Restore chunk arrival information ==========
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
            // std::cerr << "WARNING: SCTPSocket::internalReceive() - sctp_receive() failed!" << std::endl;
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
         case SCTP_PEER_ADDR_CHANGE:
            assocID = notification.Content.sn_paddr_change.spc_assoc_id;
          break;
#ifndef DISABLE_WARNINGS
         default:
            std::cerr << "INTERNAL ERROR: Unexpected notification type #" << notification.Content.sn_header.sn_type << std::endl;
            abort();
          break;
#endif
      }

      // ====== Copy notification ===========================================
      if((receiveNotifications) &&
         (((notification.Content.sn_header.sn_type == SCTP_ASSOC_CHANGE)     && (notificationFlags & SCTP_RECVASSOCEVNT)) ||
          ((notification.Content.sn_header.sn_type == SCTP_PEER_ADDR_CHANGE) && (notificationFlags & SCTP_RECVPADDREVNT)) ||
          ((notification.Content.sn_header.sn_type == SCTP_REMOTE_ERROR)     && (notificationFlags & SCTP_RECVPEERERR))   ||
          ((notification.Content.sn_header.sn_type == SCTP_SEND_FAILED)      && (notificationFlags & SCTP_RECVSENDFAILEVNT)) ||
          ((notification.Content.sn_header.sn_type == SCTP_SHUTDOWN_EVENT)   && (notificationFlags & SCTP_RECVSHUTDOWNEVNT)))) {
         const cardinal toCopy = std::min((cardinal)notification.Content.sn_header.sn_length - notification.ContentPosition,(cardinal)bufferSize);
         const char* from = (char*)&notification.Content;
         memcpy(buffer,&from[notification.ContentPosition],toCopy);
         bufferSize = toCopy;
         notification.ContentPosition += toCopy;
         if(notification.ContentPosition < notification.Content.sn_header.sn_length) {
            if(flags & MSG_PEEK) {
               notification.ContentPosition = 0;
            }
            queue.updateNotification(notification);
            updatedNotification = true;
            flags |= MSG_NOTIFICATION;
         }
         else {
            if(flags & MSG_PEEK) {
               notification.ContentPosition = 0;
               queue.updateNotification(notification);
               updatedNotification = true;
            }
            flags |= (MSG_EOR|MSG_NOTIFICATION);
         }

#ifdef PRINT_DATA
         std::cout << "Received " << bufferSize << " bytes notification data from association " << assocID << ", stream " << streamID << ":" << std::endl;
         for(size_t i = 0;i < bufferSize;i++) {
            char str[32];
            snprintf((char*)&str,sizeof(str),"%02x ",((unsigned char*)buffer)[i]);
            std::cout << str;
         }
         std::cout << std::endl;
#endif
         result = (int)bufferSize;
      }
      else {
#ifdef PRINT_NOTIFICATION_SKIP
            std::cout << "WARNING: Skipping " << notification.Content.sn_header.sn_length << " bytes notification data (type "
                      << notification.Content.sn_header.sn_type << ") from association " << assocID << ", stream " << streamID << ":" << std::endl;
#endif
#ifdef PRINT_DATA
            for(size_t i = 0;i < notification.Content.sn_header.sn_length;i++) {
               char str[32];
               snprintf((char*)&str,sizeof(str),"%02x ",((unsigned char*)&notification.Content)[i]);
               std::cout << str;
            }
            std::cout << std::endl;
#endif
         result = getErrorCode(assocID);
         if(result == 0) {
            result = -EAGAIN;
            flags &= ~MSG_NOTIFICATION;
         }
      }
   }


   // ====== Drop notification, if not updated ==============================
   if(!updatedNotification) {
      queue.dropNotification();
      SCTPAssociation* association = getAssociationForAssociationID(assocID, false);
      if(association != NULL) {
         association->LastUsage = getMicroTime();
         if(association->UseCount > 0) {
#ifdef PRINT_ASSOC_USECOUNT
            std::cout << "Receive: UseCount decrement for A" << association->getID() << ": "
                      << association->UseCount << " -> ";
#endif
            association->UseCount--;
#ifdef PRINT_ASSOC_USECOUNT
            std::cout << association->UseCount << std::endl;
#endif
         }
#ifndef DISABLE_WARNINGS
         else {
            std::cerr << "INTERNAL ERROR: SCTPSocket::internalReceive() - Too many association usecount decrements for association ID " << assocID << "!" << std::endl;
            abort();
         }
#endif
         association->ReadReady = (association->hasData() || (getErrorCode(association->AssociationID) < 0));
#ifdef PRINT_RECVSTATUS
         std::cout << "Association " << association->AssociationID << ": ReadReady=" << association->ReadReady
                   << " ErrorCode=" << getErrorCode(association->AssociationID) << std::endl;
#endif
      }
      ReadReady = hasData() || (ConnectionRequests != NULL);
#ifdef PRINT_RECVSTATUS
      std::cout << "Instance " << InstanceName << ": ReadReady=" << ReadReady << std::endl;
#endif
   }

#ifdef TEST_PARTIAL_DELIVERY
#ifdef PRINT_PARTIAL_DELIVERY
   std::cout << "got " << result << " bytes " << ((flags & MSG_EOR) ? "---EOR---" : "") << std::endl;
#endif
#endif

   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Internal send implementation ######################################
int SCTPSocket::internalSend(const char*          buffer,
                             const size_t         length,
                             const int            flags,
                             const unsigned int   assocID,
                             const unsigned short streamID,
                             const unsigned int   protoID,
                             const unsigned int   timeToLive,
                             Condition*           waitCondition,
                             const SocketAddress* pathDestinationAddress)
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

      int pathIndex = sctp_getPrimary(assocID);
      if((pathDestinationAddress) && (flags & MSG_ADDR_OVER)) {
         SCTP_PathStatus pathStatus;
         pathIndex = getPathIndexForAddress(assocID, pathDestinationAddress, pathStatus);
      }

#ifdef PRINT_DATA
      std::cout << "Sending " << length << " bytes of data to association "
                << assocID << ", stream " << streamID << ", PPID "
                << protoID << ", path index " << pathIndex << ":" << std::endl;
      for(size_t i = 0;i < length;i++) {
         char str[32];
         snprintf((char*)&str,sizeof(str),"%02x ",((unsigned char*)buffer)[i]);
         std::cout << str;
      }
      std::cout << std::endl;
#endif

#ifdef PRINT_PRSCTP
      if(timeToLive != SCTP_INFINITE_LIFETIME) {
         std::cout << "Sending " << length << " bytes with lifetime " << timeToLive << "." << std::endl;
      }
#endif

#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
      result = sctp_send_private(
                  assocID, streamID,
                  (unsigned char*)buffer, length,
                  protoID,
                  pathIndex,
                  SCTP_NO_CONTEXT,
                  timeToLive,
                  ((flags & MSG_UNORDERED) ? SCTP_UNORDERED_DELIVERY : SCTP_ORDERED_DELIVERY),
                  ((flags & MSG_UNBUNDLED) ? SCTP_BUNDLING_DISABLED : SCTP_BUNDLING_ENABLED));
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
      result = sctp_send_private(
                  assocID, streamID,
                  (unsigned char*)buffer, length,
                  protoID,
                  pathIndex,
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

#ifdef PRINT_RECVSTATUS
   std::cout << "Association " << assocID << ": WriteReady=" << WriteReady << " sctp_send()=" << result << std::endl;
#endif
   if(result == 0) {
      return((int)length);
   }
   if(result == SCTP_PARAMETER_PROBLEM) {
      return(-EINVAL);
   }
   if(result == SCTP_QUEUE_EXCEEDED) {
      return(-ENOBUFS);
   }
   return(-EIO);
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
   return(receiveFrom(buffer,bufferSize,
                      flags,
                      assocID, streamID, protoID,
                      ssn, tsn,
                      NULL));
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
int SCTPSocket::receiveFrom(char*           buffer,
                            size_t&         bufferSize,
                            int&            flags,
                            unsigned int&   assocID,
                            unsigned short& streamID,
                            unsigned int&   protoID,
                            uint16_t&       ssn,
                            uint32_t&       tsn,
                            SocketAddress** address)
{
   // ====== Receive ========================================================
   if(!(Flags & SSF_GlobalQueue)) {
      // std::cerr << "WARNING: SCTPSocket::receiveFrom() - No global queue!" << std::endl;
      return(-EBADF);
   }
   assocID = 0;
   const int result = internalReceive(
                         GlobalQueue,
                         buffer, bufferSize,
                         flags,
                         assocID, streamID, protoID,
                         ssn, tsn,
                         address,
                         NotificationFlags);

   // ====== Check, if association has to be closed =========================
   checkAutoConnect();

   return(result);
}


// ###### Find association for given destination address ####################
SCTPAssociation* SCTPSocket::findAssociationForDestinationAddress(
                    std::multimap<unsigned int, SCTPAssociation*>& list,
                    const SocketAddress** destinationAddressList)
{
   SCTP_PathStatus pathStatus;
   short           pathIndex;

   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator = list.begin();
   while(iterator != list.end()) {
      SCTP_Association_Status assocStatus;
      if(iterator->second->PreEstablishmentAddressList == NULL) {
         if(sctp_getAssocStatus(iterator->second->AssociationID, &assocStatus) == 0) {
            size_t i = 0;
            while(destinationAddressList[i] != NULL) {
#ifdef PRINT_ASSOCSEARCH
               std::cout << "Check "
                         << destinationAddressList[i]->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy)
                         << " in AssocID=" << iterator->second->AssociationID << "?" << std::endl;
#endif
               if( (!iterator->second->IsShuttingDown) &&
                  (destinationAddressList[i]->getPort() == assocStatus.destPort) &&
                  ((pathIndex = getPathIndexForAddress(iterator->second->AssociationID, destinationAddressList[i], pathStatus)) >= 0) ) {
#ifdef PRINT_ASSOCSEARCH
                  std::cout << "Found: index=" << pathIndex << std::endl;
#endif
                  return(iterator->second);
               }
               i++;
            }
         }
      }
      else {
         size_t i = 0;
         size_t j = 0;
         while(destinationAddressList[i] != NULL) {
            while(iterator->second->PreEstablishmentAddressList[j] != NULL) {
#ifdef PRINT_ASSOCSEARCH
               std::cout << "PreEstablishmentAddressList Check "
                         << destinationAddressList[i]->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy)
                         << " == "
                         << iterator->second->PreEstablishmentAddressList[j]->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy) << std::endl;
#endif
               if(destinationAddressList[i]->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy) ==
                  iterator->second->PreEstablishmentAddressList[j]->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy)) {
#ifdef PRINT_ASSOCSEARCH
                  std::cout << "Found" << std::endl;
#endif
                  return(iterator->second);
               }
               j++;
            }
            i++;
         }
      }
      iterator++;
   }
   return(NULL);
}


// ###### Send ##############################################################
int SCTPSocket::sendTo(const char*           buffer,
                       const size_t          length,
                       const int             flags,
                       unsigned int&         assocID,
                       const unsigned short  streamID,
                       const unsigned int    protoID,
                       const unsigned int    timeToLive,
                       const unsigned short  maxAttempts,
                       const unsigned short  maxInitTimeout,
                       const bool            useDefaults,
                       const SocketAddress** destinationAddressList,
                       const cardinal        noOfOutgoingStreams)
{
   int result;

   SCTPSocketMaster::MasterInstance.lock();
#ifdef PRINT_SENDTO
   std::cout << "SendTo: length=" << length << ", PPID=" << protoID << ", flags=" << flags << std::endl;
#endif

   // ====== Send to one association ========================================
   if(!(flags & MSG_SEND_TO_ALL)) {
      // ====== Check for already created association =======================
      SCTPAssociation* association = NULL;
      if(destinationAddressList != NULL) {
         if(Flags & SSF_AutoConnect) {
#ifdef PRINT_ASSOCSEARCH
            std::cout << "Assoc lookup in ConnectionlessAssociationList..." << std::endl;
#endif
            association = findAssociationForDestinationAddress(ConnectionlessAssociationList,
                                                               destinationAddressList);
         }
         if(association == NULL) {
#ifdef PRINT_ASSOCSEARCH
            std::cout << "Assoc lookup in AssociationList..." << std::endl;
#endif
            association = findAssociationForDestinationAddress(AssociationList,
                                                               destinationAddressList);
         }
      }
      else {
#ifdef PRINT_ASSOCSEARCH
         std::cout << "AssocIDLookup " << assocID << "... ";
#endif
         std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
            AssociationList.find(assocID);
         if(iterator != AssociationList.end()) {
#ifdef PRINT_ASSOCSEARCH
            std::cout << "ok." << std::endl;
#endif
            association = iterator->second;
         }
         else {
#ifdef PRINT_ASSOCSEARCH
            std::cout << "failed!" << std::endl;
#endif
         }
      }
      if(association != NULL) {
         /* Is sendTo called by ext_connect() (buffer = NULL, length = 0 and
            no EOF or ABORT flags): association is not zero => there
            already is an association. */
         if( ((buffer == NULL) || (length == 0)) &&
             (!((flags & MSG_ABORT) || (flags & MSG_EOF))) ) {
#ifdef PRINT_ASSOCSEARCH
            std::cout << "Already connected (association " << association->AssociationID << ") -> returning!" << std::endl;
#endif
            SCTPSocketMaster::MasterInstance.unlock();
            return(-EISCONN);
         }

#ifdef PRINT_ASSOC_USECOUNT
         std::cout << "Send: UseCount increment for A" << association->getID() << ": "
                   << association->UseCount << " -> ";
#endif
         association->UseCount++;
#ifdef PRINT_ASSOC_USECOUNT
         std::cout << association->UseCount << std::endl;
#endif

         if(flags & MSG_ABORT) {
#ifdef PRINT_SHUTDOWNS
            std::cout << "Sending ABORT for association " << association->AssociationID << std::endl;
#endif
            association->abort();
            SCTPSocketMaster::MasterInstance.unlock();
            return(0);
         }

#ifdef PRINT_SENDTO
   std::cout << "SendTo: length=" << length << ", PPID=" << protoID << ", flags=" << flags
             << ": association=" << association->getID() << std::endl;
#endif
      }

      SCTPSocketMaster::MasterInstance.unlock();

      // ====== Create new association ======================================
      if((Flags & SSF_AutoConnect) && (association == NULL) && (destinationAddressList != NULL)) {
#ifdef PRINT_NEW_ASSOCIATIONS
         std::cout << "AutoConnect: New outgoing association to "
                   << destinationAddressList[0]->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy)
                   << "..." << std::endl;
#endif
         association = associate(noOfOutgoingStreams,
                                 maxAttempts, maxInitTimeout,
                                 destinationAddressList,
                                 (flags & MSG_DONTWAIT) ? false : true);
         if(association != NULL) {
#ifdef PRINT_NEW_ASSOCIATIONS
            std::cout << "AutoConnect: New outgoing association to "
                      << destinationAddressList[0]->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy)
                      << ", #" << association->getID() << " established!" << std::endl;
#endif
            SCTPSocketMaster::MasterInstance.lock();
#ifdef PRINT_ASSOC_USECOUNT
            std::cout << "AutoConnect: UseCount increment for A" << association->getID() << ": "
                      << association->UseCount << " -> ";
#endif
            association->UseCount++;
#ifdef PRINT_ASSOC_USECOUNT
            std::cout << association->UseCount << std::endl;
#endif
            ConnectionlessAssociationList.insert(std::pair<unsigned int, SCTPAssociation*>(association->getID(),association));
            SCTPSocketMaster::MasterInstance.unlock();
         }
#ifdef PRINT_NEW_ASSOCIATIONS
         else {
            std::cout << "AutoConnect: New outgoing association to "
                      << destinationAddressList[0]->getAddressString(InternetAddress::PF_Address|InternetAddress::PF_Legacy)
                      << " failed!" << std::endl;
         }
#endif
      }


      // ====== Send data ===================================================
      if(association != NULL) {
         assocID = association->getID();
         if((buffer != NULL) && (length > 0)) {
            result = association->sendTo(buffer, length, flags,
                                         streamID, protoID, timeToLive, useDefaults,
                                         destinationAddressList ? destinationAddressList[0] : NULL);
         }
         else {
            result = 0;
         }

         // ====== Remove association, if SHUTDOWN flag is set ==============
         if((flags & MSG_EOF) || (flags & MSG_ABORT)) {
#ifdef PRINT_SENDTO
            std::cout << "SendTo: length=" << length << ", PPID=" << protoID << ", flags=" << flags
                      << ", association=" << association->getID() << ": handling MSG_EOF or MSG_ABORT" << std::endl;
#endif
            if(flags & MSG_ABORT) {
#ifdef PRINT_SHUTDOWNS
               std::cout << "Sending ABORT..." << std::endl;
#endif
               association->abort();
            }
            if(flags & MSG_EOF) {
#ifdef PRINT_SHUTDOWNS
               std::cout << "Sending SHUTDOWN..." << std::endl;
#endif
               association->shutdown();
            }
            if(Flags & SSF_AutoConnect) {
#ifdef PRINT_SHUTDOWNS
               std::cout << "AutoConnect: Shutdown of outgoing association ";
               if(destinationAddressList != NULL) {
                  std::cout << "to " << *(destinationAddressList[0]) << "..." << std::endl;
               }
               else {
                  std::cout << "A" << assocID << "..." << std::endl;
               }
#endif
               SCTPSocketMaster::MasterInstance.lock();
               std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
                  ConnectionlessAssociationList.find(association->getID());
               if(iterator != ConnectionlessAssociationList.end()) {
                  ConnectionlessAssociationList.erase(iterator);
               }
               SCTPSocketMaster::MasterInstance.unlock();
               delete association;
               association = NULL;
#ifdef PRINT_SHUTDOWNS
               std::cout << "AutoConnect: Shutdown of outgoing association ";
               if(destinationAddressList != NULL) {
                  std::cout << "to " << *(destinationAddressList[0]) << std::endl;
               }
               else {
                  std::cout << "A" << assocID << std::endl;
               }
               std::cout << " completed!" << std::endl;
#endif
            }
#ifdef PRINT_SENDTO
            std::cout << "SendTo: handling AutoConnect" << std::endl;
#endif
            checkAutoConnect();
         }
      }
      else {
         result = -EIO;
      }


      SCTPSocketMaster::MasterInstance.lock();
      if(association != NULL) {
#ifdef PRINT_SENDTO
         std::cout << "SendTo: length=" << length << ", PPID=" << protoID << ", flags=" << flags
                   << ", association=" << association->getID() << ": handling UseCount decrement;  ptr=" << (void*)association << std::endl;
#endif
         association->LastUsage = getMicroTime();
         if(association->UseCount > 0) {
#ifdef PRINT_ASSOC_USECOUNT
            std::cout << "Send: UseCount decrement for A" << association->getID() << ": "
                      << association->UseCount << " -> ";
#endif
            association->UseCount--;
#ifdef PRINT_ASSOC_USECOUNT
            std::cout << association->UseCount << std::endl;
#endif
         }
#ifndef DISABLE_WARNINGS
         else {
            std::cerr << "INTERNAL ERROR: SCTPSocket::sendTo() - Too many association usecount decrements for association ID " << assocID << "!" << std::endl;
            abort();
         }
#endif
      }
      SCTPSocketMaster::MasterInstance.unlock();


      return(result);
   }

   // ====== Send to all ====================================================
   else {
      std::multimap<unsigned int, SCTPAssociation*>::iterator iterator = ConnectionlessAssociationList.begin();
      while(iterator != ConnectionlessAssociationList.end()) {
#ifdef PRINT_SEND_TO_ALL
         std::cout << "SendToAll: AssocID=" << iterator->second->AssociationID << std::endl;
#endif
         result = iterator->second->sendTo(buffer, length, flags,
                                           streamID, protoID, timeToLive, useDefaults,
                                           NULL);
         iterator++;
      }
      result = length;
   }

   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Peel UDP-like association off #####################################
SCTPAssociation* SCTPSocket::peelOff(const SocketAddress& destinationAddress)
{
   SCTPAssociation* association = NULL;
   SCTPSocketMaster::MasterInstance.lock();

   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.begin();
   while(iterator != ConnectionlessAssociationList.end()) {
      SCTP_Association_Status status;
      if(sctp_getAssocStatus(iterator->second->AssociationID,&status) == 0) {
#ifdef PRINT_ASSOCSEARCH
         std::cout << "CL "
                   << destinationAddress.getAddressString(InternetAddress::PF_HidePort|InternetAddress::PF_Address|InternetAddress::PF_Legacy)
                   << " == "
                   << String((const char*)&status.primaryDestinationAddress)
                   << "?" << std::endl;
#endif
         if( (!iterator->second->IsShuttingDown)               &&
             (destinationAddress.getPort() == status.destPort) &&
             (destinationAddress.getAddressString(InternetAddress::PF_HidePort|InternetAddress::PF_Address|InternetAddress::PF_Legacy) == String((const char*)&status.primaryDestinationAddress)) ) {
            association = iterator->second;
            association->PeeledOff = true;
            ConnectionlessAssociationList.erase(iterator);
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
   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      ConnectionlessAssociationList.find(assocID);
   if( (iterator != ConnectionlessAssociationList.end()) &&
       (!iterator->second->IsShuttingDown) ) {
      association = iterator->second;
      association->PeeledOff = true;
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
int SCTPSocket::getPathIndexForAddress(const unsigned int   assocID,
                                       const SocketAddress* address,
                                       SCTP_PathStatus&     pathParameters)
{
   if(address == NULL) {
#ifdef PRINT_PATHFORINDEX
      std::cout << "pathForIndex - primary" << std::endl;
#endif
      return(sctp_getPrimary(assocID));
   }

#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
   SCTP_Association_Status status;
   if(sctp_getAssocStatus(assocID,&status) != 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "INTERNAL ERROR: SCTPSocket::getPathIndexForAddress() - Unable to get association status!" << std::endl;
#endif
      return(-1);
   }
#endif

   const String addressString = address->getAddressString(SocketAddress::PF_Address|SocketAddress::PF_HidePort|SocketAddress::PF_Legacy);

   for(unsigned int i = 0;;i++) {
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
      const int index = i;
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
      const int index = status.destinationPathIDs[i];
#else
#error Wrong sctplib version!
#endif

      const int ok = sctp_getPathStatus(assocID, index, &pathParameters);
      if(ok != 0) {
         break;
      }
#ifdef PRINT_PATHFORINDEX
      std::cout << "pathForIndex: " << index << ": " << addressString << " == " << pathParameters.destinationAddress << "?" << std::endl;
#endif
      if(addressString == String((char*)&pathParameters.destinationAddress)) {
#ifdef PRINT_PATHFORINDEX
         std::cout << "   => " << index << std::endl;
#endif
         return(index);
      }
   }
#ifdef PRINT_PATHFORINDEX
      std::cout << "pathForIndex - failed" << std::endl;
#endif
   return(-1);
}


// ###### Get path parameters ###############################################
bool SCTPSocket::getPathParameters(const unsigned int   assocID,
                                   const SocketAddress* address,
                                   SCTP_PathStatus&     pathParameters)
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
bool SCTPSocket::setPathParameters(const unsigned int     assocID,
                                   const SocketAddress*   address,
                                   const SCTP_PathStatus& pathParameters)
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
   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
   std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
      std::cout << "setPrimary: Setting primary address to " << primary << std::endl;
#endif
      result = sctp_setPrimary(assocID,index);
#ifdef PRINT_SETPRIMARY
      if(result != 0) {
         std::cerr << "WARNING: sctp_setPrimary() failed, error #" << result << std::endl;
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
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
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
      std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_0_0) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
   std::cerr << "NOT IMPLEMENTED: sctp_addIPAddress()" << std::endl;
   const int result = -1;
#else
   const int result = sctp_addIPAddress(assocID,address,&CorrelationID);
#endif
#ifdef PRINT_ADDIP
   std::cout << "AddIP: " << addAddress << " -> result=" << result << std::endl;
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
      std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
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
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_0_0) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
   std::cerr << "NOT IMPLEMENTED: sctp_deleteIPAddress()" << std::endl;
   const int result = -1;
#else
   const int result = sctp_deleteIPAddress(assocID,address,&CorrelationID);
#endif
#ifdef PRINT_ADDIP
   std::cout << "DeleteIP: " << delAddress << " -> result=" << result << std::endl;
#endif
   CorrelationID++;
   SCTPSocketMaster::MasterInstance.unlock();
   return(result == 0);
}


// ###### Check for necessity to auto-close associations ####################
void SCTPSocket::checkAutoClose()
{
   if(AutoCloseRecursion) {
      AutoCloseNewCheckRequired = true;
      return;
   }
   AutoCloseRecursion = true;

   do {
      AutoCloseNewCheckRequired = false;

      const card64 now = getMicroTime();
      std::multimap<unsigned int, SCTPAssociation*>::iterator iterator =
         ConnectionlessAssociationList.begin();
      while(iterator != ConnectionlessAssociationList.end()) {
         SCTPAssociation* association = iterator->second;
#ifdef PRINT_AUTOCLOSE_CHECK
         std::cout << "AutoConnect: Check for AutoClose:" << std::endl
                   << "   AssocID          = " << association->getID() << std::endl
                   << "   UseCount         = " << association->UseCount << std::endl
                   << "   LastUsage        = " << now - association->LastUsage << std::endl
                   << "   AutoCloseTimeout = " << AutoCloseTimeout << std::endl;
#endif

         /* ====== Association has no active users ======================= */
         if(association->UseCount == 0) {
            /* ====== Association is closed -> remove it ================= */
            if((association->ShutdownCompleteNotification) ||
               (association->CommunicationLostNotification)) {
#ifdef PRINT_AUTOCLOSE_TIMEOUT
               const unsigned int assocID = association->getID();
               std::cout << "AutoConnect: Removing association #" << assocID << ": ";
               if(association->ShutdownCompleteNotification) {
                  std::cout << "shutdown complete";
               }
               else if(association->CommunicationLostNotification) {
                  std::cout << "communication lost";
               }
               std::cout << "..." << std::endl;
#endif

               // Important! Removal will invalidate iterator!
               std::multimap<unsigned int, SCTPAssociation*>::iterator delIterator = iterator;
               iterator++;
               ConnectionlessAssociationList.erase(delIterator);

               delete association;
#ifdef PRINT_AUTOCLOSE_TIMEOUT
               std::cout << "AutoConnect: AutoClose of association #" << assocID << " completed!" << std::endl;
#endif
            }

            /* ====== Association still active, time to send ABORT! ====== */
            else if((AutoCloseTimeout > 0) &&
                    (now - association->LastUsage > 4 * AutoCloseTimeout)) {
#ifdef PRINT_AUTOCLOSE_TIMEOUT
               const unsigned int assocID = association->getID();
               std::cout << "AutoConnect: Abort of association #" << assocID << " due to timeout" << std::endl;
#endif
               iterator++;   // Important! shutdown() may invalidate iterator!
               association->abort();
            }

            /* ====== Association still active, but timeout has expired == */
            else if((AutoCloseTimeout > 0) &&
                    (now - association->LastUsage > AutoCloseTimeout) &&
                    (!association->IsShuttingDown)) {
#ifdef PRINT_AUTOCLOSE_TIMEOUT
               const unsigned int assocID = association->getID();
               std::cout << "AutoConnect: Doing shutdown of association #" << assocID << " due to timeout" << std::endl;
#endif
               iterator++;   // Important! shutdown() may invalidate iterator!
               association->shutdown();
            }

            /* ====== Association is waiting for shutdown/abort ========== */
            else {
               iterator++;
            }
         }

         /* ====== Association is still in use =========================== */
         else {
            // Skip this association
            iterator++;

            // The association is closed, but somebody is still using it.
            // We will remove it later ...
            if((association->ShutdownCompleteNotification) ||
               (association->CommunicationLostNotification)) {
#ifdef PRINT_AUTOCLOSE_TIMEOUT
               std::cout << "AutoConnect: Association #" << association->getID() << " is disconnected but still has users!" << std::endl;
#endif
            }
         }
      }
   } while(AutoCloseNewCheckRequired == true);
   AutoCloseRecursion = false;
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
         std::cout << "AutoConnect: New incoming association #" << association->getID() << "..." << std::endl;
#endif
         ConnectionlessAssociationList.insert(std::pair<unsigned int, SCTPAssociation*>(association->getID(),association));
         association = accept(NULL,false);
      }
      Flags = oldFlags;

      SCTPSocketMaster::MasterInstance.unlock();
   }
}
