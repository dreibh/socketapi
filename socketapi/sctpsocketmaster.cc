/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2024 by Thomas Dreibholz
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
 *          thomas.dreibholz@gmail.com
 *          tuexen@fh-muenster.de
 *
 * Purpose: SCTP Socket Master
 *
 */



#include "tdsystem.h"
#include "tools.h"
#include "sctpsocketmaster.h"
#include "extsocketdescriptor.h"


#include <signal.h>
#include <poll.h>
#if (SYSTEM == OS_SOLARIS)
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif


// #define PRINT_NOTIFICATIONS
// #define PRINT_ARRIVENOTIFICATION
// #define PRINT_USERCALLBACK
// #define PRINT_ASSOC_USECOUNT
// #define PRINT_RTOMAXRESTORE
//
// #define PRINT_PIPE
// #define PRINT_GC

// Do not show a warning on initialization failure.
// #define NO_INITFAIL_WARNING



// ###### SCTPSocketMaster static attributes ################################
int                              SCTPSocketMaster::InitializationResult     = -1000;
int                              SCTPSocketMaster::GarbageCollectionTimerID = -1;
cardinal                         SCTPSocketMaster::LockLevel                = 0;
cardinal                         SCTPSocketMaster::OldCancelState           = true;
card64                           SCTPSocketMaster::LastGarbageCollection;
std::set<int>                    SCTPSocketMaster::ClosingSockets;
std::multimap<unsigned int, int> SCTPSocketMaster::ClosingAssociations;
std::multimap<int, SCTPSocket*>  SCTPSocketMaster::SocketList;
SCTP_ulpCallbacks                SCTPSocketMaster::Callbacks;
SCTPSocketMaster                 SCTPSocketMaster::MasterInstance;
Randomizer                       SCTPSocketMaster::Random;
int                              SCTPSocketMaster::BreakPipe[2];
SCTPSocketMaster::UserSocketNotification
                                 SCTPSocketMaster::BreakNotification;

ExtSocketDescriptor              ExtSocketDescriptorMaster::Sockets[ExtSocketDescriptorMaster::MaxSockets];
ExtSocketDescriptorMaster        ExtSocketDescriptorMaster::MasterInstance;



// ###### Library version check #############################################
static bool versionCheck()
{
   bool result = true;

   const unsigned int sctplibLinkedVersion   = sctp_getLibraryVersion();
   const unsigned int sctplibCompiledVersion = (SCTP_MAJOR_VERSION << 16) | SCTP_MINOR_VERSION;

   if(sctplibLinkedVersion != sctplibCompiledVersion) {
      std::cerr << "INTERNAL ERROR: sctp.h and linked sctplib library are different!" << std::endl;
      result = false;
   }

   if(result == false) {
      char str[128];
      snprintf((char*)&str, sizeof(str),
               "Compiled = $%04x\nLinked   = $%04x\n",
               sctplibCompiledVersion, sctplibLinkedVersion);
      std::cerr << str;
   }

   return(result);
}


