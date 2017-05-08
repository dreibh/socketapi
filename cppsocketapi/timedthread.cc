/*
 *  $Id$
 *
 * SocketAPI implementation for the sctplib.
 * Copyright (C) 2005-2017 by Thomas Dreibholz
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
 * Purpose: Timed Thread Implementation
 *
 */


#include "tdsystem.h"
#include "timedthread.h"
#include "tools.h"


#include <sys/time.h>
#include <signal.h>



// ###### Constructor #######################################################
TimedThread::TimedThread(const card64   usec,
                         const char*    name,
                         const cardinal flags)
   : SingleTimerThread(name,flags)
{
   setInterval(usec);
}


// ###### Destructor ########################################################
TimedThread::~TimedThread()
{
}


// ###### The MultiTimerThread's timerEvent() implementation ################
void TimedThread::timerEvent(const cardinal timer)
{
   timerEvent();
}
