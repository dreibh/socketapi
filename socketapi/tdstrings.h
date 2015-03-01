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
 * Purpose: Strings Implementation
 *
 */


#ifndef TDSTRINGS_H
#define TDSTRINGS_H


#include "tdsystem.h"


/**
  * This class implements the String datatype.
  *
  * @short   String
  * @author  Thomas Dreibholz (dreibh@iem.uni-due.de)
  * @version 1.0
  */
class String
{
   // ====== Constructors/destructor ========================================
   public:
   /**
     * Constructor for an empty string.
     */
   String();

   /**
     * Constructor for a copy of a string.
     *
     * @param string String to be copied.
     */
   String(const String& string);

   /**
     * Constructor for a copy of a string.
     *
     * @param string String to be copied.
     */
   String(const char* string);

   /**
     * Constructor for a copy of a string with a given length to be copied.
     *
     * @param string String to be copied.
     * @param length Number of bytes to be copied.
     */
   String(const char* string, const cardinal length);

   /**
     * Constructor for a string from a number.
     *
     * @param value Number.
     */
   String(const cardinal value);

   /**
     * Destructor.
     */
   ~String();


   // ====== String functions ===============================================
   /**
     * Get string data.
     *
     * @return String data.
     */
   inline const char* getData() const;

   /**
     * Get string length.
     *
     * @return Length in bytes.
     */
   inline cardinal length() const;

   /**
     * Check, if string is NULL.
     *
     * @return true, if string is NULL; false otherwise.
     */
   inline bool isNull() const;

   /**
     * Find first position of a character in string.
     *
     * @param c Character.
     * @return Position of -1, if character is not in string.
     */
   inline integer index(const char c) const;

   /**
     * Find last position of a character in string.
     *
     * @param c Character.
     * @return Position of -1, if character is not in string.
     */
   inline integer rindex(const char c) const;

   /**
     * Find first position of a string in a string
     *
     * @param string String to find in string.
     * @return Position of -1, if string is not in string.
     */
   inline integer find(const String& string) const;

   /**
     * Get uppercase string from string.
     *
     * @return Uppercase string.
     */
   String toUpper() const;

   /**
     * Get lowercase string from string.
     *
     * @return Lowercase string.
     */
   String toLower() const;

   /**
     * Get left part of string.
     *
     * @param maxChars Maximum number of characters to be copied.
     * @return String.
     */
   String left(const cardinal maxChars) const;

   /**
     * Get middle part of string.
     *
     * @param start Start position in String.
     * @param maxChars Maximum number of characters to be copied.
     * @return String.
     */
   String mid(const cardinal start, const cardinal maxChars) const;

   /**
     * Get part from start to end of string.
     *
     * @param start Start position in String.
     * @return String.
     */
   inline String mid(const cardinal start) const;

   /**
     * Get right part of string.
     *
     * @param maxChars Maximum number of characters to be copied.
     * @return String.
     */
   String right(const cardinal maxChars) const;


   /**
     * Get string with spaces from beginning and end of the string removed.
     *
     * @return New string.
     */
   String stripWhiteSpace() const;


   /**
     * Scan setting string, e.g. " FileName = Test.file ".
     * Spaces are removed, the first string (name) is converted to uppercase. The
     * second string (value) may contain "-chars for values with spaces. The "-chars
     * will be removed from the result.
     *
     * @param name Reference to store the name.
     * @param value Reference to store the value.
     * @return true, if scan was successful; false otherwise.
     */
   bool scanSetting(String& s1, String& s2) const;


   // ====== Operators ======================================================
   /**
     * Implementation of = operator.
     */
   String& operator=(const String& string);

   /**
     * Implementation of = operator.
     */
   String& operator=(const char* string);

   /**
     * Implementation of = operator.
     */
   String& operator=(const cardinal value);

   /**
     * Implementation of == operator.
     */
   inline int operator==(const String& string) const;

   /**
     * Implementation of != operator.
     */
   inline int operator!=(const String& string) const;

   /**
     * Implementation of < operator.
     */
   inline int operator<(const String& string) const;

   /**
     * Implementation of <= operator.
     */
   inline int operator<=(const String& string) const;

   /**
     * Implementation of > operator.
     */
   inline int operator>(const String& string) const;

   /**
     * Implementation of >= operator.
     */
   inline int operator>=(const String& string) const;

   /**
     * Implementation of [] operator.
     */
   inline char operator[](const int index) const;


   /**
     * Compute length of a string.
     *
     * @param string String.
     * @return Length.
     */
   inline static cardinal stringLength(const char* string);

   /**
     * Compare two strings.
     *
     * @param str1 First string.
     * @param str2 Second string.
     * @return str1 < str1 => -1; str1 == str2 => 0; str1 > str2 => 1.
     */
   inline static integer stringCompare(const char* str1, const char* str2);

   /**
     * Duplicate a string. The new string can be deallocated with the delete
     * operator.
     *
     * @param string String to be duplicated.
     * @return New string.
     */
   inline static char* stringDuplicate(const char* string);


   // ====== Private data ===================================================
   private:
   inline void setData(char* string);


   char* Data;
};


// ====== Operators =========================================================
/**
  * Implementation of << operator.
  */
std::ostream& operator<<(std::ostream& out, const String& string);

/**
  * Implementation of + operator.
  */
String operator+(const String& string1, const String& string2);


#include "tdstrings.icc"


#endif