// ###### Constructor #######################################################
SCTPSocketMaster::SCTPSocketMaster()
   : Thread("SCTPSocketMaster")
{
   if(InitializationResult == -1000) {
      Callbacks.dataArriveNotif           = &dataArriveNotif;
      Callbacks.sendFailureNotif          = &sendFailureNotif;
      Callbacks.networkStatusChangeNotif  = &networkStatusChangeNotif;
      Callbacks.communicationUpNotif      = &communicationUpNotif;
      Callbacks.communicationLostNotif    = &communicationLostNotif;
      Callbacks.communicationErrorNotif   = &communicationErrorNotif;
      Callbacks.restartNotif              = &restartNotif;
      Callbacks.shutdownCompleteNotif     = &shutdownCompleteNotif;
      Callbacks.peerShutdownReceivedNotif = &shutdownReceivedNotif;
      Callbacks.queueStatusChangeNotif    = &queueStatusChangeNotif;
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
      Callbacks.asconfStatusNotif         = &asconfStatusNotif;
#endif

      if(!versionCheck()) {
         return;
      }

      int sd;
      sd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
      if(sd >= 0) {
#ifndef SCTP_OVER_UDP
         close(sd);
         std::cerr << "ERROR: Kernel SCTP seems to be available! You cannout use sctplib and kernel SCTP simultaneously!" << std::endl;
         ::abort();
#else
         std::cerr << "NOTE: The socket API assumes SCTP over UDP. Kernel SCTP has been found, but this should be okay." << std::endl;
#endif
      }

      InitializationResult = sctp_initLibrary();
      if(InitializationResult == 0) {
         enableOOTBHandling(false);
         enableCRC32(true);
         LastGarbageCollection = getMicroTime();

         if(pipe((int*)&BreakPipe) == 0) {
#ifdef PRINT_PIPE
            std::cout << "Break Pipe: in=" << BreakPipe[0] << " out=" << BreakPipe[1] << std::endl;
#endif
            int flags = fcntl(BreakPipe[0],F_GETFL,0);
            if(flags != -1) {
               flags |= O_NONBLOCK;
               if(fcntl(BreakPipe[0],F_SETFL,flags) == 0) {
                  BreakNotification.FileDescriptor = BreakPipe[0];
                  BreakNotification.EventMask      = POLLIN|POLLPRI;
                  BreakNotification.UpdateCondition.setName("BreakPipe");
                  SCTPSocketMaster::MasterInstance.addUserSocketNotification(&BreakNotification);

                  // The thread may not be started here, since
                  // its static initializers may *not* be initialized!
               }
               else {
#ifndef DISABLE_WARNINGS
                  std::cerr << "WARNING: SCTPSocketMaster::SCTPSocketMaster() - Failed to set Break Pipe to non-blocking mode!" << std::endl;
#endif
                  close(BreakPipe[0]);
                  close(BreakPipe[1]);
                  BreakPipe[0] = -1;
                  BreakPipe[1] = -1;
               }
            }
            else {
#ifndef DISABLE_WARNINGS
               std::cerr << "WARNING: SCTPSocketMaster::SCTPSocketMaster() - Failed reading Break Pipe flags!" << std::endl;
#endif
               close(BreakPipe[0]);
               close(BreakPipe[1]);
               BreakPipe[0] = -1;
               BreakPipe[1] = -1;
            }
         }
         else {
            BreakPipe[0] = -1;
            BreakPipe[1] = -1;
#ifndef DISABLE_WARNINGS
            std::cerr << "WARNING: SCTPSocketMaster::SCTPSocketMaster() - Break Pipe not available!" << std::endl;
#endif
         }
      }
      else {
         BreakPipe[0] = -1;
         BreakPipe[1] = -1;
#ifndef NO_INITFAIL_WARNING
         std::cerr << "ERROR: SCTP Library initialization failed!" << std::endl;
         if(getuid() != 0) {
            std::cerr << "       You need root permissions to use the SCTP Library!" << std::endl;
         }
#endif
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: SCTPSocketMaster::SCTPSocketMaster() - "
              "Do not try to initialice SCTPSocketMaster singleton twice!" << std::endl;
#endif
   }
}


// ###### Destructor ########################################################
SCTPSocketMaster::~SCTPSocketMaster()
{
#ifdef PRINT_GC
   std::cout << "garbageCollection: SocketMaster destructor..." << std::endl;
#endif

   // ====== Stop master thread =============================================
   lock();
   cancel();
   PThread = 0;
   unlock();

   // ====== Do garbage collection for associations =========================
   lock();
   if(GarbageCollectionTimerID != -1) {
      sctp_stopTimer(GarbageCollectionTimerID);
      GarbageCollectionTimerID = -1;
   }

   std::multimap<unsigned int, int>::iterator iterator = ClosingAssociations.begin();
   while(iterator != ClosingAssociations.end()) {
      // associationGarbageCollection(iterator->first,true) may not be called
      // here, since sctp_abort() directly calls communicationLostNotification(),
      // which itself calls associationGarbageCollection(iterator->first,true)
      // again. Therefore, sctp_abort() is used here.
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
      sctp_abort(iterator->first, 0, NULL);
#else
      sctp_abort(iterator->first);
#endif
      iterator = ClosingAssociations.begin();
   }
   unlock();

   // ====== Do garbage collection for sockets ==============================
   socketGarbageCollection();

   // ====== Wait for thread to finish ======================================
   join();

   // ====== Remove break pipe ==============================================
   if(BreakPipe[0] != -1) {
      SCTPSocketMaster::MasterInstance.deleteUserSocketNotification(&BreakNotification);
      close(BreakPipe[0]);
      close(BreakPipe[1]);
      BreakPipe[0] = -1;
      BreakPipe[1] = -1;
   }

#ifdef PRINT_GC
   std::cout << "garbageCollection: SocketMaster destructor completed!" << std::endl;
#endif
}


// ###### SCTP event loop thread ############################################
void SCTPSocketMaster::run()
{
   for(;;) {
      card64 now         = getMicroTime();
      const card64 usecs =
         (LastGarbageCollection + GarbageCollectionInterval > now) ?
             (LastGarbageCollection + GarbageCollectionInterval - now) : 0;

      MasterInstance.lock();
      GarbageCollectionTimerID = sctp_startTimer((unsigned int)(usecs / 1000000),
                                                 (unsigned int)(usecs % 1000000),
                                                 timerCallback, NULL, NULL);
      MasterInstance.unlock();

      sctp_extendedEventLoop(lock,unlock,(void*)this);

      MasterInstance.lock();
      sctp_stopTimer(GarbageCollectionTimerID);
      GarbageCollectionTimerID = -1;
      MasterInstance.unlock();

      now = getMicroTime();
      if(now - LastGarbageCollection >= GarbageCollectionInterval) {
         socketGarbageCollection();
      }
   }
}


// ###### SCTP timer callback ###############################################
void SCTPSocketMaster::timerCallback(unsigned int tid,
                                     void*        param1,
                                     void*        param2)
{
}


// ###### Enable or disable OOTB handling ###################################
bool SCTPSocketMaster::enableOOTBHandling(const bool enable)
{
   bool result = true;
   MasterInstance.lock();
   SCTP_Library_Parameters parameters;
   if(sctp_getLibraryParameters(&parameters) == SCTP_SUCCESS) {
      parameters.sendOotbAborts    = (enable == false) ? 0 : 1;
      if(sctp_setLibraryParameters(&parameters) != SCTP_SUCCESS) {
#ifndef DISABLE_WARNINGS
         std::cerr << "WARNING: SCTPSocketMaster::enableOOTBHandling() - Setting of SCTP Library parameters failed!" << std::endl;
#endif
         result = false;
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: SCTPSocketMaster::enableOOTBHandling() - Getting of SCTP Library parameters failed!" << std::endl;
#endif
      result = false;
   }
   MasterInstance.unlock();
   return(result);
}


// ###### Enable or disable CRC32 checksum ##################################
bool SCTPSocketMaster::enableCRC32(const bool enable)
{
   bool result = true;
   MasterInstance.lock();
   SCTP_Library_Parameters parameters;
   if(sctp_getLibraryParameters(&parameters) == SCTP_SUCCESS) {
      parameters.checksumAlgorithm =
         (enable == true) ? SCTP_CHECKSUM_ALGORITHM_CRC32C : SCTP_CHECKSUM_ALGORITHM_ADLER32;
      if(sctp_setLibraryParameters(&parameters) != SCTP_SUCCESS) {
#ifndef DISABLE_WARNINGS
         std::cerr << "WARNING: SCTPSocketMaster::enableOOTBHandling() - Setting of SCTP Library parameters failed!" << std::endl;
#endif
         result = false;
      }
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: SCTPSocketMaster::enableOOTBHandling() - Getting of SCTP Library parameters failed!" << std::endl;
#endif
      result = false;
   }
   MasterInstance.unlock();
   return(result);
}


// ###### Add AssociationID to be deleted ###################################
void SCTPSocketMaster::delayedDeleteAssociation(const unsigned short instanceID,
                                                const unsigned int   assocID)
{
#ifdef PRINT_GC
   std::cout << "delayedDeleteAssociation: A=" << assocID << " I=" << instanceID << std::endl;
#endif
   ClosingAssociations.insert(std::pair<unsigned int, unsigned short>(assocID,instanceID));
}


// ###### Add InstanceID to be deleted ######################################
void SCTPSocketMaster::delayedDeleteSocket(const unsigned short instanceID)
{
#ifdef PRINT_GC
   std::cout << "delayedDeleteSocket: I=" << instanceID << std::endl;
#endif
   ClosingSockets.insert(instanceID);
}


// ###### Try to delete instance ############################################
void SCTPSocketMaster::socketGarbageCollection()
{
#ifdef PRINT_GC
   std::cout << "Socket garbage collection..." << std::endl;
#endif
   MasterInstance.lock();
   LastGarbageCollection = getMicroTime();

   // ====== Try to auto-close connectionless associations ==================
   std::multimap<int, SCTPSocket*>::iterator socketIterator = SocketList.begin();
   while(socketIterator != SCTPSocketMaster::SocketList.end()) {
      SCTPSocket* socket = socketIterator->second;
      socket->checkAutoClose();
      socketIterator++;
   }

   // ====== Try to delete already removed sockets using sctplib ============
   std::set<int>::iterator iterator = ClosingSockets.begin();
   while(iterator != ClosingSockets.end()) {
      const unsigned short instanceID = *iterator;

      bool used = false;
      std::multimap<unsigned int, int>::iterator assocIterator = ClosingAssociations.begin();
      while(assocIterator != ClosingAssociations.end()) {
         if(instanceID == assocIterator->second) {
            used = true;
            break;
         }
         assocIterator++;
      }

      if(!used) {
#ifdef PRINT_GC
         std::cout << "socketGarbageCollection: Removing instance #" << instanceID << "." << std::endl;
#endif
         iterator++;
         ClosingSockets.erase(instanceID);
         if(sctp_unregisterInstance(instanceID) != SCTP_SUCCESS) {
#ifndef DISABLE_WARNINGS
            std::cerr << "INTERNAL ERROR: SCTPSocketMaster::socketGarbageCollection() - sctp_unregisterInstance() failed!" << std::endl;
#endif
            ::abort();
         }
      }
      else {
         iterator++;
      }
   }

   MasterInstance.unlock();
#ifdef PRINT_GC
   std::cout << "Socket garbage collection completed in "
             << getMicroTime() - LastGarbageCollection << " s" << std::endl;
#endif
}


// ###### Try to delete association and instance ############################
bool SCTPSocketMaster::associationGarbageCollection(const unsigned int assocID,
                                                    const bool         sendAbort)
{
   // ====== Delayed removal ================================================
   std::multimap<unsigned int, int>::iterator iterator = ClosingAssociations.find(assocID);
   if(iterator != ClosingAssociations.end()) {
#ifdef PRINT_GC
      std::cout << "associationGarbageCollection: Removing association #" << assocID << "." << std::endl;
#endif

      // ====== Delete association ==========================================
      if(sendAbort) {
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
         sctp_abort(assocID, 0, NULL);
#else
         sctp_abort(assocID);
#endif
      }
      if(sctp_deleteAssociation(assocID) != SCTP_SUCCESS) {
#ifndef DISABLE_WARNINGS
         std::cerr << "INTERNAL ERROR: SCTPSocketMaster::associationGarbageCollection() - sctp_deleteAssociation() failed!" << std::endl;
#endif
         ::abort();
      }
      ClosingAssociations.erase(iterator);

      socketGarbageCollection();
      return(true);
   }
   return(false);
}


// ###### SCTP data arrive notification callback ############################
void SCTPSocketMaster::dataArriveNotif(unsigned int   assocID,
                                       unsigned short streamID,
                                       unsigned int   length,
                                       unsigned short ssn,
                                       unsigned int   tsn,
                                       unsigned int   protoID,
                                       unsigned int   unordered,
                                       void*          ulpDataPtr)
{
#ifdef PRINT_ARRIVENOTIFICATION
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d S%02d: Data Arrive Notification - length=%d, PPID=%u",
               assocID, streamID, length, protoID);
   std::cerr << str << std::endl;
#endif


   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      SCTPNotification notification;
      initNotification(notification, assocID, streamID);
      sctp_data_arrive* sda = &notification.Content.sn_data_arrive;
      sda->sda_type          = SCTP_DATA_ARRIVE;
      sda->sda_flags         = (unordered == 1) ? SCTP_ARRIVE_UNORDERED : 0;
      sda->sda_length        = sizeof(sctp_data_arrive);
      sda->sda_assoc_id      = assocID;
      sda->sda_stream        = streamID;
      sda->sda_ppid          = protoID;
      sda->sda_bytes_arrived = length;
      addNotification(socket,assocID,notification);
   }
}


