/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2021 by Thomas Dreibholz
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
 * Purpose: SCTP Terminal
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


#include <signal.h>



#define NOTIFICATION_COLOR 10
#define CONTROL_COLOR      12
#define ERROR_COLOR        9
#define INFO_COLOR         11

#define SCTP_MAXADDRESSES  20



class EchoThread : public Thread
{
   public:
   EchoThread(Socket*        socket,
              const cardinal incoming);
   ~EchoThread();

   private:
   void run();


   Socket*  EchoSocket;
   cardinal Incoming;
};



// ###### Constructor #######################################################
EchoThread::EchoThread(Socket*        socket,
                       const cardinal incoming)
   : Thread("EchoThread")
{
   EchoSocket = socket;
   Incoming   = incoming;
   start();
}


// ###### Destructor ########################################################
EchoThread::~EchoThread()
{
}


// ###### Thread main loop ##################################################
void EchoThread::run()
{
   char buffer[16384];
   for(;;) {
      SocketMessage<sizeof(sctp_sndrcvinfo)> message;
      message.setBuffer((char*)&buffer,sizeof(buffer));

      // ====== Receive string from association =============================
      const ssize_t dataLength = EchoSocket->receiveMsg(&message.Header,0);
      if(dataLength <= 0) {
         sendBreak(true);
         return;
      }


      // ====== Get control data ============================================
      printControl(&message.Header);
      unsigned short streamID = 0;
      unsigned int   protoID  = 0;
      cmsghdr* cmsg = message.getFirstHeader();
      if(cmsg != NULL) {
         while(cmsg != NULL) {
            if((cmsg->cmsg_level == IPPROTO_SCTP) &&
               (cmsg->cmsg_type  == SCTP_SNDRCV)) {
               sctp_sndrcvinfo* info = (sctp_sndrcvinfo*)CData(cmsg);
               streamID = info->sinfo_stream;
               protoID  = ntohl(info->sinfo_ppid);
            }
            cmsg = message.getNextHeader(cmsg);
         }
      }


      // ====== Print data ==================================================
      if(message.Header.msg_flags & MSG_NOTIFICATION) {
         sctp_notification* notification = (sctp_notification*)&buffer;
         printNotification(notification);
      }
      else {
         if(dataLength > 0) {
            if(ColorMode) {
               std::cout << "\x1b[" << getANSIColor(streamID + 1) << "m";
            }
            if(Incoming > 1) {
               char str[128];
               if(protoID == 0) {
                  snprintf((char*)&str,sizeof(str),"#%d: ",streamID);
               }
               else {
                  snprintf((char*)&str,sizeof(str),"#%d/$%08x: ",
                           streamID,protoID);
               }
               std::cout << str;
            }

            int i;
            buffer[dataLength] = 0x00;
            for(i = dataLength - 1;i >= 0;i--) {
               if(buffer[i] < ' ') {
                  buffer[i] = 0x00;
               }
               else {
                  break;
               }
            }
            for(   ;i >= 0;i--) {
               if(buffer[i] < ' ') {
                  buffer[i] = '.';
               }
            }

            std::cout << buffer << std::endl;
            if(ColorMode) {
               std::cout << "\x1b[" << getANSIColor(0) << "m";
            }
            std::cout.flush();
         }
      }
   }
}




class CopyThread : public Thread
{
   public:
   CopyThread(Socket*        socket,
              const cardinal outgoing,
              const cardinal unreliable);
   ~CopyThread();

   private:
   void run();

   Socket*  CopySocket;
   cardinal Outgoing;
   cardinal Unreliable;
};


// ###### Constructor #######################################################
CopyThread::CopyThread(Socket*        socket,
                       const cardinal outgoing,
                       const cardinal unreliable)
   : Thread("CopyThread", Thread::TF_CancelAsynchronous)
{
   CopySocket = socket;
   Outgoing   = outgoing;
   Unreliable = unreliable;
   start();
}


// ###### Destructor ########################################################
CopyThread::~CopyThread()
{
}


