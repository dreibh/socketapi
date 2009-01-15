/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 2005-2009 by Thomas Dreibholz
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
 * Purpose: Ring Buffer Implementation
 *
 */


#include "tdsystem.h"
#include "ringbuffer.h"


// Debug modes
// #define DEBUG
// #define DEBUG_VERBOSE



// ###### Constructor #######################################################
RingBuffer::RingBuffer()
   : Condition("RingBufferCondition", NULL, true)
{
   Buffer = NULL;
}


// ###### Destructor ########################################################
RingBuffer::~RingBuffer()
{
   if(Buffer != NULL) {
      delete Buffer;
      Buffer = NULL;
   }
}


// ###### Initialize buffer #################################################
bool RingBuffer::init(const cardinal bytes)
{
   synchronized();
   flush();
   if(Buffer != NULL) {
      delete Buffer;
   }
   Buffer = new char[bytes + 16];
   Buffer[bytes]=0x00;
   bool ok;
   if(Buffer == NULL) {
      ok         = false;
      BufferSize = 0;
   }
   else {
      ok         = true;
      BufferSize = bytes;
   }
   unsynchronized();
   return(ok);
}


// ###### Flush buffer ######################################################
void RingBuffer::flush()
{
   synchronized();
   WriteStart   = 0;
   WriteEnd     = 0;
   BytesStored  = 0;
   WriteStart   = 0;
   unsynchronized();
   broadcast();
}


// ###### Write bytes into buffer ###########################################
ssize_t RingBuffer::write(char*        data,
                          const size_t length)
{
   synchronized();

   cardinal copy1 = 0;
   cardinal copy2 = 0;
   if(BytesStored < BufferSize) {
      if(WriteEnd >= WriteStart) {
         copy1 =std:: min(length, BufferSize - WriteEnd);
         memcpy(&Buffer[WriteEnd],data,copy1);
         WriteEnd += copy1;
         if(WriteEnd >= BufferSize) {
            WriteEnd = 0;
         }
#ifdef DEBUG_VERBOSE
         printf("write #1: we=%d ws=%d   c1=%d\n",WriteEnd,WriteStart,copy1);
#endif
      }
      copy2 = std::min(length - copy1, WriteStart);
      if(copy2 > 0) {
         memcpy(&Buffer[WriteEnd],&data[copy1],copy2);
         WriteEnd += copy2;
#ifdef DEBUG_VERBOSE
         printf("write #2: we=%d ws=%d   c2=%d\n",WriteEnd,WriteStart,copy2);
#endif
      }

      BytesStored += copy1 + copy2;
      if((copy1 != 0) || (copy2 != 0)) {
         broadcast();
      }
   }

#ifdef DEBUG
   printf("write: we=%d ws=%d   c1=%d c2=%d  BytesStored=%d/%d\n",
          WriteEnd,WriteStart,copy1,copy2,BytesStored,BufferSize);
#endif

   unsynchronized();
   return(copy1 + copy2);
}


// ###### Read bytes from buffer ############################################
ssize_t RingBuffer::read(char*        data,
                         const size_t length)
{
   synchronized();

   cardinal copy1 = 0;
   cardinal copy2 = 0;
   if(BytesStored > 0) {
      if(WriteStart >= WriteEnd) {
         copy1 = std::min(length, BufferSize - WriteStart);
         memcpy(data,&Buffer[WriteStart],copy1);
         memset(&Buffer[WriteStart],'-',copy1);
         WriteStart += copy1;
         if(WriteStart >= BufferSize) {
            WriteStart = 0;
         }
#ifdef DEBUG_VERBOSE
         printf("read #1: we=%d ws=%d   c1=%d\n",WriteEnd,WriteStart,copy1);
#endif
      }
      copy2 = std::min(length - copy1, WriteEnd - WriteStart);
      if(copy2 > 0) {
         memcpy(&data[copy1],&Buffer[WriteStart],copy2);
#ifdef DEBUG
         memset(&Buffer[WriteStart],'-',copy2);
#endif
         WriteStart += copy2;
#ifdef DEBUG
         printf("read #2: we=%d ws=%d   c2=%d\n",WriteEnd,WriteStart,copy2);
#endif
      }

      if(copy1 + copy2 > BytesStored) {
         std::cerr << "INTERNAL ERROR: RingBuffer::read() - Corrupt structures!" << std::endl;
         exit(1);
      }

      BytesStored -= copy1 + copy2;
   }

#ifdef DEBUG_VERBOSE
   printf("read: we=%d ws=%d   c1=%d c2=%d  BytesStored=%d/%d\n",
          WriteEnd,WriteStart,copy1,copy2,BytesStored,BufferSize);
#endif

   unsynchronized();
   return(copy1 + copy2);
}
