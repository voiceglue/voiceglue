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

#ifndef _SBTRD_TIMEOFDAY_H__
#define _SBTRD_TIMEOFDAY_H__

#include "VXIheaderPrefix.h"          // For SYMBOL_EXPORT_CPP_DECL
#include <time.h>                     // For time_t
#include "VXItypes.h"                 // For VXIint, VXIlong, etc.

#ifdef SBTRDUTIL_EXPORTS
#define SBTRDUTIL_API_CLASS SYMBOL_EXPORT_CPP_CLASS_DECL
#else
#define SBTRDUTIL_API_CLASS SYMBOL_IMPORT_CPP_CLASS_DECL
#endif

class SBTRDUTIL_API_CLASS SBtrdTimeOfDay {
 public:
  // Constructor and destructor
  SBtrdTimeOfDay();
  virtual ~SBtrdTimeOfDay();

  // Set the time to the current time of day
  bool SetCurrentTime( );

  // Clear the time
  void Clear( );

  // Offset the time of day
  void operator+= (VXIint32 offsetMs);

  // Determine if the time is set
  bool IsSet( ) const;

  // Get the milliseconds offset from the current time value
  VXIlong GetMsecBeforeTime( ) const;
  VXIlong GetMsecAfterTime( ) const;

  // Get the milliseconds offset from another time value
  VXIlong GetMsecBeforeTime (const SBtrdTimeOfDay &tod) const;
  VXIlong GetMsecAfterTime (const SBtrdTimeOfDay &tod) const;

  // Comparison operators
  bool operator< (const SBtrdTimeOfDay &tod) const;
  bool operator> (const SBtrdTimeOfDay &tod) const;
  bool operator== (const SBtrdTimeOfDay &tod) const;

 private:
  time_t          _time;
  unsigned short  _millitm;
};

#include "VXIheaderSuffix.h"

#endif  // include guard
