/* SBtrdTimeOfDay, utility class for handling a time of day */

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

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#define SBTRDUTIL_EXPORTS
#include "SBtrdTimeOfDay.hpp"           // For this class

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <sys/types.h>
#include <sys/timeb.h>                 // for ftime( )/_ftime( )

#ifdef WIN32
#define SBtrdFtime(x) _ftime(x)
#define SBtrdTimeb struct _timeb
#else
#define SBtrdFtime(x) ftime(x)
#define SBtrdTimeb struct timeb
#endif

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Constructor
SBTRDUTIL_API_CLASS SBtrdTimeOfDay::SBtrdTimeOfDay( ) :
  _time(0), _millitm(0)
{
}


// Destructor
SBTRDUTIL_API_CLASS SBtrdTimeOfDay::~SBtrdTimeOfDay( )
{
}


// Set the time to the current time of day
SBTRDUTIL_API_CLASS bool SBtrdTimeOfDay::SetCurrentTime( )
{
  SBtrdTimeb tbuf;
  SBtrdFtime(&tbuf);
  _time = tbuf.time;
  _millitm = tbuf.millitm;

  return true;
}


// Clear the time
SBTRDUTIL_API_CLASS void SBtrdTimeOfDay::Clear( ) 
{
  _time = 0;
  _millitm = 0;
}


// Offset the time of day
SBTRDUTIL_API_CLASS void SBtrdTimeOfDay::operator+=(VXIint32 offsetMs)
{
  _time += static_cast<time_t>(offsetMs / 1000);
  _millitm += static_cast<unsigned short>(offsetMs % 1000);
  if ( _millitm > 1000 ) {
    _time++;
    _millitm -= 1000;
  }
}


// Determine if the time is set
SBTRDUTIL_API_CLASS bool SBtrdTimeOfDay::IsSet( ) const 
{
  return ((_time) || (_millitm) ? true : false);
}


// Get the milliseconds offset from the current time value
SBTRDUTIL_API_CLASS VXIlong SBtrdTimeOfDay::GetMsecBeforeTime( ) const
{
  SBtrdTimeOfDay current;
  current.SetCurrentTime( );
  return (_time - current._time) * 1000 + (_millitm - current._millitm);
}


// Get the milliseconds offset from the current time value
SBTRDUTIL_API_CLASS VXIlong SBtrdTimeOfDay::GetMsecAfterTime( ) const 
{
  return -GetMsecBeforeTime( );
}


// Get the milliseconds offset from another time value
SBTRDUTIL_API_CLASS VXIlong 
SBtrdTimeOfDay::GetMsecBeforeTime (const SBtrdTimeOfDay &tod) const
{
  return (_time - tod._time) * 1000 + (_millitm - tod._millitm);
}


// Get the milliseconds offset from another time value
SBTRDUTIL_API_CLASS VXIlong 
SBtrdTimeOfDay::GetMsecAfterTime (const SBtrdTimeOfDay &tod) const 
{
  return -GetMsecBeforeTime (tod); 
}


// Comparison operator
SBTRDUTIL_API_CLASS bool 
SBtrdTimeOfDay::operator< (const SBtrdTimeOfDay &tod) const 
{
  return ((_time < tod._time) || 
          ((_time == tod._time) && (_millitm < tod._millitm)));
}


// Comparison operator
SBTRDUTIL_API_CLASS bool 
SBtrdTimeOfDay::operator> (const SBtrdTimeOfDay &tod) const 
{
  return ((_time > tod._time) || 
          ((_time == tod._time) && (_millitm > tod._millitm)));
}


// Comparison operator
SBTRDUTIL_API_CLASS bool 
SBtrdTimeOfDay::operator== (const SBtrdTimeOfDay &tod) const 
{
  return ((_time != tod._time) || (_millitm != tod._millitm));
}
