/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2012 by Thomas Dreibholz
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


#ifndef THREAD_H
#define THREAD_H


#include "tdsystem.h"
#include "synchronizable.h"
#include "condition.h"


#include <pthread.h>
#include <set>



/**
  * This abstract class realizes threads based on Linux's pthreads. The user
  * of this class has to implement run().
  * Synchronization is implemented by inheriting Synchronizable.
  * IMPORTANT: Do *not* use Thread methods within async signal handlers.
  * This may cause deadlocks. See PThread's pthread_mutex_lock man-page,
  * section "Async Signal Safety" for more information!
  *
  * @short   Thread
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  * @see Synchronizable
  */
class Thread : public Synchronizable
{
   // ====== Constants ======================================================
   public:
   static const cardinal TF_CancelAsynchronous = 0;
   static const cardinal TF_CancelDeferred     = (1 << 0);
   static const cardinal TCS_CancelEnabled  = PTHREAD_CANCEL_ENABLE;
   static const cardinal TCS_CancelDisabled = PTHREAD_CANCEL_DISABLE;
   static const cardinal TCS_CancelDeferred = PTHREAD_CANCEL_DEFERRED;


   // ====== Constructor/Destructor =========================================
   public:
   /**
     * Constructor. A new thread will be created but *not* started!
     * To start the new thread, call start().
     *
     * @param name Thread name.
     * @param flags  Flags for the thread to be created.
     *
     * @see start
     */
   Thread(const char*    name  = "Thread",
          const cardinal flags = TF_CancelDeferred);

   /**
     * Destructor. The thread will be stopped (if running) and deleted.
     */
   virtual ~Thread();


   // ====== Status =========================================================
   /**
     * Check, if the thread is running.
     *
     * @return true, if the thread is running, false otherwise
     */
   inline bool running() const;

   /**
     * Get thread's PID.
     */
   inline pid_t getPID() const;


   // ====== Delay ==========================================================
   /**
     * Delay execution of current thread for a given timeout.
     * This function uses nanosleep(), so no signals are affected.
     *
     * @param delayTime Timeout in microseconds.
     * @param interruptable true, if delay may be interrupted by signals; false otherwise.
     * @return Remaining delay, if interrupted; 0 otherwise.
     */
   static card64 delay(const card64 delayTimeout, const bool interruptable = false);


   // ====== Thread control =================================================
   /**
     * Start the thread, if not already started.
     *
     * @param name Thread name (NULL for default).
     * @return true, if the thread has been started; false, if not.
     */
   virtual bool start(const char* name = NULL);

   /**
     * Stop the thread, if not already stopped.
     * If the thread flag ThreadCancelAsynchronous is set, it will be stopped
     * immediately. If the flag ThreadCancelDeferred is set, it will be stopped
     * when a cancellation point is reached (-> see pthreads documentation).
     * testCancel() is such a cancellation point.
     *
     * @see testCancel
     * @return Return value from stopped thread.
     */
   virtual void* stop();

   /**
     * Wait for the thread to be finished.
     *
     * @return Return value from joined thread.
     */
   void* join();

   /**
     * Cancel the thread.
     */
   virtual void cancel();

   /**
     * Enable or disable cancelability of calling thread. The previous state
     * is returned.
     * Important note: The result may include additional state information,
     * depending on the operating system. This state can be restored by giving
     * this complete information to a setCancelState() call.
     *
     * @param enabled TCS_CancelEnable to enable cancellation; TCS_CancelDisable otherwise.
     * @return Old state.
     */
   inline static cardinal setCancelState(const cardinal state);


   // ====== Tests for cancellation =========================================
   protected:
   /**
     * Test for cancellation. If the thread received a cancel signal, it will
     * be cancelled.
     */
   virtual void testCancel();

   /**
     * Exit current thread.
     *
     * @param result Result to return.
     */
   inline static void exit(void* result = NULL);

   /**
     * Voluntarily move current thread to end of queue of threads waiting for
     * CPU time (sched_yield() call). This will result in scheduling to next
     * waiting thread, if there is any.
     */
   inline static void yield();


   // ====== run() method to be implemented by subclass =====================
   protected:
   /**
     * The virtual run() method, which contains the thread's implementation.
     * It has to be implemented by classes, which inherit Thread.
     */
   virtual void run() = 0;


   // ====== Protected data =================================================
   protected:
   pthread_t PThread;
   pid_t     PID;


   // ====== Private data ===================================================
   private:
   static void* go(void* argument);


   cardinal        Flags;
   pthread_mutex_t StartupMutex;
   pthread_cond_t  StartupCondition;
};


#include "thread.icc"


#endif
