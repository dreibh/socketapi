/*
 *  $Id: sctpportscanner.cc,v 1.3 2003/08/19 19:28:34 tuexen Exp $
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
 * Purpose: SCTP Port Scanner
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


#include <signal.h>


#define SCTP_MAXADDRESSES 20



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   bool            optForceIPv4      = false;
   cardinal        instreams         = 1;
   cardinal        outstreams        = 1;
   cardinal        localAddresses    = 0;
   cardinal        startPort         = 1;
   cardinal        endPort           = 20;
   cardinal        simultaneous      = 128;
   card64          connectTimeout    = 10000000;
   SocketAddress** localAddressArray = SocketAddress::newAddressList(SCTP_MAXADDRESSES);
   if(localAddressArray == NULL) {
      cerr << "ERROR: Out of memory!" << endl;
      exit(1);
   }
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
           << "[Remote address] {-force-ipv4|-use-ipv6} {-local=address1} ... {-local=addressN} {-in=instreams} {-out=outstreams} {-start=first port} {-end=last port} {-simultaneous=connections} {-timeout=seconds}"
           << endl;
      exit(1);
   }
   for(unsigned int i = 2;i < (cardinal)argc;i++) {
      if(!(strcasecmp(argv[i],"-force-ipv4")))    optForceIPv4 = true;
      else if(!(strcasecmp(argv[i],"-use-ipv6"))) optForceIPv4 = false;
      else if(!(strncasecmp(argv[i],"-local=",7))) {
         if(localAddresses < SCTP_MAXADDRESSES) {
            localAddressArray[localAddresses] =
               SocketAddress::createSocketAddress(SocketAddress::PF_HidePort,
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
      else if(!(strncasecmp(argv[i],"-start=",7))) {
         startPort = atol(&argv[i][7]);
      }
      else if(!(strncasecmp(argv[i],"-end=",5))) {
         endPort = atol(&argv[i][5]);
      }
      else if(!(strncasecmp(argv[i],"-timeout=",9))) {
         connectTimeout = 1000000 * (card64)atol(&argv[i][9]);
      }
      else if(!(strncasecmp(argv[i],"-simultaneous=",14))) {
         simultaneous = atol(&argv[i][14]);
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
      else {
         cerr << "ERROR: Bad parameter " << argv[i] << "!" << endl;
         exit(1);
      }
   }
   if(optForceIPv4) {
      InternetAddress::UseIPv6 = false;
   }
   SocketAddress* remoteAddress = SocketAddress::createSocketAddress(0,argv[1]);
   if(remoteAddress == NULL) {
      cerr << "ERROR: Bad remote address! Use <address>:<port> format." << endl;
      exit(1);
   }
   if(localAddresses < 1) {
      SocketAddress::deleteAddressList(localAddressArray);
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
   if(startPort < 1) {
      startPort = 1;
   }
   if(startPort > 65535) {
      startPort = 65535;
   }
   if(endPort < 1) {
      endPort = 1;
   }
   if(endPort > 65535) {
      endPort = 65535;
   }
   if(startPort > endPort) {
      startPort = endPort;
   }
   if(connectTimeout > 300000000) {
      connectTimeout = 300000000;
   }
   if(simultaneous < 1) {
      simultaneous = 1;
   }
   if(simultaneous > 512) {
      simultaneous = 512;
   }


   // ====== Print information ==============================================
   cout << "SCTP Port Scanner - Copyright (C) 2001-2002 Thomas Dreibholz" << endl;
   cout << "------------------------------------------------------------" << endl;
   cout << "Version:               " << __DATE__ << ", " << __TIME__ << endl;
   localAddressArray[0]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_HidePort);
   cout << "Local Addresses:       " << *(localAddressArray[0]) << endl;
   for(cardinal i = 1;i < localAddresses;i++) {
      localAddressArray[i]->setPrintFormat(SocketAddress::PF_Address|SocketAddress::PF_HidePort);
      cout << "                       " << *(localAddressArray[i]) << endl;
   }
   remoteAddress->setPrintFormat(SocketAddress::PF_Address);
   cout << "Remote Address:        " << *remoteAddress << endl;
   cout << "Outgoing Streams:      " << outstreams     << endl;
   cout << "Max. Incoming Streams: " << instreams      << endl;
   cout << "Simultaneous Connects: " << simultaneous << endl;
   cout << "Connect Timeout:       " << connectTimeout << " [µs]" << endl;
   cout << "Port Range:            " << startPort << " - " << endPort << endl;
   cout << endl << endl;


   installBreakDetector();


   // ====== Create socket and connect ======================================
   cardinal nextPort = startPort;
   Socket* clientSocket[simultaneous];
   card64  timeout[simultaneous];
   card16  port[simultaneous];
   for(cardinal i = 0;i < simultaneous;i++) {
      clientSocket[i] = NULL;
      port[i]         = 0;
      timeout[i]      = 0;
   }
   bool success[endPort + 1];
   for(cardinal i = 0;i <= endPort;i++) {
      success[i] = false;
   }
   cardinal running = 0;
   do {
      for(cardinal i = 0;(i < simultaneous) && (nextPort <= endPort);i++) {
         if(clientSocket[i] == NULL) {
            clientSocket[i] = new Socket(Socket::IP,Socket::Stream,Socket::SCTP);
            if(clientSocket[i] == NULL) {
               cerr << "ERROR: Out of memory!" << endl;
               exit(1);
            }
            clientSocket[i]->setBlockingMode(false);
            sctp_initmsg init;
            init.sinit_num_ostreams   = outstreams;
            init.sinit_max_instreams  = instreams;
            init.sinit_max_attempts   = 0;
            init.sinit_max_init_timeo = 60;
            if(clientSocket[i]->setSocketOption(IPPROTO_SCTP,SCTP_INITMSG,(char*)&init,sizeof(init)) < 0) {
               cerr << "ERROR: Unable to set SCTP_INITMSG parameters!" << endl;
               exit(1);
            }

            /*
            for(cardinal j = 0;j < localAddresses;j++) {
               localAddressArray[j]->setPort(0);
            }
            */
            if(clientSocket[i]->bindx((const SocketAddress**)localAddressArray,
                                         localAddresses,
                                         SCTP_BINDX_ADD_ADDR) == false) {
               cerr << "ERROR: Unable to bind socket!" << endl;
               exit(1);
            }

            sctp_event_subscribe events;
            memset((char*)&events,1,sizeof(events));
            if(clientSocket[i]->setSocketOption(IPPROTO_SCTP,SCTP_EVENTS,&events,sizeof(events)) < 0) {
               cerr << "ERROR: SCTP_EVENTS failed!" << endl;
               exit(1);
            }

            port[i]    = nextPort;
            timeout[i] = connectTimeout + getMicroTime();
            remoteAddress->setPort(port[i]);
            cout << "Trying " << *remoteAddress << "..." << endl;
            if((clientSocket[i]->connect(*remoteAddress) == false) && (clientSocket[i]->getLastError() != EINPROGRESS)) {
               cout << "failed!" << endl;
               cerr << "ERROR: Unable to connect to remote address!" << endl;
               exit(1);
            }
            nextPort++;
            running++;
         }
      }


      int n = 0;
      fd_set readfds;
      FD_ZERO(&readfds);
      for(cardinal i = 0;i < simultaneous;i++) {
         if(clientSocket[i] != NULL) {
            const int fd = clientSocket[i]->getSystemSocketDescriptor();
            FD_SET(fd,&readfds);
            n = max(n,fd);
         }
      }
      timeval to;
      to.tv_sec  = 0;
      to.tv_usec = 250000;
      int result = ext_select(n + 1, &readfds, &readfds, NULL, &to);
      if(result > 0) {
         for(cardinal i = 0;i < simultaneous;i++) {
            if(clientSocket[i] != NULL) {
               const int fd = clientSocket[i]->getSystemSocketDescriptor();
               if(FD_ISSET(fd,&readfds)) {
                  result = ext_write(fd,"Test\n",5);
                  if(result > 0) {
                     cerr << "Port #" << port[i] << " is active!" << endl;
                     success[port[i]] = true;
                  }
                  else {
                     cerr << "Port #" << port[i] << " is unused." << endl;
                  }
                  delete clientSocket[i];
                  clientSocket[i] = NULL;
                  running--;
               }
            }
         }
      }
      else {
         for(cardinal i = 0;i < simultaneous;i++) {
            if((clientSocket[i] != NULL) && (timeout[i] <= getMicroTime())) {
               cerr << "Port #" << port[i] << " is inactive, timeout reached." << endl;
               delete clientSocket[i];
               clientSocket[i] = NULL;
               running--;
            }
         }
      }
   } while((!breakDetected()) && ((nextPort <= endPort) || (running > 0)));


   // ====== Print summary ==================================================
   cout << endl << "Summary of active ports:" << endl;
   for(cardinal i = 1;i <= endPort;i++) {
      if(success[i]) {
         cout << "Port #" << i << endl;
      }
   }
   cout << endl;


   // ====== Clean up =======================================================
   for(cardinal i = 0;i < simultaneous;i++) {
      if(clientSocket[i] != NULL) {
         delete clientSocket[i];
         clientSocket[i] = NULL;
      }
   }
   Thread::delay(500000);
   SocketAddress::deleteAddressList(localAddressArray);
   delete remoteAddress;
   cout << "\x1b[" << getANSIColor(0) << "mTerminated!" << endl;
   return 0;
}
