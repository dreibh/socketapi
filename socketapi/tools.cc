/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2006 by Thomas Dreibholz
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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de which should be
 * used for any discussion related to this implementation.
 *
 * Contact: discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *          tuexen@fh-muenster.de
 *
 * Purpose: Tools Implementation
 *
 */

#include "tdsystem.h"
#include "tdstrings.h"
#include "tools.h"


#include <pwd.h>
#include <time.h>
#include <sys/utsname.h>



// Print new and delete calls (if libefence is used)
#define PRINT_ALLOCATIONS



// ###### Calculate packets per second ######################################
cardinal calculatePacketsPerSecond(const cardinal payloadBytesPerSecond,
                                   const cardinal framesPerSecond,
                                   const cardinal maxPacketSize,
                                   const cardinal headerLength)
{
   const cardinal frameSize = (cardinal)ceil((double)payloadBytesPerSecond /
                                         (double)framesPerSecond);
   return((cardinal)ceil(frameSize / (double)(maxPacketSize - headerLength)) *
          (cardinal)ceil((double)payloadBytesPerSecond / (double)frameSize));
}


// ###### Calculate bytes per second ########################################
cardinal calculateBytesPerSecond(const cardinal payloadBytesPerSecond,
                                 const cardinal framesPerSecond,
                                 const cardinal maxPacketSize,
                                 const cardinal headerLength)
{
   const cardinal frameSize = (cardinal)ceil((double)payloadBytesPerSecond /
                                         (double)framesPerSecond);
   const cardinal headerBytesPerFrame =
      (cardinal)ceil(frameSize / (double)(maxPacketSize - headerLength)) *
      headerLength;

   return((headerBytesPerFrame + frameSize) *
            (cardinal)ceil((double)payloadBytesPerSecond / frameSize));
}


// ###### Scan URL ##########################################################
bool scanURL(const String& location,
             String&       protocol,
             String&       host,
             String&       path)
{
   String url = location;

   // ====== Get protocol ===================================================
   integer p1 = url.find("://");
   if(p1 < 0) {
      if(protocol.isNull()) {
         return(false);
      }
      p1 = 0;
   }
   else {
      protocol = url.left(p1);
      p1 += 3;
   }

   // ====== Get host =======================================================
   url = url.mid(p1);
   integer p2 = url.index('/');
   if(p2 < 0) {
      return(false);
   }
   host = url.left(p2);

   // ====== Get path =======================================================
   path = url.mid(p2 + 1);

   protocol = protocol.toLower();
   host     = host.toLower();
   return(true);
}


// ###### Print time stamp ##################################################
void printTimeStamp(std::ostream& os)
{
   char str[128];
   const card64 microTime = getMicroTime();
   const time_t timeStamp = microTime / 1000000;
   const struct tm *timeptr = localtime(&timeStamp);
   strftime((char*)&str,sizeof(str),"%d-%b-%Y %H:%M:%S",timeptr);
   os << str;
   snprintf((char*)&str,sizeof(str),
            ".%04d: ",(cardinal)(microTime % 1000000) / 100);
   os << str;
}


// ###### Get user name #####################################################
bool getUserName(char*        str,
                 const size_t size,
                 const bool   realName,
                 uid_t        uid)
{
#if (SYSTEM == OS_Linux)
   char    buffer[BUFSIZ];
   passwd  pwent;
   passwd* result;
   int error = getpwuid_r(uid,&pwent,(char*)&buffer,sizeof(buffer),&result);
   if(error != 0) {
      result = NULL;
   }
#else
   passwd* result = getpwuid(uid);
#endif

   if(result != NULL) {
      if(!realName) {
         snprintf(str,size,"%s",result->pw_name);
      }
      else {
         snprintf(str,size,"%s",result->pw_gecos);
      }
      return(true);
   }
   str[0] = 0x00;
   return(false);
}


#ifdef USE_EFENCE


// VERY IMPORTANT:
// 1. malloc() and free() have to be synchronized in order to get correct
//    results!!!
// 2. new() and delete() are replaced by compatible versions.
//    This does *not* include the C-functions malloc() and free()!
//    C sources using malloc() and free() require additional care!


#include "thread.h"


// Allocation balance to detect memory leaks.
static int64 AllocationBalance = 0;


// ###### Operator new() replacement for libefence usage ####################
void* operator new(size_t size) throw (std::bad_alloc)
{
   const cardinal oldstate = Thread::setCancelState(Thread::TCS_CancelDisabled);
   Thread::MemoryManagementLock.synchronized();

   if(size <= 0) {
      size = 1;
#ifndef DISABLE_WARNINGS
      cerr << "WARNING: operator new() - Size is 0!" << endl;
#endif
   }
   void* ptr = malloc(size + 4);
   if(ptr != NULL) {
      card32* ptr32 = (card32*)ptr;
      *ptr32 = (card32)size;
      ptr = (void*)((long)ptr + (long)4);
      AllocationBalance += (int64)size;

#ifdef PRINT_ALLOCATIONS
   cout << "Allocated " << size << " bytes -> Balance is "
        << AllocationBalance << "." << endl;
#endif
   }

   Thread::MemoryManagementLock.unsynchronized();
   Thread::setCancelState(oldstate);
   return(ptr);
}


// ###### Operator delete() replacement for libefence usage #################
void operator delete(void* ptr) throw ()
{
   const cardinal oldstate = Thread::setCancelState(Thread::TCS_CancelDisabled);
   Thread::MemoryManagementLock.synchronized();

   card32* ptr32 = (card32*)((long)ptr - (long)4);
   card32 size = *ptr32;
   AllocationBalance -= (int64)size;
   free(ptr32);

#ifdef PRINT_ALLOCATIONS
   cout << "Freed " << size << " bytes -> Balance is "
        << AllocationBalance << "." << endl;
#endif

   Thread::MemoryManagementLock.unsynchronized();
   Thread::setCancelState(oldstate);
}


#endif
