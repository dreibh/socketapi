/*
 *  $Id: staticglobals.cc,v 1.1 2003/05/15 11:35:50 dreibh Exp $
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
set<Thread*>         Thread::ThreadSet;
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


int                         SCTPSocketMaster::InitializationResult     = -1000;
int                         SCTPSocketMaster::GarbageCollectionTimerID = -1;
cardinal                    SCTPSocketMaster::LockLevel                = 0;
cardinal                    SCTPSocketMaster::OldCancelState           = true;
card64                      SCTPSocketMaster::LastGarbageCollection;
set<int>                    SCTPSocketMaster::ClosingSockets;
multimap<unsigned int, int> SCTPSocketMaster::ClosingAssociations;
multimap<int, SCTPSocket*>  SCTPSocketMaster::SocketList;
SCTP_ulpCallbacks           SCTPSocketMaster::Callbacks;
SCTPSocketMaster            SCTPSocketMaster::MasterInstance;
Randomizer                  SCTPSocketMaster::Random;
int                         SCTPSocketMaster::BreakPipe[2];
struct SCTPSocketMaster::UserSocketNotification SCTPSocketMaster::BreakNotification;

ExtSocketDescriptor         ExtSocketDescriptorMaster::Sockets[ExtSocketDescriptorMaster::MaxSockets];
ExtSocketDescriptorMaster   ExtSocketDescriptorMaster::MasterInstance;


#endif
#endif
