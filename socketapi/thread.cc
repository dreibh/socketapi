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
 * Purpose: Thread Implementation
 *
 */



#include "tdsystem.h"
#include "thread.h"


#include <sys/time.h>



// Print starts/stops
// #define PRINT_STARTSTOP



// ###### Do compatibility check for SyncDebugger ###########################
#ifdef SYNCDEBUGGER
bool Thread::checkSyncDebugger()
{
   std::cerr << "*****************************************" << std::endl
             << "**** SyncDebugger mode is activated! ****" << std::endl
             << "*****************************************" << std::endl;
   std::cerr << "Testing SyncDebugger... ";
   Synchronizable test("CheckSyncDebuggerTest");
   test.synchronized();
   Thread::pthread_descr pthread = (Thread::pthread_descr)(test.Mutex.__m_owner);
   if(pthread->p_pid != getpid()) {
      std::cerr << "INTERNAL ERROR: _Thread::pthread_descr_struct definition is incompatible to your "
                   "libpthread version! Check linuxthreads/internals.h of your glibc "
                   "source package and update _Thread::pthread_descr_struct definition in Threads/thread.h!"
                << std::endl;
      std::cerr << "Your glibc version: " << __GLIBC__ << "." << __GLIBC_MINOR__ << "!" << std::endl;
      ::exit(1);
   }
   test.unsynchronized();
   std::cerr << "okay!" << std::endl;
   return(true);
}
#endif



// ###### Constructor #######################################################
Thread::Thread(const char* name, cardinal flags)
   : Synchronizable(name)
{
   PThread = 0;
   PID     = 0;
   Flags   = flags;

#ifdef SYNCDEBUGGER
   InternalPThreadPtr = NULL;
#endif
}


// ###### Destructor ########################################################
Thread::~Thread()
{
   stop();
}


// ###### Start the thread ##################################################
bool Thread::start(const char* name)
{
   int result = -1;
   synchronized();


   // ====== Create new thread ==============================================
   if(PThread == 0) {
      // ====== Initialize ==================================================
      PID = 0;
      if(name != NULL) {
         setName(name);
      }
#ifdef SYNCDEBUGGER
      InternalPThreadPtr = NULL;
#endif

      pthread_mutex_init(&StartupMutex,NULL);
      pthread_cond_init(&StartupCondition,NULL);
      pthread_mutex_lock(&StartupMutex);

      // ====== Create new thread ===========================================
      result = pthread_create(&PThread,NULL,&go,(void*)this);
      if(result == 0) {
         // Wait for startup. Note: The Mutex will always be locked. Unlocking
         // is only done within this call!
         pthread_cond_wait(&StartupCondition,&StartupMutex);

#ifdef PRINT_STARTSTOP
         cout << "Process #" << PID
#ifdef SYNCDEBUGGER
              << " \"" << MutexName << "\""
#endif
              << " started." << endl;
#endif
         ThreadSet.insert(this);
      }
      else {
#ifndef DISABLE_WARNINGS
         std::cerr << "WARNING: Thread::start() - Unable to create pthread!" << std::endl;
#endif
      }

      pthread_cond_destroy(&StartupCondition);
      pthread_mutex_unlock(&StartupMutex);
      pthread_mutex_destroy(&StartupMutex);
   }
   else {
#ifndef DISABLE_WARNINGS
      std::cerr << "WARNING: Thread::start() - Thread already started!" << std::endl;
#endif
   }

   unsynchronized();
   return(result == 0);
}


// ###### Start function for pthread ########################################
void* Thread::go(void* argument)
{
   Thread* thisThread = (Thread*)argument;

   // ====== Set parameters of the new thread ===============================
   int dummy;
   if(thisThread->Flags & TF_CancelDeferred) {
      pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,&dummy);
   }
   else {
      pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,&dummy);
   }

   // ====== Initialize PID and signalize successful startup ================
   thisThread->PID = getpid();
   pthread_mutex_lock(&thisThread->StartupMutex);
#ifdef SYNCDEBUGGER
   thisThread->InternalPThreadPtr = (Thread::pthread_descr)(thisThread->StartupMutex.__m_owner);
#endif
   pthread_cond_signal(&thisThread->StartupCondition);
   pthread_mutex_unlock(&thisThread->StartupMutex);

   // ====== Call user's run() function =====================================
   thisThread->run();
   return((void*)NULL);
}