// ###### SCTP send failure notification callback ###########################
void SCTPSocketMaster::sendFailureNotif(unsigned int   assocID,
                                        unsigned char* unsent_data,
                                        unsigned int   dataLength,
                                        unsigned int*  context,
                                        void*          dummy)
{
#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: Send Failure Notification",assocID);
   std::cerr << str << std::endl;
#endif


   // ====== Generate "Send Failure" notification ===========================
   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      SCTPNotification notification;
      initNotification(notification, assocID, 0);
      sctp_send_failed* ssf = &notification.Content.sn_send_failed;
      ssf->ssf_type     = SCTP_REMOTE_ERROR;
      ssf->ssf_flags    = 0;
      ssf->ssf_length   = sizeof(sctp_send_failed);
      ssf->ssf_error    = 0;
      ssf->ssf_assoc_id = assocID;
      ssf->ssf_info.sinfo_stream     = 0;
      ssf->ssf_info.sinfo_ssn        = 0;
      ssf->ssf_info.sinfo_flags      = 0;
      ssf->ssf_info.sinfo_ppid       = 0;
      ssf->ssf_info.sinfo_context    = 0;
      ssf->ssf_info.sinfo_timetolive = 0;
      ssf->ssf_info.sinfo_assoc_id   = assocID;
      // ??? Sufficient space within SCTPNotification ???
      // memcpy((char*)&ssf->ssf_data,(char*)unsent_data,dataLength);
      addNotification(socket,assocID,notification);
   }
}


