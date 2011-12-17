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
 * Purpose: Strings Implementation
 *
 */


#include "tdsystem.h"
#include "tdstrings.h"


#include <ctype.h>


// ###### Constructor #######################################################
String::String()
{
   setData(NULL);
}


// ###### Constructor #######################################################
String::String(const String& string)
{
   setData(stringDuplicate(string.getData()));
}


// ###### Constructor #######################################################
String::String(const char* string)
{
   setData(stringDuplicate(string));
}


// ###### Constructor #######################################################
String::String(const char* string, const cardinal length)
{
   if(string != NULL) {
      char str[length + 1];
      memcpy((void*)&str,string,length);
      str[length] = 0x00;
      setData(stringDuplicate((char*)&str));
   }
   else {
      setData(NULL);
   }
}


// ###### Constructor #######################################################
String::String(const cardinal value)
{
   char string[64];
   snprintf((char*)&string,sizeof(string),"%llu",(unsigned long long int)value);
   setData(stringDuplicate((char*)&string));
}


// ###### Destructor ########################################################
String::~String()
{
   free(Data);
}


// ###### "="-operator ######################################################
String& String::operator=(const String& string)
{
   if(this != &string) {
      free(Data);
      setData(stringDuplicate(string.getData()));
   }
   return(*this);
}


// ###### "="-operator ######################################################
String& String::operator=(const char* string)
{
   free(Data);
   setData(stringDuplicate(string));
   return(*this);
}


// ###### "="-operator ######################################################
String& String::operator=(const cardinal value)
{
   free(Data);
   char string[64];
   snprintf((char*)&string,sizeof(string),"%d",value);
   setData(stringDuplicate((char*)&string));
   return(*this);
}


// ###### Convert string to lowercase #######################################
String String::toLower() const
{
   const cardinal len = length();
   char buffer[len + 1];

   cardinal i;
   for(i = 0;i < len;i++) {
      buffer[i] = tolower(Data[i]);
   }
   buffer[i] = 0x00;
   return((char*)&buffer);
}


// ###### Convert string to uppercase #######################################
String String::toUpper() const
{
   const cardinal len = length();
   char buffer[len + 1];

   cardinal i;
   for(i = 0;i < len;i++) {
      buffer[i] = toupper(Data[i]);
   }
   buffer[i] = 0x00;
   return((char*)&buffer);
}


// ###### Get left part of string ###########################################
String String::left(const cardinal maxChars) const
{
   const cardinal len = std::min(length(),maxChars);
   char buffer[len + 1];

   cardinal i;
   for(i = 0;i < len;i++) {
      buffer[i] = Data[i];
   }
   buffer[i] = 0x00;
   return((char*)&buffer);
}


// ###### Get middle part of string #########################################
String String::mid(const cardinal start, const cardinal maxChars) const
{
   const cardinal strlen = length();
   if(strlen <= start) {
      return("");
   }

   const cardinal len = std::min(strlen - start,maxChars);
   char buffer[len + 1];

   cardinal i;
   for(i = 0;i < len;i++) {
      buffer[i] = Data[i + start];
   }
   buffer[i] = 0x00;
   return((char*)&buffer);
}


// ###### Get right part of string ##########################################
String String::right(const cardinal maxChars) const
{
   const cardinal strlen = length();
   const cardinal len    = std::min(strlen,maxChars);
   char buffer[len + 1];

   cardinal i,j;
   for(i = 0, j = strlen - len;i < len;i++,j++) {
      buffer[i] = Data[j];
   }
   buffer[i] = 0x00;
   return((char*)&buffer);
}


// ###### Remove spaces #####################################################
String String::stripWhiteSpace() const
{
   integer strlen = length();
   integer i = 0;
   while((Data[i] == ' ') && (i < strlen)) {
      i++;
   }
   integer j = strlen - 1;
   while((j >= i) && (Data[j] == ' ')) {
      j--;
   }
   return(mid(i,j - i + 1));
}


// ###### Scan setting string ###############################################
bool String::scanSetting(String& s1, String& s2) const
{
   integer found = index('=');
   if(found >= 1) {
      s1 = left(found).stripWhiteSpace().toUpper();
      s2 = right(length() - found - 1).stripWhiteSpace();
      if((s1.length() > 0) && (s2.length() > 0)) {
         const char* data      = s2.getData();
         const cardinal length = s2.length();
         if((data[0] == '\"') && (data[length - 1] == '\"')) {
            s2 = s2.mid(1,length - 2);
         }
         return(true);
      }
   }
   return(false);
}


// ###### "<<"-operator #####################################################
std::ostream& operator<<(std::ostream& os, const String& string)
{
   const char* data = string.getData();
   if(data) {
      os << data;
   }
   return(os);
}


// ###### "+"-operator ######################################################
String operator+(const String& string1, const String& string2)
{
   char str[string1.length() + string2.length() + 1];
   const char* data1 = string1.getData();
   const char* data2 = string2.getData();

   if(data1 != NULL) {
      strcpy((char*)&str,data1);
   }
   else {
      str[0] = 0x00;
   }
   if(data2 != NULL) {
      strcat((char*)&str,data2);
   }
   return String((const char*)&str);
}
