/*
 *  $Id: sctpassociation.cc,v 1.5 2003/07/11 09:45:02 dreibh Exp $
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
 * Purpose: SCTP Association
 *
 */


#include "tdsystem.h"
#include "sctpassociation.h"
#include "sctpsocketmaster.h"



// #define PRINT_SHUTDOWN
// #define PRINT_SOCKTYPE
// #define PRINT_RTOMAX


// ###### Constructor #######################################################
SCTPAssociation::SCTPAssociation(SCTPSocket*        socket,
                                 const unsigned int associationID,
                                 const unsigned int notificationFlags,
                                 const bool         udpLike)
{
   Socket                        = socket;
   AssociationID                 = associationID;
   StreamDefaultTimeoutArray     = NULL;
   StreamDefaultTimeoutCount     = 0;
   CommunicationUpNotification   = false;
   CommunicationLostNotification = false;
   ShutdownCompleteNotification  = false;
   IsShuttingDown                = false;
   UseCount                      = 0;
   LastUsage                     = getMicroTime();
   NotificationFlags             = notificationFlags;
   Defaults.ProtoID              = 0x00000000;
   Defaults.StreamID             = 0x0000;
   Defaults.TimeToLive           = SCTP_INFINITE_LIFETIME;
   ReadReady                     = false;
   WriteReady                    = false;
   HasException                  = false;
   RTOMaxIsInitTimeout           = false;

   EstablishCondition.setName("SCTPAssociation::EstablishCondition");
   ShutdownCompleteCondition.setName("SCTPAssociation::ShutdownCompleteCondition");
   ReadyForTransmit.setName("SCTPAssociation::ReadyForTransmitCondition");
   InQueue.getUpdateCondition()->setName("SCTPAssociation::InQueue");
   ReadUpdateCondition.setName("SCTPAssociation::ReadUpdateCondition");
   WriteUpdateCondition.setName("SCTPAssociation::WriteUpdateCondition");
   ExceptUpdateCondition.setName("SCTPAssociation::ExceptUpdateCondition");

   InQueue.getUpdateCondition()->addParent(&ReadUpdateCondition);

   if(!udpLike) {
#ifdef PRINT_SOCKTYPE
      cout << "SCTPAssociation::SCTPAssociation() - Initializing TCP-like socket" << endl;
#endif
      ShutdownCompleteCondition.addParent(&ExceptUpdateCondition);
      EstablishCondition.addParent(&WriteUpdateCondition);
      ReadyForTransmit.addParent(&WriteUpdateCondition);
   }
#ifdef PRINT_SOCKTYPE
   else {
      cout << "SCTPAssociation::SCTPAssociation() - Initializing UDP-like socket" << endl;
   }
#endif

   SCTPSocketMaster::MasterInstance.lock();
   Socket->AssociationList.insert(pair<unsigned int, SCTPAssociation*>(AssociationID,this));
   SCTPSocketMaster::MasterInstance.unlock();
}


// ###### Destructor ########################################################
SCTPAssociation::~SCTPAssociation()
{
   SCTPSocketMaster::MasterInstance.lock();
   if(AssociationID == 0) {
#ifndef DISABLE_WARNINGS
      cerr << "ERROR: SCTPAssociation::~SCTPAssociation() - "
              "AssociationID is 0! Destructor called twice?!" << endl;
#endif
      return;
   }

   // ====== Do shutdown ====================================================
   if(!ShutdownCompleteNotification) {
#ifdef PRINT_SHUTDOWN
      cout << "Active shutdown of association #" << AssociationID << " started..." << endl;
#endif
      SCTPSocketMaster::delayedDeleteAssociation(Socket->getID(),AssociationID);
      shutdown();
#ifdef PRINT_SHUTDOWN
      cout << "Active shutdown of association #" << AssociationID << " complete!" << endl;
#endif
   }
   else {
#ifdef PRINT_SHUTDOWN
      cout << "Passive shutdown of association #" << AssociationID << "!" << endl;
#endif
      sctp_deleteAssociation(AssociationID);
   }

   // ====== Remove association from list ================================
   multimap<unsigned int, SCTPAssociation*>::iterator iterator =
      Socket->AssociationList.find(AssociationID);
   if(iterator != Socket->AssociationList.end()) {
      Socket->AssociationList.erase(iterator);
   }
   else {
#ifndef DISABLE_WARNINGS
      cerr << "INTERNAL ERROR: SCTPAssociation::~SCTPAssociation() - "
              "Erase of association #" << AssociationID << " failed!" << endl;
      ::exit(1);
#endif
   }

   // ====== Clear ID, to make finding dangling references easier ===========
   AssociationID = 0;

   SCTPSocketMaster::MasterInstance.unlock();

   if(StreamDefaultTimeoutArray) {
      delete StreamDefaultTimeoutArray;
      StreamDefaultTimeoutArray = NULL;
      StreamDefaultTimeoutCount = 0;
   }
}