// ###### SCTP network status change notification callback ##################
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
void SCTPSocketMaster::networkStatusChangeNotif(unsigned int   assocID,
                                                short          destAddrIndex,
                                                unsigned short newState,
                                                void*          ulpDataPtr)
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
void SCTPSocketMaster::networkStatusChangeNotif(unsigned int assocID,
                                                unsigned int affectedPathID,
                                                int          newState,
                                                void*        ulpDataPtr)
#else
#error Wrong sctplib version!
#endif
{
#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: Network Status Change",assocID);
   std::cerr << str << std::endl;
#endif


   // ====== Select new primary path, if it has become inactive ============
   SCTP_PathStatus pathStatus;
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
   const int ok = sctp_getPathStatus(assocID,destAddrIndex,&pathStatus);
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
   const int ok = sctp_getPathStatus(assocID,affectedPathID,&pathStatus);
#else
#error Wrong sctplib version!
#endif
   if(ok != 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "INTERNAL ERROR: SCTPSocketMaster::networkStatusChangeNotif() - sctp_getPathStatus() failed!" << std::endl;
#endif
      return;
   }


   // ====== Get new destination address ====================================
   SocketAddress* destination = NULL;
   destination = SocketAddress::createSocketAddress(
                                      SocketAddress::PF_HidePort,
                                      (char*)&pathStatus.destinationAddress);
   if(destination == NULL) {
#ifndef DISABLE_WARNINGS
      std::cerr << "INTERNAL ERROR: SCTPSocketMaster::networkStatusChangeNotif() - Bad destination address!" << std::endl;
#endif
      return;
   }

   // ====== Generate "Network Status Change" notification ==================
   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      SCTPNotification notification;
      initNotification(notification, assocID, 0);
      sctp_paddr_change* spc = &notification.Content.sn_paddr_change;
      spc->spc_type     = SCTP_PEER_ADDR_CHANGE;
      spc->spc_flags    = 0;
      spc->spc_error    = 0;
      spc->spc_length   = sizeof(sctp_paddr_change);
      spc->spc_assoc_id = assocID;
      switch(newState) {
         case SCTP_PATH_OK:
            spc->spc_state = SCTP_ADDR_REACHABLE;
          break;
         case SCTP_PATH_UNREACHABLE:
            spc->spc_state = SCTP_ADDR_UNREACHABLE;
          break;
         case SCTP_PATH_ADDED:
            spc->spc_state = SCTP_ADDR_ADDED;
          break;
         case SCTP_PATH_REMOVED:
            spc->spc_state = SCTP_ADDR_REMOVED;
          break;
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20)
         case SCTP_ASCONF_CONFIRMED:
            spc->spc_state = SCTP_ADDR_CONFIRMED;
          break;
         case SCTP_ASCONF_FAILED:
            spc->spc_state = SCTP_ADDR_CONFIRMED;
            spc->spc_error = 1;
          break;
#elif  (SCTPLIB_VERSION == SCTPLIB_1_3_0)
         case SCTP_ASCONF_SUCCEEDED:
            spc->spc_state = SCTP_ADDR_CONFIRMED;
          break;
         case SCTP_ASCONF_FAILED:
            spc->spc_state = SCTP_ADDR_CONFIRMED;
            spc->spc_error = 1;
          break;
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
#else
#error Wrong sctplib version!
#endif
         default:
            spc->spc_state = 0;
          break;
      }
      cardinal addrlen = 0;
      if(destination) {
         if(destination->getFamily() == AF_INET6) {
            addrlen = destination->getSystemAddress((sockaddr*)&spc->spc_aaddr,
                                                    sizeof(sockaddr_storage),
                                                    AF_INET);
         }
         if(addrlen == 0) {
            addrlen = destination->getSystemAddress((sockaddr*)&spc->spc_aaddr,
                                                    sizeof(sockaddr_storage));
         }
      }
      else {
         memset((char*)&spc->spc_aaddr, 0, sizeof(spc->spc_aaddr));
      }
      addNotification(socket,assocID,notification);
   }

   delete destination;
}