// ###### Stop the thread ###################################################
void* Thread::stop()
{
   synchronized();
   if(running()) {
#ifdef PRINT_STARTSTOP
         cout << "Process #" << PID
#ifdef SYNCDEBUGGER
              << " \"" << MutexName << "\""
#endif
              << " stopping..." << endl;
#endif


      // ====== Stop thread =================================================
      pthread_cancel(PThread);
      unsynchronized();
      // Unsynchronizing is necessary, since the thread may not be locked to
      // shutdown gracefully!


      // ====== Wait for thread to shutdown =================================
      void* result = NULL;
      pthread_join(PThread,(void**)&result);
      PThread = 0;
#ifdef SYNCDEBUGGER
      InternalPThreadPtr = NULL;
#endif


      // ====== Resynchronize ===============================================
      // The stopped thread may hold its own mutex or waiting for this own
      // mutex. Therefore, it has to be reinitialized.
      /*
      printf("resynchronize: owner=%08x count=%d kind=%d lstatus=%x lspinlock=%d\n",
             (cardinal)Mutex.__m_owner,Mutex.__m_count,Mutex.__m_kind,
             (cardinal)Mutex.__m_lock.__status,(cardinal)Mutex.__m_lock.__spinlock);
      */
      resynchronize();


      // ====== Check for synchronization problems ==========================
      SyncSetLock.synchronized();
#ifdef SYNCDEBUGGER
      bool found = false;
      set<Synchronizable*>::iterator iterator = Synchronizable::MutexSet.begin();
      while(iterator != Synchronizable::MutexSet.end()) {
         Thread::pthread_descr pthread = (Thread::pthread_descr)((*iterator)->Mutex.__m_owner);
         bool nextLock = false;
         while(pthread != NULL) {

            // There seems to be an error in libpthread: Sometimes,
            // pthread is set to 0x1. In this case, a segfault occurs.
            if((pthread > (pthread_descr)0) && (pthread < (pthread_descr)10)) {
               pthread = NULL;
               continue;
            }

            if(pthread == InternalPThreadPtr) {
               if(!found) {
                  found = true;
                  cerr << "ERROR: Thread::stop() - Mutex problems detected after stopping process #"
                       << PID << " \"" << MutexName << "\"!" << endl;
               }
               if(!nextLock) {
                  cerr << "Mutex \"" << (*iterator)->getName()
                       << "\" is still owned by stopped process #" << PID
                       << " \"" << getName() << "\"!" << endl;
               }
               else {
                  cerr << "Mutex \"" << (*iterator)->getName()
                       << "\" is in *p_nextlock* list of stopped process #" << PID
                       << " \"" << getName() << "\"!" << endl;
               }
            }
            pthread = pthread->p_nextlock;
            nextLock = true;
         }
         iterator++;
      }
      if(found) {
         cerr << endl;
         iterator = Synchronizable::MutexSet.begin();
         while(iterator != Synchronizable::MutexSet.end()) {
            Thread::pthread_descr pthread = (Thread::pthread_descr)((*iterator)->Mutex.__m_owner);
            if(pthread != NULL) {
               if(pthread->p_pid != PID) {
                  cerr << "Mutex \"" << (*iterator)->getName()
                       << "\" is owned by process #" << pthread->p_pid;
                  set<Thread*>::iterator threadIterator = ThreadSet.begin();
                  while(threadIterator != ThreadSet.end()) {
                     if((*threadIterator)->PID == pthread->p_pid) {
                        cerr << " \"" << (*threadIterator)->MutexName << "\"";
                        break;
                     }
                     threadIterator++;
                  }
                  cerr << "." << endl;
               }
            }
            else {
               cerr << "Mutex \"" << (*iterator)->getName() << "\" is free." << endl;
            }
            iterator++;
         }
         cerr << endl << "Program HALT!" << endl;
         kill(getpid(),SYNCDEBUGGER_FAILURESIGNAL);
      }
#endif


      // ====== Remove thread from list =====================================
#ifdef PRINT_STARTSTOP
         cout << "Process #" << PID
#ifdef SYNCDEBUGGER
              << " \"" << MutexName << "\""
#endif
              << " stopped." << endl;
#endif
      ThreadSet.erase(this);
      PID = 0;


      SyncSetLock.unsynchronized();
      return(result);
   }
   unsynchronized();
   return(NULL);
}


// ###### Cancel thread #####################################################
void Thread::cancel()
{
   synchronized();
   if(running()) {
      pthread_cancel(PThread);
   }
   unsynchronized();
}


// ###### Test cancel #######################################################
void Thread::testCancel()
{
   pthread_testcancel();
}


// ###### Wait for end of thread ############################################
void* Thread::join()
{
   void* result = NULL;
   if(running()) {
      pthread_join(PThread,(void**)&result);
      PThread = 0;
   }
   return(result);
}


// ###### Sleep #############################################################
card64 Thread::delay(const card64 delayTimeout, const bool interruptable)
{
#if (SYSTEM != OS_Darwin)
   struct timespec timeout;
   struct timespec remaining;
   timeout.tv_sec  = delayTimeout / 1000000;
   timeout.tv_nsec = 1000 * (delayTimeout % 1000000);
   int result = nanosleep(&timeout,&remaining);
   if(!interruptable) {
      while((result == -1) && (errno == EINTR)) {
         timeout.tv_sec  = remaining.tv_sec;
         timeout.tv_nsec = remaining.tv_nsec;
         result = nanosleep(&timeout,&remaining);
      }
   }
   else if((result == -1) && (errno == EINTR)) {
      return(((card64)remaining.tv_sec * 1000000) +
             ((card64)remaining.tv_nsec / 1000));
   }
#else
   usleep((long)delayTimeout);
#endif
   return(0);
}
