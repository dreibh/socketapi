/*
 *  $Id: sctpsockettest.cc,v 1.1 2003/05/15 11:35:50 dreibh Exp $
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
 * Purpose: SCTP Socket Test
 *
 */


#include "tdsystem.h"
#include "thread.h"
#include "tdsocket.h"
#include "randomizer.h"
#include "ext_socket.h"



class EchoSocketServer : public Thread
{
   public:
   EchoSocketServer(Socket* socket);
   ~EchoSocketServer();

   private:
   void run();

   static unsigned int ThreadCounter;

   unsigned int        ID;
   Socket*             EchoSocket;
};


unsigned int EchoSocketServer::ThreadCounter = 0;


// ###### Constructor #######################################################
EchoSocketServer::EchoSocketServer(Socket* socket)
{
   EchoSocket = socket;
   synchronized();
   ID = ++ThreadCounter;
   unsynchronized();

   printTimeStamp(cout);
   cout << "Thread #" << ID << " start!" << endl;
   char* hello = "The SCTP Echo Socket Server - Version 1.00";
   EchoSocket->send(hello,strlen(hello),0);

   start();
}


// ###### Destructor ########################################################
EchoSocketServer::~EchoSocketServer()
{
   delete EchoSocket;
   EchoSocket = NULL;
}


// ###### Thread main loop ##################################################
void EchoSocketServer::run()
{
   char dataBuffer[16384 + 1];
   for(;;) {

      // ====== Receive string from association =============================
      const ssize_t dataLength = EchoSocket->receive((char*)&dataBuffer,
                                                     sizeof(dataBuffer));
      if(dataLength < 0) {
         printTimeStamp(cout);
         cout << "Thread #" << ID << " shutdown!" << endl;
         delete this;
         return;
      }


      // ====== Reply, if received block is a data block ====================
      if(dataLength > 0) {
         char str[16384 + 256];
         snprintf((char*)&str,sizeof(str),"T%d> %s",ID,dataBuffer);
         EchoSocket->send(str,strlen(str),0);

         // ====== Print information ========================================
         dataBuffer[dataLength] = 0x00;
         snprintf((char*)&str,sizeof(str),"T%d: [%s]",ID,dataBuffer);
         cout << str << endl;
      }

   }
}



class EchoSocketClient : public Thread
{
   public:
   EchoSocketClient(Socket* socket);
   ~EchoSocketClient();

   private:
   void run();


   Socket* EchoSocket;
};


// ###### Constructor #######################################################
EchoSocketClient::EchoSocketClient(Socket* socket)
{
   EchoSocket = socket;
   start();
}


// ###### Destructor ########################################################
EchoSocketClient::~EchoSocketClient()
{
}


// ###### Thread main loop ##################################################
void EchoSocketClient::run()
{
   char dataBuffer[16384];
   int i = 0;
   for(;;) {
      snprintf((char*)&dataBuffer,sizeof(dataBuffer),"Test #%d",i);
      const ssize_t sent = EchoSocket->send(dataBuffer,strlen(dataBuffer),0);
      if(sent < 0) {
         cerr << "ERROR: EchoSocketClient::run() - Send error #" << sent << "!" << endl;
         ::exit(1);
      }

      delay(10000);
      i++;
   }
}



struct Settings
{
   unsigned int   LocalAddresses;
   unsigned int   RemoteAddresses;
   unsigned short LocalPort;
   unsigned short RemotePort;
   char           LocalAddress[SCTP_MAXADDRESSES][INET6_ADDRSTRLEN];
   char           RemoteAddress[SCTP_MAXADDRESSES][INET6_ADDRSTRLEN];
};



// ###### Connection-oriented socket server #################################
void connectionOrientedSocketServer(const Settings* settings)
{
   InternetAddress receiverAddress((char*)&settings->LocalAddress[0],settings->LocalPort);
   Socket socket(Socket::IP,Socket::Stream,Socket::SCTP);
   if(socket.bind(receiverAddress) == false) {
      cerr << "ERROR: connectionLessSocketServer() - Unable to bind socket!" << endl;
      exit(1);
   }
   int off = 0;
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVDATAIOEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVDATAIOEVNT failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVASSOCEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVASSOCEVNT failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPADDREVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVPADDREVNT failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSENDFAILEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVSENDFAILEVNT failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPEERERR,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVPEERERR failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSHUTDOWNEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVSHUTDOWNEVNT failed!" << endl;
      exit(1);
   }


   socket.listen(10);
   for(;;) {
      SocketAddress* peer;
      Socket* newSocket = socket.accept(&peer);
      if(newSocket != NULL) {
         printTimeStamp(cout);
         if(peer != NULL) {
            peer->setPrintFormat(InternetAddress::PF_Address);
            cout << "Accepted association from " << peer << "." << endl;
            delete peer;
         }
         else {
            cout << "Accepted association from (Unknown)." << endl;
         }

         new EchoSocketServer(newSocket);
      }
   }
}