// ###### SCTP communication up notification callback #######################
void* SCTPSocketMaster::communicationUpNotif(unsigned int   assocID,
                                             int            status,
                                             unsigned int   noOfDestinations,
                                             unsigned short noOfInStreams,
                                             unsigned short noOfOutStreams,
                                             int            supportPRSCTP,
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
                                             int            adaptationLayerIndicationLen,
                                             void*          adaptationLayerIndication,
#endif
                                             void*          dummy)
{
   SCTPAssociation* association = NULL;
   SCTPNotification notification;
   initNotification(notification, assocID, 0);

#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: Communication Up Notification",assocID);
   std::cerr << str << std::endl;
#endif

   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      // ====== Successfull associate() execution ===========================
      association = socket->getAssociationForAssociationID(assocID);
      if(association != NULL) {
         // ====== Correct RTOMax ============================================
         if(association->RTOMaxIsInitTimeout) {
            SCTP_Association_Status status;
            if(socket->getAssocStatus(assocID,status)) {
#ifdef PRINT_RTOMAXRESTORE
               char str[256];
               snprintf((char*)&str,sizeof(str),
                           "A%04d: Setting InitTimeout %d to saved RTOMax %d",assocID,status.rtoMax,association->RTOMax);
               std::cerr << str << std::endl;
#endif
               status.rtoMax = association->RTOMax;
               socket->setAssocStatus(assocID,status);
            }
            association->RTOMaxIsInitTimeout = false;
         }

         association->CommunicationUpNotification = true;
         association->EstablishCondition.broadcast();
         association->WriteReady   = true;
         association->HasException = false;

         if(association->PreEstablishmentAddressList) {
            SocketAddress::deleteAddressList(association->PreEstablishmentAddressList);
            association->PreEstablishmentAddressList = NULL;
         }
         association->sendPreEstablishmentPackets();
      }
      // ====== Incoming connection =========================================
      else if(socket->Flags & SCTPSocket::SSF_Listening) {
         association = new SCTPAssociation(socket, assocID, socket->NotificationFlags,
                                           socket->Flags & SCTPSocket::SSF_GlobalQueue);
         if(association != NULL) {
            association->CommunicationUpNotification = true;
            SCTPSocket::IncomingConnection* newConnection = new SCTPSocket::IncomingConnection;
            if(newConnection != NULL) {
               newConnection->NextConnection = NULL;
               newConnection->Association    = association;
               newConnection->Notification   = notification;

               if(socket->ConnectionRequests == NULL) {
                  socket->ConnectionRequests = newConnection;
               }
               else {
                  SCTPSocket::IncomingConnection* c = socket->ConnectionRequests;
                  while(c->NextConnection != NULL) {
                     c = c->NextConnection;
                  }
                  c->NextConnection = newConnection;
               }

               socket->ReadReady = true;
               socket->EstablishCondition.broadcast();
            }
            association->WriteReady   = true;
            association->HasException = false;
         }
      }
      // ====== Unwanted incoming association ===============================
      else {
         std::cerr << "Incoming association, but not in listen mode -> rejecting association!" << std::endl;
         association = NULL;
      }
   }

   // ====== Generate "Communication Up" notification =======================
   if(association != NULL) {
      sctp_assoc_change* sac = &notification.Content.sn_assoc_change;
      sac->sac_type   = SCTP_ASSOC_CHANGE;
      sac->sac_flags  = 0;
      sac->sac_length = sizeof(sctp_assoc_change);
      sac->sac_state  = SCTP_COMM_UP;
      sac->sac_error  = 0;
      sac->sac_outbound_streams = noOfOutStreams;
      sac->sac_inbound_streams  = noOfInStreams;
      sac->sac_assoc_id         = assocID;
      addNotification(socket,assocID,notification);
   }
   // ====== We do not want this new association -> remove it! ==============
   else {
#ifdef PRINT_NOTIFICATIONS
      std::cerr << "Aborting and deleting unwanted incoming association!" << std::endl;
#endif
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
      sctp_abort(assocID, 0, NULL);
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
      sctp_abort(assocID);
#else
#error Wrong sctplib version!
#endif
      if(sctp_deleteAssociation(assocID) != SCTP_SUCCESS) {
#ifndef DISABLE_WARNINGS
         std::cerr << "INTERNAL ERROR: SCTPSocketMaster::communicationUpNotif() - sctp_deleteAssociation() or rejected association failed!" << std::endl;
#endif
         ::abort();
      }
   }

   return(NULL);
}


// ###### SCTP communication lost notification callback #####################
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
void SCTPSocketMaster::communicationLostNotif(unsigned int   assocID,
                                              unsigned short status,
                                              void*          ulpDataPtr)
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
void SCTPSocketMaster::communicationLostNotif(unsigned int assocID,
                                              int          status,
                                              void*        ulpDataPtr)
