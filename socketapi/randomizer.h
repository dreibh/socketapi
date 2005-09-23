/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2003 by Thomas Dreibholz
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
 * Purpose: Randomizer Implementation
 *
 */


#ifndef RANDOM_H
#define RANDOM_H


#include "tdsystem.h"



/**
  * This class is an randomizer. The randomizer algorithm will calculate
  * random numbers with seed given by system timer (microseconds since
  * January 01, 1970) or given by a number.
  *
  * @short   Randomizer
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
  * @version 1.0
  */            
class Randomizer
{
   // ====== Constructor ====================================================
   public:
   /**
     * Constructor. Seed will be initialized by system timer (microseconds
     * since January 01, 1970).
     */
   Randomizer();

   // ====== Random functions ===============================================
   /**
     * Set seed by system timer (microseconds since January 01, 1970).
     */
   void setSeed();

   /**
     * Set seed by given number.
     *
     * @param seed Seed value.
     */
   void setSeed(const cardinal seed);

   /**
     * Get 8-bit random number.
     *
     * @return The generated number.
     */
   inline card8 random8();

   /**
     * Get 16-bit random number.
     *
     * @return The generated number.
     */
   inline card16 random16();

   /**
     * Get 32-bit random number.
     *
     * @return The generated number.
     */
   inline card32 random32();

   /**
     * Get 64-bit random number.
     *
     * @return The generated number.
     */
   inline card64 random64();

   /**
     * Get double random number out of interval [0,1].
     *
     * @return The generated number.
     */
   inline double random();

   /**
     * Get double random cardinal number out of interval [a,b].
     *
     * @return The generated number.
     */
   cardinal random(const cardinal a, const cardinal b);

   /**
     * Get double random double number out of interval [a,b].
     *
     * @return The generated number.
     */
   double random(const double a, const double b);


   // ====== Private data ===================================================
   private:
   card32 Value;
};


#include "randomizer.icc"


#endif
