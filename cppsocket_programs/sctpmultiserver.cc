/*
 *  $Id: sctpmultiserver.cc,v 1.4 2003/08/19 19:28:34 tuexen Exp $
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
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de which should be
 * used for any discussion related to this implementation.
 *
 * Contact: discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *          tuexen@fh-muenster.de
 *
 * Purpose: SCTP Multi Server
 *
 */


/*
 Important: The FreeBSD macros cause Socket methods to be translated
            to __pthread_* names. This must be prevented!
 */
#if (SYSTEM == OS_FreeBSD)
#define _POSIX_THREAD_SYSCALL_SOFT 0
#endif


#include "tdsystem.h"
#include "thread.h"
#include "tdsocket.h"
#include "tdmessage.h"
#include "randomizer.h"
#include "breakdetector.h"
#include "ext_socket.h"
#include "ansicolor.h"
#include "sctpinfoprinter.h"
#include "sctptftp.h"


#include <ctype.h>
#include <set>


#define SERVER_ECHO
#define SERVER_DISCARD
#define SERVER_DAYTIME
#define SERVER_CHARGEN
#define SERVER_TFTP
#define SERVER_COMTEST

#define ECHO_PORT       7
#define DISCARD_PORT    9
#define DAYTIME_PORT   13
#define CHARGEN_PORT   19
#define TFTP_PORT      69
#define COMTEST_PORT 2415

#define SCTP_MAXADDRESSES 20


class ServerSet : public Thread
{
   // ====== Public data ====================================================
   public:
   ServerSet(const char* name, Socket* socket);
   ~ServerSet();

   static void        shutdown();
   static inline void garbageCollection();


   // ====== Protected data =================================================
   protected:
   void        killMe();
   static void garbageCollection(set<ServerSet*>& gcSet);

   Socket*                ServerSocket;
   static Synchronizable  ServersSynchronizable;
   static set<ServerSet*> Servers;
   static set<ServerSet*> DeletedServers;
};


// ###### Static variables ##################################################
Synchronizable  ServerSet::ServersSynchronizable("ServerSet::ServersSynchronizable");
set<ServerSet*> ServerSet::Servers;
set<ServerSet*> ServerSet::DeletedServers;


// ###### Constructor #######################################################
ServerSet::ServerSet(const char* name, Socket* socket)
   : Thread(name)
{
   ServerSocket = socket;
   ServersSynchronizable.synchronized();
   Servers.insert(this);
   ServersSynchronizable.unsynchronized();
}


// ###### Destructor ########################################################
ServerSet::~ServerSet()
{
   stop();
   delete ServerSocket;
   ServerSocket = NULL;
}


// ###### Mark thread for deletion ##########################################
void ServerSet::killMe()
{
   Thread::delay(250000);
   ServerSocket->close();
   ServersSynchronizable.synchronized();
   Servers.erase(this);
   DeletedServers.insert(this);
   ServersSynchronizable.unsynchronized();
}


// ###### Do garbage collection #############################################
void ServerSet::garbageCollection(set<ServerSet*>& gcSet)
{
   ServerSet::ServersSynchronizable.synchronized();
   set<ServerSet*>::iterator iterator = gcSet.begin();
   while(iterator != gcSet.end()) {
      ServerSet* server = *iterator;
      delete server;
      gcSet.erase(iterator);
      iterator = gcSet.begin();
   }
   ServerSet::ServersSynchronizable.unsynchronized();
}


// ###### Do shutdown #######################################################
void ServerSet::shutdown()
{
   garbageCollection(DeletedServers);
   garbageCollection(Servers);
}


// ###### Do garbage collection #############################################
inline void ServerSet::garbageCollection()
{
   garbageCollection(DeletedServers);
}



class EchoServer : public ServerSet
{
   // ====== Public methods =================================================
   public:
   EchoServer(Socket* socket, const cardinal unreliable, const bool discardMode);

   // ====== Private data ===================================================
   private:
   void run();

   cardinal Unreliable;
   bool     DiscardMode;
};


// ###### Constructor #######################################################
EchoServer::EchoServer(Socket* socket, const cardinal unreliable, const bool discardMode)
   : ServerSet("EchoServer", socket)
{
   Unreliable  = unreliable;
   DiscardMode = discardMode;
   start();
}