// ###### Get local address #################################################
bool SCTPAssociation::getLocalAddresses(SocketAddress**& addressArray)
{
   return(Socket->getLocalAddresses(addressArray));
}


// ###### Get remote address ################################################
bool SCTPAssociation::getRemoteAddresses(SocketAddress**& addressArray)
{
   if(addressArray != NULL) {
      addressArray = NULL;
   }
   bool                    result = true;
   SCTP_Association_Status status;
   addressArray = NULL;

   SCTPSocketMaster::MasterInstance.lock();

   if(sctp_getAssocStatus(AssociationID,&status) == 0) {
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
      const unsigned int addresses = status.numberOfAddresses;
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
      const unsigned int addresses = status.numberOfDestinationPaths;
#else
#error Wrong sctplib version!
#endif
      addressArray = SocketAddress::newAddressList(addresses);
      if(addressArray == NULL) {
         return(false);
      }
      unsigned int i;
      for(i = 0;i < addresses;i++) {
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
         const int index = i;
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
         const int index = status.destinationPathIDs[i];
#else
#error Wrong sctplib version!
#endif

         SCTP_Path_Status pathStatus;
         if(sctp_getPathStatus(AssociationID,index,&pathStatus) != 0) {
#ifndef DISABLE_WARNINGS
            cerr << "INTERNAL ERROR: SCTPAssociation::getRemoteAddress() - sctp_getPathStatus() failure!"
                 << endl;
            ::exit(1);
#endif
         }
         else {
            addressArray[i] = SocketAddress::createSocketAddress(
                                 0,(char*)&pathStatus.destinationAddress,status.destPort);
            if(addressArray[i] == NULL) {
#ifndef DISABLE_WARNINGS
               cerr << "WARNING: SCTPAssociation::getRemoteAddresses() - Bad address "
                    << pathStatus.destinationAddress << ", port " << status.destPort << "!" << endl;
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


// ###### Check, if socket has data for given flags #########################
bool SCTPAssociation::hasData()
{
   SCTPSocketMaster::MasterInstance.lock();
   const bool result = InQueue.hasData(NotificationFlags);
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Receive ###########################################################
int SCTPAssociation::receive(char*           buffer,
                             size_t&         bufferSize,
                             int&            flags,
                             unsigned short& streamID,
                             unsigned int&   protoID,
                             uint16_t&       ssn,
                             uint32_t&       tsn)
{
   unsigned int assocID = AssociationID;
   streamID             = (unsigned short)-1;
   SCTPNotification notification;

   const int result = Socket->internalReceive(
                                 InQueue,
                                 buffer, bufferSize,
                                 flags,
                                 assocID, streamID, protoID,
                                 ssn, tsn,
                                 notification,
                                 NotificationFlags);
   return(result);
}


// ###### Receive ###########################################################
int SCTPAssociation::receiveFrom(char*             buffer,
                                 size_t&           bufferSize,
                                 int&              flags,
                                 unsigned short&   streamID,
                                 unsigned int&     protoID,
                                 uint16_t&         ssn,
                                 uint32_t&         tsn,
                                 SocketAddress***  addressArray,
                                 SCTPNotification& notification)
{
   // ====== Receive data ===================================================
   unsigned int assocID = AssociationID;
   streamID             = (unsigned short)-1;
   const int result = Socket->internalReceive(
                         InQueue,
                         buffer, bufferSize,
                         flags,
                         assocID, streamID, protoID,
                         ssn, tsn,
                         notification,
                         NotificationFlags);

   // ====== Extract sender's addresses =====================================
   if((bufferSize > 0) && (addressArray != NULL)) {
      unsigned int i;
      *addressArray = SocketAddress::newAddressList(notification.RemoteAddresses);
      if(*addressArray != NULL) {
         for(i = 0;i < notification.RemoteAddresses;i++) {
            (*addressArray)[i] = SocketAddress::createSocketAddress(
                                    0,
                                    (char*)&notification.RemoteAddress[i],
                                    notification.RemotePort);
         }
      }
   }

   return(result);
}


// ###### Send via specified path ############################################
int SCTPAssociation::sendTo(const char*          buffer,
                            const size_t         length,
                            const int            flags,
                            const unsigned short streamID,
                            const unsigned int   protoID,
                            const unsigned int   timeToLive,
                            const bool           useDefaults,
                            const SocketAddress* pathDestinationAddress)
{
   int result;
   if(!useDefaults) {
      result = Socket->internalSend(
                          buffer, length,
                          flags,
                          AssociationID, streamID, protoID,
                          timeToLive,
                          &ReadyForTransmit, pathDestinationAddress);
   }
   else {
      if((buffer != NULL) && (length > 0)) {
         unsigned int timeout;
         if(!getDefaultStreamTimeout(Defaults.StreamID, timeout)) {
            timeout = Defaults.TimeToLive;
         }
         result = Socket->internalSend(
                             buffer, length,
                             flags,
                             AssociationID,
                             Defaults.StreamID, Defaults.ProtoID, Defaults.TimeToLive,
                             &ReadyForTransmit,
                             pathDestinationAddress);
      }
      else {
         result = 0;
      }
   }
   return(result);
}


// ###### Shutdown ##########################################################
void SCTPAssociation::shutdown()
{
   SCTPSocketMaster::MasterInstance.lock();
   if(!IsShuttingDown) {
      IsShuttingDown = true;
      sctp_shutdown(AssociationID);
   }
   SCTPSocketMaster::MasterInstance.unlock();
}


// ###### Abort #############################################################
void SCTPAssociation::abort()
{
   SCTPSocketMaster::MasterInstance.lock();
   IsShuttingDown = true;
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
   sctp_abort(AssociationID, 0, NULL);
#else
   sctp_abort(AssociationID);
#endif
   SCTPSocketMaster::MasterInstance.unlock();
}


// ###### Get association parameters ########################################
bool SCTPAssociation::getAssocStatus(
                         SCTP_Association_Status& assocStatus)
{
   const bool result = Socket->getAssocStatus(AssociationID,assocStatus);
   if(result) {
      if(RTOMaxIsInitTimeout) {
#ifdef PRINT_RTOMAX
         char str[256];
         snprintf((char*)&str,sizeof(str),"getAssocStatus() - Replying RTOMax=%d instead of InitTimeout=%d\n",assocStatus.rtoMax,RTOMax);
         cerr << str << endl;
#endif
         assocStatus.rtoMax = RTOMax;
      }
   }
   return(result);
}


// ###### Set association parameters ########################################
bool SCTPAssociation::setAssocStatus(
                         const SCTP_Association_Status& assocStatus)
{
   SCTP_Association_Status newStatus = assocStatus;
   if(RTOMaxIsInitTimeout) {
#ifdef PRINT_RTOMAX
      char str[256];
      snprintf((char*)&str,sizeof(str),"setAssocStatus() - Saving RTOMax=%d, using InitTimeout=%d\n",assocStatus.rtoMax,InitTimeout);
      cerr << str << endl;
#endif
      newStatus.rtoMax = InitTimeout;
      RTOMax = assocStatus.rtoMax;
   }
   return(Socket->setAssocStatus(AssociationID,assocStatus));
}


// ###### Set default stream timeouts #######################################
bool SCTPAssociation::setDefaultStreamTimeouts(const unsigned int   timeout,
                                               const unsigned short start,
                                               const unsigned short end)
{
   if(start > end) {
      return(false);
   }

   SCTPSocketMaster::MasterInstance.lock();

   if((unsigned int)end + 1 <= StreamDefaultTimeoutCount) {
      for(unsigned int i = start;i <= end;i++) {
         StreamDefaultTimeoutArray[i].Valid   = true;
         StreamDefaultTimeoutArray[i].Timeout = timeout;
      }
   }
   else {
      StreamDefaultTimeout* newArray = new StreamDefaultTimeout[end + 1];
      if(newArray == 0) {
         SCTPSocketMaster::MasterInstance.unlock();
         return(false);
      }
      if(StreamDefaultTimeoutArray != NULL) {
         for(unsigned int i = 0;i <= StreamDefaultTimeoutCount;i++) {
            newArray[i] = StreamDefaultTimeoutArray[i];
         }
      }
      for(unsigned int i = StreamDefaultTimeoutCount;i < start;i++) {
         newArray[i].Valid = false;
      }
      for(unsigned int i = start;i <= end;i++) {
         newArray[i].Valid   = true;
         newArray[i].Timeout = timeout;
      }
      if(StreamDefaultTimeoutArray != NULL) {
         delete StreamDefaultTimeoutArray;
      }
      StreamDefaultTimeoutArray = newArray;
      StreamDefaultTimeoutCount = end + 1;
   }

   SCTPSocketMaster::MasterInstance.unlock();
   return(true);
}


// ###### Get default stream timeout ########################################
bool SCTPAssociation::getDefaultStreamTimeout(const unsigned short streamID,
                                              unsigned int&        timeout)
{
   bool result = false;
   SCTPSocketMaster::MasterInstance.lock();
   if(streamID < StreamDefaultTimeoutCount) {
      if(StreamDefaultTimeoutArray[streamID].Valid) {
         timeout = StreamDefaultTimeoutArray[streamID].Timeout;
         result  = true;
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Get path parameters ###############################################
bool SCTPAssociation::getPathParameters(const struct SocketAddress* address,
                                        SCTP_PathStatus&            pathParameters)
{
   return(Socket->getPathParameters(AssociationID,address,pathParameters));
}


// ###### Set path parameters ###############################################
bool SCTPAssociation::setPathParameters(const struct SocketAddress* address,
                                        const SCTP_PathStatus&      pathParameters)
{
   return(Socket->setPathParameters(AssociationID,address,pathParameters));
}


// ###### Get send buffer size ##############################################
ssize_t SCTPAssociation::getSendBuffer()
{
   ssize_t result = -1;
   SCTPSocketMaster::MasterInstance.lock();
   SCTP_Association_Status status;
   if(sctp_getAssocStatus(AssociationID,&status) == 0) {
      result = status.maxSendQueue;
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Set send buffer size ##############################################
bool SCTPAssociation::setSendBuffer(const size_t size)
{
   bool result = false;
   SCTPSocketMaster::MasterInstance.lock();
   SCTP_Association_Status status;
   if(sctp_getAssocStatus(AssociationID,&status) == 0) {
      status.maxSendQueue = size;
      if(sctp_setAssocStatus(AssociationID,&status) == 0) {
         result = true;
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Get receive buffer size ###########################################
ssize_t SCTPAssociation::getReceiveBuffer()
{
   ssize_t result = -1;
   SCTPSocketMaster::MasterInstance.lock();
   SCTP_Association_Status status;
   if(sctp_getAssocStatus(AssociationID,&status) == 0) {
      result = status.maxRecvQueue;
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Set receive buffer size ###########################################
bool SCTPAssociation::setReceiveBuffer(const size_t size)
{
   bool result = false;
   SCTPSocketMaster::MasterInstance.lock();
   SCTP_Association_Status status;
   if(sctp_getAssocStatus(AssociationID,&status) == 0) {
      status.maxRecvQueue = size;
      if(sctp_setAssocStatus(AssociationID,&status) == 0) {
         result = true;
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}

// ###### Set primary address ###############################################
SocketAddress* SCTPAssociation::getPrimaryAddress()
{
   return(Socket->getPrimaryAddress(AssociationID));
}


// ###### Set primary address ###############################################
bool SCTPAssociation::setPrimary(const SocketAddress& primary)
{
   return(Socket->setPrimary(AssociationID,primary));
}


// ###### Set peer primary address ##########################################
bool SCTPAssociation::setPeerPrimary(const SocketAddress& primary)
{
   return(Socket->setPeerPrimary(AssociationID,primary));
}


// ###### Add address #######################################################
bool SCTPAssociation::addAddress(const SocketAddress& addAddress)
{
   return(Socket->addAddress(AssociationID,addAddress));
}


// ###### Delete address ####################################################
bool SCTPAssociation::deleteAddress(const SocketAddress& delAddress)
{
   return(Socket->deleteAddress(AssociationID,delAddress));
}



// ###### Get traffic class #################################################
int SCTPAssociation::getTrafficClass(const int streamID)
{
   ssize_t result = -1;
   SCTPSocketMaster::MasterInstance.lock();
   SCTP_Association_Status status;
   if(sctp_getAssocStatus(AssociationID,&status) == 0) {
      result = status.ipTos;
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Set traffic class #################################################
bool SCTPAssociation::setTrafficClass(const card8 trafficClass,
                                      const int   streamID)
{
   bool result = false;
   SCTPSocketMaster::MasterInstance.lock();
   SCTP_Association_Status status;
   if(sctp_getAssocStatus(AssociationID,&status) == 0) {
      status.ipTos = trafficClass;
      if(sctp_setAssocStatus(AssociationID,&status) == 0) {
         result = true;
      }
   }
   SCTPSocketMaster::MasterInstance.unlock();
   return(result);
}


// ###### Get default settings ##############################################
void SCTPAssociation::getAssocIODefaults(struct AssocIODefaults& defaults)
{
   SCTPSocketMaster::MasterInstance.lock();
   defaults = Defaults;
   SCTPSocketMaster::MasterInstance.unlock();
}


// ###### Set default settings ##############################################
void SCTPAssociation::setAssocIODefaults(const struct AssocIODefaults& defaults)
{
   SCTPSocketMaster::MasterInstance.lock();
   Defaults = defaults;
   SCTPSocketMaster::MasterInstance.unlock();
}
