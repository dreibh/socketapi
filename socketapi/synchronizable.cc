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
 * Purpose: Synchronizable Implementation
 *
 */


#include "tdsystem.h"
#include "synchronizable.h"
#include "thread.h"


#include <pthread.h>
#include <signal.h>


#ifdef SYNCDEBUGGER
#undef synchronized
#undef unsynchronized
#undef synchronizedTry
#undef resynchronize
#endif



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

#ifdef SYNCDEBUGGER
#ifdef SYNCDEBUGGER_PRINTING
   std::cout << "Created mutex \"" << MutexName << "\"." << std::endl;
#endif
   const cardinal oldstate = Thread::setCancelState(Thread::TCS_CancelDisabled);
   Thread::SyncSetLock.synchronized();
   MutexSet.insert(this);
   Thread::SyncSetLock.unsynchronized();
   Thread::setCancelState(oldstate);
#endif
}


// ###### Destructor ########################################################
Synchronizable::~Synchronizable()
{
#ifdef SYNCDEBUGGER
#ifdef SYNCDEBUGGER_PRINTING
   std::cout << "Deleted mutex \"" << MutexName << "\"." << std::endl;
#endif
   const cardinal oldstate = Thread::setCancelState(Thread::TCS_CancelDisabled);
   Thread::setCancelState(oldstate);
   Thread::SyncSetLock.synchronized();
   MutexSet.erase(this);
   Thread::SyncSetLock.unsynchronized();
   Thread::setCancelState(oldstate);
#endif

   pthread_mutex_destroy(&Mutex);
}