// ###### Main loop #########################################################
void EchoServer::run()
{
   cardinal inStreams  = 1;
   cardinal outStreams = 1;
   bool     shutdown   = false;
   char dataBuffer[16384 + 1];
   while(!shutdown) {
      // ====== Receive string from association =============================
      SocketMessage<CSpace(sizeof(sctp_sndrcvinfo))> message;
      message.setBuffer((char*)&dataBuffer,sizeof(dataBuffer));

      // ====== Receive string from association =============================
      const ssize_t dataLength = ServerSocket->receiveMsg(&message.Header,0);
      if(dataLength <= 0) {
         printTimeStamp(cout);
         if(dataLength < 0) {
            cout << "Thread shutdown due to receive error #"
                 << -dataLength << "." << endl
                 << "   Description: " << strerror(-dataLength) << "!" << endl;
         }
         else {
            cout << "Thread shutdown!" << endl;
         }
         killMe();
         return;
      }


      // ====== Get control data ============================================
      unsigned int   assocID  = 0;
      unsigned short streamID = 0;
      unsigned int   protoID  = 0;
      cmsghdr* cmsg = message.getFirstHeader();
      if(cmsg != NULL) {
         while(cmsg != NULL) {
            if((cmsg->cmsg_level == IPPROTO_SCTP) &&
               (cmsg->cmsg_type  == SCTP_SNDRCV)) {
               sctp_sndrcvinfo* info = (sctp_sndrcvinfo*)CData(cmsg);
               assocID  = info->sinfo_assoc_id;
               streamID = info->sinfo_stream;
               protoID  = info->sinfo_ppid;
            }
            cmsg = message.getNextHeader(cmsg);
         }
      }
      printControl(&message.Header);


      // ====== Reply, if received block is a data block ====================
      if(!(message.Header.msg_flags & MSG_NOTIFICATION)) {
         if(!DiscardMode) {
            message.clear();
            message.setBuffer(dataBuffer,dataLength);
            sctp_sndrcvinfo* info = (sctp_sndrcvinfo*)message.addHeader(sizeof(sctp_sndrcvinfo),IPPROTO_SCTP,SCTP_SNDRCV);
            info->sinfo_assoc_id  = 0;
            info->sinfo_stream    = (streamID % outStreams);
            info->sinfo_ppid      = protoID;
            if((Unreliable > 0) && (info->sinfo_stream <= Unreliable - 1)) {
               info->sinfo_flags      = MSG_PR_SCTP_TTL;
               info->sinfo_timetolive = 0;
            }
            else {
               info->sinfo_flags      = 0;
               info->sinfo_timetolive = 0;
            }
            const ssize_t result = ServerSocket->sendMsg(&message.Header,0);
            if(result < 0) {
               printTimeStamp(cout);
               cout << "Thread shutdown due to send error #"
                    << -dataLength << "!" << endl;
               killMe();
               return;
            }
         }

         // ====== Print information ========================================
         char str[16384 + 256];
         dataBuffer[dataLength - 1] = 0x00;
         snprintf((char*)&str,sizeof(str),"#%d.%d/$%08x: %s [%s]",
                     assocID, streamID, protoID,
                     (DiscardMode == true) ? "Discard" : "Echo",
                     dataBuffer);
         if(ColorMode) {
            cout << "\x1b[" << getANSIColor(streamID + 1) << "m";
         }
         cout << str << endl;
         if(ColorMode) {
            cout << "\x1b[" << getANSIColor(0) << "m";
         }
      }

      // ====== Handle notification =========================================
      else {
         const sctp_notification* notification = (sctp_notification*)&dataBuffer;
         printNotification(notification);
         if(notification->sn_header.sn_type == SCTP_ASSOC_CHANGE) {
            const sctp_assoc_change* sac = &notification->sn_assoc_change;
            inStreams  = sac->sac_inbound_streams;
            outStreams = sac->sac_outbound_streams;
         }
         else if(notification->sn_header.sn_type == SCTP_SHUTDOWN_EVENT) {
            shutdown = true;
         }
      }
   }
   printTimeStamp(cout);
   cout << "Thread shutdown!" << endl;
   killMe();
}




class CharGenServer : public ServerSet
{
   // ====== Public methods =================================================
   public:
   CharGenServer(Socket* socket, const cardinal unreliable);

   // ====== Private data ===================================================
   private:
   cardinal Unreliable;

   void run();
};


