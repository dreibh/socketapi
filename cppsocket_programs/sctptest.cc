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
 * Purpose: SCTP Test
 *
 */


#include "tdsystem.h"
#include "thread.h"
#include "randomizer.h"
#include "sctpsocket.h"
#include "sctpassociation.h"
#include "ext_socket.h"



class EchoServer : public Thread
{
   public:
   EchoServer(SCTPAssociation* association);
   ~EchoServer();

   private:
   void run();

   static unsigned int ThreadCounter;

   unsigned int        ID;
   SCTPAssociation*    Association;
};

unsigned int EchoServer::ThreadCounter = 0;



// ###### Constructor #######################################################
EchoServer::EchoServer(SCTPAssociation* association)
{
   Association = association;
   synchronized();
   ID          = ++ThreadCounter;
   unsynchronized();

   printTimeStamp(cout);
   std::cout << "Thread #" << ID << " start!" << std::endl;
   char* hello = "The SCTP Echo Server - Version 1.00";
   Association->send(hello,strlen(hello),0,0,0,0);

   start();
}


// ###### Destructor ########################################################
EchoServer::~EchoServer()
{
   delete Association;
   Association = NULL;
}



// ###### Thread main loop ##################################################
void EchoServer::run()
{
   char dataBuffer[16384 + 1];
   for(;;) {

      // ====== Receive string from association =============================
      unsigned short streamID;
      unsigned int   protoID;
      uint16_t       ssn;
      uint32_t       tsn;
      int            flags = 0;
      size_t         dataLength    = sizeof(dataBuffer);
      const int error = Association->receive(
                           (char*)&dataBuffer, dataLength,
                           flags,
                           streamID, protoID,
                           ssn, tsn);
      if(error != 0) {
         printTimeStamp(cout);
         std::cout << "Thread #" << ID << " shutdown!" << std::endl;
         delete this;
         return;
      }


      // ====== Reply, if received block is a data block ====================
      if(dataLength > 0) {
         char str[16384 + 256];
         snprintf((char*)&str,sizeof(str),"T%d> %s",ID,dataBuffer);
         Association->send(str,strlen(str),0,streamID,0,0);

         // ====== Print information ========================================
         dataBuffer[dataLength] = 0x00;
         snprintf((char*)&str,sizeof(str),"A%d: [%s]",Association->getID(),dataBuffer);
         std::cout << str << std::endl;
      }

   }
}



class EchoClient : public Thread
{
   public:
   EchoClient(SCTPAssociation* peer);
   ~EchoClient();

   private:
   void run();

   SCTPAssociation* Association;
};


// ###### Constructor #######################################################
EchoClient::EchoClient(SCTPAssociation* peer)
{
   Association = peer;
   start();
}


// ###### Destructor ########################################################
EchoClient::~EchoClient()
{
}


// ###### Thread main loop ##################################################
void EchoClient::run()
{
   char dataBuffer[16384];
   int i = 0;
   for(;;) {
      snprintf((char*)&dataBuffer,sizeof(dataBuffer),"Test #%d",i);
      const int error = Association->send(dataBuffer,strlen(dataBuffer),0,0,0,0);
      if(error < 0) {
         std::cerr << "ERROR: EchoClient::run() - Send error #" << error << "!" << std::endl;
         ::exit(1);
      }

      delay(750000);
      i++;
   }
}



struct Settings
{
   unsigned int   LocalAddresses;
   unsigned int   RemoteAddresses;
   unsigned short LocalPort;
   unsigned short RemotePort;
   char           LocalAddress[SCTP_MAX_NUM_ADDRESSES][SCTP_MAX_IP_LEN];
   char           RemoteAddress[SCTP_MAX_NUM_ADDRESSES][SCTP_MAX_IP_LEN];
};