#else
#error Wrong sctplib version!
#endif
{
#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: Communication Lost Notification, Status=%d",assocID,status);
   std::cerr << str << std::endl;
#endif


   // ====== Delayed removal ================================================
   if(associationGarbageCollection(assocID,false)) {
      return;
   }


   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      SCTPAssociation* association = socket->getAssociationForAssociationID(assocID,false);
      if(association != NULL) {
         // ====== Correct RTOMax ============================================
         if(association->RTOMaxIsInitTimeout) {
            SCTP_Association_Status status;
            if(socket->getAssocStatus(assocID,status)) {
#ifdef PRINT_RTOMAXRESTORE
               char str[256];
               snprintf((char*)&str,sizeof(str),
                           "A%04d: Setting InitTimeout %d to saved RTOMax %d",assocID,status.rtoMax,association->RTOMax);
               std::cerr << str << std::endl;
#endif
               status.rtoMax = association->RTOMax;
               socket->setAssocStatus(assocID,status);
            }
            association->RTOMaxIsInitTimeout = false;
         }

         // ====== Notify waiting main thread ===============================
         association->CommunicationLostNotification = true;
         association->ShutdownCompleteNotification  = true;
         association->ShutdownCompleteCondition.broadcast();
         association->ReadUpdateCondition.broadcast();


         // ====== Generate "Communication Lost" notification ===============
         SCTPNotification notification;
         initNotification(notification);
         sctp_assoc_change* sac = &notification.Content.sn_assoc_change;
         sac->sac_type             = SCTP_ASSOC_CHANGE;
         sac->sac_flags            = 0;
         sac->sac_length           = sizeof(sctp_assoc_change);
         sac->sac_state            = SCTP_COMM_LOST;
         sac->sac_error            = 0;
         sac->sac_outbound_streams = 0;
         sac->sac_inbound_streams  = 0;
         sac->sac_assoc_id         = assocID;
         addNotification(socket,assocID,notification);

         // If association() is waiting for connection establishment,
         // send signal abort.
         association->HasException = true;
         association->WriteReady   = true;
         association->ReadReady    = true;
         association->EstablishCondition.broadcast();
         association->ReadyForTransmit.broadcast();
      }
      socket->checkAutoClose();
   }
}


// ###### SCTP communication error notification callback ####################
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
void SCTPSocketMaster::communicationErrorNotif(unsigned int   assocID,
                                               unsigned short status,
                                               void*          dummy)
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
void SCTPSocketMaster::communicationErrorNotif(unsigned int assocID,
                                               int          status,
                                               void*        dummy)
#else
#error Wrong sctplib version!
#endif
{
#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: Communication Error Notification, Status=%d",assocID,status);
   std::cerr << str << std::endl;
#endif


   // ====== Generate "Communication Error" notification ====================
   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      SCTPNotification notification;
      initNotification(notification, assocID, 0);
      sctp_remote_error* sre = &notification.Content.sn_remote_error;
      sre->sre_type      = SCTP_REMOTE_ERROR;
      sre->sre_flags     = 0;
      sre->sre_length    = sizeof(sctp_remote_error);
      sre->sre_error     = 0;
      sre->sre_assoc_id  = assocID;
      addNotification(socket,assocID,notification);
   }
}


// ###### SCTP restart notification callback ################################
void SCTPSocketMaster::restartNotif(unsigned int assocID, void* ulpDataPtr)
{
#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: Restart Notification",assocID);
   std::cerr << str << std::endl;
#endif


   // ====== Generate "Restart" notification ==================================
   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      SCTPNotification notification;
      initNotification(notification);
      sctp_assoc_change* sac = &notification.Content.sn_assoc_change;
      sac->sac_type             = SCTP_ASSOC_CHANGE;
      sac->sac_flags            = 0;
      sac->sac_length           = sizeof(sctp_assoc_change);
      sac->sac_state            = SCTP_RESTART;
      sac->sac_error            = 0;
      SCTP_Association_Status status;
      if(sctp_getAssocStatus(assocID,&status) == 0) {
         sac->sac_outbound_streams = status.outStreams;
         sac->sac_inbound_streams  = status.inStreams;
      }
      else {
#ifndef DISABLE_WARNINGS
         std::cerr << "WARNING: SCTPSocketMaster::restartNotif() - sctp_getAssocStatus() failed!" << std::endl;
#endif
         sac->sac_outbound_streams = 1;
         sac->sac_inbound_streams  = 1;
      }
      sac->sac_assoc_id = assocID;
      addNotification(socket,assocID,notification);
   }
}


// ###### SCTP shutdown complete notification callback ######################
void SCTPSocketMaster::shutdownReceivedNotif(unsigned int assocID, void* ulpDataPtr)
{
#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: Shutdown Received Notification",assocID);
   std::cerr << str << std::endl;
#endif

   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      SCTPAssociation* association = socket->getAssociationForAssociationID(assocID, false);
      if(association != NULL) {
         // ====== Generate "Shutdown Complete" notification ================
         SCTPNotification notification;
         initNotification(notification);
         sctp_shutdown_event* sse = &notification.Content.sn_shutdown_event;
         sse->sse_type            = SCTP_SHUTDOWN_EVENT;
         sse->sse_flags           = 0;
         sse->sse_length          = sizeof(sctp_assoc_change);
         sse->sse_assoc_id        = assocID;
         addNotification(socket,assocID,notification);
      }
   }
}


// ###### SCTP shutdown complete notification callback ######################
void SCTPSocketMaster::shutdownCompleteNotif(unsigned int assocID, void* ulpDataPtr)
{
#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: Shutdown Complete Notification",assocID);
   std::cerr << str << std::endl;
#endif

   // ====== Delayed removal ================================================
   if(associationGarbageCollection(assocID,false)) {
      return;
   }

   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      SCTPAssociation* association = socket->getAssociationForAssociationID(assocID,false);
      if(association != NULL) {
         // ====== Notify waiting main thread ===============================
         association->WriteReady   = true;
         association->ReadReady    = true;
         association->HasException = true;
         association->ShutdownCompleteNotification = true;
         association->ShutdownCompleteCondition.broadcast();
         association->ReadyForTransmit.broadcast();
         association->ReadUpdateCondition.broadcast();

         // ====== Generate "Shutdown Complete" notification ================
         SCTPNotification notification;
         initNotification(notification);
         sctp_assoc_change* sac = &notification.Content.sn_assoc_change;
         sac->sac_type             = SCTP_ASSOC_CHANGE;
         sac->sac_flags            = 0;
         sac->sac_length           = sizeof(sctp_assoc_change);
         sac->sac_state            = SCTP_SHUTDOWN_COMP;
         sac->sac_error            = 0;
         sac->sac_outbound_streams = 0;
         sac->sac_inbound_streams  = 0;
         sac->sac_assoc_id         = assocID;
         addNotification(socket,assocID,notification);
      }
      socket->checkAutoClose();
   }
}


