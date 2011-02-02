/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2011 by Thomas Dreibholz
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
 * Purpose: Extended Socket Descriptor
 *
 */


#ifndef EXTSOCKETDESCRIPTOR_H
#define EXTSOCKETDESCRIPTOR_H


#include "tdsystem.h"
#include "sctpsocket.h"
#include "sctpassociation.h"



// ###### ExtSocketDescriptor structure #####################################
struct ExtSocketDescriptor
{
   enum ExtSocketDescriptorTypes {
      ESDT_Invalid = 0,
      ESDT_System  = 1,
      ESDT_SCTP    = 2
   };

   unsigned int Type;

   union ExtSocketDescriptorUnion {
      int SystemSocketID;
      struct SCTP {
         int              Domain;
         int              Type;
         SCTPSocket*      SCTPSocketPtr;
         SCTPAssociation* SCTPAssociationPtr;
         int              Flags;
         sctp_initmsg     InitMsg;
         linger           Linger;
         bool             ConnectionOriented;
      } SCTPSocketDesc;
   } Socket;
};


// ###### Class for ExtSocketDescriptor management ##########################
class ExtSocketDescriptorMaster
{
   protected:
   ExtSocketDescriptorMaster();

   public:
   ~ExtSocketDescriptorMaster();

   public:
   inline static ExtSocketDescriptor* getSocket(const int id);
   static int setSocket(const ExtSocketDescriptor& newSocket);
   static const unsigned int MaxSockets = FD_SETSIZE;

   private:
   static ExtSocketDescriptorMaster MasterInstance;
   static ExtSocketDescriptor Sockets[MaxSockets];
};


#endif
