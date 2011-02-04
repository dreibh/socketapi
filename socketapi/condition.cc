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
 * Purpose: Condition
 *
 */


#include "tdsystem.h"
#include "condition.h"
#include "thread.h"

#include <sys/time.h>


// #define PRINT_SIGNAL



// ###### Constructor #######################################################
Condition::Condition(const char* name,
                     Condition*  parentCondition,
                     const bool  recursive)
   : Synchronizable(name,recursive)
{
   Valid = true;
   addParent(parentCondition);
   pthread_cond_init(&ConditionVariable,NULL);
   Fired = false;
}


// ###### Destructor ########################################################
Condition::~Condition()
{
   Valid = false;
   if(pthread_cond_destroy(&ConditionVariable) != 0) {
#ifndef DISABLE_WARNINGS
      std::cerr << "ERROR: Condition::~Condition() - "
                   "Another thread is still waiting for this condition!" << std::endl;
      std::cerr << "Condition name is \"" << MutexName << "\"." << std::endl;
      abort();
#endif
   }
}


// ###### Add parent condition ##############################################
void Condition::addParent(Condition* parentCondition)
{
   if(parentCondition != NULL) {
      synchronized();
      ParentSet.insert(parentCondition);
      if(Fired) {
         parentCondition->broadcast();
      }
      unsynchronized();
   }
}


// ###### Remove parent condition ###########################################
void Condition::removeParent(Condition* parentCondition)
{
   if(parentCondition != NULL) {
      synchronized();
      ParentSet.erase(parentCondition);
      unsynchronized();
   }
}


// ###### Wait for condition ################################################
void Condition::wait()
{
   while(!timedWait(3600000000ULL)) {
      sched_yield();
   }
}


// ###### Fire condition ####################################################
void Condition::signal()
{
   synchronized();
   Fired = true;
   pthread_cond_signal(&ConditionVariable);

#ifdef PRINT_SIGNAL
   std::cout << "signal: " << getName() << std::endl;
#endif

   std::set<Condition*>::iterator iterator = ParentSet.begin();
   while(iterator != ParentSet.end()) {
      (*iterator)->signal();
      iterator++;
   }

   unsynchronized();
}


// ###### Broadcast condition ###############################################
void Condition::broadcast()
{
   synchronized();
   Fired = true;

   pthread_cond_broadcast(&ConditionVariable);

#ifdef PRINT_SIGNAL
   std::cout << "broadcast: " << getName() << std::endl;
#endif

   std::set<Condition*>::iterator iterator = ParentSet.begin();
   while(iterator != ParentSet.end()) {
      (*iterator)->broadcast();
      iterator++;
   }

   unsynchronized();
}


// ###### Wait for condition with timeout ###################################
bool Condition::timedWait(const card64 microseconds)
{
   cardinal oldstate = Thread::setCancelState(Thread::TCS_CancelDisabled);
   synchronized();

   // ====== Initialize timeout settings ====================================
   timeval  now;
   timespec timeout;
   gettimeofday(&now,NULL);
   timeout.tv_sec  = now.tv_sec + (long)(microseconds / 1000000);
   timeout.tv_nsec = (now.tv_usec + (long)(microseconds % 1000000)) * 1000;
   if(timeout.tv_nsec >= 1000000000) {
      timeout.tv_sec++;
      timeout.tv_nsec -= 1000000000;
   }

   // ====== Wait ===========================================================
   int result = EINTR;
   if(Fired) {
      result = 0;
   }
   else {
      result = pthread_cond_timedwait(&ConditionVariable,&Mutex,&timeout);
      while(result == EINTR) {
         unsynchronized();
         Thread::setCancelState(oldstate);
         if(oldstate == Thread::TCS_CancelEnabled) {
            pthread_testcancel();
         }
         oldstate = Thread::setCancelState(Thread::TCS_CancelDisabled);
         synchronized();

         if(Fired) {
            result = 0;
         }
         else {
            result = pthread_cond_timedwait(&ConditionVariable,&Mutex,&timeout);
         }
      }
   }
   if(result == 0) {
      Fired = false;
   }

   unsynchronized();
   Thread::setCancelState(oldstate);
   if(oldstate == Thread::TCS_CancelEnabled) {
      pthread_testcancel();
   }
   return(result == 0);
}