// ###### SCTP queue status change notification callback ######################
void SCTPSocketMaster::queueStatusChangeNotif(unsigned int assocID,
                                              int          queueType,
                                              int          queueIdentifier,
                                              int          queueLength,
                                              void*        ulpData)
{
#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: Queue Status Change Notification, Qid=%d Qtype=%d Qlength=%d",
               assocID,queueIdentifier,queueType,queueLength);
   std::cerr << str << std::endl;
#endif

   SCTPSocket* socket = getSocketForAssociationID(assocID);
   if(socket != NULL) {
      SCTPAssociation* association = socket->getAssociationForAssociationID(assocID,false);
      if(association != NULL) {
         association->ReadyForTransmit.broadcast();
         association->WriteReady = true;
         association->sendPreEstablishmentPackets();
      }
   }
}


#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
// ###### SCTP asconf status change notification callback #####################
void SCTPSocketMaster::asconfStatusNotif(unsigned int assocID,
                                         unsigned int correlationID,
                                         int          result,
                                         void*        request,
                                         void*        ulpData)
{
#ifdef PRINT_NOTIFICATIONS
   char str[256];
   snprintf((char*)&str,sizeof(str),
               "A%04d: ASConf Status Change Notification, Cid=%d result=%d",
               assocID,correlationID,result);
   std::cerr << str << std::endl;
#endif
}
#endif


// ###### User callback #####################################################
void SCTPSocketMaster::userCallback(int        fileDescriptor,
                                    short int  eventMask,
                                    short int* registeredEvents,
                                    void*      userData)
{
   char str[256];
#ifdef PRINT_USERCALLBACK
   snprintf((char*)&str,sizeof(str),
               "F%04d: User Callback, mask=$%x",
               fileDescriptor,eventMask);
   std::cerr << str << std::endl;
#endif

   UserSocketNotification* usn = (UserSocketNotification*)userData;
   if(userData != NULL) {
      if(usn->FileDescriptor != BreakPipe[0]) {
         usn->Events |= eventMask;
         *registeredEvents &= ~eventMask;
         if(eventMask & usn->EventMask) {
            usn->UpdateCondition.broadcast();
         }
      }
      else {
#ifdef PRINT_PIPE
         std::cout << getpid() << ": Break via break pipe received" << std::endl;
#endif
         ssize_t received = read(BreakPipe[0],(char*)&str,sizeof(str));
         while(received > 0) {
#ifdef PRINT_PIPE
           std::cout << "Break Pipe: " << received << " calls" << std::endl;
#endif
           received = read(BreakPipe[0],(char*)&str,sizeof(str));
         }
         BreakNotification.UpdateCondition.fired();
      }
   }
}


// ###### Get socket for given association ID ###############################
SCTPSocket* SCTPSocketMaster::getSocketForAssociationID(const unsigned int assocID)
{
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
   unsigned short instanceID = 0;
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
   int instanceID = 0;
#else
#error Wrong sctplib version!
#endif

   if(sctp_getInstanceID(assocID,&instanceID) == 0) {
      if(instanceID != 0) {
         std::multimap<int, SCTPSocket*>::iterator iterator =
            SocketList.find((int)instanceID);
         if(iterator != SocketList.end()) {
            return(iterator->second);
         }
      }
#ifdef DEBUG
      else {
        std::cerr << "WARNING: SCTPSocketMaster::getSocketForAssociationID() - "
                     "Instance ID for association ID #" << assocID << " is zero!" << std::endl;
      }
#endif
   }
#ifdef DEBUG
   else {
      std::cerr << "WARNING: SCTPSocketMaster::getSocketForAssociationID() - "
                   "No instance ID for association ID #" << assocID << " found!" << std::endl;
   }
#endif
   return(NULL);
}


// ###### Clear SCTPNotification structure ##################################
void SCTPSocketMaster::initNotification(SCTPNotification& notification)
{
   notification.Content.sn_header.sn_type = SCTP_UNDEFINED;
   notification.ContentPosition = 0;
   notification.RemotePort      = 0;
   notification.RemoteAddresses = 0;
   for(unsigned int i = 0;i < SCTP_MAX_NUM_ADDRESSES;i++) {
      notification.RemoteAddress[i][0] = 0x00;
   }
}