// ###### Constructor #######################################################
CharGenServer::CharGenServer(Socket* socket, const cardinal unreliable)
   : ServerSet("CharGenServer",socket)
{
   Unreliable = unreliable;
   start();
}


// ###### Main loop #########################################################
void CharGenServer::run()
{
   SocketMessage<CSpace(sizeof(sctp_sndrcvinfo))> message;
   char   dataBuffer[512];
   card64 line = 1;

   for(;;) {
      // ====== Generate string =============================================
      snprintf((char*)&dataBuffer,256,"#%08Ld: ",line++);
      integer i,j;
      j = strlen(dataBuffer);
      for(i = 30;i < 128;i++) {
         dataBuffer[j++] = (char)i;
      }
      dataBuffer[j++] = '\n';


      // ====== Receive string from association =============================
      testCancel();
      message.clear();
      message.setBuffer(dataBuffer, j);
      sctp_sndrcvinfo* info = (sctp_sndrcvinfo*)message.addHeader(sizeof(sctp_sndrcvinfo),IPPROTO_SCTP,SCTP_SNDRCV);
      info->sinfo_assoc_id  = 0;
      info->sinfo_stream    = 0;
      info->sinfo_ppid      = 0;
      if((Unreliable > 0) && (info->sinfo_stream <= Unreliable - 1)) {

         info->sinfo_flags      = MSG_PR_SCTP_TTL;
         info->sinfo_timetolive = 0;
      }
      else {
         info->sinfo_flags      = 0;
         info->sinfo_timetolive = 0;
      }
      const ssize_t result = ServerSocket->sendMsg(&message.Header,0);
      if(result < 0) {
         printTimeStamp(cout);
         cout << "Thread shutdown due to send error #"
              << -result << "!" << endl;
         killMe();
         return;
      }
   }
}




class TrivialFileTransferServer : public ServerSet
{
   // ====== Public methods =================================================
   public:
   TrivialFileTransferServer(Socket* socket);

   // ====== Private data ===================================================
   private:
   void run();
   bool getName(const ssize_t size);
   ssize_t sendAck(const char code);
   ssize_t sendError(const char  code,
                     const char* text);

   TFTPPacket Packet;
   char       FileName[512];
};


// ###### Constructor #######################################################
TrivialFileTransferServer::TrivialFileTransferServer(Socket* socket)
   : ServerSet("TrivialFileTransferServer",socket)
{
   start();
}


// ###### Get file name from packet #########################################
bool TrivialFileTransferServer::getName(const ssize_t size)
{
   if(size >= 3) {
      ssize_t i;
      for(i = 0;i < size - 2;i++) {
         FileName[i] = Packet.Data[i];
         if((!isprint(FileName[i])) ||
            (!isascii(FileName[i])) ||
            (FileName[i] == ':')    ||
            (FileName[i] == '/')    ||
            (FileName[i] == '\\')   ||
            (FileName[i] == '*')    ||
            (FileName[i] == '?')) {
            return(false);
         }
      }
      FileName[i] = 0x00;
      return(true);
   }
   return(false);
}


// ###### Send error code ###################################################
ssize_t TrivialFileTransferServer::sendError(const char  code,
                                             const char* text)
{
   Packet.Type = TFTP_ERR;
   Packet.Code = code;
   snprintf((char*)&Packet.Data,sizeof(Packet.Data),"%s",text);
   return(ServerSocket->send((char*)&Packet,2 + strlen(text)));
}


// ###### Send acknowledgement ##############################################
ssize_t TrivialFileTransferServer::sendAck(const char code)
{
   Packet.Type = TFTP_ACK;
   Packet.Code = code;
   return(ServerSocket->send((char*)&Packet,2));
}