// ###### Connection-less server ############################################
void connectionLessServer(const Settings* settings)
{
   SocketAddress* addressArray[SCTP_MAX_NUM_ADDRESSES + 1];
   for(unsigned int i = 0;i < settings->LocalAddresses;i++) {
      addressArray[i] = SocketAddress::createSocketAddress(
                          SocketAddress::PF_HidePort,
                          (char*)&settings->LocalAddress[i]);
   }
   addressArray[settings->LocalAddresses] = NULL;

   SCTPSocket receiverSocket(SCTPSocket::SSF_AutoConnect|SCTPSocket::SSF_GlobalQueue);
   if(receiverSocket.bind(settings->LocalPort,1,1,(const SocketAddress**)&addressArray) != 0) {
      std::cerr << "ERROR: connectionLessServer() - Unable to bind local socket!" << std::endl;
      exit(1);
   }

   SCTPSocket senderSocket(SCTPSocket::SSF_AutoConnect|SCTPSocket::SSF_GlobalQueue);
   if(senderSocket.bind(settings->LocalPort + 1,1,1,(const SocketAddress**)&addressArray) != 0) {
      std::cerr << "ERROR: connectionLessServer() - Unable to bind local socket #2!" << std::endl;
      exit(1);
   }

   char dataBuffer[16384 + 1];
   for(;;) {
      unsigned int     assocID;
      unsigned short   streamID;
      unsigned int     protoID;
      SCTPNotification notification;
      uint16_t         ssn;
      uint32_t         tsn;
      int              flags       = 0;
      size_t           dataLength  = sizeof(dataBuffer);
      SocketAddress**  addressList = NULL;

      const int error = receiverSocket.receiveFrom(
                           (char*)&dataBuffer,dataLength,
                           flags,
                           assocID, streamID, protoID,
                           ssn, tsn,
                           &addressList,
                           notification);
      if(error < 0) {
         std::cerr << "ERROR: connectionLessServer() - Receive error #" << error << "!" << std::endl;
         exit(1);
      }

      if(dataLength > 0) {
         dataBuffer[dataLength] = 0x00;

         if((addressList != NULL) && (addressList[0] != NULL)) {
            // ====== Set destination port, send reply ======================
            addressList[0]->setPort(addressList[0]->getPort() - 1);

            char str[16384 + 256];
            snprintf((char*)&str,sizeof(str),"Re> %s",dataBuffer);
            const int error = senderSocket.sendTo(
                                 (char*)&str, strlen(str),
                                 MSG_ABORT,
                                 0,
                                 streamID,
                                 0x00000000,
                                 0,
                                 addressList[0]);
            if(error < 0) {
               std::cerr << "WARNING: connectionLessServer() - Send error #" << error << "!" << std::endl;
            }


            // ====== Print information =====================================
            addressList[0]->setPrintFormat(InternetAddress::PF_Address);
            std::cout << *(addressList[0]) << "> [" << dataBuffer << "]" << std::endl;
         }
      }
   }

}


// ###### Connection-less client ############################################
void connectionLessClient(const Settings* settings)
{
   SocketAddress* addressArray[SCTP_MAX_NUM_ADDRESSES + 1];
   for(unsigned int i = 0;i < settings->LocalAddresses;i++) {
      addressArray[i] = SocketAddress::createSocketAddress(
                          SocketAddress::PF_HidePort,
                          (char*)&settings->LocalAddress[i]);
   }
   addressArray[settings->LocalAddresses] = NULL;


   SCTPSocket receiverSocket(SCTPSocket::SSF_AutoConnect|SCTPSocket::SSF_GlobalQueue);
   if(receiverSocket.bind(settings->LocalPort,1,1,(const SocketAddress**)&addressArray) != 0) {
      std::cerr << "ERROR: connectionLessClient() - Unable to bind local socket!" << std::endl;
      exit(1);
   }

   SCTPSocket senderSocket(SCTPSocket::SSF_AutoConnect|SCTPSocket::SSF_GlobalQueue);
   if(senderSocket.bind(settings->LocalPort + 1,1,1,(const SocketAddress**)&addressArray) != 0) {
      std::cerr << "ERROR: connectionLessClient() - Unable to bind local socket!" << std::endl;
      exit(1);
   }

   SocketAddress* remoteAddress = SocketAddress::createSocketAddress(
                                     0,
                                     settings->RemoteAddress[0],settings->RemotePort);
   if(remoteAddress == NULL) {
      std::cerr << "ERROR: connectionLessClient() - Bad remote address!" << std::endl;
      exit(1);
   }

   char dataBuffer[16384];
   for(unsigned int i = 0;i < 10000;i++) {
      snprintf((char*)&dataBuffer,sizeof(dataBuffer),"Test #%d",i);
      const int error = senderSocket.sendTo(
                           dataBuffer,strlen(dataBuffer),
                           MSG_ABORT,
                           0,
                           0,
                           0x00000000,
                           0,
                           remoteAddress);
      if(error < 0) {
         std::cerr << "ERROR: connectionLessClient() - Send error #" << error << "!" << std::endl;
         exit(1);
      }


      bool complete = false;
      do {
         char dataBuffer[16384 + 1];
         unsigned int      assocID;
         unsigned short    streamID;
         unsigned int      protoID;
         uint16_t          ssn;
         uint32_t          tsn;
         SCTPNotification  notification;
         int               flags         = MSG_DONTWAIT;
         size_t            dataLength    = sizeof(dataBuffer);
         const int error = receiverSocket.receiveFrom(
                              (char*)&dataBuffer,dataLength,
                              flags,
                              assocID, streamID, protoID,
                              ssn, tsn,
                              NULL,
                              notification);
         if(error < 0) {
            std::cerr << "ERROR: connectionLessClient() - Receive error #" << error << "!" << std::endl;
            exit(1);
         }

         if(dataLength > 0) {
            dataBuffer[dataLength] = 0x00;
            std::cout << "--> " << dataBuffer << std::endl;
         }

         complete = (dataLength == 0);
      } while(!complete);

      Thread::delay(750000);
   }

   delete remoteAddress;
}


