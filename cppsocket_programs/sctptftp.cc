/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 2005-2008 by Thomas Dreibholz
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
   std::cerr << "TFTP ERROR #" << (cardinal)packet.Code << ": ";
   if(size >= 3) {
      for(ssize_t i = 0;i < size - 2;i++) {
         std::cerr << packet.Data[i];
      }
   }
   std::cerr << std::endl;
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
               std::cerr << "ERROR: Unable to create file \"" << name << "\"!" << std::endl;
               return;
            }
            std::cout << name << " -> ";
            std::cout.flush();
            while(result > 2) {
               if(packet.Type == TFTP_DAT) {
                  if(fwrite((char*)packet.Data,result - 2,1,out) != 1) {
                     std::cerr << "ERROR: Write error!" << std::endl;
                     break;
                  }
                  std::cout << ".";
                  std::cout.flush();
               }
               else if(packet.Type == TFTP_ERR) {
                  printError(packet,result);
                  break;
               }
               else {
                  std::cerr << "ERROR: tftpGet() - Unexpected packet type #" << packet.Type << "!" << std::endl;
                  break;
               }
               if(result < (ssize_t)sizeof(packet)) {
                  std::cout << ". OK!" << std::endl;
                  break;
               }
               result = socket->receive((char*)&packet,sizeof(packet));
            }
            fclose(out);
         }
      }
   }

   if(result < 0) {
      std::cerr << "ERROR: tftpGet() - I/O error!" << std::endl;
   }
}


// ###### Upload ############################################################
void tftpPut(const char* name, Socket* socket)
{
   FILE* in = fopen(name,"r");
   if(in == NULL) {
      std::cerr << "ERROR: Unable to open file \"" << name << "\"!" << std::endl;
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
   std::cout << "Local File:  " << name << std::endl
             << "Remote File: " << (char*)&packet.Data << std::endl
             << std::endl;

   if(result >= 0) {
      result = socket->receive((char*)&packet,sizeof(packet));
      if(result > 0) {
         if(packet.Type == TFTP_ERR) {
            printError(packet,result);
         }
         else if(packet.Type == TFTP_ACK) {
            std::cout << name << " -> ";
            std::cout.flush();

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
               std::cout << ".";
               std::cout.flush();
               packet.Code++;
            }

            result = socket->receive((char*)&packet,sizeof(packet));
            if(result > 0) {
               if(packet.Type == TFTP_ERR) {
                  printError(packet,result);
               }
               else if(packet.Type == TFTP_ACK) {
                  std::cout << " OK!" << std::endl;
               }
               else {
                  std::cerr << "ERROR: tftpPut() - Unexpected packet type #" << (cardinal)packet.Type << "!" << std::endl;
               }
            }
         }
         else {
            std::cerr << "ERROR: tftpGet() - Received invalid packet type #" << (cardinal)packet.Type << "!" << std::endl;
         }
      }
   }

   if(result < 0) {
      std::cerr << "ERROR: tftpGet() - I/O error!" << std::endl;
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
      std::cerr << "ERROR: Out of memory!" << std::endl;
      exit(1);
   }
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
                << " [Remote address] [get|put] [Filename] {-force-ipv4|-use-ipv6} {-local=address1} ... {-local=addressN}"
                << std::endl;
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
               std::cerr << "ERROR: Argument #" << i << " is an invalid address!" << std::endl;
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
         std::cerr << "Usage: " << argv[0] << " "
                   << "[get|put] [Filename] [Remote address] {-force-ipv4|-use-ipv6} {-local=address1} ... {-local=addressN}"
                   << std::endl;
         exit(1);
      }
   }
   if(optForceIPv4) {
      InternetAddress::UseIPv6 = false;
   }
   SocketAddress* remoteAddress = SocketAddress::createSocketAddress(0,argv[1]);
   if(remoteAddress == NULL) {
      std::cerr << "ERROR: Bad remote address! Use <address>:<port> format." << std::endl;
      exit(1);
   }
   if(localAddresses < 1) {
      localAddressArray[0] = SocketAddress::getLocalAddress(*remoteAddress);
      if(localAddressArray[0] == NULL) {
         std::cerr << "ERROR: Out of memory!" << std::endl;
         exit(1);
      }
      localAddresses = 1;
   }
   TFTPPacket packet;
   if(strlen(argv[3]) > sizeof(packet.Data) - 1) {
      std::cerr << "ERROR: File name too long!" << std::endl;
      exit(1);
   }


   // ====== Print information ==============================================
   std::cout << "SCTP TFTP - Copyright (C) 2001-2007 Thomas Dreibholz" << std::endl;
   std::cout << "----------------------------------------------------" << std::endl;
   std::cout << "Version:               " << __DATE__ << ", " << __TIME__ << std::endl;
   std::cout << "Local Addresses:       " << *(localAddressArray[0]) << std::endl;
   for(cardinal i = 1;i < localAddresses;i++) {
      std::cout << "                       " << *(localAddressArray[i]) << std::endl;
   }
   std::cout << "Remote Address:        " << *remoteAddress << std::endl;
   std::cout << std::endl;


   // ====== Create socket and connect ======================================
   Socket clientSocket(Socket::IP,Socket::Stream,Socket::SCTP);
   sctp_initmsg init;
   init.sinit_num_ostreams   = 1;
   init.sinit_max_instreams  = 1;
   init.sinit_max_attempts   = 0;
   init.sinit_max_init_timeo = 60;
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_INITMSG,(char*)&init,sizeof(init)) < 0) {
      std::cerr << "ERROR: Unable to set SCTP_INITMSG parameters!" << std::endl;
      exit(1);
   }
   if(clientSocket.bindx((const SocketAddress**)localAddressArray,
                         localAddresses,
                         SCTP_BINDX_ADD_ADDR) == false) {
      std::cerr << "ERROR: Unable to bind socket!" << std::endl;
      exit(1);
   }

   sctp_event_subscribe events;
   memset((char*)&events,0,sizeof(events));
   if(clientSocket.setSocketOption(IPPROTO_SCTP,SCTP_EVENTS,&events,sizeof(events)) < 0) {
      std::cerr << "ERROR: SCTP_EVENTS failed!" << std::endl;
      exit(1);
   }


   std::cout << "Connecting... ";
   std::cout.flush();
   if(clientSocket.connect(*remoteAddress) == false) {
      std::cout << "failed!" << std::endl;
      std::cerr << "ERROR: Unable to connect to remote address!" << std::endl;
      exit(1);
   }
   std::cout << "done." << std::endl << std::endl;


   if(!(strcasecmp(argv[2],"PUT"))) {
      tftpPut(argv[3],&clientSocket);
   }
   else {
      tftpGet(argv[3],&clientSocket);
   }


   SocketAddress::deleteAddressList(localAddressArray);
   return 0;
}