// ###### Connection-oriented socket client #################################
void connectionOrientedSocketClient(const Settings* settings)
{
   const InternetAddress localAddress((char*)&settings->LocalAddress[0],settings->LocalPort);
   const InternetAddress remoteAddress((char*)&settings->RemoteAddress[0],settings->RemotePort);
   Socket socket(Socket::IP,Socket::Stream,Socket::SCTP);
   if(socket.bind(localAddress) == false) {
      cerr << "ERROR: connectionLessSocketServer() - Unable to bind socket!" << endl;
      exit(1);
   }
   int off = 0;
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVDATAIOEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVDATAIOEVNT failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVASSOCEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVASSOCEVNT failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPADDREVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVPADDREVNT failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSENDFAILEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVSENDFAILEVNT failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPEERERR,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVPEERERR failed!" << endl;
      exit(1);
   }
   if(socket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSHUTDOWNEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVSHUTDOWNEVNT failed!" << endl;
      exit(1);
   }
   if(socket.connect(remoteAddress) == false) {
      cerr << "ERROR: connectionLessSocketServer() - Unable to connect socket!" << endl;
      exit(1);
   }

   EchoSocketClient* echoClient = new EchoSocketClient(&socket);
   if(echoClient == NULL) {
      cerr << "ERROR: connectionOrientedSocketClient() - Out of memory!" << endl;
      exit(1);
   }


   char dataBuffer[16384 + 1];
   for(;;) {
      const ssize_t dataLength = socket.receive((char*)&dataBuffer,sizeof(dataBuffer),0);
      if(dataLength < 0) {
         cerr << "ERROR: connectionOrientedSocketClient() - Receive error #" << dataLength << "!" << endl;
         exit(1);
      }

      if(dataLength > 0) {
         dataBuffer[dataLength] = 0x00;
         cout << dataBuffer << endl;
      }
   }

   Thread::delay(1000000);
   delete echoClient;
}


// ###### Connection-less socket server #####################################
void connectionLessSocketServer(const Settings* settings)
{
   InternetAddress localReceiverAddress((char*)&settings->LocalAddress[0],settings->LocalPort);
   Socket receiverSocket(Socket::IP,Socket::Datagram,Socket::SCTP);
   if(receiverSocket.bind(localReceiverAddress) == false) {
      cerr << "ERROR: connectionLessSocketServer() - Unable to bind local socket!" << endl;
      exit(1);
   }
   int off = 0;
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVDATAIOEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVDATAIOEVNT failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVASSOCEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVASSOCEVNT failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPADDREVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVPADDREVNT failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSENDFAILEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVSENDFAILEVNT failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPEERERR,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVPEERERR failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSHUTDOWNEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVSHUTDOWNEVNT failed!" << endl;
      exit(1);
   }


   InternetAddress localSenderAddress((char*)&settings->LocalAddress[0],settings->LocalPort + 1);
   Socket senderSocket(Socket::IP,Socket::Datagram,Socket::SCTP);
   if(senderSocket.bind(localSenderAddress) == false) {
      cerr << "ERROR: connectionLessSocketServer() - Unable to bind local socket!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVDATAIOEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVDATAIOEVNT failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVASSOCEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVASSOCEVNT failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPADDREVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVPADDREVNT failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSENDFAILEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVSENDFAILEVNT failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPEERERR,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVPEERERR failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSHUTDOWNEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketServer() - SCTP_RECVSHUTDOWNEVNT failed!" << endl;
      exit(1);
   }

   InternetAddress peer;

   char dataBuffer[16384 + 1];
   for(cardinal i = 0;i < 1000000;i++) {
      cout << "------ Recv Iteration #" << (i + 1) << " -------------" << endl;
      const int dataLength = receiverSocket.receiveFrom(
                                (char*)&dataBuffer,sizeof(dataBuffer),
                                peer,
                                0);
      if(dataLength < 0) {
         cerr << "ERROR: connectionLessSocketServer() - Receive error #" << dataLength << "!" << endl;
         exit(1);
      }
      else {
         dataBuffer[dataLength] = 0x00;

         // ====== Print information ========================================
         peer.setPrintFormat(InternetAddress::PF_Address);
         cout << peer << "> [" << dataBuffer << "]" << endl;


         // ====== Set destination port, send reply =========================
         cout << dataBuffer << " from " << peer << endl;
         peer.setPort(peer.getPort() - 1);

         // ====== Send reply ===============================================
         char str[16384 + 256];
         snprintf((char*)&str,sizeof(str),"Re> %s",dataBuffer);
         cout << "------ Send Iteration #" << (i + 1) << " -------------" << endl;
         const int sent = senderSocket.sendTo(
                              (char*)&str, strlen(str),
                              MSG_ABORT,
                              peer);
         if(sent < 0) {
            cerr << "WARNING: connectionLessSocketServer() - Send error #" << sent << "!" << endl;
         }
      }
   }

}