// ###### Connection-oriented server ########################################
void connectionOrientedServer(const Settings* settings)
{
   SocketAddress* addressArray[SCTP_MAX_NUM_ADDRESSES + 1];
   for(unsigned int i = 0;i < settings->LocalAddresses;i++) {
      addressArray[i] = SocketAddress::createSocketAddress(
                          SocketAddress::PF_HidePort,
                          (char*)&settings->LocalAddress[i]);
   }
   addressArray[settings->LocalAddresses] = NULL;

   SCTPSocket socket;
   if(socket.bind(settings->LocalPort,1,1,(const SocketAddress**)&addressArray) != 0) {
      std::cerr << "ERROR: connectionOrientedServer() - Unable to bind local socket!" << std::endl;
      exit(1);
   }

   socket.listen(10);
   for(;;) {
      SocketAddress** addressList = NULL;
      SCTPAssociation* association = socket.accept(&addressList);
      if(association == NULL) {
         std::cerr << "ERROR: connectionOrientedServer() - accept() failed!" << std::endl;
         exit(1);
      }

      printTimeStamp(cout);
      std::cout << "Accepted association from {";
      unsigned int i = 0;
      while(addressList[i] != NULL) {
         addressList[i]->setPrintFormat(InternetAddress::PF_Address);
         std::cout << " " << *(addressList[i]) << " ";
         i++;
      }
      std::cout << "}." << std::endl;

      SocketAddress::deleteAddressList(addressList);
      new EchoServer(association);
   }
}


// ###### Connection-oriented client ########################################
void connectionOrientedClient(const Settings* settings)
{
   SocketAddress* addressArray[SCTP_MAX_NUM_ADDRESSES + 1];
   for(unsigned int i = 0;i < settings->LocalAddresses;i++) {
      addressArray[i] = SocketAddress::createSocketAddress(
                          SocketAddress::PF_HidePort,
                          (char*)&settings->LocalAddress[i]);
   }
   addressArray[settings->LocalAddresses] = NULL;

   SCTPSocket socket;
   if(socket.bind(settings->LocalPort,1,1,(const SocketAddress**)&addressArray) != 0) {
      std::cerr << "ERROR: connectionOrientedClient() - Unable to bind local socket!" << std::endl;
      exit(1);
   }


   SocketAddress* remoteAddress = SocketAddress::createSocketAddress(
                                     0,
                                     settings->RemoteAddress[0],settings->RemotePort);
   if(remoteAddress == NULL) {
      std::cerr << "ERROR: connectionOrientedClient() - Bad remote address!" << std::endl;
      exit(1);
   }
   SCTPAssociation* association = socket.associate(1,*remoteAddress);
   if(association == NULL) {
      std::cerr << "ERROR: connectionOrientedClient() - associate() failed!" << std::endl;
      exit(1);
   }
   delete remoteAddress;

   EchoClient*echoClient = new EchoClient(association);
   if(echoClient == NULL) {
      std::cerr << "ERROR: connectionOrientedClient() - Out of memory!" << std::endl;
      exit(1);
   }


   char dataBuffer[16384 + 1];
   for(;;) {
      unsigned short    streamID;
      unsigned int      protoID;
      uint16_t          ssn;
      uint32_t          tsn;
      int               flags      = 0;
      size_t            dataLength = 5; // sizeof(dataBuffer);

      const int error = association->receive(
                           (char*)&dataBuffer,dataLength,
                           flags,
                           streamID, protoID,
                           ssn, tsn);
      if(error < 0) {
         std::cerr << "ERROR: connectionOrientedClient() - Receive error #" << error << "!" << std::endl;
         exit(1);
      }

      if(dataLength > 0) {
         dataBuffer[dataLength] = 0x00;
         std::cout << dataBuffer;
         std::cout.flush();
         if(flags & MSG_EOR) {
            std::cout << std::endl;
         }
         else std::cout << "|";
      }
   }

   Thread::delay(1000000);
   association->shutdown();
   delete echoClient;
   delete association;
}




