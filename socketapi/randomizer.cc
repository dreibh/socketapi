/*
 *  $Id: randomizer.cc,v 1.1 2003/05/15 11:35:50 dreibh Exp $
 *
 * SCTP implementation according to RFC 2960.
 * Copyright (C) 1999-2002 by Thomas Dreibholz
 *
 * Realized in co-operation between Siemens AG
 * and University of Essen, Institute of Computer Networking Technology.
 *
 * Acknowledgement
 * This work was partially funded by the Bundesministerium für Bildung und
 * Forschung (BMBF) of the Federal Republic of Germany (Förderkennzeichen 01AK045).
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
 *
 * Purpose: Randomizer Implementation
 *
 */

#include "tdsystem.h"
#include "randomizer.h"
#include "tools.h"



// ###### Constructor #######################################################
Randomizer::Randomizer()
{
   setSeed();
}

   
// ###### Set randomizer seed ###############################################
void Randomizer::setSeed()
{
   Value = (card32)getMicroTime();
}


// ###### Set randomizer seed ###############################################
void Randomizer::setSeed(const cardinal seed)
{
   Value = seed;
}


// ###### Generate random cardinal number out of interval [a,b] #############
cardinal Randomizer::random(const cardinal a, const cardinal b)
{
   const cardinal c = b - a + 1;
   cardinal number;
   if(sizeof(cardinal) == 4)
      number = random32();
   else
      number = random64();
   
   if(c == 0)
      return(a);
   else
      return((number % c) + a);
}


// ###### Generate random double number out of interval [a,b] #############
double Randomizer::random(const double a, const double b)
{
   const double c = b - a;
   return((random() * c) + a);
}
