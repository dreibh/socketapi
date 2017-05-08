/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2017 by Thomas Dreibholz
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
 * Purpose: Synchronizable Implementation
 *
 */


#ifndef SYNCHRONIZABLE_H
#define SYNCHRONIZABLE_H


#include "tdsystem.h"


#include <sys/types.h>
#include <pthread.h>
#include <set>


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
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
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


   // ====== Synchronizable naming for debugging ============================
   /**
     * Get name of synchronizable object.
     *
     * @return Name.
     */
   inline const char* getName() const;

   /**
     * Set name of synchronizable object.
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
   char            MutexName[64];
};


#include "synchronizable.icc"


#endif
