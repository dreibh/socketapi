/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 2005-2017 by Thomas Dreibholz
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
 * Purpose: Multi Timer Thread Implementation
 *
 */


#ifndef MULTITIMERTHREAD_H
#define MULTITIMERTHREAD_H


#include "tdsystem.h"
#include "tools.h"
#include "thread.h"



/**
  * This abstract class realizes a timer thread with multiple timers,
  * based on Thread. The user
  * of this class has to implement timerEvent(). Inaccurate system timers
  * are corrected by calling user's timerEvent() implementation multiple
  * times if necessary. This feature can be modified by setTimerCorrection
  * (Default is on at a maximum of 10 calls).
  *
  * @short   Multi Timer Thread
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  * @see Thread
  */
template<const cardinal Timers> class MultiTimerThread : public Thread
{
   // ====== Constructor/Destructor =========================================
   public:
   /**
     * Constructor. A new multitimer thread with a given interval will be created
     * but *not* started! To start the new thread, call start(). The
     * interval gives the time for the interval in microseconds, the virtual
     * function timerEvent() is called.
     * The default timer correction is set to 10. See setTimerCorrection() for
     * more information on timer correction.
     * The first call of timerEvent() will be made immediately, if the fast
     * start option is set (default). Otherwise it will be made after the
     * given interval.
     *
     * @param usec Interval in microseconds.
     * @param name Thread name.
     * @param flags Thread flags.
     *
     * @see Thread#start
     * @see timerEvent
     * @see Thread#Thread
     * @see setTimerCorrection
     * @see setFastStart
     */
   MultiTimerThread(const char*    name  = "MultiTimerThread",
                    const cardinal flags = TF_CancelDeferred);

   /**
     * Destructor.
     */
   ~MultiTimerThread();


   // ====== Interval functions =============================================
   /**
     * Get timer interval.
     *
     * @param timer Timer number.
     * @return Interval in microseconds.
     */
   inline card64 getInterval(const cardinal timer);

   /**
     * Set timer interval. Note, that the first timerEvent() call will
     * be immediately, is FastStart mode is set, see also setFastStart().
     * For single shot timers, you probably have to call setFastStart(nr,0)
     * first!
     *
     * @param timer Timer number.
     * @param usec Interval in microseconds (0 to deactivate timer).
     * @param callLimit Call count limit (0 for infinite).
     *
     * @see setFastStart
     */
   inline void setInterval(const cardinal timer,
                           const card64   usec,
                           const card64   callLimit = 0);


   /**
     * Like setInterval(), but disabling FastStart first. This method
     * can be used e.g. for single shot timers.
     *
     * @param timer Timer number.
     * @param usec Time to next invokation (0 = immediately).
     * @param callLimit Call count limit (0 for infinite, default: 1).
     *
     * @see setInterval
     */
   inline void setNextAction(const cardinal timer,
                             const card64   usec      = 0,
                             const card64   callLimit = 1);

   /**
     * Like setNextAction(), but the time stamp of the next invokation
     * is given as absolute time (microseconds since January 01, 1970).
     *
     * @param timer Timer number.
     * @param usec Time to next invokation (0 = immediately).
     * @param callLimit Call count limit (0 for infinite, default: 1).
     *
     * @see setInterval
     * @see setNextAction
     */
   inline void setNextActionAbs(const cardinal timer,
                                const card64   timeStamp = 0,
                                const card64   callLimit = 1);

   /**
     * Get maximum correction value for inaccurate system timer.
     *
     * @param timer Timer number.
     * @return true, if activated; false if not.
     *
     * @see setTimerCorrection
     */
   inline cardinal getTimerCorrection(const cardinal timer);

   /**
     * Set correction of inaccurate system timer to given value.
     * This on will cause the timerEvent() function to be called
     * a maximum of maxCorrection times, if the total number of calls is lower
     * than the calculated number of times the function should have been called.
     * If the number of correction calls is higher than maxCorrection,
     * *no* correction will be done!
     * Default is 0, which turns correction off.
     *
     * @param timer Timer number.
     * @param of true to activate correction; false to deactivate.
     */
   inline void setTimerCorrection(const cardinal timer,
                                  const cardinal maxCorrection = 0);

   /**
     * Leave timer correction loop: If the thread is in a timer correction
     * loop, the loop will be finished after the current timerEvent() call
     * returns.
     *
     * @param timer Timer number.
     */
   inline void leaveCorrectionLoop(const cardinal timer);

   /**
     * Set fast start option: If false, the first call of timerEvent() will
     * be made *after* the given interval; otherwise it will be made immediately.
     * The default is true.
     *
     * @param timer Timer number.
     * @param on true, to set option; false otherwise.
     */
   inline void setFastStart(const cardinal timer, const bool on);

   /**
     * Get fast start option: If false, the first call of timerEvent() will
     * be made *after* the given interval; otherwise it will be made immediately.
     *
     * @param timer Timer number.
     * @return true, if option is set; false otherwise.
     */
   inline bool getFastStart(const cardinal timer) const;

   /**
     * Reimplementation of Thread's cancel() method.
     *
     * @see Thread#cancel
     */
   void cancel();

   /**
     * Reimplementation of Thread's stop() method.
     *
     * @see Thread#stop
     */
   void* stop();


   // ====== timerEvent() to be implemented by subclass =====================
   protected:
   /**
     * The virtual timerEvent() method, which contains the multitimer thread's
     * implementation. It has to be implemented by classes, which inherit
     * MultiTimerThread.
     * This method is called regularly with the given interval.
     */
   virtual void timerEvent(const cardinal timer) = 0;


   // ====== Private data ===================================================
   private:
   void run();
   inline bool isShuttingDown();

   struct TimerParameters {
      card64   Interval;
      card64   CallLimit;
      cardinal TimerCorrection;
      bool     FastStart;
      bool     Running;
      bool     Updated;
      bool     LeaveCorrectionLoop;
   };

   TimerParameters     Parameters[Timers];
   bool                ParametersUpdated;
   bool                Shutdown;
   bool                LeaveCorrectionLoop[Timers];

   static const card64 UpdateResolution = 100000;
};


typedef MultiTimerThread<1> SingleTimerThread;


#include "multitimerthread.icc"


#endif
