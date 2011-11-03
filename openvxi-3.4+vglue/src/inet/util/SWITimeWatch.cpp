
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
#include "SWITimeWatch.hpp"

SWITimeWatch::SWITimeWatch():_elapsed(0L)
{
}

void SWITimeWatch::start()
{
  _timestamp.setTimeStamp();
}

void SWITimeWatch::reset()
{
  _elapsed = 0;
  _timestamp.reset();
}

void SWITimeWatch::stop()
{
  if (_timestamp.getSecs() == 0)
  {
    return;
  }

  SWITimeStamp now;
  now.setTimeStamp();
  _elapsed += now.delta(_timestamp);
  _timestamp.reset();
}

unsigned long SWITimeWatch::getElapsed()
{
  if (_timestamp.getSecs() != 0)
  {
    SWITimeStamp now;
    now.setTimeStamp();
    return _elapsed + now.delta(_timestamp);
  }
  else
  {
    return _elapsed;
  }
}
