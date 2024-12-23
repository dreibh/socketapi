/*
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2025 by Thomas Dreibholz
 *
 * Realized in co-operation between
 * - Siemens AG
 * - University of Duisburg-Essen, Institute for Experimental Mathematics
 * - Münster University of Applied Sciences, Burgsteinfurt
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
 *          thomas.dreibholz@gmail.com
 *          tuexen@fh-muenster.de
 *
 * Purpose: Synchronizable Implementation
 *
 */


#include "tdsystem.h"
#include "synchronizable.h"
#include "thread.h"


#include <pthread.h>
#include <signal.h>



// ###### Constructor #######################################################
Synchronizable::Synchronizable(const char* name, const bool recursive)
{
   pthread_mutexattr_t mutexattr;
   pthread_mutexattr_init(&mutexattr);
   Recursive = recursive;
#ifndef NO_RECURSIVE_MUTEX
   if(Recursive) {
      // Initialize mutex for synchronization; mutextype has to be set to
      // PTHREAD_MUTEX_RECURSIVE to allow nested calls of synchronized() and
      // unsynchronized()!
      pthread_mutexattr_settype(&mutexattr,PTHREAD_MUTEX_RECURSIVE);
   }
#else
   // Keep track of recursion, if recursive mutexes are not supported.
   RecursionLevel = 0;
   Owner          = 0;
#endif
   pthread_mutex_init(&Mutex,&mutexattr);
   pthread_mutexattr_destroy(&mutexattr);
   setName(name);
}


// ###### Destructor ########################################################
Synchronizable::~Synchronizable()
{
   pthread_mutex_destroy(&Mutex);
}


// ###### Reinitialize ######################################################
void Synchronizable::resynchronize()
{
   pthread_mutex_destroy(&Mutex);
   pthread_mutexattr_t mutexattr;
   pthread_mutexattr_init(&mutexattr);
#ifndef NO_RECURSIVE_MUTEX
   if(Recursive) {
      pthread_mutexattr_settype(&mutexattr,PTHREAD_MUTEX_RECURSIVE);
   }
#else
   RecursionLevel = 0;
   Owner          = 0;
#endif
   pthread_mutex_init(&Mutex,&mutexattr);
   pthread_mutexattr_destroy(&mutexattr);
}
