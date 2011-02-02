/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 1999-2011 by Thomas Dreibholz
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
 * Purpose: Static Globals
 *
 */


#include "tdsystem.h"



/*
   Very important: Do *not* change the order of these declarations!
*/


// ###### Synchronizable/Thread static attributes ###########################
#ifndef STATICGLOBALS_SKIP_THREAD
#include "synchronizable.h"
#include "thread.h"


// Important: Synchronizable::MutexSet has to be initialized *before*
//            the SyncSetLock, therefore it is here in thread.cc!
#ifdef SYNCDEBUGGER
set<Synchronizable*> Synchronizable::MutexSet;
#endif

Synchronizable       Thread::SyncSetLock("SyncSetLock");
std::set<Thread*>    Thread::ThreadSet;
Synchronizable       Thread::MemoryManagementLock("MemoryManagementLock");

#ifdef SYNCDEBUGGER
bool                 Thread::syncDebuggerChecked = Thread::checkSyncDebugger();
#endif


#endif


// ###### InternetAddress static attributes #################################
#ifndef STATICGLOBALS_SKIP_INTERNETADDRESS
#include "internetaddress.h"

// Check, if IPv6 is available on this host.
bool InternetAddress::UseIPv6 = checkIPv6();

#endif



// ###### SCTPSocketMaster static attributes ################################
#ifndef STATICGLOBALS_SKIP_SCTP
#ifndef HAVE_KERNEL_SCTP
#include "sctpsocketmaster.h"
#include "extsocketdescriptor.h"


int                              SCTPSocketMaster::InitializationResult     = -1000;
int                              SCTPSocketMaster::GarbageCollectionTimerID = -1;
cardinal                         SCTPSocketMaster::LockLevel                = 0;
cardinal                         SCTPSocketMaster::OldCancelState           = true;
card64                           SCTPSocketMaster::LastGarbageCollection;
std::set<int>                    SCTPSocketMaster::ClosingSockets;
std::multimap<unsigned int, int> SCTPSocketMaster::ClosingAssociations;
std::multimap<int, SCTPSocket*>  SCTPSocketMaster::SocketList;
SCTP_ulpCallbacks                SCTPSocketMaster::Callbacks;
SCTPSocketMaster                 SCTPSocketMaster::MasterInstance;
Randomizer                       SCTPSocketMaster::Random;
int                              SCTPSocketMaster::BreakPipe[2];
struct SCTPSocketMaster::UserSocketNotification SCTPSocketMaster::BreakNotification;

ExtSocketDescriptor              ExtSocketDescriptorMaster::Sockets[ExtSocketDescriptorMaster::MaxSockets];
ExtSocketDescriptorMaster        ExtSocketDescriptorMaster::MasterInstance;


#endif
#endif
