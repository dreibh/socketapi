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
 * Purpose: System dependent definitions
 *
 */


#ifndef TDSYSTEM_H
#define TDSYSTEM_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


// Use traffic shaper
// #define USE_TRAFFICSHAPER

// Disable all warning outputs. Not recommended!
// #define DISABLE_WARNINGS


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <iostream>


// ###### Operating system definitions ######################################
#define OS_Linux   1
#define OS_FreeBSD 2
#define OS_Darwin  3
#define OS_SOLARIS 4

#ifdef LINUX
 #define SYSTEM OS_Linux
#endif
#ifdef FreeBSD
 #define SYSTEM OS_FreeBSD
#endif
#ifdef DARWIN
 #define SYSTEM OS_Darwin
 #define MAXDNAME 256
#endif
#ifdef SOLARIS
 #define SYSTEM OS_SOLARIS
#endif

#ifndef SYSTEM
 #warning Variable SYSTEM with operating system name not defined! Use e.g. -DOS_Linux compiler option.
 #warning Trying Linux...
 #define SYSTEM OS_Linux
#endif


// ###### CPU defintions ####################################################
// Set correct number of CPU bits (32 or 64) here!
#if (SYSTEM == OS_Linux)
 #include <endian.h>
 #include <stdint.h>
#elif (SYSTEM == OS_FreeBSD)
 #include <machine/endian.h>
 #include <inttypes.h>
#elif (SYSTEM == OS_Darwin)
 #include <machine/endian.h>
 #include <stdint.h>
#elif (SYSTEM == OS_SOLARIS)
 #include <inttypes.h>
 #include <arpa/nameser_compat.h>
#endif


// ###### Type definitions ##################################################
/**
  * Datatype for storing a signed char.
  */
typedef int8_t sbyte;

/**
  * Datatype for storing an unsigned char.
  */
typedef uint8_t ubyte;

/**
  * Datatype for storing an 8-bit integer.
  */
typedef int8_t int8;

/**
  * Datatype for storing a 8-bit cardinal.
  */
typedef uint8_t card8;

/**
  * Datatype for storing a 16-bit integer.
  */
typedef int16_t int16;

/**
  * Datatype for storing a 16-bit cardinal.
  */
typedef uint16_t card16;

/**
  * Datatype for storing a 32-bit intger.
  */
typedef int32_t int32;

/**
  * Datatype for storing a default-sized integer (32 bits minimum).
  */
#if defined (int_least32_t)
typedef int_least32_t integer;
#else
typedef int32 integer;
#endif

/**
  * Datatype for storing a 32-bit cardinal.
  */
typedef uint32_t card32;

/**
  * Datatype for storing an 64-bit integer.
  */
typedef int64_t int64;

/**
  * Datatype for storing a 64-bit cardinal.
  */
typedef uint64_t card64;

/**
  * Datatype for storing a default-sized cardinal (32 bits minimum).
  */
#if defined (uint_least32_t)
typedef uint_least32_t cardinal;
#else
typedef card32 cardinal;
#endif


#endif