// ###### Thread main loop ##################################################
void CopyThread::run()
{
   unsigned int stream = 0;
   unsigned int ppid   = 0;
   for(;;) {
      // ====== Read data from cin ==========================================
      Thread::testCancel();
      char str[8192];

      // For some reason, cin.getline() causes problems with Ctrl-C
      // handling. Therefore, we use fgets() and feof() instead.
      if(fgets((char*)&str, sizeof(str), stdin) == NULL) {
         CopySocket->shutdown(2);
         sendBreak(true);
         return;
      }
      if(feof(stdin)) {
         CopySocket->shutdown(2);
         sendBreak(true);
         return;
      }

      // ====== Check for new stream ID and protocol ID =====================
      unsigned int position = 0;
      bool         update   = true;
      if(sscanf(str,"#%d/$%x:%n",&stream,&ppid,&position) < 2) {
         if(sscanf(str,"#%d/%x:%n",&stream,&ppid,&position) < 2) {
            if(sscanf(str,"#%d:%n",&stream,&position) < 1) {
               position = 0;
               update   = false;
            }
         }
      }


      // ====== Check parameters ============================================
      char* toSend = (char*)&str[position];
      if(stream >= Outgoing) {
         stream = 0;
         if(ColorMode) {
            std::cerr << "\x1b[" << getANSIColor(ERROR_COLOR) << "m\x07";
         }
         std::cerr << "*** Bad stream setting! Using #0. ***" << std::endl;
         if(ColorMode) {
            std::cerr << "\x1b[" << getANSIColor(0) << "m";
         }
         std::cerr.flush();
      }
      if(update == true) {
         if(ColorMode) {
            std::cout << "\x1b[" << getANSIColor(INFO_COLOR) << "m";
         }
         char str2[128];
         snprintf((char*)&str2,sizeof(str2),"Via stream #%d, protocol $%08x...",
                  stream,ppid);
         std::cout << str2;
         if(ColorMode) {
            std::cout << "\x1b[" << getANSIColor(0) << "m";
         }
         std::cout.flush();
      }


      // ====== Send message ================================================
      cardinal length = strlen(toSend);
      if(length > 0) {
         toSend[length]   = '\n';
         toSend[length++] = 0x00;

         SocketMessage<sizeof(sctp_sndrcvinfo)> message;
         message.setBuffer(toSend,length);
         sctp_sndrcvinfo* info = (sctp_sndrcvinfo*)message.addHeader(sizeof(sctp_sndrcvinfo),IPPROTO_SCTP,SCTP_SNDRCV);
         info->sinfo_assoc_id = 0;
         info->sinfo_stream   = (unsigned short)stream;
         info->sinfo_flags    = MSG_PR_SCTP_TTL;
         info->sinfo_ppid     = htonl(ppid);
         if((Unreliable > 0) && (info->sinfo_stream <= Unreliable - 1)) {
            info->sinfo_flags      = MSG_PR_SCTP_TTL;
            info->sinfo_timetolive = 0;
         }
         else {
            info->sinfo_flags      = MSG_PR_SCTP_TTL;
            info->sinfo_timetolive = 0;
         }
         const ssize_t result = CopySocket->sendMsg(&message.Header,0);
         if(result < 0) {
            if(ColorMode) {
               std::cerr << "\x1b[" << getANSIColor(ERROR_COLOR) << "m\x07";
            }
            std::cerr << "*** sendMsg() error #" << result << "! ***" << std::endl;
            if(ColorMode) {
               std::cerr << "\x1b[" << getANSIColor(0) << "m";
            }
         }
      }
   }
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   bool            optForceIPv4       = false;
   cardinal        instreams          = 128;
   cardinal        outstreams         = 128;
   cardinal        localAddresses     = 0;
   cardinal        remoteAddresses    = 0;
   cardinal        unreliable         = 0;
   SocketAddress** localAddressArray  = SocketAddress::newAddressList(SCTP_MAXADDRESSES);
   SocketAddress** remoteAddressArray = SocketAddress::newAddressList(SCTP_MAXADDRESSES);
   if((localAddressArray == NULL) || (remoteAddressArray == NULL)) {
      std::cerr << "ERROR: Out of memory!" << std::endl;
      exit(1);
   }
   PrintControl       = false;
   PrintNotifications = false;
   if(!sctp_isavailable()) {
#ifdef HAVE_KERNEL_SCTP
      std::cerr << "ERROR: Kernel-based SCTP is not available!" << std::endl;
#else
     if(getuid() != 0) {
        std::cerr << "ERROR: You need root permissions to use the SCTP Library!" << std::endl;
     }
     else {
        std::cerr << "ERROR: SCTP is not available!" << std::endl;
     }
#endif
      exit(1);
   }


   // ====== Get arguments ==================================================
   if(argc < 2) {
      std::cerr << "Usage: " << argv[0] << " "
           << "[Remote address 1] {{Remote address 2} ...} {-force-ipv4|-use-ipv6} {-local=address1} ... {-local=addressN} {-in=instreams} {-out=outstreams} {-nocolor} {-color} {-control|-nocontrol} {-notif|-nonotif}"
           << std::endl;
      exit(1);
   }
   for(unsigned int i = 1;i < (cardinal)argc;i++) {
      if(!(strcasecmp(argv[i],"-force-ipv4"))) {
         optForceIPv4 = true;
      }
      else if(!(strcasecmp(argv[i],"-use-ipv6"))) {
         optForceIPv4 = false;
      }
      else if(!(strncasecmp(argv[i],"-in=",4))) {
         instreams = atol(&argv[i][4]);
         if(instreams < 1) {
            instreams = 1;
         }
         else if(instreams > 65535) {
            instreams = 65535;
         }
      }
      else if(!(strncasecmp(argv[i],"-out=",5))) {
         outstreams = atol(&argv[i][5]);
         if(outstreams < 1) {
            outstreams = 1;
         }
         else if(outstreams > 65535) {
            outstreams = 65535;
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
      else if(!(strncasecmp(argv[i],"-local=",7))) {
         if(localAddresses < SCTP_MAXADDRESSES) {
            localAddressArray[localAddresses] =
               SocketAddress::createSocketAddress(0,
                                                  &argv[i][7]);
            if(localAddressArray[localAddresses] == NULL) {
               std::cerr << "ERROR: Argument \"" << argv[i] << "\" specifies an invalid local address!" << std::endl;
               exit(1);
            }
            localAddresses++;
         }
         else {
            std::cerr << "ERROR: Too many local addresses!" << std::endl;
            exit(1);
         }
      }
      else {
         if(remoteAddresses < SCTP_MAXADDRESSES) {
            remoteAddressArray[remoteAddresses] =
               SocketAddress::createSocketAddress(0, argv[i]);
            if(remoteAddressArray[remoteAddresses] == NULL) {
               std::cerr << "ERROR: Argument \"" << argv[i] << "\" is an invalid remote address!" << std::endl;
               exit(1);
            }
            remoteAddresses++;
         }
         else {
            std::cerr << "ERROR: Too many remote addresses!" << std::endl;
            exit(1);
         }
      }
   }
   if(optForceIPv4) {
      InternetAddress::UseIPv6 = false;
   }
   if(remoteAddresses < 1) {
      std::cerr << "ERROR: No remote addresses given!" << std::endl;
      exit(1);
   }
   if(remoteAddressArray[0]->getPort() == 0) {
      std::cerr << "ERROR: No remote port number is given with first remote address!" << std::endl;
      exit(1);
   }
   if(localAddresses < 1) {
/*
      localAddressArray[0] = new InternetAddress(0);
      if(localAddressArray[0] == NULL) {
         std::cerr << "ERROR: Out of memory!" << std::endl;
         exit(1);
      }
      localAddresses = 1;
*/
      SocketAddress::deleteAddressList(localAddressArray);
      if(!Socket::getLocalAddressList(
            localAddressArray,
            localAddresses,
            Socket::GLAF_HideBroadcast|Socket::GLAF_HideMulticast|Socket::GLAF_HideAnycast)) {
         std::cerr << "ERROR: Cannot obtain local addresses!" << std::endl;
         exit(1);
      }
      if(localAddresses < 1) {
         std::cerr << "ERROR: No valid local addresses have been found?!" << std::endl
                   << "       Check your network interface configuration!" << std::endl;
         exit(1);
      }
   }
   for(cardinal i = 1;i < localAddresses;i++) {
      localAddressArray[i]->setPort(localAddressArray[0]->getPort());
   }
   for(cardinal i = 1;i < remoteAddresses;i++) {
      remoteAddressArray[i]->setPort(remoteAddressArray[0]->getPort());
   }
   unreliable = std::min(outstreams,unreliable);


   // ====== Print information ==============================================
   std::cout << "SCTP Terminal - Copyright (C) 2001-2019 Thomas Dreibholz" << std::endl;
   std::cout << "--------------------------------------------------------" << std::endl;
   localAddressArray[0]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_Legacy|SocketAddress::PF_HidePort);
   std::cout << "Local Addresses:       " << *(localAddressArray[0]) << std::endl;
   for(cardinal i = 1;i < localAddresses;i++) {
      localAddressArray[i]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_Legacy|SocketAddress::PF_HidePort);
      std::cout << "                       " << *(localAddressArray[i]) << std::endl;
   }
   std::cout << "Remote Addresses:      " << *(remoteAddressArray[0]) << std::endl;
   remoteAddressArray[0]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_Legacy|SocketAddress::PF_HidePort);
   for(cardinal i = 1;i < remoteAddresses;i++) {
      remoteAddressArray[i]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_Legacy|SocketAddress::PF_HidePort);
      std::cout << "                       " << *(remoteAddressArray[i]) << std::endl;
   }
   std::cout << "Outgoing Streams:      " << outstreams << std::endl;
   std::cout << "Max. Incoming Streams: " << instreams  << std::endl;
   std::cout << "Unreliable:            " << unreliable << std::endl;
   std::cout << std::endl << std::endl;


   // ====== Create socket and connect ======================================
   Socket clientSocket(Socket::IP,Socket::Stream,Socket::SCTP);
   if(clientSocket.bindx((const SocketAddress**)localAddressArray,
                         localAddresses,
                         SCTP_BINDX_ADD_ADDR) == false) {
      std::cerr << "ERROR: Unable to bind socket!" << std::endl;
      exit(1);
   }

   sctp_initmsg init;
   init.sinit_num_ostreams   = outstreams;
   init.sinit_max_instreams  = instreams;
   init.sinit_max_attempts   = 0;
   init.sinit_max_init_timeo = 60;
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_INITMSG,(char*)&init,sizeof(init)) < 0) {
      std::cerr << "ERROR: Unable to set SCTP_INITMSG parameters!" << std::endl;
      exit(1);
   }

   sctp_event_subscribe events;
   memset((char*)&events,1,sizeof(events));
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_EVENTS,&events,sizeof(events)) < 0) {
      std::cerr << "ERROR: SCTP_EVENTS failed!" << std::endl;
      exit(1);
   }

   std::cout << "Connecting... ";
   std::cout.flush();
   if(clientSocket.connectx((const SocketAddress**)remoteAddressArray,
                            remoteAddresses) == false) {
      std::cout << "failed!" << std::endl;
      std::cerr << "ERROR: Unable to connect to remote address(es)!" << std::endl;
      exit(1);
   }
   std::cout << "done. Use Ctrl-D to end transmission." << std::endl << std::endl;


   // ====== Start threads ==================================================
   installBreakDetector();
   EchoThread echo(&clientSocket,instreams);
   CopyThread copy(&clientSocket,outstreams,unreliable);


   // ====== Main loop ======================================================
   while(!breakDetected()) {
      Thread::delay(100000);
   }


   // ====== Stop threads ===================================================
   copy.stop();
   echo.stop();
   SocketAddress::deleteAddressList(localAddressArray);
   SocketAddress::deleteAddressList(remoteAddressArray);
   std::cout << "\x1b[" << getANSIColor(0) << "mTerminated!" << std::endl;
   return 0;
}
