/* SBtrdEvent, utility class for managing events */

/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * vglue mods Copyright 2006,2007 Ampersand Inc., Doug Campbell
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
#include "SBtrdEvent.hpp"             // Header for this class

#include <stdio.h>
#include <limits.h>                   // For INT_MAX

#include "VXIlog.h"                   // For logging

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Destructor
SBtrdEvent::~SBtrdEvent( ) 
{ 
  Diag (0, L"SBtrdEvent::~SBtrdEvent", L"enter: this 0x%p", this);
  
  if ( _timer ) 
    VXItrdTimerDestroy (&_timer); 
  if ( _sleepMutex ) 
    VXItrdMutexDestroy (&_sleepMutex); 
}


// Creation method
VXItrdResult SBtrdEvent::Create( )
{ 
  VXItrdResult rc;
  Diag (0, L"SBtrdEvent::Create", L"enter: this 0x%p", this);

  if ( _timer != NULL ) {
    rc = VXItrd_RESULT_FATAL_ERROR;
  } else {
    rc = VXItrdTimerCreate (&_timer);
    if ( rc == VXItrd_RESULT_SUCCESS )
      rc = VXItrdMutexCreate (&_sleepMutex);
  }

  return rc;
}


// Reset the event
VXItrdResult SBtrdEvent::Reset( ) 
{ 
  Diag (0, L"SBtrdEvent::Reset", L"enter: this 0x%p", this);
  _alerted = FALSE; 
  return VXItrd_RESULT_SUCCESS; 
}


// Set the event
VXItrdResult SBtrdEvent::Set( ) 
{ 
  Diag (0, L"SBtrdEvent::Set", L"enter: this 0x%p", this);
  _alerted = TRUE; 
  return VXItrdTimerWake (_timer); 
}


// Wait on the event
VXItrdResult SBtrdEvent::Wait( ) 
{
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;
  Diag (0, L"SBtrdEvent::Wait", L"enter: this 0x%p", this);

  if (( ! _alerted ) &&
      ( (rc = VXItrdMutexLock (_sleepMutex)) == VXItrd_RESULT_SUCCESS )) {
    while (( ! _alerted ) && 
	   ( (rc = VXItrdTimerSleep (_timer, INT_MAX, NULL)) ==
	     VXItrd_RESULT_SUCCESS )) {
      // keep waiting
      Diag (0, L"SBtrdEvent::Wait", L"woke up: %d", _alerted);
    }
    
    if ( VXItrdMutexUnlock (_sleepMutex) != VXItrd_RESULT_SUCCESS )
      rc = VXItrd_RESULT_SYSTEM_ERROR;
  }

  Diag (0, L"SBtrdEvent::Wait", L"exit: %d", rc);
  return rc;
}


// Error logging
SBTRDUTIL_API_CLASS void
SBtrdEvent::Error (VXIunsigned errorID, const VXIchar *format, ...) const
{
  if ( _log ) {
    if ( format ) {
      va_list arguments;
      va_start(arguments, format);
      (*_log->VError)(_log, COMPANY_DOMAIN L".SBtrdUtil", errorID, format,
		      arguments);
      va_end(arguments);
    } else {
      (*_log->Error)(_log, COMPANY_DOMAIN L".SBtrdUtil", errorID, NULL);
    }
  }  
}


// Diagnostic logging
SBTRDUTIL_API_CLASS void
SBtrdEvent::Diag (VXIunsigned tag, const VXIchar *subtag, 
		  const VXIchar *format, ...) const
{
  if ( _log ) {
    if ( format ) {
      va_list arguments;
      va_start(arguments, format);
      (*_log->VDiagnostic)(_log, tag + _diagTagBase, subtag, format, 
			   arguments);
      va_end(arguments);
    } else {
      (*_log->Diagnostic)(_log, tag + _diagTagBase, subtag, NULL);
    }
#if 0
  } else {
    VXIchar temp[1024];
    va_list arguments;
    va_start(arguments, format);
    wcscpy (temp, subtag);
    wcscat (temp, L"|");
    wcscat (temp, format);
    wcscat (temp, L"\n");
    vfwprintf(stderr, temp, arguments);
    va_end(arguments);
#endif
  }
}

