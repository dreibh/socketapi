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
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
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


   // ====== Public data ====================================================
   public:
   /**
     * Mutex for set of Synchronizable objects (for SYNCDEBUGGER mode only!).
     *
     * @see Synchronizable
     */
   static Synchronizable SyncSetLock;

   /**
     * This mutex is necessary to avoid simultaneous access to malloc()
     * and free() when using libefence.
     */
   static Synchronizable MemoryManagementLock;

   /**
     * Set of all running threads.
     */
   static std::set<Thread*> ThreadSet;


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


#ifdef SYNCDEBUGGER
   friend class Synchronizable;
   struct _pthread_descr_struct;
   typedef struct _pthread_descr_struct* pthread_descr;

   // ***********************************************************************
   // * IMPORTANT: Insert _pthread_descr_struct from
   // *            linuxthreads/internals.h of your glibc package here!
   // ***********************************************************************
   struct _pthread_descr_struct {
#if ((__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 2))
     union {
       struct {
         Thread::pthread_descr self;	/* Pointer to this structure */
       } data;
       void *__padding[16];
     } p_header;
#endif
     pthread_descr p_nextlive, p_prevlive;
                                   /* Double chaining of active threads */
     pthread_descr p_nextwaiting;  /* Next element in the queue holding the thr */
     pthread_descr p_nextlock;	/* can be on a queue and waiting on a lock */
     pthread_t p_tid;              /* Thread identifier */
     int p_pid;                    /* PID of Unix process */
   };
   // ***********************************************************************

   pthread_descr InternalPThreadPtr;


   static bool checkSyncDebugger();
   static bool syncDebuggerChecked;
#endif
};


#include "thread.icc"


#endif
