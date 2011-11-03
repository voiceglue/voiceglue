
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
 * Implementation of the SBjsi functions defined in SBjsi.h and the
 * OSBjsi functions defined in OSBjsi.h, see those headers for details.
 * Those headers are functionally redundant, the OSB version and entry
 * points are for the OpenVXI open source release, while the SB version
 * and entry points are for the OpenSpeech Browser PIK product release.
 *
 *****************************************************************************
 ****************************************************************************/


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


#include "SBjsiInternal.h"

#include <string.h>                     // For memset( )

#define SBJSI_EXPORTS
#include "SBjsi.h"                      // Header for this interface
#include "SBjsiAPI.h"                   // Header for the API functions
#include "SBjsiLog.h"                   // For logging
#include "JsiRuntime.hpp"               // For JsiRuntime class
#include "SBjsiInterface.h"             // For SBjsiInterface

#include "jsapi.h"                      // For JS_ShutDown( )

// Global variable to track whether this is initialized
static bool gblInitialized = false;

// Global runtime, used across the entire system
static JsiRuntime *gblJsiRuntime = NULL;

// Runtime and Context sizes in bytes for each new runtime/context
static long gblRuntimeSize = 0;
static long gblContextSize = 0;

// Maximum number of JavaScript branches per script evaluation
static long gblMaxBranches = 0;

// Offset for diagnostic logging
static VXIunsigned gblDiagTagBase = 0;


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


/**
 * Global platform initialization of JavaScript
 *
 * @param log           VXI Logging interface used for error/diagnostic 
 *                      logging, only used for the duration of this 
 *                      function call
 * @param  diagLogBase  Base tag number for diagnostic logging purposes.
 *                      All diagnostic tags for SBjsi will start at this
 *                      ID and increase upwards.
 * @param  runtimeSize  Size of the JavaScript runtime environment, 
 *                      in bytes, see above for a recommended default
 * @param  contextSize  Size of each JavaScript context, in bytes, see
 *                      above for a recommended default
 * @param  maxBranches  Maximum number of JavaScript branches for each
 *                      JavaScript evaluation, used to interrupt infinite
 *                      loops from (possibly malicious) scripts
 *
 * @result VXIjsiResult 0 on success
 */
SBJSI_API VXIjsiResult SBjsiInit (VXIlogInterface  *log,
				  VXIunsigned       diagTagBase,
				  VXIlong           runtimeSize,
				  VXIlong           contextSize,
				  VXIlong           maxBranches)
{
  static const wchar_t func[] = L"SBjsiInit";
  if ( log )
    log->Diagnostic (log, diagTagBase + SBJSI_LOG_API, func, 
		     L"entering: 0x%p, %u, %ld, %ld, %ld",
		     log, diagTagBase, runtimeSize, contextSize, maxBranches);

  // Make sure this wasn't already called
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  if ( gblInitialized == true ) {
    SBjsiLogger::Error (log, MODULE_SBJSI, JSI_ERROR_INIT_FAILED, NULL);
    rc = VXIjsi_RESULT_FATAL_ERROR;
    if ( log )
      log->Diagnostic (log, diagTagBase + SBJSI_LOG_API, func, 
		       L"exiting: returned %d", rc);
    return rc;
  }

  // Check arguments
  if (( ! log ) || ( runtimeSize <= 0 ) || ( contextSize <= 0 ) || 
      ( maxBranches <= 0 )) {
    SBjsiLogger::Error (log, MODULE_SBJSI, JSI_ERROR_INIT_FAILED, NULL);
    rc = VXIjsi_RESULT_INVALID_ARGUMENT;
    if ( log )
      log->Diagnostic (log, diagTagBase + SBJSI_LOG_API, func, 
		       L"exiting: returned %d", rc);
    return rc;
  }

  // Create the global runtime environment
  gblJsiRuntime = new JsiRuntime( );
  if ( gblJsiRuntime == NULL ){
    SBjsiLogger::Error (log, MODULE_SBJSI, JSI_ERROR_OUT_OF_MEMORY, NULL);
    rc = VXIjsi_RESULT_OUT_OF_MEMORY;
  }
  else
    rc = gblJsiRuntime->Create (runtimeSize, log, diagTagBase);

  // Finish creation
  if ( rc == VXIjsi_RESULT_SUCCESS ) {
    gblRuntimeSize = runtimeSize;
    gblContextSize = contextSize;
    gblMaxBranches = maxBranches;
    gblDiagTagBase = diagTagBase;
    gblInitialized = true;
  } else if ( gblJsiRuntime ) {
    delete gblJsiRuntime;
  }

  log->Diagnostic (log, diagTagBase + SBJSI_LOG_API, func, 
		   L"exiting: returned %d", rc);
  return rc;
}

/**
 * Global platform shutdown of Jsi
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               only used for the duration of this function call
 *
 * @result VXIjsiResult 0 on success
 */
SBJSI_API VXIjsiResult SBjsiShutDown (VXIlogInterface  *log)
{
  static const wchar_t func[] = L"SBjsiShutDown";
  if ( log )
    log->Diagnostic (log, gblDiagTagBase + SBJSI_LOG_API, func, 
		     L"entering: 0x%p", log);

  // Make sure we've been created successfully
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  if ( gblInitialized == false ) {
    SBjsiLogger::Error (log, MODULE_SBJSI, JSI_ERROR_NOT_INITIALIZED, NULL);
    rc = VXIjsi_RESULT_FATAL_ERROR;
    if ( log )
      log->Diagnostic (log, gblDiagTagBase + SBJSI_LOG_API, func, 
		       L"exiting: returned %d", rc);
    return rc;
  }

  if ( ! log )
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  // Destroy the runtime environment
  if ( gblJsiRuntime )
    delete gblJsiRuntime;
  
  // Shut down SpiderMonkey 
  JS_ShutDown( );
  
  // Finish shutdown
  gblRuntimeSize = 0;
  gblContextSize = 0;
  gblMaxBranches = 0;
  gblInitialized = false;

  log->Diagnostic (log, gblDiagTagBase + SBJSI_LOG_API, func, 
		   L"exiting: returned %d", rc);
  return rc;
}

