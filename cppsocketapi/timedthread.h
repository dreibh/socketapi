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
 * Purpose: Timed Thread Implementation
 *
 */


#ifndef TIMEDTHREAD_H
#define TIMEDTHREAD_H


#include "tdsystem.h"
#include "multitimerthread.h"



/**
  * This abstract class realizes a timed thread based on MultiTimerThread.
  * The user of this class has to implement timerEvent(). Inaccurate system
  * timers are corrected by calling user's timerEvent() implementation multiple
  * times if necessary. This feature can be modified by setTimerCorrection
  * (Default is on at a maximum of 10 calls).
  *
  * @short   Timed Thread
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  * @see Thread
  */
class TimedThread : public SingleTimerThread
{
   // ====== Constructor/Destructor =========================================
   public:
   /**
     * Constructor. A new timed thread with a given interval will be created
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
   TimedThread(const card64   usec,
               const char*    name  = "TimedThread",
               const cardinal flags = TF_CancelDeferred);

   /**
     * Destructor.
     */
   ~TimedThread();


   // ====== Interval functions =============================================
   /**
     * Get timed thread's interval.
     *
     * @return Interval in microseconds.
     */
   inline card64 getInterval() const;

   /**
     * Set timed thread's interval.
     *
     * @param usec Interval in microseconds.
     */
   inline void setInterval(const card64 usec);

   /**
     * Like setInterval(), but disabling FastStart first. This method
     * can be used e.g. for a single shot timer.
     *
     * @param usec Time to next invokation (0 = immediately).
     * @param callLimit Call count limit (0 for infinite, default: 1).
     *
     * @see setInterval
     */
   inline void setNextAction(const card64 usec      = 0,
                             const card64 callLimit = 1);

   /**
     * Like setNextAction(), but the time stamp of the next invokation
     * is given as absolute time (microseconds since January 01, 1970).
     *
     * @param usec Time to next invokation (0 = immediately).
     * @param callLimit Call count limit (0 for infinite, default: 1).
     *
     * @see setInterval
     * @see setNextAction
     */
   inline void setNextActionAbs(const card64 timeStamp = 0,
                                const card64 callLimit = 1);

   /**
     * Get maxCorrection value for inaccurate system timer.
     *
     * @return true, if activated; false if not.
     *
     * @see setTimerCorrection
     */
   inline cardinal getTimerCorrection() const;

   /**
     * Set correction of inaccurate system timer to given value.
     * This on will cause the timerEvent() function to be called
     * a maximum of maxCorrection times, if the total number of calls is lower
     * than the calculated number of times the function should have been called.
     * If the number of correction calls is higher than maxCorrection,
     * *no* correction will be done!
     * Default is 0, which turns correction off.
     *
     * @param of true to activate correction; false to deactivate.
     */
   inline void setTimerCorrection(const cardinal maxCorrection = 0);

   /**
     * Leave timer correction loop: If the thread is in a timer correction
     * loop, the loop will be finished after the current timerEvent() call
     * returns.
     */
   inline void leaveCorrectionLoop();

   /**
     * Set fast start option: If false, the first call of timerEvent() will
     * be made *after* the given interval; otherwise it will be made immediately.
     * The default is true.
     *
     * @param on true, to set option; false otherwise.
     */
   inline void setFastStart(const bool on);

   /**
     * Get fast start option: If false, the first call of timerEvent() will
     * be made *after* the given interval; otherwise it will be made immediately.
     *
     * @return true, if option is set; false otherwise.
     */
   inline bool getFastStart() const;


   // ====== timerEvent() to be implemented by subclass =====================
   protected:
   /**
     * The virtual timerEvent() method, which contains the timed thread's
     * implementation. It has to be implemented by classes, which inherit
     * TimedThread.
     * This method is called regularly with the given interval.
     */
   virtual void timerEvent() = 0;


   // ====== Private data ===================================================
   private:
   void timerEvent(const cardinal timer);
};


#include "timedthread.icc"


#endif
