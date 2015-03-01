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
 * Purpose: Condition
 *
 */

#ifndef CONDITION_H
#define CONDITION_H


#include "tdsystem.h"
#include "synchronizable.h"



/**
  * This class realizes a condition variable.
  * @short   Condition
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  * @see Synchronizable
  * @see Thread
*/
class Condition : public Synchronizable
{
   // ====== Constructor/Destructor =========================================
   public:
   /**
     * Constructor.
     *
     * @param name Name.
     * @param parentCondition Parent condition.
     * @param recursive true to make condition's mutex recursive; false otherwise (default for Condition!).
     */
   Condition(const char* name            = "Condition",
             Condition*  parentCondition = NULL,
             const bool  recursive       = true);

   /**
     * Destructor.
     */
   ~Condition();


   // ====== Condition variable functions ===================================
   /**
     * Fire condition: One thread waiting for this variable will be
     * resumed.
     */
   void signal();

   /**
     * Broadcast condition: All threads waiting for this variable will be
     * resumed.
     */
   void broadcast();

   /**
     * Check, if condition has been fired. This call will reset
     * the fired state.
     *
     * @return true, if condition has been fired; false otherwise.
     */
   inline bool fired();

   /**
     * Check, if condition has been fired. This call will *not*
     * reset the fired state.
     *
     * @return true, if condition has been fired; false otherwise.
     */
   inline bool peekFired();

   /**
     * Wait for condition without timeout.
     */
   void wait();

   /**
     * Wait for condition with timeout.
     *
     * @param microseconds Timeout in microseconds.
     * @return true, if condition has been received; false for timeout.
     */
   bool timedWait(const card64 microseconds);


   // ====== Parent condition management ====================================
   /**
     * Add parent condition.
     *
     * @param parentCondition Parent condition to be added.
     */
   void addParent(Condition* parentCondition);

   /**
     * Remove parent condition.
     *
     * @param parentCondition Parent condition to be removed.
     */
   void removeParent(Condition* parentCondition);


   // ====== Private data ===================================================
   private:
   std::set<Condition*> ParentSet;
   pthread_cond_t       ConditionVariable;
   bool                 Fired;
   bool                 Valid;
};


#include "condition.icc"


#endif