// ###### Reinitialize ######################################################
void Synchronizable::resynchronize()
{
#ifdef SYNCDEBUGGER_PRINTING
   std::cerr << "<R>";
#endif
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


// ###### Debug version of synchronized #####################################
void Synchronizable::synchronized_debug(const char* file, const cardinal line)
{
#ifdef SYNCDEBUGGER_VERBOSE_PRINTING
   std::cerr << "#" << getpid() << ": "
             << file << ", line " << line << ": " << "\"" << MutexName << "\" synchronize..." << std::endl;
#endif


#ifdef SYNCDEBUGGER
   static bool alreadyFailed = false;
   const card64 start = getMicroTime();
   bool ok = Synchronizable::synchronizedTry();
   while(ok == false) {
      const card64 now = getMicroTime();
      if(now - start >= SYNCDEBUGGER_TIMEOUT) {
         if(!alreadyFailed) {
            alreadyFailed = true;

            // ====== Print debug information ===============================
            const Thread::pthread_descr pthread = (Thread::pthread_descr)Mutex.__m_owner;
            std::cerr << "ERROR: Synchronizable::synchronized_debug() - Mutex problems detected!" << std::endl;
            printTimeStamp(cerr);
            std::cerr << "Process #" << getpid();
            set<Thread*>::iterator iterator = Thread::ThreadSet.begin();
            while(iterator != Thread::ThreadSet.end()) {
               if((*iterator)->PID == getpid()) {
                  std::cerr << " \"" << (*iterator)->getName() << "\"";
                  break;
               }
               iterator++;
            }
            std::cerr << ": " << file << ", line " << line << ": " << " synchronize failed -> TIMEOUT!" << std::endl;
            std::cerr << "Name of wanted mutex is \"" << MutexName << "\"." << std::endl;
            std::cerr << "Mutex is locked by process #";
            std::cerr << pthread->p_pid;
            iterator = Thread::ThreadSet.begin();
            while(iterator != Thread::ThreadSet.end()) {
               if((*iterator)->PID == pthread->p_pid) {
                  std::cerr << " \"" << (*iterator)->getName() << "\"";
                  break;
               }
               iterator++;
            }
            std::cerr << "." << std::endl;

            if(strcmp(MutexName,"SyncSetLock")) {
               Thread::SyncSetLock.synchronized();
            }
            std::cerr << std::endl;
            set<Synchronizable*>::iterator mutexIterator = MutexSet.begin();
            while(mutexIterator != MutexSet.end()) {
               Thread::pthread_descr pthread = (Thread::pthread_descr)((*mutexIterator)->Mutex.__m_owner);
               if(pthread != NULL) {
                  std::cerr << "Mutex \"" << (*mutexIterator)->getName()
                            << "\" is owned by process #" << pthread->p_pid;
                  set<Thread*>::iterator threadIterator = Thread::ThreadSet.begin();
                  while(threadIterator != Thread::ThreadSet.end()) {
                     if((*threadIterator)->PID == pthread->p_pid) {
                        std::cerr << " \"" << (*threadIterator)->MutexName << "\"";
                        break;
                     }
                     threadIterator++;
                  }
                  std::cerr << "." << std::endl;
               }
               else {
                  std::cerr << "Mutex \"" << (*mutexIterator)->getName() << "\" is free." << std::endl;
               }
               mutexIterator++;
            }
            if(strcmp(MutexName,"SyncSetLock")) {
               Thread::SyncSetLock.unsynchronized();
            }

            // ====== Kill program ==========================================
            std::cerr << std::endl << "Program HALT!" << std::endl;
            kill(getpid(),SYNCDEBUGGER_FAILURESIGNAL);
         }
         else {
            // This will deadlock...
            for(;;) {
               Thread::delay(1000000000,false);
            }
         }
      }
      Thread::yield();
      ok = Synchronizable::synchronizedTry();
   }

#else

   synchronized();

#endif

#ifdef SYNCDEBUGGER_VERBOSE_PRINTING
   std::cerr << "#" << getpid() << ": "
        << file << ", line " << line << ": " << "\"" << MutexName << "\" synchronized!" << std::endl;
#endif
}


// ###### Debug version of unsynchronized ###################################
void Synchronizable::unsynchronized_debug(const char* file, const cardinal line)
{
#ifdef SYNCDEBUGGER_VERBOSE_PRINTING
   std::cerr << "#" << getpid() << ": "
             << file << ", line " << line << ": " << "\"" << MutexName << "\" unsynchronize..." << std::endl;
#endif

   Synchronizable::unsynchronized();

#ifdef SYNCDEBUGGER_VERBOSE_PRINTING
   std::cerr << "#" << getpid() << ": "
             << file << ", line " << line << ": " << "\"" << MutexName << "\" unsynchronized" << std::endl;
#endif
}


// ###### Debug version of synchronized #####################################
bool Synchronizable::synchronizedTry_debug(const char* file, const cardinal line)
{
#ifdef SYNCDEBUGGER_VERBOSE_PRINTING
   std::cerr << "#" << getpid() << ": "
             << file << ", line " << line << ": " << "\"" << MutexName << "\" try synchronize..." << std::endl;
#endif

   bool ok = Synchronizable::synchronizedTry();

#ifdef SYNCDEBUGGER_VERBOSE_PRINTING
   std::cerr << "#" << getpid() << ": " << file << ", line " << line << ": ";
   if(ok) {
      std::cerr << "\"" << MutexName << "\" synchronized!" << std::endl;
   }
   else {
      std::cerr << "\"" << MutexName << "\" locked -> not synchronized." << std::endl;
   }
#endif
   return(ok);
}


// ###### Debug version of resynchronize ####################################
void Synchronizable::resynchronize_debug(const char* file, const cardinal line)
{
#ifdef SYNCDEBUGGER_VERBOSE_PRINTING
   std::cerr << "#" << getpid() << ": "
             << file << ", line " << line << ": " << "\"" << MutexName << "\" re-synchronize..." << std::endl;
#endif

   Synchronizable::resynchronize();

#ifdef SYNCDEBUGGER_VERBOSE_PRINTING
   std::cerr << "#" << getpid() << ": "
             << file << ", line " << line << ": " << "\"" << MutexName << "\" re-synchronized!" << std::endl;
#endif
}
