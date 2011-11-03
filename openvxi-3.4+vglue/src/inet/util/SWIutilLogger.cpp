/*****************************************************************************
 *****************************************************************************
 *
 * SWIutilLogger - logging class for SWIutil
 *
 * This provides logging definitions for SWIutil use of VXIlog, along
 * with some convenience macros.
 *
 *****************************************************************************
 ****************************************************************************/

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

#include "SWIutilInternal.h"
#include "SWIutilLogger.hpp"                // For this class

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

SWIutilLogger::SWIutilLogger(const VXIchar *moduleName, VXIlogInterface *log,
                             VXIunsigned diagTagBase):
  _moduleName(NULL),_log(log), _diagTagBase(diagTagBase)
{
  if (moduleName != NULL)
  {
    _moduleName = new VXIchar[::wcslen(moduleName) + 1];
    if (_moduleName != NULL)
      ::wcscpy(_moduleName, moduleName);
  }
}

SWIutilLogger::SWIutilLogger(const SWIutilLogger& l):
  _moduleName(NULL), _log(l._log), _diagTagBase(l._diagTagBase)
{
  if (l._moduleName != NULL)
  {
    _moduleName = new VXIchar[::wcslen(l._moduleName) + 1];
    if (_moduleName != NULL)
      ::wcscpy(_moduleName, l._moduleName);
  }
}

SWIutilLogger::~SWIutilLogger()
{
  if (_moduleName != NULL)
    delete [] _moduleName;
}

void SWIutilLogger::SetLog(VXIlogInterface *log, VXIunsigned diagTagBase)
{
  _log = log;
  _diagTagBase = diagTagBase;
}

const VXIchar *SWIutilLogger::GetModuleName() const
{
  return _moduleName;
}

VXIlogInterface *SWIutilLogger::GetLog() const
{
  return _log;
}

VXIunsigned SWIutilLogger::GetDiagBase() const
{
  return _diagTagBase;
}

// Determine if a tag is enabled
VXIbool SWIutilLogger::DiagIsEnabled (VXIunsigned tagID) const
{
  if ( ! _log )
    return FALSE;
  return (*_log->DiagnosticIsEnabled)(_log, _diagTagBase + tagID);
}


// Error logging
VXIlogResult
SWIutilLogger::Error (VXIunsigned errorID, const VXIchar *format, ...) const
{
  va_list arguments;

  if ( ! _log )
    return VXIlog_RESULT_FAILURE;

  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*_log->VError)(_log, _moduleName, errorID, format,
			 arguments);
    va_end(arguments);
  } else {
    rc = (*_log->Error)(_log, _moduleName, errorID, NULL);
  }

  return rc;
}


// Error logging
VXIlogResult
SWIutilLogger::Error (VXIlogInterface *log, VXIunsigned errorID,
		     const VXIchar *format, ...) const
{
  va_list arguments;

  if ( ! log )
    return VXIlog_RESULT_FAILURE;

  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*log->VError)(log, _moduleName, errorID, format, arguments);
    va_end(arguments);
  } else {
    rc = (*log->Error)(log, _moduleName, errorID, NULL);
  }

  return rc;
}


// Error logging, static
VXIlogResult
SWIutilLogger::Error (VXIlogInterface *log, const VXIchar *moduleName,
		     VXIunsigned errorID, const VXIchar *format, ...)
{
  va_list arguments;

  if ( ! log )
    return VXIlog_RESULT_FAILURE;

  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*log->VError)(log, moduleName, errorID, format, arguments);
    va_end(arguments);
  } else {
    rc = (*log->Error)(log, moduleName, errorID, NULL);
  }

  return rc;
}


// Diagnostic logging
VXIlogResult
SWIutilLogger::Diag (VXIunsigned tag, const VXIchar *subtag,
		   const VXIchar *format, ...) const
{
  va_list arguments;

  if ( ! _log )
    return VXIlog_RESULT_FAILURE;

  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*_log->VDiagnostic)(_log, tag + _diagTagBase, subtag, format,
			      arguments);
    va_end(arguments);
  } else {
    rc = (*_log->Diagnostic)(_log, tag + _diagTagBase, subtag, NULL);
  }

  return rc;
}


// Diagnostic logging
VXIlogResult
SWIutilLogger::Diag (VXIlogInterface *log, VXIunsigned tag,
		    const VXIchar *subtag, const VXIchar *format, ...) const
{
  va_list arguments;

  if ( ! log )
    return VXIlog_RESULT_FAILURE;

  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*log->VDiagnostic)(log, tag + _diagTagBase, subtag, format,
			     arguments);
    va_end(arguments);
  } else {
    rc = (*log->Diagnostic)(log, tag + _diagTagBase, subtag, NULL);
  }

  return rc;
}


// Diagnostic logging, static
VXIlogResult
SWIutilLogger::Diag (VXIlogInterface *log, VXIunsigned diagTagBase,
		    VXIunsigned tag, const VXIchar *subtag,
		    const VXIchar *format, ...)
{
  va_list arguments;

  if ( ! log )
    return VXIlog_RESULT_FAILURE;

  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*log->VDiagnostic)(log, tag + diagTagBase, subtag, format,
			     arguments);
    va_end(arguments);
  } else {
    rc = (*log->Diagnostic)(log, tag + diagTagBase, subtag, NULL);
  }

  return rc;
}


// Diagnostic logging, va args
VXIlogResult
SWIutilLogger::VDiag (VXIunsigned tag, const VXIchar *subtag,
		     const VXIchar *format, va_list args) const
{
  if ( ! _log )
    return VXIlog_RESULT_FAILURE;

  VXIlogResult rc;
  if ( format ) {
    rc = (*_log->VDiagnostic)(_log, tag + _diagTagBase, subtag, format, args);
  } else {
    rc = (*_log->Diagnostic)(_log, tag + _diagTagBase, subtag, NULL);
  }

  return rc;
}
