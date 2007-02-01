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
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * There are two mailinglists available at http://www.sctp.de which should be
 * used for any discussion related to this implementation.
 *
 * Contact: discussion@sctp.de
 *          dreibh@exp-math.uni-essen.de
 *          tuexen@fh-muenster.de
 *
 * Purpose: Range Template
 *
 */


#ifndef RANGE_H
#define RANGE_H


#include "tdsystem.h"


/**
  * This class implements the Range datatype template. It manages a value which
  * has to be in the range from Min to Max. The only allowed exception is the
  * value 0, which is available even if it is outside of the given range.
  *
  * @short   Range
  * @author  Thomas Dreibholz (dreibh@exp-math.uni-essen.de)
  * @version 1.0
  */
template<class T> class Range
{
   // ====== Constructors ===================================================
   public:
   /**
     * Default constructor.
     */
   Range();

   /**
     * Create new range with given parameters.
     *
     * @param min Minimum.
     * @param max Maximum.
     * @param value Value between Minimum and Maximum.
     */
   Range(const T min, const T max, const T value);


   // ====== Initialization =================================================
   /**
     * Initialize range with given parameters.
     *
     * @param min Minimum.
     * @param max Maximum.
     * @param value Value between Minimum and Maximum.
     */
   void init(const T min, const T max, const T value);


   // ====== Range functions ================================================
   /**
     * Get minimum.
     *
     * @return Minimum.
     */
   inline T getMin()   const;

   /**
     * Get maximum.
     *
     * @return Maximum.
     */
   inline T getMax()   const;

   /**
     * Get value.
     *
     * @return Value.
     */
   inline T getValue() const;

   /**
     * Set limits.
     *
     * @param min Minimum.
     * @param max Maximum.
     */
   inline void setLimits(const T min, const T max);

   /**
     * Set value.
     *
     * @param value Value.
     */
   inline void setValue(const T value);


   // ====== "="-operator ===================================================
   /**
     * Implementation of = operator
     */
   Range<T>& operator=(const Range<T>& range);


   // ====== Comparision operators ==========================================
   /**
     * == operator.
     */
   inline int operator==(const Range<T>& ti) const;

   /**
     * != operator.
     */
   inline int operator!=(const Range<T>& ti) const;


   // ====== Private data ===================================================
   public:  // Public because of byte order translation!
   T Min;
   T Max;
   T Value;
};


/**
  * << operator.
  */
template<class T> std::ostream& operator<<(std::ostream& os, const Range<T>& range);


#include "range.icc"
#include "range.cc"


#endif
