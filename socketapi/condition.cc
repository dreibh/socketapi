/*
 *  $Id: condition.cc,v 1.1 2003/05/15 11:35:50 dreibh Exp $
 *
 * SCTP implementation according to RFC 2960.
 * Copyright (C) 1999-2002 by Thomas Dreibholz
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
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
 *
 * Purpose: Condition
 *
 */


#include "tdsystem.h"
#include "condition.h"
#include "thread.h"


#include <sys/time.h>



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
      cerr << "ERROR: Condition::~Condition() - "
              "Another thread is still waiting for this condition!" << endl;
      cerr << "Condition name is \"" << MutexName << "\"." << endl;
      exit(1);
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
