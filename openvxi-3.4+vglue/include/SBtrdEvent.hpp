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

#ifndef _SBTRD_EVENT_H__
#define _SBTRD_EVENT_H__

#include "VXIheaderPrefix.h"          // For SYMBOL_EXPORT_CPP_DECL
#include "VXItrd.h"                   // For VXItrdTimer, VXItrdMutex

#ifdef SBTRDUTIL_EXPORTS
#define SBTRDUTIL_API_CLASS SYMBOL_EXPORT_CPP_CLASS_DECL
#else
#define SBTRDUTIL_API_CLASS SYMBOL_IMPORT_CPP_CLASS_DECL
#endif

extern "C" struct VXIlogInterface;

class SBTRDUTIL_API_CLASS SBtrdEvent {
 public:
  // Constructor and destructor
  SBtrdEvent (VXIlogInterface *log = NULL, VXIunsigned diagTagBase = 0) :
    _diagTagBase(diagTagBase), _log(log), _timer(NULL), _sleepMutex(NULL),
    _alerted(FALSE) { }
  virtual ~SBtrdEvent( );

  // Creation method
  VXItrdResult Create( );

  // Reset the event
  VXItrdResult Reset( );

  // Set the event
  VXItrdResult Set( );

  // Wait on the event
  VXItrdResult Wait( );

 private:
  // Error logging
  void Error (VXIunsigned errorID, const VXIchar *format, ...) const;

  // Diagnostic logging
  void Diag (VXIunsigned tag, const VXIchar *subtag, 
	     const VXIchar *format, ...) const;

  // Disable the copy constructor and equality operator to catch making
  // copies at compile or link time, neither is really implemented
  SBtrdEvent (const SBtrdEvent &);
  SBtrdEvent & operator= (const SBtrdEvent &);

 private:
  VXIunsigned       _diagTagBase;
  VXIlogInterface  *_log;
  VXItrdTimer      *_timer;
  VXItrdMutex      *_sleepMutex;
  volatile VXIbool  _alerted;
};

#include "VXIheaderSuffix.h"

#endif  // _SBTRD_EVENT_H__
