/*
 *  $Id: sctptftp.cc,v 1.3 2003/08/19 19:28:34 tuexen Exp $
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
 * Purpose: SCTP TFTP Client
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
#include "sctptftp.h"


#define SCTP_MAXADDRESSES 20


// ###### Print error #######################################################
void printError(const TFTPPacket& packet, const ssize_t size)
{
   cerr << "TFTP ERROR #" << (cardinal)packet.Code << ": ";
   if(size >= 3) {
      for(ssize_t i = 0;i < size - 2;i++) {
         cerr << packet.Data[i];
      }
   }
   cerr << endl;
}


// ###### Download ##########################################################
void tftpGet(const char* name, Socket* socket)
{
   TFTPPacket packet;
   packet.Type = TFTP_RRQ;
   packet.Code = 0;
   snprintf((char*)&packet.Data,sizeof(packet.Data),"%s",name);
   ssize_t result = socket->send((char*)&packet,2 + strlen(name));
   if(result >= 0) {
      result = socket->receive((char*)&packet,sizeof(packet));
      if(result > 0) {
         if(packet.Type == TFTP_ERR) {
            printError(packet,result);
         }
         else {
            FILE* out = fopen(name,"w");
            if(out == NULL) {
               cerr << "ERROR: Unable to create file \"" << name << "\"!" << endl;
               return;
            }
            cout << name << " -> ";
            cout.flush();
            while(result > 2) {
               if(packet.Type == TFTP_DAT) {
                  if(fwrite((char*)packet.Data,result - 2,1,out) != 1) {
                     cerr << "ERROR: Write error!" << endl;
                     break;
                  }
                  cout << ".";
                  cout.flush();
               }
               else if(packet.Type == TFTP_ERR) {
                  printError(packet,result);
                  break;
               }
               else {
                  cerr << "ERROR: tftpGet() - Unexpected packet type #" << packet.Type << "!" << endl;
                  break;
               }
               if(result < (ssize_t)sizeof(packet)) {
                  cout << ". OK!" << endl;
                  break;
               }
               result = socket->receive((char*)&packet,sizeof(packet));
            }
            fclose(out);
         }
      }
   }

   if(result < 0) {
      cerr << "ERROR: tftpGet() - I/O error!" << endl;
   }
}


// ###### Upload ############################################################
void tftpPut(const char* name, Socket* socket)
{
   FILE* in = fopen(name,"r");
   if(in == NULL) {
      cerr << "ERROR: Unable to open file \"" << name << "\"!" << endl;
      return;
   }

   ssize_t    result;
   TFTPPacket packet;
   packet.Type = TFTP_WRQ;
   packet.Code = 0;
   const char* index = rindex(name,'/');
   if(index != NULL) {
      snprintf((char*)&packet.Data,sizeof(packet.Data),"%s",(char*)&index[1]);
      result = socket->send((char*)&packet,2 + strlen((char*)&index[1]));
   }
   else {
      snprintf((char*)&packet.Data,sizeof(packet.Data),"%s",name);
      result = socket->send((char*)&packet,2 + strlen(name));
   }
   cout << "Local File:  " << name << endl
        << "Remote File: " << (char*)&packet.Data << endl
        << endl;

   if(result >= 0) {
      result = socket->receive((char*)&packet,sizeof(packet));
      if(result > 0) {
         if(packet.Type == TFTP_ERR) {
            printError(packet,result);
         }
         else if(packet.Type == TFTP_ACK) {
            cout << name << " -> ";
            cout.flush();

            packet.Type = TFTP_DAT;
            packet.Code = 0;
            while(!feof(in)) {
               ssize_t bytes = fread((char*)&packet.Data,1,sizeof(packet.Data),in);
               if(bytes >= 0) {
                  result = socket->send((char*)&packet,2 + bytes);
                  if(result < 0) {
                     break;
                  }
               }
               cout << ".";
               cout.flush();
               packet.Code++;
            }

            result = socket->receive((char*)&packet,sizeof(packet));
            if(result > 0) {
               if(packet.Type == TFTP_ERR) {
                  printError(packet,result);
               }
               else if(packet.Type == TFTP_ACK) {
                  cout << " OK!" << endl;
               }
               else {
                  cerr << "ERROR: tftpPut() - Unexpected packet type #" << (cardinal)packet.Type << "!" << endl;
               }
            }
         }
         else {
            cerr << "ERROR: tftpGet() - Received invalid packet type #" << (cardinal)packet.Type << "!" << endl;
         }
      }
   }

   if(result < 0) {
      cerr << "ERROR: tftpGet() - I/O error!" << endl;
   }

   fclose(in);
}



// ###### Main program ######################################################
int main(int argc, char** argv)
{
   bool            optForceIPv4       = false;
   cardinal        localAddresses     = 0;
   SocketAddress** localAddressArray  = SocketAddress::newAddressList(SCTP_MAXADDRESSES);
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
           << " [Remote address] [get|put] [Filename] {-force-ipv4|-use-ipv6} {-local=address1} ... {-local=addressN}"
           << endl;
      exit(1);
   }
   for(unsigned int i = 4;i < (cardinal)argc;i++) {
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
      else {
         cerr << "Usage: " << argv[0] << " "
              << "[get|put] [Filename] [Remote address] {-force-ipv4|-use-ipv6} {-local=address1} ... {-local=addressN}"
              << endl;
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
      localAddressArray[0] = SocketAddress::getLocalAddress(*remoteAddress);
      if(localAddressArray[0] == NULL) {
         cerr << "ERROR: Out of memory!" << endl;
         exit(1);
      }
      localAddresses = 1;
   }
   TFTPPacket packet;
   if(strlen(argv[3]) > sizeof(packet.Data) - 1) {
      cerr << "ERROR: File name too long!" << endl;
      exit(1);
   }


   // ====== Print information ==============================================
   cout << "SCTP TFTP - Copyright (C) 2001-2003 Thomas Dreibholz" << endl;
   cout << "----------------------------------------------------" << endl;
   cout << "Version:               " << __DATE__ << ", " << __TIME__ << endl;
   cout << "Local Addresses:       " << *(localAddressArray[0]) << endl;
   for(cardinal i = 1;i < localAddresses;i++) {
      cout << "                       " << *(localAddressArray[i]) << endl;
   }
   cout << "Remote Address:        " << *remoteAddress << endl;
   cout << endl;


   // ====== Create socket and connect ======================================
   Socket clientSocket(Socket::IP,Socket::Stream,Socket::SCTP);
   sctp_initmsg init;
   init.sinit_num_ostreams   = 1;
   init.sinit_max_instreams  = 1;
   init.sinit_max_attempts   = 0;
   init.sinit_max_init_timeo = 60;
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_INITMSG,(char*)&init,sizeof(init)) < 0) {
      cerr << "ERROR: Unable to set SCTP_INITMSG parameters!" << endl;
      exit(1);
   }
   if(clientSocket.bindx((const SocketAddress**)localAddressArray,
                         localAddresses,
                         SCTP_BINDX_ADD_ADDR) == false) {
      cerr << "ERROR: Unable to bind socket!" << endl;
      exit(1);
   }

   sctp_event_subscribe events;
   memset((char*)&events,0,sizeof(events));
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_EVENTS,&events,sizeof(events)) < 0) {
      cerr << "ERROR: SCTP_EVENTS failed!" << endl;
      exit(1);
   }

/*
   int on = 0;
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVDATAIOEVNT,&on,sizeof(on)) < 0) {
      cerr << "ERROR: SCTP_RECVDATAIOEVNT failed!" << endl;
      exit(1);
   }
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVASSOCEVNT,&on,sizeof(on)) < 0) {
      cerr << "ERROR: SCTP_RECVASSOCEVNT failed!" << endl;
      exit(1);
   }
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPADDREVNT,&on,sizeof(on)) < 0) {
      cerr << "ERROR: SCTP_RECVPADDREVNT failed!" << endl;
      exit(1);
   }
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSENDFAILEVNT,&on,sizeof(on)) < 0) {
      cerr << "ERROR: SCTP_RECVSENDFAILEVNT failed!" << endl;
      exit(1);
   }
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPEERERR,&on,sizeof(on)) < 0) {
      cerr << "ERROR: SCTP_RECVPEERERR failed!" << endl;
      exit(1);
   }
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSHUTDOWNEVNT,&on,sizeof(on)) < 0) {
      cerr << "ERROR: SCTP_RECVSHUTDOWNEVNT failed!" << endl;
      exit(1);
   }
*/

   cout << "Connecting... ";
   cout.flush();
   if(clientSocket.connect(*remoteAddress) == false) {
      cout << "failed!" << endl;
      cerr << "ERROR: Unable to connect to remote address!" << endl;
      exit(1);
   }
   cout << "done." << endl << endl;


   if(!(strcasecmp(argv[2],"PUT"))) {
      tftpPut(argv[3],&clientSocket);
   }
   else {
      tftpGet(argv[3],&clientSocket);
   }


   SocketAddress::deleteAddressList(localAddressArray);
   return 0;
}
