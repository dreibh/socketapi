/*
 *  $Id: breakdetector.h,v 1.1 2003/05/15 11:35:50 dreibh Exp $
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
 * Purpose:    Break Detector
 *
 */

#ifndef BREAKDETECTOR_H
#define BREAKDETECTOR_H


#include "tdsystem.h"



/**
  * Install break handler.
  */
void installBreakDetector();

/**
  * Uninstall break handler.
  */
void uninstallBreakDetector();

/**
  * Check, if break has been detected.
  */
bool breakDetected();

/**
  * Send break to main thread.
  *
  * @param quiet true to print no break message in breakDetected(), false otherwise (default).
  */
void sendBreak(const bool quiet = false);


#endif