// ###### Main loop #########################################################
void TrivialFileTransferServer::run()
{
   ssize_t result;
   result = ServerSocket->receive((char*)&Packet,sizeof(Packet));
   if(result >= 3) {
      // ====== Read Request ================================================
      if(Packet.Type == TFTP_RRQ) {
         if(getName(result)) {
            FILE* in = fopen(FileName,"r");
            if(in != NULL) {
               // ====== Read loop ==========================================
               Packet.Type = TFTP_DAT;
               Packet.Code = 0;
               while(!feof(in)) {
                  ssize_t bytes = fread((char*)&Packet.Data,1,sizeof(Packet.Data),in);
                  if(bytes >= 0) {
                     result = ServerSocket->send((char*)&Packet,2 + bytes);
                     if(result < 0) {
                        break;
                     }
                  }
                  Packet.Code++;
               }
               fclose(in);
            }
            else {
               sendError(3,"Unable to open file!");
            }
         }
         else {
            sendError(1,"Bad file name!");
         }
      }

      // ====== Write Request ===============================================
      else if(Packet.Type == TFTP_WRQ) {
         if(getName(result)) {
            FILE* out = fopen(FileName,"w");
            if(out != NULL) {
               result = sendAck(0);
               if(result > 0) {
                  // ====== Write loop ======================================
                  result = ServerSocket->receive((char*)&Packet,sizeof(Packet));
                  while(result >= 2) {
                     if(Packet.Type != TFTP_DAT) {
                        sendError(0,"Bad request!");
                        break;
                     }
                     const ssize_t bytes = result - 2;
                     if((ssize_t)fwrite((char*)&Packet.Data,bytes,1,out) != 1) {
                        sendError(4,"Write error!");
                        break;
                     }
                     if(bytes < (ssize_t)sizeof(Packet.Data)) {
                        sendAck(Packet.Code);
                        break;
                     }
                     result = ServerSocket->receive((char*)&Packet,sizeof(Packet));
                  }
               }
               fclose(out);
            }
            else {
               sendError(2,"Unable to create file!");
            }
         }
         else {
            sendError(1,"Bad file name!");
         }
      }

      // ====== Bad request =================================================
      else {
         sendError(0,"Bad request!");
      }
   }

   if(result < 0) {
      printTimeStamp(cout);
      cout << "Thread shutdown due to send error #"
           << -result << "!" << endl;
   }
   else {
      printTimeStamp(cout);
      cout << "Thread shutdown!" << endl;
   }

   killMe();
}




class DaytimeServer : public ServerSet
{
   // ====== Public methods =================================================
   public:
   DaytimeServer(Socket* socket);

   // ====== Private data ===================================================
   private:
   void run();

   bool TestMode;
};


// ###### Constructor #######################################################
DaytimeServer::DaytimeServer(Socket* socket)
   : ServerSet("DaytimeServer",socket)
{
   start();
}


// ###### Main loop #########################################################
void DaytimeServer::run()
{
   char str1[256];
   char str2[128];
   const card64 microTime = getMicroTime();
   const time_t timeStamp = microTime / 1000000;
   const struct tm *timeptr = localtime(&timeStamp);
   strftime((char*)&str1,sizeof(str1),"%A %d-%b-%Y %H:%M:%S",timeptr);
   snprintf((char*)&str2,sizeof(str2),
            ".%06d ",(cardinal)(microTime % 1000000));
   strcat((char*)&str1,(char*)&str2);
   strftime((char*)&str2,sizeof(str2),"%Z\n",timeptr);
   strcat((char*)&str1,(char*)&str2);

   const int result = ServerSocket->send(str1,strlen(str1),0);
   if(result < 0) {
      printTimeStamp(cout);
      cout << "Daytime server got send error #"
           << -result << "!" << endl;
   }

   killMe();
}




class MultiServer : public Thread
{
   public:
   MultiServer();
   ~MultiServer();


   enum MultiServerType {
      MST_Echo    = 0,
      MST_Discard = 1,
      MST_Daytime = 2,
      MST_CharGen = 3,
      MST_TFTP    = 4,
      MST_Test    = 999
   };

   bool init(SocketAddress** addressArray,
             const cardinal  addresses,
             const cardinal  port,
             const cardinal  type,
             const cardinal  maxIncoming,
             const cardinal  maxOutgoing,
             const cardinal  unreliable);


   private:
   void run();

   cardinal MaxIncoming;
   cardinal MaxOutgoing;
   cardinal Unreliable;
   cardinal Type;
   Socket*  ServerSocket;
};



// ###### Constructor #######################################################
MultiServer::MultiServer()
   : Thread("MultiServer")
{
   ServerSocket = NULL;
}


// ###### Destructor ########################################################
MultiServer::~MultiServer()
{
   stop();
   if(ServerSocket != NULL) {
      delete ServerSocket;
      ServerSocket = NULL;
   }
}


