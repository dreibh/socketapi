/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2008 by Thomas Dreibholz
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
 * Purpose: SCTP Info Printer
 *
 */


#include "tdsystem.h"
#include "sctpinfoprinter.h"
#include "ansicolor.h"
#include "tdmessage.h"


bool ColorMode          = true;
bool PrintControl       = true;
bool PrintNotifications = true;



// ###### Print notification ################################################
void printNotification(const sctp_notification* notification)
{
   if(!PrintNotifications) {
      return;
   }

   if(ColorMode) {
      std::cout << "\x1b[" << getANSIColor(NOTIFICATION_COLOR)
                << "mNotification:" << std::endl;
   }
   switch(notification->sn_header.sn_type) {
      case SCTP_ASSOC_CHANGE: {
           const sctp_assoc_change* sac = &notification->sn_assoc_change;
           char str[16];
           snprintf((char*)&str,sizeof(str),"$%04x",sac->sac_flags);
           std::cout << "   Type    = SCTP_ASSOC_CHANGE"   << std::endl
                     << "   Length  = " << sac->sac_length << std::endl
                     << "   Flags   = " << str             << std::endl
                     << "   State   = " << sac->sac_state  << std::endl
                     << "   Error   = " << sac->sac_error  << std::endl
                     << "   OutStrs = " << sac->sac_outbound_streams << std::endl
                     << "   InStrs  = " << sac->sac_inbound_streams  << std::endl
                     << "   AssocID = #" << sac->sac_assoc_id        << std::endl;
         }
       break;
      case SCTP_SHUTDOWN_EVENT: {
           const sctp_shutdown_event* sse = &notification->sn_shutdown_event;
           char str[16];
           snprintf((char*)&str,sizeof(str),"$%04x",sse->sse_flags);
           std::cout << "   Type    = SCTP_SHUTDOWN_EVENT"    << std::endl
                     << "   Length  = " << sse->sse_length    << std::endl
                     << "   Flags   = " << str                << std::endl
                     << "   AssocID = #" << sse->sse_assoc_id << std::endl;
         }
       break;
      default:
          char str[16];
          snprintf((char*)&str,sizeof(str),"$%04x",
                   notification->sn_header.sn_flags);
          std::cout << "   Type   = " << notification->sn_header.sn_type   << std::endl
                    << "   Length = " << notification->sn_header.sn_length << std::endl
                    << "   Flags  = " << str                               << std::endl;
       break;
   }
   if(ColorMode) {
      std::cout << "\x1b[" << getANSIColor(0) << "m";
   }
   std::cout.flush();
}


// ###### Print control data ################################################
void printControl(const msghdr* header)
{
   if(!PrintControl) {
      return;
   }

   cmsghdr* cmsg = CFirst(header);
   if((cmsg != NULL) && (header->msg_controllen >= (socklen_t)sizeof(cmsghdr))) {
      if(ColorMode) {
         std::cout << "\x1b[" << getANSIColor(CONTROL_COLOR)
              << "mControl Data:" << std::endl;
      }
      while(cmsg != NULL) {
         if((cmsg->cmsg_level == IPPROTO_SCTP) &&
            (cmsg->cmsg_type  == SCTP_SNDRCV)) {
            sctp_sndrcvinfo* info = (sctp_sndrcvinfo*)CData(cmsg);
            char str1[16];
            char str2[16];
            snprintf((char*)&str1,sizeof(str1),"$%04x",info->sinfo_flags);
            snprintf((char*)&str2,sizeof(str2),"$%08x",ntohl(info->sinfo_ppid));
            std::cout << "   SCTP_SNDRCV"  << std::endl;
            std::cout << "    AssocID    = #" << info->sinfo_assoc_id  << std::endl;
            std::cout << "    Stream     = #" << info->sinfo_stream    << std::endl;
            std::cout << "    Flags      = " << str1 << std::endl;
            std::cout << "    PPID       = " << str2 << std::endl;
            std::cout << "    SSN        = " << info->sinfo_ssn        << std::endl;
            std::cout << "    TSN        = " << info->sinfo_tsn        << std::endl;
            std::cout << "    TimeToLive = " << info->sinfo_timetolive << std::endl;
         }
         else {
            std::cout << "   Level #"   << cmsg->cmsg_level
                      << ", Type #"     << cmsg->cmsg_type
                      << ". Length is " << cmsg->cmsg_len  << "." << std::endl;
         }
         cmsg = CNext(header,cmsg);
      }
      if(ColorMode) {
         std::cout << "\x1b[" << getANSIColor(0) << "m";
      }
      std::cout.flush();
   }
}
