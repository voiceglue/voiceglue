
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

#ifndef _VXIPLATFORM_H
#define _VXIPLATFORM_H

#include "VXItypes.h"                  /* For VXIchar definition */
#include "VXIvalue.h"                  /* For VXIMap and VXIMap */

#include "VXIheaderPrefix.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
** ==================================================
** VXIplatform functions and definitions
** ==================================================
*/

 /**
  * @name VXIplatform
  * @memo VoiceXML platform interface
  * @version 1.0
  * @doc
  * Interface that supports multiple system operator user interfaces 
  * by initializing the OpenVXI browser components and coordinating
  * them to service calls.
  */

/*@{*/
 /**
  * Result codes for the platform functions
  *
  * Result codes less then zero are severe errors (likely to be
  * platform faults), those greater then zero are warnings (likely to
  * be application issues) 
  */
typedef enum VXIplatformResult {
  /* Fatal error, shutdown          */
  VXIplatform_RESULT_FATAL_ERROR         = -100,
  /* Platform already initialized   */
  VXIplatform_RESULT_ALREADY_INITIALIZED =  -60,
  /* Platform not initialized       */
  VXIplatform_RESULT_NOT_INITIALIZED     =  -59,
  /* VXIinet interface error        */
  VXIplatform_RESULT_INET_ERROR          =  -58,
  /* VXIinterpreter interface error */
  VXIplatform_RESULT_INTERPRETER_ERROR   =  -57,
  /* VXIjsi interface error         */
  VXIplatform_RESULT_JSI_ERROR           =  -56,
  /* VXIlog interface error         */
  VXIplatform_RESULT_LOG_ERROR           =  -55,
  /* VXIprompt interface error      */
  VXIplatform_RESULT_PROMPT_ERROR        =  -54,
  /* VXIrec interface error         */
  VXIplatform_RESULT_REC_ERROR           =  -53,
  /* VXItel interface error         */
  VXIplatform_RESULT_TEL_ERROR           =  -52,
  /* VXItrd interface error         */
  VXIplatform_RESULT_TRD_ERROR           =  -51,
  /* VXIvalue interface error       */
  VXIplatform_RESULT_VALUE_ERROR         =  -50,
  /* I/O error                      */
  VXIplatform_RESULT_IO_ERROR            =   -8,
  /* Out of memory                  */
  VXIplatform_RESULT_OUT_OF_MEMORY       =   -7,
  /* System error, out of service   */
  VXIplatform_RESULT_SYSTEM_ERROR        =   -6,
  /* Errors from platform services  */
  VXIplatform_RESULT_PLATFORM_ERROR      =   -5,
  /* Return buffer too small        */
  VXIplatform_RESULT_BUFFER_TOO_SMALL    =   -4,
  /* Property name is not valid     */
  VXIplatform_RESULT_INVALID_PROP_NAME   =   -3,
  /* Property value is not valid    */
  VXIplatform_RESULT_INVALID_PROP_VALUE  =   -2,
  /* Invalid function argument      */
  VXIplatform_RESULT_INVALID_ARGUMENT    =   -1,
  /* Success                        */
  VXIplatform_RESULT_SUCCESS             =    0,
  /* Normal failure, nothing logged */
  VXIplatform_RESULT_FAILURE             =    1,
  /* Non-fatal non-specific error   */
  VXIplatform_RESULT_NON_FATAL_ERROR     =    2,
  /* Operation is not supported     */
  VXIplatform_RESULT_UNSUPPORTED         =  100
} VXIplatformResult;


/* VXIplatform is an opaque data structure whose actual definition depends on
 * the specific platform details.  Its actual definition is only required in
 * the actual implementations of the functions defined below.  */
struct VXIplatform;
typedef struct VXIplatform VXIplatform;


/**
 * Performs initialization of the platform layer.
 *
 * @param args Configuration arguments. Implementation dependant.
 * @param nbChannels The address of a pre-allocated VXIunsigned that is
 *                   initialized with the number of available channels.
 *
 * @return  VXIplatform_SUCCES: success.
 *
 *          VXIplatform_INVALID_ARGUMENT: the availableChannels is NULL or
 *          args does not contain valid configuration information.
 *
 *          VXIplatform_ALREADY_INITIALIZED: the platform already had been
 *          successfully initialized.
 *
 *          VXIplatform_TEL_ERROR: Cannot initialize the VXItel layer.
 *          VXIplatform_PROMPT_ERROR: Cannot initialize the VXIprompt layer.
 *          VXIplatform_REC_ERROR: Cannot initialize the VXIrec layer.
 *
 **/
VXIplatformResult VXIplatformInit(VXIMap *args, VXIunsigned *nbChannels);

/**
 * Performs final cleanup of the platform layer.
 *
 * @return VXIplatform_SUCCESS: success.
 *         VXIplatform_NOT_INITIALIZED: layer not currently initialized.
 **/
VXIplatformResult VXIplatformShutdown(void);

/**
 * Creates a VXIplatform object for the specified channel number.  This
 * creates the VXItel, VXIprompt and VXIrec objects associated with this
 * channel.
 *
 * @param channelNum[in] The channel number of the platform to be created.
 * @param args[in] Configuration arguments. Implementation dependant.
 * @param platform[out] A pointer to the platform to be allocated.
 **/
VXIplatformResult VXIplatformCreateResources(VXIunsigned channelNum,
					     VXIMap *args,
					     VXIplatform **platform);

/**
 * Destroys the VXIplatform and any associated resources.
 * Destroys the VXItel, VXIprompt and VXIrec objects associated with this
 * channel.
 **/
VXIplatformResult VXIplatformDestroyResources(VXIplatform **platform);

/**
 * Set platform diagnostic tracing on or off
 **/
void VXIplatformSetTrace (int state);

/**
 * Enables the hardware to wait for call. Will return when the
 * hardware is enabled.  Must be called before WaitForCall.
 **/
VXIplatformResult VXIplatformEnableCall(VXIplatform *platform);

/**
 * Waits for a call on the specified channel and answers the call.
 **/
VXIplatformResult VXIplatformWaitForCall(VXIplatform *platform);

/**
 * Starts the processing of the root document associated with the
 * channel.

 * @param url    [IN] Name of the VoiceXML document to fetch and 
 *                  execute, may be a URL or a platform dependant path.
 *                  See the Open( ) method in VXIinet.h for details
 *                  about supported names, however for URLs this
 *                  must always be an absolute URL and any query arguments
 *                  must be embedded.
 * @param sessionArgs [IN] Any arguments to be passed to the VXI.  Some of
 *                  these, such as ANI, DNIS, etc. as required by VXML, but
 *                  anything may be passed in.  These values are available
 *                  through the session variable in ECMA script.
 * @param result  [OUT] (Optional, pass NULL if not desired.) Return
 *                  value for the VoiceXML document (from <exit/>), this
 *                  is allocated on success and when there is an
 *                  exit value (a NULL pointer is returned otherwise),
 *                  the caller is responsible for destroying the returned
 *                  value by calling VXIValueDestroy( ). If
 *                  VXIinterp_RESULT_UNCAUGHT_FATAL_EVENT is returned,
 *                  this will be a VXIString that provides the name
 *                  of the VoiceXML event that caused the interpreter
 *                  to exit.
 *
 * @return         VXIplatformResult
 **/
VXIplatformResult VXIplatformProcessDocument(const VXIchar *url,
					     VXIMap *callInfo,
					     VXIchar **sessionArgScript,
					     VXIValue **result,
					     VXIplatform *platform);

/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
