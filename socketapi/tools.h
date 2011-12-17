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
 * Purpose: Tools Implementation
 *
 */


#ifndef TOOLS_H
#define TOOLS_H


#include "tdsystem.h"
#include "tdstrings.h"

#ifdef   HAVE_SYS_TIME_H
#include <sys/time.h>
#ifdef  TIME_WITH_SYS_TIME
#include <time.h>
#endif
#endif



/**
  * Debug output.
  *
  * @param string Debug string to be written to cerr.
  */
inline void debug(const char* string);


/**
  * Get microseconds since January 01, 1970.
  *
  * @return Microseconds since January 01, 1970.
  */
inline card64 getMicroTime();


/**
  * Translate 16-bit value to network byte order.
  *
  * @param x Value to be translated.
  * @return Translated value.
  */
inline card16 translate16(const card16 x);

/**
  * Translate 32-bit value to network byte order.
  *
  * @param x Value to be translated.
  * @return Translated value.
  */
inline card32 translate32(const card32 x);

/**
  * Translate 64-bit value to network byte order.
  *
  * @param x Value to be translated.
  * @return Translated value.
  */
inline card64 translate64(const card64 x);

/**
  * Translate double to 64-bit binary.
  *
  * @param x Value to be translated.
  * @return Translated value.
  */
inline card64 translateToBinary(const double x);

/**
  * Translate 64-bit binary to double.
  *
  * @param x Value to be translated.
  * @return Translated value.
  */
inline double translateToDouble(const card64 x);


/**
  * Calculate packets per second.
  *
  * Asumption: Every frame has it's own packets.
  *
  * @param payloadBytesPerSecond Byte rate of payload data.
  * @param framesPerSecond Frame rate.
  * @param maxPacketSize Maximum size of a packet.
  * @param headerLength Length of header for each frame.
  * @return Total bytes per second.
  */
cardinal calculatePacketsPerSecond(const cardinal payloadBytesPerSecond,
                                   const cardinal framesPerSecond,
                                   const cardinal maxPacketSize,
                                   const cardinal headerLength);

/**
  * Calculate frames per second.
  *
  * Asumption: Every frame has it's own packets.
  *
  * @param payloadBytesPerSecond Byte rate of payload data.
  * @param framesPerSecond Frame rate.
  * @param maxPacketSize Maximum size of a packet.
  * @param headerLength Length of header for each frame.
  * @return Total frames per second.
  */
cardinal calculateBytesPerSecond(const cardinal payloadBytesPerSecond,
                                 const cardinal framesPerSecond,
                                 const cardinal maxPacketSize,
                                 const cardinal headerLength);

/**
  * Scan protocol, host and path from an URL string. The protocol my be
  * missing, if the String "protocol" is initialized with a default.
  *
  * @param location String with URL.
  * @param protocol Place to store the protocol name.
  * @param host Place to store the host name.
  * @param path Place to store the path.
  * @return true on success; false otherwise.
  */
bool scanURL(const String& location,
             String&       protocol,
             String&       host,
             String&       path);

/**
  * Get user name for given user ID.
  *
  * @param str Buffer to store name to.
  * @param size Size of buffer.
  * @param realName true to get real name (e.g. John Miller); false to get user name (e.g. jmiller).
  * @param uid User ID.
  * @return true for success; false otherwise.
  */
bool getUserName(char*        str,
                 const size_t size,
                 const bool   realName  = false,
                 const        uid_t uid = getuid());

/**
  * Sort array using QuickSort algorithm.
  *
  * @param array Array to be sorted.
  * @param start Start offset in array.
  * @param end End offset in array.
  */
template<class T> void quickSort(T*            array,
                                 const integer start,
                                 const integer end);

/**
  * Sort pointer array using QuickSort algorithm.
  *
  * @param array Array to be sorted.
  * @param start Start offset in array.
  * @param end End offset in array.
  * @param lt Less than comparision routine.
  * @param gt Greater than comparision routine.
  */
template<class T> void quickSortPtr(T*            array,
                                    const integer start,
                                    const integer end,
                                    bool (*lt)(T,T),
                                    bool (*gt)(T,T));


/**
  * Sort pointer array using QuickSort algorithm.
  *
  * @param array Array to be sorted.
  * @param start Start offset in array.
  * @param end End offset in array.
  * @param lt Less than comparision routine for sorting.
  * @param gt Greater than comparision routine for sorting.
  * @param geq Equal routine for separation of groups.
  */
template<class T> void quickSortGroupPtr(T*            array,
                                         const integer start,
                                         const integer end,
                                         bool (*lt)(T,T),
                                         bool (*gt)(T,T),
                                         bool (*geq)(T,T));


/**
  * Remove duplicates from *sorted* array.
  *
  * @param array Array to be sorted.
  * @param length Length of array.
  */
template<class T> cardinal removeDuplicates(T*             array,
                                            const cardinal length);


/**
  * Print time stamp (date and time) to given output stream.
  *
  * @param os Output stream.
  */
void printTimeStamp(std::ostream& os = std::cout);


#include "tools.icc"


#endif