// ###### Initialize ########################################################
bool MultiServer::init(SocketAddress** addressArray,
                       const cardinal  addresses,
                       const cardinal  port,
                       const cardinal  type,
                       const cardinal  maxIncoming,
                       const cardinal  maxOutgoing,
                       const cardinal  unreliable)
{
   // ====== Create socket ==================================================
   MaxIncoming = maxIncoming;
   MaxOutgoing = maxOutgoing;
   Unreliable  = unreliable;
   Type = type;
   SocketAddress* socketAddressArray[addresses];
   for(cardinal i = 0;i < addresses;i++) {
      addressArray[i]->setPort(port);
      socketAddressArray[i] = addressArray[i];
   }
   ServerSocket = new Socket(Socket::IP,Socket::Stream,Socket::SCTP);
   if(ServerSocket == NULL) {
      cerr << "ERROR: MultiServer::init() - Out of memory!" << endl;
      return(false);
   }

   // ====== Set sctp_initmsg parameters ====================================
   sctp_initmsg init;
   init.sinit_num_ostreams   = maxOutgoing;
   init.sinit_max_instreams  = maxIncoming;
   init.sinit_max_attempts   = 0;
   init.sinit_max_init_timeo = 60;
   if(ServerSocket->setSocketOption(IPPROTO_SCTP,SCTP_INITMSG,(char*)&init,sizeof(init)) < 0) {
      cerr << "ERROR: MultiServer::init() - Unable to set SCTP_INITMSG parameters!" << endl;
      delete ServerSocket;
      return(false);
   }


   // ====== Bind socket ====================================================
   if(ServerSocket->bindx((const SocketAddress**)&socketAddressArray,
                          addresses,
                          SCTP_BINDX_ADD_ADDR) == false) {
      cerr << "ERROR: MultiServer::init() - Unable to bind socket!" << endl;
      delete ServerSocket;
      return(false);
   }

   sctp_event_subscribe events;
   memset((char*)&events,1,sizeof(events));
   if(ServerSocket->setSocketOption(IPPROTO_SCTP,SCTP_EVENTS,&events,sizeof(events)) < 0) {
      cerr << "ERROR: MultiServer::init() - SCTP_EVENTS failed!" << endl;
      delete ServerSocket;
      return(false);
   }

   start();
   return(true);
}


// ###### Thread main loop ##################################################
void MultiServer::run()
{
   if(ServerSocket != NULL) {
      ServerSocket->listen(10);

      for(;;) {
         SocketAddress* peer = NULL;
         Socket* newSocket = ServerSocket->accept(&peer);
         if(newSocket != NULL) {
            sctp_event_subscribe events;
            memset((char*)&events,1,sizeof(events));
            if(newSocket->setSocketOption(IPPROTO_SCTP,SCTP_EVENTS,&events,sizeof(events)) < 0) {
               cerr << "WARNING: MultiServer::run() - SCTP_EVENTS failed!" << endl;
            }

            printTimeStamp(cout);
            if(peer != NULL) {
               peer->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_Legacy);
               cout << "Accepted association from " << *peer << " to ";
               delete peer;
            }
            else {
               cout << "Accepted association from (Unknown) to ";
            }

            switch(Type) {
               case MST_Echo:
                  {
                     cout << "Echo server." << endl;
                     EchoServer* server = new EchoServer(newSocket,Unreliable,false);
                     if(server == NULL) {
                        delete newSocket;
                     }
                  }
                break;
               case MST_Discard:
                  {
                     cout << "Discard server." << endl;
                     EchoServer* server = new EchoServer(newSocket,0,true);
                     if(server == NULL) {
                        delete newSocket;
                     }
                  }
                break;
               case MST_CharGen:
                  {
                     cout << "CharGen server." << endl;
                     CharGenServer* server = new CharGenServer(newSocket,Unreliable);
                     if(server == NULL) {
                        delete newSocket;
                     }
                  }
                break;
               case MST_TFTP:
                  {
                     cout << "TFTP server." << endl;
                     TrivialFileTransferServer* server = new TrivialFileTransferServer(newSocket);
                     if(server == NULL) {
                        delete newSocket;
                     }
                  }
                break;
               case MST_Test: {
                     cout << "Test server." << endl;
                     delete newSocket;
                  }
                break;
               case MST_Daytime:
               default:
                  {
                     cout << "Daytime server." << endl;
                     DaytimeServer* server = new DaytimeServer(newSocket);
                     if(server == NULL) {
                        delete newSocket;
                     }
                  }
                break;
            }
         }
      }
   }
}