/**
 * Create a new jsi service handle
 *
 * @param log    VXI Logging interface used for error/diagnostic 
 *               logging, must remain a valid pointer throughout the 
 *               lifetime of the resource (until SBjsiDestroyResource( )
 *               is called)
 *
 * @result VXIjsiResult 0 on success 
 */
SBJSI_API 
VXIjsiResult SBjsiCreateResource(VXIlogInterface     *log,
				 VXIjsiInterface    **jsi)
{
  static const wchar_t func[] = L"SBjsiCreateResource";
  if ( log )
    log->Diagnostic (log, gblDiagTagBase + SBJSI_LOG_API, func, 
		     L"entering: 0x%p, 0x%p", log, 
		     jsi);

  // Make sure we've been created successfully
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  if ( gblInitialized == false ) {
    SBjsiLogger::Error (log, MODULE_SBJSI, JSI_ERROR_NOT_INITIALIZED, NULL);
    rc = VXIjsi_RESULT_FATAL_ERROR;
    if ( log )
      log->Diagnostic (log, gblDiagTagBase + SBJSI_LOG_API, func, 
		       L"exiting: returned %d", rc);
    return rc;
  }

  if (( log == NULL ) || ( jsi == NULL )) {
    SBjsiLogger::Error (log, MODULE_SBJSI, JSI_ERROR_INVALID_ARG, NULL);
    rc = VXIjsi_RESULT_INVALID_ARGUMENT;
    if ( log )
      log->Diagnostic (log, gblDiagTagBase + SBJSI_LOG_API, func, 
		       L"exiting: returned %d", rc);
    return rc;
  }

  // Get a new interface instance
  SBjsiInterface *newJsi = new SBjsiInterface;
  if ( ! newJsi ) {
    SBjsiLogger::Error (log, MODULE_SBJSI, JSI_ERROR_INTERFACE_ALLOC_FAILED,
			 NULL);
    rc = VXIjsi_RESULT_OUT_OF_MEMORY;
  } else {
    memset (newJsi, 0, sizeof (SBjsiInterface));
    
    // Initialize the function pointers
    newJsi->jsi.GetVersion = SBjsiGetVersion;
    newJsi->jsi.GetImplementationName = SBjsiGetImplementationName;
    newJsi->jsi.CreateContext = SBjsiCreateContext;
    newJsi->jsi.DestroyContext = SBjsiDestroyContext;
    newJsi->jsi.CreateVarExpr = SBjsiCreateVarExpr;
    newJsi->jsi.CreateVarValue = SBjsiCreateVarValue;
	newJsi->jsi.CreateVarDOM = SBjsiCreateVarDOM;
    newJsi->jsi.SetVarExpr = SBjsiSetVarExpr;
    newJsi->jsi.SetVarValue = SBjsiSetVarValue;
    newJsi->jsi.SetReadOnly = SBjsiSetReadOnly;
    newJsi->jsi.GetVar = SBjsiGetVar;
    newJsi->jsi.CheckVar = SBjsiCheckVar;
    newJsi->jsi.Eval = SBjsiEval;
    newJsi->jsi.PushScope = SBjsiPushScope;
    newJsi->jsi.PopScope = SBjsiPopScope;
    newJsi->jsi.ClearScopes = SBjsiClearScopes;
	newJsi->jsi.GetLastError = SBjsiGetLastError;
    
    // Initialize the data members
    newJsi->contextSize = gblContextSize;
    newJsi->maxBranches = gblMaxBranches;
    newJsi->jsiRuntime = gblJsiRuntime;
    newJsi->log = log;
    newJsi->diagTagBase = gblDiagTagBase;
  }

  if ( rc != VXIjsi_RESULT_SUCCESS ) {
    if ( newJsi )
      delete newJsi;
  } else {
    // Return the object
    *jsi = &(newJsi->jsi);
  }

  log->Diagnostic (log, gblDiagTagBase + SBJSI_LOG_API, func, 
		   L"exiting: returned %d", rc);
  return rc;
}

/**
 * Destroy the interface and free internal resources. Once this is
 *  called, the logging interface passed to SBjsiCreateResource( ) may
 *  be released as well.
 *
 * @result VXIjsiResult 0 on success 
 */
SBJSI_API 
VXIjsiResult SBjsiDestroyResource(VXIjsiInterface **jsi)
{
  static const wchar_t func[] = L"SBjsiDestroyResource";
  // Can't log yet, don't have a log handle

  // Make sure we've been created successfully
  if ( gblInitialized == false )
    return VXIjsi_RESULT_FATAL_ERROR;

  if (( jsi == NULL ) || ( *jsi == NULL ))
    return VXIjsi_RESULT_INVALID_ARGUMENT;

  // Get the real underlying interface
  VXIjsiResult rc = VXIjsi_RESULT_SUCCESS;
  SBjsiInterface *sbJsi = (SBjsiInterface *) *jsi;

  VXIlogInterface *log = sbJsi->log;
  log->Diagnostic (log, gblDiagTagBase + SBJSI_LOG_API, func, 
		   L"entering: 0x%p (0x%p)", jsi, *jsi);

  // Delete the object
  delete sbJsi;
  *jsi = NULL;

  log->Diagnostic (log, gblDiagTagBase + SBJSI_LOG_API, func, 
		   L"exiting: returned %d", rc);
  return rc;
}