// ###### Main program ######################################################
int main(int argc, char** argv)
{
   Settings settings;
   bool     server         = false;
   bool     connectionLess = false;
   bool     optForceIPv4   = true;
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
   settings.LocalPort       = 0;
   settings.RemotePort      = 4711;
   settings.LocalAddresses  = 0;
   settings.RemoteAddresses = 0;
   for(unsigned int i = 1;i < (cardinal)argc;i++) {
      if(!(strcasecmp(argv[i],"-client")))                  server = false;
      else if(!(strcasecmp(argv[i],"-server")))             server = true;
      else if(!(strcasecmp(argv[i],"-connectionless")))     connectionLess = true;
      else if(!(strcasecmp(argv[i],"-connectionoriented"))) connectionLess = false;
      else if(!(strcasecmp(argv[i],"-cl")))                 connectionLess = true;
      else if(!(strcasecmp(argv[i],"-co")))                 connectionLess = false;
      else if(!(strcasecmp(argv[i],"-force-ipv4")))         optForceIPv4 = true;
      else if(!(strcasecmp(argv[i],"-use-ipv6")))           optForceIPv4 = false;
      else if(!(strncasecmp(argv[i],"-local=",7))) {
         if(settings.LocalAddresses < SCTP_MAX_NUM_ADDRESSES) {
            snprintf((char*)&settings.LocalAddress[settings.LocalAddresses],SCTP_MAX_IP_LEN,"%s",&argv[i][7]);
            settings.LocalAddresses++;
         }
         else {
            std::cerr << "ERROR: Too many local addresses!" << std::endl;
            exit(1);
         }
      }
      else if(!(strncasecmp(argv[i],"-remote=",8))) {
         if(settings.RemoteAddresses < SCTP_MAX_NUM_ADDRESSES) {
            snprintf((char*)&settings.RemoteAddress[settings.RemoteAddresses],SCTP_MAX_IP_LEN,"%s",&argv[i][8]);
            settings.RemoteAddresses++;
         }
         else {
            std::cerr << "ERROR: Too many remote addresses!" << std::endl;
            exit(1);
         }
      }
      else if(!(strncasecmp(argv[i],"-localport=",11)))  settings.LocalPort  = atol(&argv[i][11]);
      else if(!(strncasecmp(argv[i],"-remoteport=",12))) settings.RemotePort = atol(&argv[i][12]);
      else {
         std::cerr << "Usage: " << argv[0] << " "
                 "[-client|-server] [-connectionless|-cl|-connectionoriented|-co] "
                 "[-force-ipv4|-use-ipv6] [-local=address1] ... [-local=addressN] "
                 "[-remote=address1] ... [-remote=addressN] [-localport=port] [-remoteport=port]"
              << std::endl;
         exit(1);
      }
   }
   if(settings.RemoteAddresses < 1) {
      settings.RemoteAddresses = 1;
      strcpy((char*)&settings.RemoteAddress[0],"127.0.0.1");
   }
   if(settings.LocalAddresses < 1) {
      settings.LocalAddresses = 1;
      strcpy((char*)&settings.LocalAddress[0],"127.0.0.1");
   }
   if(settings.LocalPort == 0) {
      if(!server) {
         Randomizer randomizer;
         settings.LocalPort = 10000 + (randomizer.random16() % 50000);
      }
      else {
         std::cerr << "ERROR: A server requires given local port!" << std::endl;
         exit(1);
      }
   }
   if(optForceIPv4) {
      InternetAddress::UseIPv6 = false;
   }


   // ====== Print information ==============================================
   std::cout << "SCTP Test - Copyright (C) 2001 Thomas Dreibholz" << std::endl;
   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "Version:           " << __DATE__ << ", " << __TIME__ << std::endl;
   std::cout << "Server Mode:       " << ((server == true) ? "on" : "off") << std::endl;
   std::cout << "Connection Mode:   " << ((connectionLess == false) ? "connection-oriented" : "connection-less") << std::endl;
   std::cout << "Local Addresses:   " << settings.LocalAddress[0] << std::endl;
   for(unsigned int i = 1;i < settings.LocalAddresses;i++) {
      std::cout << "                   " << settings.LocalAddress[i] << std::endl;
   }
   std::cout << "Local Port:        " << settings.LocalPort << std::endl;
   if(!server) {
      std::cout << "Remote Addresses:  " << settings.RemoteAddress[0] << std::endl;
      for(unsigned int i = 1;i < settings.RemoteAddresses;i++) {
         std::cout << "                   " << settings.RemoteAddress[i] << std::endl;
      }
      std::cout << "Remote Port:       " << settings.RemotePort << std::endl;
   }
   std::cout << std::endl << std::endl;


   // ====== Execute main program ===========================================
   if(server) {
      if(connectionLess) {
         std::cout << "Start connection-less server..." << std::endl;
         connectionLessServer(&settings);
      }
      else {
         std::cout << "Start connection-oriented server..." << std::endl;
         connectionOrientedServer(&settings);
      }
   }
   else {
      if(connectionLess) {
         std::cout << "Start connection-less client..." << std::endl;
         connectionLessClient(&settings);
      }
      else {
         std::cout << "Start connection-oriented client..." << std::endl;
         connectionOrientedClient(&settings);
      }
   }

   return 0;
}