// ###### Main program ######################################################
int main(int argc, char** argv)
{
   SocketAddress** localAddressArray = NULL;
   cardinal        localAddresses    = 0;
   cardinal        maxIncoming       = 128;
   cardinal        maxOutgoing       = 128;
   cardinal        unreliable        = 0;
   bool            optForceIPv4      = false;
   PrintControl       = false;
   PrintNotifications = false;
   if(!sctp_isavailable()) {
#ifdef HAVE_KERNEL_SCTP
      cerr << "ERROR: Kernel-based SCTP is not available!" << endl;
#else
     if(getuid() != 0) {
        cerr << "ERROR: You need root permissions to use the SCTP Library!" << endl;
     }
     else {
        cerr << "ERROR: SCTP is not available!" << endl;
     }
#endif
      exit(1);
   }


   // ====== Get arguments ==================================================
   for(unsigned int i = 1;i < (cardinal)argc;i++) {
      if(!(strcasecmp(argv[i],"-force-ipv4")))    optForceIPv4 = true;
      else if(!(strcasecmp(argv[i],"-use-ipv6"))) optForceIPv4 = false;
      else if(!(strncasecmp(argv[i],"-in=",4))) {
         maxIncoming = atol(&argv[i][4]);
         if(maxIncoming < 1) {
            maxIncoming = 1;
         }
         else if(maxIncoming > 65535) {
            maxIncoming = 65535;
         }
      }
      else if(!(strncasecmp(argv[i],"-out=",5))) {
         maxOutgoing = atol(&argv[i][5]);
         if(maxOutgoing < 1) {
            maxOutgoing = 1;
         }
         else if(maxOutgoing > 65535) {
            maxOutgoing = 65535;
         }
      }
      else if(!(strncasecmp(argv[i],"-local=",7))) {
         if(localAddresses < SCTP_MAXADDRESSES) {
            if(localAddressArray == NULL) {
               localAddressArray = SocketAddress::newAddressList(SCTP_MAXADDRESSES);
               if(localAddressArray == NULL) {
                  cerr << "ERROR: Out of memory!" << endl;
                  exit(1);
               }
            }
            localAddressArray[localAddresses] = SocketAddress::createSocketAddress(
                                                   SocketAddress::PF_HidePort,
                                                   &argv[i][7]);
            if(localAddressArray[localAddresses] == NULL) {
               cerr << "ERROR: Argument #" << i << " is an invalid address!" << endl;
               exit(1);
            }
            localAddresses++;
         }
         else {
            cerr << "ERROR: Too many local addresses!" << endl;
            exit(1);
         }
      }
      else if(!(strncasecmp(argv[i],"-unreliable=",12))) {
         unreliable = atol(&argv[i][12]);
      }
      else if(!(strcasecmp(argv[i],"-color"))) {
         ColorMode = true;
      }
      else if(!(strcasecmp(argv[i],"-nocolor"))) {
         ColorMode = false;
      }
      else if(!(strcasecmp(argv[i],"-control"))) {
         PrintControl = true;
      }
      else if(!(strcasecmp(argv[i],"-notif"))) {
         PrintNotifications = true;
      }
      else if(!(strcasecmp(argv[i],"-nocontrol"))) {
         PrintControl = false;
      }
      else if(!(strcasecmp(argv[i],"-nonotif"))) {
         PrintNotifications = false;
      }
      else {
         cerr << "Usage: " << argv[0] << " "
                 "{-force-ipv4|-use-ipv6} {-local=address1} ... {-local=addressN}  {-nocolor} {-color} {-control|-nocontrol} {-notif|-nonotif}"
              << endl;
         exit(1);
      }
   }
   if(optForceIPv4) {
      InternetAddress::UseIPv6 = false;
   }
   if(localAddressArray == NULL) {
      if(!Socket::getLocalAddressList(
            localAddressArray,
            localAddresses,
            Socket::GLAF_HideBroadcast|Socket::GLAF_HideMulticast|Socket::GLAF_HideAnycast)) {
         cerr << "ERROR: Cannot obtain local addresses!" << endl;
         exit(1);
      }
      if(localAddresses < 1) {
         cerr << "ERROR: No valid local addresses have been found?!" << endl
              << "       Check your network interface configuration!" << endl;
         exit(1);
      }
   }
   unreliable = min(maxOutgoing,unreliable);


   // ====== Print information ==============================================
   cout << "SCTP Multi Server - Copyright (C) 2001-2003 Thomas Dreibholz" << endl;
   cout << "------------------------------------------------------------" << endl;
   cout << "Version:           " << __DATE__ << ", " << __TIME__ << endl;
   localAddressArray[0]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_Legacy|SocketAddress::PF_HidePort);
   cout << "Local Addresses:   " << *(localAddressArray[0]) << endl;
   for(cardinal i = 1;i < localAddresses;i++) {
      localAddressArray[i]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_Legacy|SocketAddress::PF_HidePort);
      cout << "                   " << *(localAddressArray[i]) << endl;
   }
   cout << "Max Incoming:      " << maxIncoming << endl;
   cout << "Max Outgoing:      " << maxOutgoing << endl;
   cout << "Unreliable:        " << unreliable     << endl;
   cout << endl << endl;


   // ====== Execute main program ===========================================
