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
 * Purpose: Synchronizable Implementation
 *
 */


#ifndef SYNCHRONIZABLE_H
#define SYNCHRONIZABLE_H


#include "tdsystem.h"


#include <sys/types.h>
#include <pthread.h>
#include <set>



// Do verbose error checks to detect possible deadlock conditions.
// IMPORTANT: This will use *internal* libpthread variables. See thread.h for details!
// #define SYNCDEBUGGER

// Assume synchronization failure after waiting for given number of microseconds
// Note: A too low value can cause false alarms!
#define SYNCDEBUGGER_TIMEOUT 20000000

// Print all synchronized/unsynchronized calls with file name
// and line number using macros synchronized() and unsynchronized()
// to call debug functions.
// #define SYNCDEBUGGER_PRINTING
// #define SYNCDEBUGGER_VERBOSE_PRINTING

// Signal to send to process in case of failure (e.g. SIGSEGV). Using a recent
// Alan Cox kernel with multithreaded core dump patch, SIGSEGV can be used to abort
// with core dump for gdb debugging.
#ifdef SYNCDEBUGGER
#include <signal.h>
#define SYNCDEBUGGER_FAILURESIGNAL SIGSEGV
#endif


// System does not support recursive mutex.
// #define NO_RECURSIVE_MUTEX


#ifdef NO_RECURSIVE_MUTEX
#warning Usage of recursive pthread mutexes disabled!
#endif

#if (SYSTEM == OS_Darwin)
#ifndef NO_RECURSIVE_MUTEX
#define NO_RECURSIVE_MUTEX
#endif
#endif


/**
  * This class realizes synchronized access to a thread's data by other threads.
  * Synchronization is done by using a global pthread mutex and obtaining
  * access to this mutex by synchronized() for synchronized access and releasing
  * this mutex for unsynchronized access.
  * IMPORTANT: Do *not* use synchronized()/unsynchronized() within async signal
  * handlers. This may cause deadlocks. See PThread's pthread_mutex_lock man-page,
  * section "Async Signal Safety" for more information!
  *
  * @short   Synchronizable
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
  * @version 1.0
  * @see Thread
  */
class Synchronizable
{
   // ====== Constructor/Destructor =========================================
   public:
   /**
     * Constructor.
     *
     * @param name Name.
     * @param recursive true to make mutex recursive (default); false otherwise.
     */
   Synchronizable(const char* name      = "Synchronizable",
                  const bool  recursive = true);

   /**
     * Destructor.
     */
   ~Synchronizable();


   // ====== Synchronization ================================================
   /**
     * synchronized() begins a synchronized block. The block has to be
     * finished by unsynchronized(). synchronized() will wait until the mutex
     * is available.
     *
     * @see unsynchronized
     * @see synchronizedTry
     */
   inline void synchronized();

   /**
     * synchronizedTry() tries to begins a synchronized block. It does the same
     * as synchronized(), but returns immediately, if the mutex is obtained by
     * another thread.
     *
     * @see synchronized
     * @see unsynchronized
     */
   inline bool synchronizedTry();

   /**
     * unsynchronized() ends a synchronized block, which has begun by
     * synchronized().
     *
     * @see synchronized
     */
   inline void unsynchronized();


   /**
     * Do reinitialization of Synchronizable.
     */
   void resynchronize();


   // ====== Disable/enable cancelability of current thread =================
   /**
     * Enable or disable cancelability of calling thread.
     *
     * @param enabled true to enable cancellation; false otherwise.
     */
   inline static bool setCancelState(const bool enabled);


   // ====== Debug functions ================================================
   /**
     * Debug version of synchronized. This will print PID, file name
     * and line number, followed by debug information.
     *
     * @param file File name.
     * @param line Line number.
     *
     * @see synchronized
     */
   void synchronized_debug(const char* file, const cardinal line);

   /**
     * Debug version of unsynchronized. This will print PID, file name
     * and line number, followed by debug information.
     *
     * @param file File name.
     * @param line Line number.
     *
     * @see unsynchronized
     */
   void unsynchronized_debug(const char* file, const cardinal line);

   /**
     * Debug version of synchronizedTry. This will print PID, file name
     * and line number, followed by debug information.
     *
     * @param file File name.
     * @param line Line number.
     *
     * @see synchronizedTry
     */
   bool synchronizedTry_debug(const char* file, const cardinal line);

   /**
     * Debug version of resynchronize. This will print PID, file name
     * and line number, followed by debug information.
     *
     * @param file File name.
     * @param line Line number.
     *
     * @see resynchronize
     */
   void resynchronize_debug(const char* file, const cardinal line);


   // ====== Synchronizable naming for debugging ============================
   /**
     * Get name of synchronizable object (SYNCDEBUGGER mode only, see synchronizable.h).
     *
     * @return Name.
     */
   inline const char* getName() const;

   /**
     * Set name of synchronizable object (SYNCDEBUGGER mode only, see synchronizable.h).
     *
     * @param name Name.
     */
   inline void setName(const char* name);


   // ====== Protected data =================================================
   protected:
   pthread_mutex_t Mutex;
#ifdef NO_RECURSIVE_MUTEX
   cardinal        RecursionLevel;
   pthread_t       Owner;
#endif
   bool            Recursive;


   // ====== Private data ===================================================
   char                        MutexName[256];
#ifdef SYNCDEBUGGER
   friend class Thread;
   static set<Synchronizable*> MutexSet;
#endif
};


#include "synchronizable.icc"


#ifdef SYNCDEBUGGER
#define synchronized()    synchronized_debug(__FILE__,__LINE__)
#define unsynchronized()  unsynchronized_debug(__FILE__,__LINE__)
#define synchronizedTry() synchronizedTry_debug(__FILE__,__LINE__)
#define resynchronize()   resynchronize_debug(__FILE__,__LINE__)
#endif


#endif
