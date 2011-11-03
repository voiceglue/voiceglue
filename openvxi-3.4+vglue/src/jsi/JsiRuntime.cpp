/*****************************************************************************
 *****************************************************************************
 *
 * JsiRuntime, class for managing JavaScript runtime environments
 *
 * The JsiRuntime class represents a JavaScript interpreter runtime
 * environment, which is the overall JavaScript engine execution space
 * (compiler, interpreter, decompiler, garbage collector, etc.) which
 * can hold zero or more JavaScript contexts (JsiContext objects).
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


#include "SBjsiInternal.h"

#include "JsiRuntime.hpp"          // Defines this class

#ifndef JS_THREADSAFE
#include "VXItrd.h"                // For VXItrdMutex object and functions
#endif
#include "SBjsiLog.h"              // For logging

#include <syslog.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>

#include <jsapi.h>                 // SpiderMonkey JavaScript API

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Constructor, only does initialization that cannot fail
JsiRuntime::JsiRuntime( ) : SBjsiLogger(MODULE_SBJSI, NULL, 0), runtime(NULL)
#ifndef JS_THREADSAFE
  , mutex(NULL)
#endif
{
}


// Destructor
JsiRuntime::~JsiRuntime( )
{
  // Destroy the runtime
  if ( runtime )
    JS_DestroyRuntime (runtime);

#ifndef JS_THREADSAFE
  // Destroy the mutex
  if ( mutex )
    VXItrdMutexDestroy (&mutex);
#endif
}


// Creation method
VXIjsiResult JsiRuntime::Create (long runtimeSize,
				 VXIlogInterface *l,
				 VXIunsigned diagTagBase)
{
  if (( runtimeSize < 1 ) || ( ! l ))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  // Initialize the logging base class
  SetLog (l, diagTagBase);

  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;

#ifndef JS_THREADSAFE
  // Create the mutex
  if ( VXItrdMutexCreate (&mutex) != VXItrd_RESULT_SUCCESS )
    rc = VXIjsi_RESULT_SYSTEM_ERROR;
#endif

  if ( rc == VXIjsi_RESULT_SUCCESS ) {
    // Create the runtime, must do this within a mutex to ensure
    // this is thread safe due to us using a non-thread safe build
    // of SpiderMonkey for performance reasons
    runtime = JS_NewRuntime (runtimeSize);
    if ( runtime == NULL ) {
      rc = VXIjsi_RESULT_OUT_OF_MEMORY;
    } else {
      // Set this as the runtime's private data
      JS_SetRuntimePrivate (runtime, this);

      // Enable garbage collection tracking
      JS_SetGCCallbackRT (runtime, JsiRuntime::GCCallback);
    }
  }
  
  return rc;
}


// Get a new JavaScript context for the runtime
VXIjsiResult JsiRuntime::NewContext (long contextSize, JSContext **context)
{
  if (( contextSize < 1 ) || ( context == NULL ))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

#ifndef JS_THREADSAFE
  if ( AccessBegin( ) == false )
    return VXIjsi_RESULT_SYSTEM_ERROR;
#endif

  //  Log to voiceglue
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "JS_NewContext()";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  // Create the context
  JSContext *newContext = JS_NewContext (runtime, contextSize);
  if ( newContext == NULL )
    return VXIjsi_RESULT_OUT_OF_MEMORY;

#ifndef JS_THREADSAFE
  if ( AccessEnd( ) == false ) {
    DestroyContext (&newContext);
    return VXIjsi_RESULT_SYSTEM_ERROR;
  }
#endif

  *context = newContext;
  return VXIjsi_RESULT_SUCCESS;
}


// Destroy a JavaScript context for the runtime
VXIjsiResult JsiRuntime::DestroyContext (JSContext **context)
{
  if (( context == NULL ) || ( *context == NULL ))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

#ifndef JS_THREADSAFE
  if ( AccessBegin( ) == false )
    return VXIjsi_RESULT_SYSTEM_ERROR;
#endif

  //  Log to voiceglue
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "JS_DestroyContext()";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  // Destroy the context
  JS_DestroyContext (*context);

#ifndef JS_THREADSAFE
  if ( AccessEnd( ) == false )
    return VXIjsi_RESULT_SYSTEM_ERROR;
#endif

  *context = NULL;
  return VXIjsi_RESULT_SUCCESS;
}


#ifndef JS_THREADSAFE

// Flag that we are beginning and ending access of a context within
// this runtime, used to ensure thread safety. For simplicity,
// return true on success, false on failure (mutex error).
bool JsiRuntime::AccessBegin( ) const
{
  return (VXItrdMutexLock (mutex) == VXItrd_RESULT_SUCCESS ? true : false);
}


bool JsiRuntime::AccessEnd( ) const
{
  return (VXItrdMutexUnlock (mutex) == VXItrd_RESULT_SUCCESS ? true : false);
}

#endif /* JS_THREADSAFE */


// Static callback for diagnostic logging of garbage collection
JSBool JS_DLL_CALLBACK 
JsiRuntime::GCCallback (JSContext *context, JSGCStatus status)
{
  JSBool rc = JS_TRUE;

  // Get the runtime, then this runtime object
  JSRuntime *rt = JS_GetRuntime (context);
  if ( rt == NULL ) {
    rc = JS_FALSE; // Fatal error
  } else {
    const JsiRuntime *pThis = (const JsiRuntime *) JS_GetRuntimePrivate (rt);
    if ( pThis == NULL ) {
      return JS_FALSE; // Fatal error
    } else {
      // Log the garbage collection state
      switch (status) {
      case JSGC_BEGIN:
	pThis->Diag (SBJSI_LOG_GC, NULL, L"begin 0x%p", context);
	break;
      case JSGC_END:
	pThis->Diag (SBJSI_LOG_GC, NULL, L"end 0x%p", context);
	break;
      case JSGC_MARK_END:
	// Diag (SBJSI_LOG_GC, NULL, L"mark end 0x%p", context);
	break;
      default:
	pThis->Diag (SBJSI_LOG_GC, NULL, L"unknown %d 0x%p", status, context);
	break;
      }
    }
  }

  return rc;
}