// ###### Initialize SCTPNotification structure #############################
bool SCTPSocketMaster::initNotification(SCTPNotification& notification,
                                        unsigned int      assocID,
                                        unsigned short    streamID)
{
   notification.Content.sn_header.sn_type = SCTP_UNDEFINED;
   notification.ContentPosition = 0;
   SCTP_Association_Status status;
   if(sctp_getAssocStatus(assocID, &status) == 0) {
      notification.RemotePort      = status.destPort;
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
      notification.RemoteAddresses = std::min((unsigned short)SCTP_MAX_NUM_ADDRESSES,
                                              status.numberOfAddresses);
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
      notification.RemoteAddresses = std::min((unsigned short)SCTP_MAX_NUM_ADDRESSES,
                                              status.numberOfDestinationPaths);
#else
#error Wrong sctplib version!
#endif
      for(unsigned int i = 0;i < notification.RemoteAddresses;i++) {
         SCTP_Path_Status pathStatus;
#if (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE19) || (SCTPLIB_VERSION == SCTPLIB_1_0_0)
         if(sctp_getPathStatus(assocID,i,&pathStatus) != 0) {
#elif (SCTPLIB_VERSION == SCTPLIB_1_0_0_PRE20) || (SCTPLIB_VERSION == SCTPLIB_1_3_0)
         if(sctp_getPathStatus(assocID,status.destinationPathIDs[i],&pathStatus) != 0) {
#else
#error Wrong sctplib version!
#endif

#ifndef DISABLE_WARNINGS
            std::cerr << "WARNING: SCTPSocketMaster::initNotification() - sctp_getPathStatus() failure!"
                      << std::endl;
#endif

         }
         else {
            memcpy((char*)&notification.RemoteAddress[i],
                   (char*)&pathStatus.destinationAddress,
                   sizeof(pathStatus.destinationAddress));
         }
      }
      return(true);
   }
#ifndef DISABLE_WARNINGS
         std::cerr << "WARNING: SCTPSocketMaster::initNotification() - sctp_getAssocStatus() failure!"
                   << std::endl;
#endif
   return(false);
}


// ###### Add SCTPNotification structure to socket's queue ##################
void SCTPSocketMaster::addNotification(SCTPSocket*             socket,
                                       unsigned int            assocID,
                                       const SCTPNotification& notification)
{
   // ====== Get notification flags =========================================
   SCTPAssociation* association = socket->getAssociationForAssociationID(assocID, false);
   if(association == NULL) {
      // Association not found -> already closed.
      return;
   }
   const unsigned int notificationFlags = association->NotificationFlags;

   // ====== Check, if notification has to be added =========================
   if((notification.Content.sn_header.sn_type == SCTP_DATA_ARRIVE)    ||
      ((notification.Content.sn_header.sn_type == SCTP_ASSOC_CHANGE)     && (notificationFlags & SCTP_RECVASSOCEVNT))    ||
      ((notification.Content.sn_header.sn_type == SCTP_PEER_ADDR_CHANGE) && (notificationFlags & SCTP_RECVPADDREVNT))    ||
      ((notification.Content.sn_header.sn_type == SCTP_REMOTE_ERROR)     && (notificationFlags & SCTP_RECVPEERERR))      ||
      ((notification.Content.sn_header.sn_type == SCTP_SEND_FAILED)      && (notificationFlags & SCTP_RECVSENDFAILEVNT)) ||
      ((notification.Content.sn_header.sn_type == SCTP_SHUTDOWN_EVENT)   && (notificationFlags & SCTP_RECVSHUTDOWNEVNT))) {

      association->UseCount++;
#ifdef PRINT_ASSOC_USECOUNT
      std::cout << association->UseCount << ". Notification Type = " << notification.Content.sn_header.sn_type << std::endl;
#endif

      // ====== Add notification to global or association's queue ===========
#ifdef PRINT_ASSOC_USECOUNT
      std::cout << "AddNotification: UseCount increment for A" << association->getID() << ": "
                << association->UseCount << " -> ";
#endif
      if( (socket->Flags & SCTPSocket::SSF_GlobalQueue) &&
          (association->PeeledOff == false) ) {
         socket->GlobalQueue.addNotification(notification);
         socket->ReadReady = socket->hasData() || (socket->ConnectionRequests != NULL);
      }
      else {
         association->InQueue.addNotification(notification);
         association->ReadReady = association->hasData();
      }
   }

   else {
      /* The user does not want a notification, but an action has to be
         signallised in order to end waiting for events (e.g. interrupting
         SCTPSocket::interalReceive() in case of a shutdown complete)! */
      if(!((socket->Flags & SCTPSocket::SSF_GlobalQueue) &&
           (association->PeeledOff == false))) {
         association->InQueue.signal();
      }
   }
}


// ###### Add user socket notification ######################################
void SCTPSocketMaster::addUserSocketNotification(UserSocketNotification* usn)
{
   lock();
   usn->Events = 0;
   const int result = sctp_registerUserCallback(usn->FileDescriptor,userCallback,(void*)usn,usn->EventMask);

   if(result < 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: SCTPSocketMaster::addUserSocketNotification() - sctp_registerUserCallback() failed!" << std::endl;
#endif
   }

   if((usn->FileDescriptor != BreakPipe[0]) && (BreakPipe[0] != -1)) {
      char dummy = 'T';
#ifdef PRINT_PIPE
      std::cout << "Sending Break via break pipe..." << std::endl;
#endif
      dummy = write(BreakPipe[1],&dummy,sizeof(dummy));
   }

   unlock();
}


// ###### Delete user socket notification ###################################
void SCTPSocketMaster::deleteUserSocketNotification(UserSocketNotification* usn)
{
   lock();
   if(sctp_unregisterUserCallback(usn->FileDescriptor) != SCTP_SUCCESS) {
#ifndef DISABLE_WARNINGS
      std::cerr << "INTERNAL ERROR: SCTPSocketMaster::deleteUserSocketNotification() - sctp_unregisterUserCallback() failed!" << std::endl;
#endif
   }
   unlock();
}


// ###### Lock callback #####################################################
void SCTPSocketMaster::lock(void* data)
{
   SCTPSocketMaster* task = (SCTPSocketMaster*)data;
   task->lock();
}


// ###### Unlock callback ###################################################
void SCTPSocketMaster::unlock(void* data)
{
   SCTPSocketMaster* task = (SCTPSocketMaster*)data;
   task->unlock();
}
