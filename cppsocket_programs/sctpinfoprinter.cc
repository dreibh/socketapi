/*
 *  $Id: sctpinfoprinter.cc,v 1.3 2003/08/19 19:28:34 tuexen Exp $
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
      cout << "\x1b[" << getANSIColor(NOTIFICATION_COLOR)
           << "mNotification:" << endl;
   }
   switch(notification->sn_header.sn_type) {
      case SCTP_ASSOC_CHANGE: {
           const sctp_assoc_change* sac = &notification->sn_assoc_change;
           char str[16];
           snprintf((char*)&str,sizeof(str),"$%04x",sac->sac_flags);
           cout << "   Type    = SCTP_ASSOC_CHANGE"   << endl
                << "   Length  = " << sac->sac_length << endl
                << "   Flags   = " << str             << endl
                << "   State   = " << sac->sac_state  << endl
                << "   Error   = " << sac->sac_error  << endl
                << "   OutStrs = " << sac->sac_outbound_streams << endl
                << "   InStrs  = " << sac->sac_inbound_streams  << endl
                << "   AssocID = #" << sac->sac_assoc_id        << endl;
         }
       break;
      case SCTP_SHUTDOWN_EVENT: {
           const sctp_shutdown_event* sse = &notification->sn_shutdown_event;
           char str[16];
           snprintf((char*)&str,sizeof(str),"$%04x",sse->sse_flags);
           cout << "   Type    = SCTP_SHUTDOWN_EVENT"    << endl
                << "   Length  = " << sse->sse_length    << endl
                << "   Flags   = " << str                << endl
                << "   AssocID = #" << sse->sse_assoc_id << endl;
         }
       break;
      default:
          char str[16];
          snprintf((char*)&str,sizeof(str),"$%04x",
                   notification->sn_header.sn_flags);
          cout << "   Type   = " << notification->sn_header.sn_type   << endl
               << "   Length = " << notification->sn_header.sn_length << endl
               << "   Flags  = " << str                                             << endl;
       break;
   }
   if(ColorMode) {
      cout << "\x1b[" << getANSIColor(0) << "m";
   }
   cout.flush();
}


// ###### Print control data ################################################
void printControl(const msghdr* header)
{
   if(!PrintControl) {
      return;
   }

   cmsghdr* cmsg = CFirst(header);
   if((cmsg != NULL) && (header->msg_controllen >= sizeof(cmsghdr))) {
      if(ColorMode) {
         cout << "\x1b[" << getANSIColor(CONTROL_COLOR)
              << "mControl Data:" << endl;
      }
      while(cmsg != NULL) {
         if((cmsg->cmsg_level == IPPROTO_SCTP) &&
            (cmsg->cmsg_type  == SCTP_SNDRCV)) {
            sctp_sndrcvinfo* info = (sctp_sndrcvinfo*)CData(cmsg);
            char str1[16];
            char str2[16];
            snprintf((char*)&str1,sizeof(str1),"$%04x",info->sinfo_flags);
            snprintf((char*)&str2,sizeof(str2),"$%08x",info->sinfo_ppid);
            cout << "   SCTP_SNDRCV"  << endl;
            cout << "    AssocID    = #" << info->sinfo_assoc_id    << endl;
            cout << "    Stream     = #" << info->sinfo_stream      << endl;
            cout << "    Flags      = " << str1 << endl;
            cout << "    PPID       = " << str2 << endl;
            cout << "    SSN        = " << info->sinfo_ssn        << endl;
            cout << "    TSN        = " << info->sinfo_tsn        << endl;
            cout << "    TimeToLive = " << info->sinfo_timetolive << endl;
         }
         else {
            cout << "   Level #"   << cmsg->cmsg_level
                 << ", Type #"     << cmsg->cmsg_type
                 << ". Length is " << cmsg->cmsg_len  << "." << endl;
         }
         cmsg = CNext(header,cmsg);
      }
      if(ColorMode) {
         cout << "\x1b[" << getANSIColor(0) << "m";
      }
      cout.flush();
   }
}
