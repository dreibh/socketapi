/*
 *  $Id: ansicolor.h,v 1.1 2003/05/15 11:35:50 dreibh Exp $
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
 * Purpose: ANSI Color
 *
 */


#ifndef ANSICOLOR_H
#define ANSICOLOR_H



#define NOTIFICATION_COLOR 10
#define CONTROL_COLOR      12
#define ERROR_COLOR        9
#define INFO_COLOR         11



// ###### Get ANSI color number #############################################
inline const int getANSIColor(const int number)
{
   int color = number % 15;
   const int a = color % 8;
   const int b = color / 8;
   int d = 30 + a;
   if(b > 0) {
      d += 60;
   }
   return(d);
}


#endif