// ###### Connection-less socket client #####################################
void connectionLessSocketClient(const Settings* settings)
{
   InternetAddress localReceiverAddress((char*)&settings->LocalAddress[0],settings->LocalPort);
   Socket receiverSocket(Socket::IP,Socket::Datagram,Socket::SCTP);
   if(receiverSocket.bind(localReceiverAddress) == false) {
      cerr << "ERROR: connectionLessSocketClient() - Unable to bind local socket!" << endl;
      exit(1);
   }
   int off = 0;
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVDATAIOEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVDATAIOEVNT failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVASSOCEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVASSOCEVNT failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPADDREVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVPADDREVNT failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSENDFAILEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVSENDFAILEVNT failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPEERERR,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVPEERERR failed!" << endl;
      exit(1);
   }
   if(receiverSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSHUTDOWNEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVSHUTDOWNEVNT failed!" << endl;
      exit(1);
   }
   InternetAddress localSenderAddress((char*)&settings->LocalAddress[0],settings->LocalPort + 1);
   Socket senderSocket(Socket::IP,Socket::Datagram,Socket::SCTP);
   if(senderSocket.bind(localSenderAddress) == false) {
      cerr << "ERROR: connectionLessSocketClient() - Unable to bind local socket!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVDATAIOEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVDATAIOEVNT failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVASSOCEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVASSOCEVNT failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPADDREVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVPADDREVNT failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSENDFAILEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVSENDFAILEVNT failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVPEERERR,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVPEERERR failed!" << endl;
      exit(1);
   }
   if(senderSocket.setSocketOption(IPPROTO_SCTP,SCTP_RECVSHUTDOWNEVNT,&off,sizeof(off)) < 0) {
      cerr << "ERROR: connectionLessSocketClient() - SCTP_RECVSHUTDOWNEVNT failed!" << endl;
      exit(1);
   }

   InternetAddress       from;
   const InternetAddress to(settings->RemoteAddress[0],settings->RemotePort);

   char dataBuffer[16384];
   for(unsigned int i = 0;i < 1000000;i++) {
      cout << "------ Send Iteration #" << (i + 1) << " -------------" << endl;
      snprintf((char*)&dataBuffer,sizeof(dataBuffer),"Test #%d",i);
      const ssize_t sent =
         senderSocket.sendTo(
            (char*)&dataBuffer,
            strlen(dataBuffer),
            MSG_ABORT,
            to);
      if(sent < 0) {
         cerr << "ERROR: connectionLessSocketClient() - Send error #" << sent << "!" << endl;
         exit(1);
      }

      cout << "------ Recv Iteration #" << (i + 1) << " -------------" << endl;
      for(;;) {
         char dataBuffer[16384 + 1];
         const ssize_t received =
            receiverSocket.receiveFrom(
               (char*)&dataBuffer, sizeof(dataBuffer),
               from, 0);
         if(received >= 0) {
            dataBuffer[received] = 0x00;
            cout << "--> " << dataBuffer << endl;
            break;
         }
         else if(received < 0) {
            break;
         }
      }
   }
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
         if(settings.LocalAddresses < SCTP_MAXADDRESSES) {
            snprintf((char*)&settings.LocalAddress[settings.LocalAddresses],INET6_ADDRSTRLEN,"%s",&argv[i][7]);
            settings.LocalAddresses++;
         }
         else {
            cerr << "ERROR: Too many local addresses!" << endl;
            exit(1);
         }
      }
      else if(!(strncasecmp(argv[i],"-remote=",8))) {
         if(settings.RemoteAddresses < SCTP_MAXADDRESSES) {
            snprintf((char*)&settings.RemoteAddress[settings.RemoteAddresses],INET6_ADDRSTRLEN,"%s",&argv[i][8]);
            settings.RemoteAddresses++;
         }
         else {
            cerr << "ERROR: Too many remote addresses!" << endl;
            exit(1);
         }
      }
      else if(!(strncasecmp(argv[i],"-localport=",11)))  settings.LocalPort  = atol(&argv[i][11]);
      else if(!(strncasecmp(argv[i],"-remoteport=",12))) settings.RemotePort = atol(&argv[i][12]);
      else {
         cerr << "Usage: " << argv[0] << " "
                 "[-client|-server] [-connectionless|-cl|-connectionoriented|-co] "
                 "[-force-ipv4|-use-ipv6] [-local=address1] ... [-local=addressN] "
                 "[-remote=address1] ... [-remote=addressN] [-localport=port] [-remoteport=port]"
              << endl;
         exit(1);
      }
   }
   if(optForceIPv4) {
      InternetAddress::UseIPv6 = false;
   }
   if((!server) && (settings.RemoteAddresses < 1)) {
      cerr << "ERROR: No remote address given!" << endl;
      exit(1);
   }
   if(settings.LocalAddresses < 1) {
      SocketAddress** localAddressArray = NULL;
      if((!Socket::getLocalAddressList(
            localAddressArray,
            settings.LocalAddresses)) || (settings.LocalAddresses < 1)) {
         cerr << "ERROR: Cannot obtain local addresses!" << endl;
         exit(1);
      }
      for(cardinal i = 0;i < settings.LocalAddresses;i++) {
         snprintf((char*)&settings.LocalAddress[i],INET6_ADDRSTRLEN,"%s",
                  localAddressArray[i]->getAddressString(SocketAddress::PF_Address|SocketAddress::PF_HidePort|SocketAddress::PF_Legacy).getData());
      }
      SocketAddress::deleteAddressList(localAddressArray);
   }
   if(settings.LocalPort == 0) {
      if(!server) {
         Randomizer randomizer;
         settings.LocalPort = 10000 + (randomizer.random16() % 50000);
      }
      else {
         settings.LocalPort = 4711;
      }
   }


   // ====== Print information ==============================================
   cout << "SCTP Socket Test - Copyright (C) 2001 Thomas Dreibholz" << endl;
   cout << "------------------------------------------------------" << endl;
   cout << "Version:           " << __DATE__ << ", " << __TIME__ << endl;
   cout << "Server Mode:       " << ((server == true) ? "on" : "off") << endl;
   cout << "Connection Mode:   " << ((connectionLess == false) ? "connection-oriented" : "connection-less") << endl;
   cout << "Local Addresses:   " << settings.LocalAddress[0] << endl;
   for(unsigned int i = 1;i < settings.LocalAddresses;i++) {
      cout << "                   " << settings.LocalAddress[i] << endl;
   }
   cout << "Local Port:        " << settings.LocalPort << endl;
   if(!server) {
      cout << "Remote Addresses:  " << settings.RemoteAddress[0] << endl;
      for(unsigned int i = 1;i < settings.RemoteAddresses;i++) {
         cout << "                   " << settings.RemoteAddress[i] << endl;
      }
      cout << "Remote Port:       " << settings.RemotePort << endl;
   }
   cout << endl << endl;


   // ====== Execute main program ===========================================
   if(server) {
      if(connectionLess) {
         cout << "Start connection-less socket server..." << endl;
         connectionLessSocketServer(&settings);
      }
      else {
         cout << "Start connection-oriented socket server..." << endl;
         connectionOrientedSocketServer(&settings);
      }
   }
   else {
      if(connectionLess) {
         cout << "Start connection-less socket client..." << endl;
         connectionLessSocketClient(&settings);
      }
      else {
         cout << "Start connection-oriented socket client..." << endl;
         connectionOrientedSocketClient(&settings);
      }
   }

   return 0;
}
