
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

#include "SBinetStoppable.hpp"
#ifdef _WIN32
#include <sys/timeb.h>
#endif

void SBinetStoppable::setTimeOut(const SWITimeStamp *timestamp)
{
  if (timestamp == NULL)
  {
    _expiration.reset();
  }
  else
  {
    _expiration = *timestamp;
  }
}

bool SBinetStoppable::hasTimedOut() const
{
  if (_expiration.getSecs() == (time_t) 0)
    return false;

  SWITimeStamp now;
  now.setTimeStamp();

  return _expiration.compare(now) <= 0;
}

long SBinetStoppable::getDelay() const
{
  if (_expiration.getSecs() == (time_t) 0)
    return -1;

  SWITimeStamp now;
  now.setTimeStamp();

  long result = _expiration.delta(now);
  if (result < 0) result = 0;
  return result;
}