#ifdef SERVER_ECHO
   MultiServer* echoServer = new MultiServer;
   if((echoServer == NULL) ||
      (echoServer->init(localAddressArray,localAddresses,
                        ECHO_PORT,MultiServer::MST_Echo,maxIncoming,maxOutgoing,unreliable) == false)) {
      cerr << "ERROR: Unable to initialize Echo server!" << endl;
      exit(1);
   }
#endif

#ifdef SERVER_DISCARD
   MultiServer* discardServer = new MultiServer;
   if((discardServer == NULL) ||
      (discardServer->init(localAddressArray,localAddresses,
                           DISCARD_PORT,MultiServer::MST_Discard,maxIncoming,1,unreliable) == false)) {
      cerr << "ERROR: Unable to initialize Discard server!" << endl;
      exit(1);
   }
#endif

#ifdef SERVER_DAYTIME
   MultiServer* daytimeServer = new MultiServer;
   if((daytimeServer == NULL) ||
      (daytimeServer->init(localAddressArray,localAddresses,
                           DAYTIME_PORT,MultiServer::MST_Daytime,1,1,unreliable) == false)) {
      cerr << "ERROR: Unable to initialize Daytime server!" << endl;
      exit(1);
   }
#endif

#ifdef SERVER_CHARGEN
   MultiServer* charGenServer = new MultiServer;
   if((charGenServer == NULL) ||
      (charGenServer->init(localAddressArray,localAddresses,
                          CHARGEN_PORT,MultiServer::MST_CharGen,1,1,unreliable) == false)) {
      cerr << "ERROR: Unable to initialize CharGen server!" << endl;
      exit(1);
   }
#endif

#ifdef SERVER_TFTP
   MultiServer* tftpServer = new MultiServer;
   if((tftpServer == NULL) ||
      (tftpServer->init(localAddressArray,localAddresses,
                          TFTP_PORT,MultiServer::MST_TFTP,1,1,unreliable) == false)) {
      cerr << "ERROR: Unable to initialize TFTP server!" << endl;
      exit(1);
   }
#endif

#ifdef SERVER_COMTEST
   MultiServer* testServer = new MultiServer;
   if((testServer == NULL) ||
      (testServer->init(localAddressArray,localAddresses,
                        COMTEST_PORT,MultiServer::MST_Test,maxIncoming,maxOutgoing,unreliable)) == false) {
      cerr << "ERROR: Unable to initialize test server!" << endl;
      exit(1);
   }
#endif


   // ====== Main loop ======================================================
   installBreakDetector();
   while(!breakDetected()) {
      Thread::delay(250000);
      ServerSet::garbageCollection();
   }

   ServerSet::shutdown();
#ifdef SERVER_COMTEST
   delete testServer;
#endif
#ifdef SERVER_TFTP
   delete tftpServer;
#endif
#ifdef SERVER_DAYTIME
   delete daytimeServer;
#endif
#ifdef SERVER_CHARGEN
   delete charGenServer;
#endif
#ifdef SERVER_DISCARD
   delete discardServer;
#endif
#ifdef SERVER_ECHO
   delete echoServer;
#endif
   SocketAddress::deleteAddressList(localAddressArray);

   cout << "\x1b[" << getANSIColor(0) << "mTerminated!" << endl;
   return 0;
}
