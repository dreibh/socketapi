/*
 *  $Id: multitimerthread.cc,v 1.2 2003/06/24 08:51:00 dreibh Exp $
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
 * Purpose: Multi Timer Thread Implementation
 *
 */


#ifndef MULTITIMERTHREAD_CC
#define MULTITIMERTHREAD_CC


#include "tdsystem.h"
#include "multitimerthread.h"
#include "tools.h"
#include "randomizer.h"


#include <sys/time.h>
#include <signal.h>



// ###### Constructor #######################################################
template<const cardinal Timers> MultiTimerThread<Timers>::MultiTimerThread(
                                   const char*    name,
                                   const cardinal flags)
   : Thread(name,flags)
{
   for(cardinal i = 0;i < Timers;i++) {
      Parameters[i].Running         = false;
      Parameters[i].Updated         = true;
      Parameters[i].FastStart       = true;
      Parameters[i].Interval        = (card64)-1;
      Parameters[i].TimerCorrection = 10;
      Parameters[i].CallLimit       = false;
      LeaveCorrectionLoop[i]        = false;
   }
}


// ###### Destructor ########################################################
template<const cardinal Timers> MultiTimerThread<Timers>::~MultiTimerThread()
{
}


// ###### Reimplementation of cancel() ######################################
template<const cardinal Timers> void MultiTimerThread<Timers>::cancel()
{
   Shutdown = true;
}


// ###### Reimplementation of stop() ########################################
template<const cardinal Timers> void* MultiTimerThread<Timers>::stop()
{
   Shutdown = true;
   join();
   return(NULL);
}


// ###### The thread's run() implementation #################################
template<const cardinal Timers> void MultiTimerThread<Timers>::run()
{
   Shutdown          = false;
   ParametersUpdated = true;

   Randomizer      random;
   TimerParameters parameters[Timers];
   card64          calls[Timers];
   card64          next[Timers];
   for(cardinal i = 0;i < Timers;i++) {
      parameters[i] = Parameters[i];
      parameters[i].Updated = true;
      next[i]  = 0;
      calls[i] = 0;
   }

   card64 now           = getMicroTime();
   card64 nextTimeStamp = now;
   while(!Shutdown) {
      synchronized();

      // ====== Check for parameter updates =================================
      if(ParametersUpdated) {
         ParametersUpdated = false;

         // ====== Update changed parameters ================================
         for(cardinal i = 0;i < Timers;i++) {
            if(Parameters[i].Updated) {
               parameters[i] = Parameters[i];
               Parameters[i].Updated = false;
               if(parameters[i].Running == true) {
                  if(parameters[i].FastStart == false) {
                     if((parameters[i].Interval != 0) && (!parameters[i].CallLimit)) {
                        next[i] = now + (random.random32() % parameters[i].Interval);
                     }
                     else {
                        next[i] = now + parameters[i].Interval;
                     }
                  }
                  else {
                     next[i] = now;
                  }
               }
               calls[i] = 0;
            }
         }
      }

      // ====== Calculate next timestamp ====================================
      now  = getMicroTime();
      nextTimeStamp = now + UpdateResolution;
      for(cardinal i = 0;i < Timers;i++) {
         if(parameters[i].Running == true) {
            nextTimeStamp = min(nextTimeStamp,next[i]);
         }
      }

      unsynchronized();

      // ====== Delay for calculated time ===================================
      if(nextTimeStamp >= now) {
         delay(nextTimeStamp - now,false);
      }


      // ====== Invoke timer event ==========================================
      now = getMicroTime();
      for(cardinal i = 0;i < Timers;i++) {
         if((parameters[i].Running == true) &&
            (now >= next[i])) {
            if((parameters[i].CallLimit > 0) && (calls[i] >= parameters[i].CallLimit)) {
               parameters[i].Running = false;
            }
            next[i] += parameters[i].Interval;
            timerEvent(i);
            calls[i]++;
         }
      }


      // ====== Do timer correction =========================================
      nextTimeStamp = now + UpdateResolution;
      for(cardinal i = 0;i < Timers;i++) {
         if(parameters[i].Running == true) {
            if(now >= next[i]) {
               if(now < next[i] + (parameters[i].TimerCorrection * parameters[i].Interval)) {
                  while(next[i] < now) {
                     if(LeaveCorrectionLoop[i]) {
                        LeaveCorrectionLoop[i] = false;
                        break;
                     }
                     next[i] += parameters[i].Interval;

                     timerEvent(i);
                     calls[i]++;

                     now = getMicroTime();
                     if((parameters[i].CallLimit > 0) && (calls[i] >= parameters[i].CallLimit)) {
                        parameters[i].Running = false;
                     }
                     if(next[i] >= now) {
                        break;
                     }
                  }
               }
               else {
                  now = getMicroTime();
                  next[i] = now + parameters[i].Interval;
               }
            }
         }
      }
   }
}


#endif
