
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

/*****************************************************************************
 *****************************************************************************
 *
 * SBjsiLogger - logging class for SBinet
 *
 * This provides logging definitions for SBinet use of VXIlog, along
 * with some convenience macros.
 *
 *****************************************************************************
 ****************************************************************************/


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#include "SBjsiInternal.h"

#include "SBjsiLogger.hpp"                // For this class

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Determine if a tag is enabled
VXIbool SBjsiLogger::DiagIsEnabled (VXIunsigned tagID) const
{
  return (*_log->DiagnosticIsEnabled)(_log, _diagTagBase + tagID);
}


// Error logging
VXIlogResult
SBjsiLogger::Error (VXIunsigned errorID, const VXIchar *format, ...) const
{
  va_list arguments;

  if ( ! _log )
    return VXIlog_RESULT_FAILURE;
  
  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*_log->VError)(_log, _moduleName.c_str( ), errorID, format, 
			 arguments);
    va_end(arguments);
  } else {
    rc = (*_log->Error)(_log, _moduleName.c_str( ), errorID, NULL);
  }

  return rc;
}


// Error logging
VXIlogResult
SBjsiLogger::Error (VXIlogInterface *log, VXIunsigned errorID, 
		     const VXIchar *format, ...) const
{
  va_list arguments;

  if ( ! log )
    return VXIlog_RESULT_FAILURE;
  
  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*log->VError)(log, _moduleName.c_str( ), errorID, format, arguments);
    va_end(arguments);
  } else {
    rc = (*log->Error)(log, _moduleName.c_str( ), errorID, NULL);
  }

  return rc;
}


// Error logging, static
VXIlogResult
SBjsiLogger::Error (VXIlogInterface *log, const VXIchar *moduleName,
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
SBjsiLogger::Diag (VXIunsigned tag, const VXIchar *subtag, 
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
SBjsiLogger::Diag (VXIlogInterface *log, VXIunsigned tag, 
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
SBjsiLogger::Diag (VXIlogInterface *log, VXIunsigned diagTagBase,
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
SBjsiLogger::VDiag (VXIunsigned tag, const VXIchar *subtag, 
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
