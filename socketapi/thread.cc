/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2015 by Thomas Dreibholz
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
 * Purpose: Thread Implementation
 *
 */


#include "tdsystem.h"
#include "thread.h"


#include <sys/time.h>


// Print starts/stops
// #define PRINT_STARTSTOP


// ###### Constructor #######################################################
Thread::Thread(const char* name, cardinal flags)
   : Synchronizable(name)
{
   PThread = 0;
   PID     = 0;
   Flags   = flags;
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

      pthread_mutex_init(&StartupMutex,NULL);
      pthread_cond_init(&StartupCondition,NULL);
      pthread_mutex_lock(&StartupMutex);

      // ====== Create new thread ===========================================
      result = pthread_create(&PThread,NULL,&go,(void*)this);
      if(result == 0) {
         // Wait for startup. Note: The Mutex will always be locked. Unlocking
         // is only done within this call!
         pthread_cond_wait(&StartupCondition,&StartupMutex);
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
      // ====== Stop thread =================================================
      pthread_cancel(PThread);
      unsynchronized();
      // Unsynchronizing is necessary, since the thread may not be locked to
      // shutdown gracefully!

      // ====== Wait for thread to shutdown =================================
      void* result = NULL;
      pthread_join(PThread,(void**)&result);
      PThread = 0;

      // ====== Resynchronize ===============================================
      // The stopped thread may hold its own mutex or waiting for this own
      // mutex. Therefore, it has to be reinitialized.
      /*
      printf("resynchronize: owner=%08x count=%d kind=%d lstatus=%x lspinlock=%d\n",
             (cardinal)Mutex.__m_owner,Mutex.__m_count,Mutex.__m_kind,
             (cardinal)Mutex.__m_lock.__status,(cardinal)Mutex.__m_lock.__spinlock);
      */
      resynchronize();
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
