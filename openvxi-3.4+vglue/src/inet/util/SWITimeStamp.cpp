
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * Vocalocity, the Vocalocity logo, and VocalOS are trademarks or 
 * registered trademarks of Vocalocity, Inc. 
 * OpenVXI is a trademark of Scansoft, Inc. and used under license 
 * by Vocalocity.
 ***********************************************************************/

#include "SWIutilInternal.h"
#include "SWITimeStamp.hpp"

#ifdef _WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

SWITimeStamp::SWITimeStamp():_secs(0),_msecs(0),_addedDelay(0)
{}

SWITimeStamp::~SWITimeStamp()
{}

time_t
SWITimeStamp::getSecs() const
{
  return _secs;
}

unsigned short
SWITimeStamp::getMsecs() const
{
  return _msecs;
}

void
SWITimeStamp::reset()
{
  _secs = 0;
  _msecs = 0;
  _addedDelay = 0;
}

/**
 * Assigns the current system time to this timestamp.
 **/
void SWITimeStamp::setTimeStamp()
{
#ifdef _WIN32
  struct _timeb now;

  _ftime(&now);
  _secs = now.time;
  _msecs = now.millitm;
#else
  struct timeval now;
  gettimeofday(&now, NULL);
  _secs = now.tv_sec;
  _msecs = now.tv_usec / 1000;
#endif
}

/**
 * Reset the timestamp to current system time then add the
 * delay that previously set by addDelay()
 **/
void SWITimeStamp::resetTimeStamp(void)
{
  setTimeStamp();
  if( _addedDelay > 0 ) addDelay(_addedDelay);
}


/**
 * Adds this number of milliseconds to the current timestamp.
 **/
void SWITimeStamp::addDelay(unsigned long msecs)
{
  _addedDelay = msecs;
  _secs += (time_t) msecs / 1000;
  _msecs += (unsigned short) (msecs % 1000);

  if (_msecs >= 1000)
  {
    _msecs -= 1000;
    _secs++;
  }
}

/**
 * return milliseconds value of the previously call to addDelay()
 **/
unsigned long SWITimeStamp::getAddedDelay(void)
{
  return _addedDelay;
}

/**
 * Returns the number of milliseconds between this timestamp and the
 * timestamp passed as an argument.
 */
long SWITimeStamp::delta(const SWITimeStamp& timestamp) const
{
  long result = (_secs - timestamp._secs) * 1000;
  result += (_msecs - timestamp._msecs);
  return result;
}

int SWITimeStamp::compare(const SWITimeStamp& timestamp) const
{
  if (_secs < timestamp._secs)
    return -1;

  if (_secs > timestamp._secs)
    return 1;

  if (_msecs < timestamp._msecs)
    return -1;

  if (_msecs > timestamp._msecs)
    return 1;

  return 0;
}
