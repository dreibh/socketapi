/*
 *  $Id: sctpterminal.cc,v 1.3 2003/06/02 17:29:42 dreibh Exp $
 *
 * SCTP implementation according to RFC 2960.
 * Copyright (C) 1999-2001 by Thomas Dreibholz
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * There are two mailinglists available at www.sctp.de which should be used for
 * any discussion related to this implementation.
 *
 * Contact: discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
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
   cardinal Unreliable;
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
      SocketMessage<CSpace(sizeof(sctp_sndrcvinfo))> message;
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
               protoID  = info->sinfo_ppid;
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
               cout << "\x1b[" << getANSIColor(streamID + 1) << "m";
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
               cout << str;
            }
            buffer[dataLength] = 0x00;
            cout << buffer;
            if(ColorMode) {
               cout << "\x1b[" << getANSIColor(0) << "m";
            }
            cout.flush();
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
   : Thread("CopyThread",Thread::TF_CancelAsynchronous)
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
      cin.getline((char*)&str,sizeof(str) - 1);
      if(cin.eof()) {
         CopySocket->shutdown(2);
         sendBreak(true);
         return;
      }
      strcat((char*)&str,"\n");

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
            cerr << "\x1b[" << getANSIColor(ERROR_COLOR) << "m\x07";
         }
         cerr << "*** Bad stream setting! Using #0. ***" << endl;
         if(ColorMode) {
            cerr << "\x1b[" << getANSIColor(0) << "m";
         }
         cerr.flush();
      }
      if(update == true) {
         if(ColorMode) {
            cout << "\x1b[" << getANSIColor(INFO_COLOR) << "m";
         }
         char str2[128];
         snprintf((char*)&str2,sizeof(str2),"Via stream #%d, protocol $%08x...",
                  stream,ppid);
         cout << str2 << endl;
         if(ColorMode) {
            cout << "\x1b[" << getANSIColor(0) << "m";
         }
         cout.flush();
      }


      // ====== Send message ================================================
      cardinal length = strlen(toSend);
      if(length > 0) {
         SocketMessage<CSpace(sizeof(sctp_sndrcvinfo))> message;
         message.setBuffer(toSend,length);
         sctp_sndrcvinfo* info = (sctp_sndrcvinfo*)message.addHeader(sizeof(sctp_sndrcvinfo),IPPROTO_SCTP,SCTP_SNDRCV);
         info->sinfo_assoc_id = 0;
         info->sinfo_stream   = (unsigned short)stream;
         info->sinfo_flags    = 0;
         info->sinfo_ppid     = ppid;
         if((Unreliable > 0) && (info->sinfo_stream <= Unreliable - 1)) {
            info->sinfo_timetolive = 0;
         }
         else {
            info->sinfo_timetolive = (unsigned int)-1;
         }
         const ssize_t result = CopySocket->sendMsg(&message.Header,0);
         if(result < 0) {
            if(ColorMode) {
               cerr << "\x1b[" << getANSIColor(ERROR_COLOR) << "m\x07";
            }
            cerr << "*** sendMsg() error #" << result << "! ***" << endl;
            if(ColorMode) {
               cerr << "\x1b[" << getANSIColor(0) << "m";
            }
         }
      }
   }
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   bool            optForceIPv4       = false;
   cardinal        instreams          = 1;
   cardinal        outstreams         = 1;
   cardinal        localAddresses     = 0;
   cardinal        remoteAddresses    = 0;
   cardinal        unreliable         = 0;
   SocketAddress** localAddressArray  = SocketAddress::newAddressList(SCTP_MAXADDRESSES);
   SocketAddress** remoteAddressArray = SocketAddress::newAddressList(SCTP_MAXADDRESSES);
   if((localAddressArray == NULL) || (remoteAddressArray == NULL)) {
      cerr << "ERROR: Out of memory!" << endl;
      exit(1);
   }
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
   if(argc < 2) {
      cerr << "Usage: " << argv[0] << " "
           << "[Remote address 1] {{Remote address 2} ...} {-force-ipv4|-use-ipv6} {-local=address1} ... {-local=addressN} {-in=instreams} {-out=outstreams} {-nocolor} {-color} {-control|-nocontrol} {-notif|-nonotif}"
           << endl;
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
               cerr << "ERROR: Argument \"" << argv[i] << "\" specifies an invalid local address!" << endl;
               exit(1);
            }
            localAddresses++;
         }
         else {
            cerr << "ERROR: Too many local addresses!" << endl;
            exit(1);
         }
      }
      else {
         if(remoteAddresses < SCTP_MAXADDRESSES) {
            remoteAddressArray[remoteAddresses] =
               SocketAddress::createSocketAddress(0, argv[i]);
            if(remoteAddressArray[remoteAddresses] == NULL) {
               cerr << "ERROR: Argument \"" << argv[i] << "\" is an invalid remote address!" << endl;
               exit(1);
            }
            remoteAddresses++;
         }
         else {
            cerr << "ERROR: Too many remote addresses!" << endl;
            exit(1);
         }
      }
   }
   if(optForceIPv4) {
      InternetAddress::UseIPv6 = false;
   }
   if(remoteAddresses < 1) {
      cerr << "ERROR: No remote addresses given!" << endl;
      exit(1);
   }
   if(remoteAddressArray[0]->getPort() == 0) {
      cerr << "ERROR: No remote port number is given with first remote address!" << endl;
      exit(1);
   }
   if(localAddresses < 1) {
/*
      localAddressArray[0] = new InternetAddress(0);
      if(localAddressArray[0] == NULL) {
         cerr << "ERROR: Out of memory!" << endl;
         exit(1);
      }
      localAddresses = 1;
*/
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
   for(cardinal i = 1;i < localAddresses;i++) {
      localAddressArray[i]->setPort(localAddressArray[0]->getPort());
   }
   for(cardinal i = 1;i < remoteAddresses;i++) {
      remoteAddressArray[i]->setPort(remoteAddressArray[0]->getPort());
   }
   unreliable = min(outstreams,unreliable);


   // ====== Print information ==============================================
   cout << "SCTP Terminal - Copyright (C) 2001-2003 Thomas Dreibholz" << endl;
   cout << "--------------------------------------------------------" << endl;
   cout << "Version:               " << __DATE__ << ", " << __TIME__ << endl;
   localAddressArray[0]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_HidePort);
   cout << "Local Addresses:       " << *(localAddressArray[0]) << endl;
   for(cardinal i = 1;i < localAddresses;i++) {
      localAddressArray[i]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_HidePort);
      cout << "                       " << *(localAddressArray[i]) << endl;
   }
   cout << "Remote Addresses:      " << *(remoteAddressArray[0]) << endl;
   for(cardinal i = 1;i < remoteAddresses;i++) {
      remoteAddressArray[i]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_HidePort);
      cout << "                       " << *(remoteAddressArray[i]) << endl;
   }
   cout << "Outgoing Streams:      " << outstreams     << endl;
   cout << "Max. Incoming Streams: " << instreams      << endl;
   cout << "Unreliable:            " << unreliable     << endl;
   cout << endl << endl;


   // ====== Create socket and connect ======================================
   Socket clientSocket(Socket::IP,Socket::Stream,Socket::SCTP);
   if(clientSocket.bindx((const SocketAddress**)localAddressArray,
                         localAddresses,
                         SCTP_BINDX_ADD_ADDR) == false) {
      cerr << "ERROR: Unable to bind socket!" << endl;
      exit(1);
   }

   sctp_initmsg init;
   init.sinit_num_ostreams   = outstreams;
   init.sinit_max_instreams  = instreams;
   init.sinit_max_attempts   = 0;
   init.sinit_max_init_timeo = 60;
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_INITMSG,(char*)&init,sizeof(init)) < 0) {
      cerr << "ERROR: Unable to set SCTP_INITMSG parameters!" << endl;
      exit(1);
   }

   sctp_event_subscribe events;
   memset((char*)&events,1,sizeof(events));
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_SET_EVENTS,&events,sizeof(events)) < 0) {
      cerr << "ERROR: SCTP_SET_EVENTS failed!" << endl;
      exit(1);
   }

   cout << "Connecting... ";
   cout.flush();
   if(clientSocket.connectx((const SocketAddress**)remoteAddressArray,
                            remoteAddresses) == false) {
      cout << "failed!" << endl;
      cerr << "ERROR: Unable to connect to remote address(es)!" << endl;
      exit(1);
   }
   cout << "done. Use Ctrl-D to end transmission." << endl << endl;


   // ====== Start threads ==================================================
   installBreakDetector();
   EchoThread echo(&clientSocket,instreams);
   CopyThread copy(&clientSocket,outstreams,unreliable);


   // ====== Main loop ======================================================
   while(!breakDetected()) {
      Thread::delay(100000);
   }


   // ====== Stop threads ===================================================
   Thread::delay(500000);
   copy.stop();
   echo.stop();
   SocketAddress::deleteAddressList(localAddressArray);
   SocketAddress::deleteAddressList(remoteAddressArray);
   cout << "\x1b[" << getANSIColor(0) << "mTerminated!" << endl;
   return 0;
}
