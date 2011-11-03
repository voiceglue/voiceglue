
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

#ifndef _VXIINTERPRETER_H
#define _VXIINTERPRETER_H

#include "VXItypes.h"                  /* For VXIchar */
#include "VXIvalue.h"                  /* For VXIValue */

#ifdef __cplusplus
struct VXIcacheInterface;
struct VXIinetInterface;
struct VXIjsiInterface;
struct VXIlogInterface;
struct VXIobjectInterface;

struct VXIpromptInterface;
struct VXIrecInterface;
struct VXItelInterface;
#else
#include "VXIcache.h"
#include "VXIinet.h"
#include "VXIjsi.h"
#include "VXIlog.h"
#include "VXIobject.h"
#include "VXIprompt.h"
#include "VXIrec.h"
#include "VXItel.h"
#endif

#include "VXIheaderPrefix.h"
#ifdef VXI_EXPORTS
#define VXI_INTERPRETER SYMBOL_EXPORT_DECL
#else
#define VXI_INTERPRETER SYMBOL_IMPORT_DECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
  \defgroup VXI Interpreter interface
  
  The OpenSpeech Browser core is the OpenVXI.  The
  VXIinterpreter interface implements the VXI interface function to run
  the interface. In addition a set of process and thread initialization
  routines are provided to set-up and destroy the interpreter per thread.
 */

/*@{*/

/**
 * VXI Runtime property for the URI to the beep audio (See SetProperties).
 * The VXIValue passed should be of type VXIString.  The default is empty.
 */
#define VXI_BEEP_AUDIO L"vxi.property.beep.uri"

/**
 * VXI Runtime property for the URI to the platform defaults script (See SetProperties).
 * The VXIValue passed should be of type VXIString.  The default is to use
 * an internal defaults script.
 */
#define VXI_PLATFORM_DEFAULTS L"vxi.property.platform.defaults"

/**
 * VXI Runtime property for the behavior of the interpreter when the 
 * ?access-control? PI is missing from a document fetched by &lt;data&gt;
 * (See SetProperties).
 * The VXIValue passed should be of type VXIInteger, and contain non-zero
 * to allow access, or 0 to deny access.  The default is 0.
 */
#define VXI_DEFAULT_ACCESS_CONTROL L"vxi.property.defaultaccesscontrol"

/*
 * Future:
 */

//L"vxi.property.maxLoopIterations"
//L"vxi.property.maxDocuments"
//L"vxi.property.maxExeStackDepth"

/*
 * VXI Runtime property for the size of the parsed document cache.
 * The VXIValue passed should be of type VXIInteger, continaing the size
 * of the cache in kB.
 */
#define VXI_DOC_MEMORY_CACHE  L"vxi.property.cache.size"


/**
 * Result codes for interface methods.
 *
 * Result codes less then zero are severe errors (likely to be
 * platform faults), those greater then zero are warnings (likely to
 * be application issues)
 */
typedef enum VXIinterpreterResult {
  /** Fatal error, terminate call    */
  VXIinterp_RESULT_FATAL_ERROR          = -100,
  /** Out of memory                  */
  VXIinterp_RESULT_OUT_OF_MEMORY        =   -7,
  /** Errors from platform services  */
  VXIinterp_RESULT_PLATFORM_ERROR       =   -5,
  /** Property name is not valid     */
  VXIinterp_RESULT_INVALID_PROP_NAME    =   -3,
  /** Property value is not valid    */
  VXIinterp_RESULT_INVALID_PROP_VALUE   =   -2,
  /** Invalid function argument      */
  VXIinterp_RESULT_INVALID_ARGUMENT     =   -1,
  /** Success                        */
  VXIinterp_RESULT_SUCCESS              =    0,
  /** Normal failure */
  VXIinterp_RESULT_FAILURE              =    1,
  /** Run call aborted */
  VXIinterp_RESULT_STOPPED              =    3,
  /** Document fetch timeout         */
  VXIinterp_RESULT_FETCH_TIMEOUT        =   51,
  /** Unable to open or read from URI */
  VXIinterp_RESULT_FETCH_ERROR          =   52,
  /** Not a valid VoiceXML document  */
  VXIinterp_RESULT_INVALID_DOCUMENT     =   53,
  /** Operation is not supported     */
  VXIinterp_RESULT_UNSUPPORTED          =  100
} VXIinterpreterResult;


/**
 * Structure containing all the interfaces required by the VXI.
 *
 * This structure must be allocated and all the pointers filled
 * with created and initialized resources before creating the VXI
 * interface.
 */
typedef struct VXIresources {
  /** log interface */
  VXIlogInterface    * log;
  /** Internet interface */
  VXIinetInterface   * inet;
  /** ECMAScript interface */
  VXIjsiInterface    * jsi;
  /** Recognizer interface */
  VXIrecInterface    * rec;
  /** Prompt interface */
  VXIpromptInterface * prompt;
  /** Telephony interface */
  VXItelInterface    * tel;
  /** Object interface. May be NULL in which case objects will not function. */
  VXIobjectInterface * object;
  /** Cache interface. May be NULL, but VXI performance will be reduced. */
  VXIcacheInterface  * cache;
} VXIresources;


/**
 * VXIinterpreter interface for VoiceXML execution
 *
 * Abstract interface for the VoiceXML intepreter, simply provides a
 * single method for running the intepreter on a document and getting
 * the document result.<p>
 *
 * There is one intepreter interface per thread/line.
 */
typedef struct VXIinterpreterInterface {
  /**
   * Get the VXI interface version implemented
   *
   * @return  VXIint32 for the version number. The high high word is
   *          the major version number, the low word is the minor version
   *          number, using the native CPU/OS byte order. The current
   *          version is VXI_CURRENT_VERSION as defined in VXItypes.h.
   */
  VXIint32 (*GetVersion)(void);

  /**
   * Get the name of the implementation
   *
   * @return  Implementation defined string that must be different from
   *          all other implementations. The recommended name is one
   *          where the interface name is prefixed by the implementator's
   *          Internet address in reverse order, such as com.xyz.rec for
   *          VXIrec from xyz.com. This is similar to how VoiceXML 1.0
   *          recommends defining application specific error types.
   */
  const VXIchar* (*GetImplementationName)(void);

  /**
   * Run a VoiceXML document and optionally return the result
   *
   * @param name    [IN] Name of the VoiceXML document to fetch and
   *                  execute, may be a URL or a platform dependant path.
   *                  See the Open() method in VXIinet.h for details
   *                  about supported names, however for URLs this
   *                  must always be an absolute URL and any query arguments
   *                  must be embedded.
   * @param sessionScript [IN] A series of ECMAScript expressions which will
   *                  be evaluated by the VXI to populate the session scope 
   *                  in ECMAScript.
   * @param result  [OUT] (Optional, pass NULL if not desired.) Return
   *                  value for the VoiceXML document (from <exit/>), this
   *                  is allocated on success and when there is an
   *                  exit value (a NULL pointer is returned otherwise),
   *                  the caller is responsible for destroying the returned
   *                  value by calling VXIValueDestroy().
   *
   * @return         [From normal operation]<br>
   *                 VXIinterp_RESULT_SUCCESS on success<br>
   *                 VXIinterp_RESULT_FAILURE if normal error occured<br>
   *                 VXIinterp_RESULT_STOPPED if aborted by Stop<br>
   *                 [During initialization from defaults]<br>
   *                 VXIinterp_RESULT_FETCH_TIMEOUT<br>
   *                 VXIinterp_RESULT_FETCH_ERROR<br>
   *                 VXIinterp_RESULT_INVALID_DOCUMENT<br>
   *                 [Serious errors]<br>
   *                 VXIinterp_RESULT_FATAL_ERROR<br>
   *                 VXIinterp_RESULT_OUT_OF_MEMORY<br>
   *                 VXIinterp_RESULT_PLATFORM_ERROR<br>
   *                 VXIinterp_RESULT_INVALID_ARGUMENT
   */
  VXIinterpreterResult (*Run)(struct VXIinterpreterInterface  *pThis,
                              const VXIchar                   *name,
                              const VXIchar                   *sessionScript,
                              VXIValue                       **result);

  /**
   * Specify runtime properties for the VoiceXML interpreter.
   *
   * @param props   [IN] Map containing a list of properties.  Currently these
   *                  are:
   *                    - VXI_BEEP_AUDIO             URI for the beep audio
   *                    - VXI_PLATFORM_DEFAULTS      URI for the platform defaults
   *                    - VXI_DEFAULT_ACCESS_CONTROL
   *
   * @return         VXIinterp_RESULT_SUCCESS on success<br>
   *                 VXIinterp_RESULT_INVALID_PROP_NAME<br>
   *                 VXIinterp_RESULT_INVALID_PROP_VALUE<br>
   *                 VXIinterp_RESULT_INVALID_ARGUMENT
   */
  VXIinterpreterResult (*SetProperties)(struct VXIinterpreterInterface *pThis,
                                        const VXIMap                   *props);

  /**
   * Load and parse an VXML document.  This tests the validity.
   *
   * @param name    [IN] Name of the VoiceXML document to fetch and
   *                  execute, may be a URL or a platform dependant path.
   *                  See the Open( ) method in VXIinet.h for details
   *                  about supported names, however for URLs this
   *                  must always be an absolute URL and any query arguments
   *                  must be embedded.
   *
   * @return        VXIinterp_RESULT_SUCCESS if document exists and is valid<br>
   *                VXIinterp_RESULT_FAILURE if document is invalid VXML<br>
   *                VXIinterp_RESULT_FETCH_ERROR if document retrieval failed<br>
   *                VXIinterp_RESULT_FETCH_TIMEOUT<br>
   *                VXIinterp_RESULT_INVALID_ARGUMENT<br>
   *                VXIinterp_RESULT_FATAL_ERROR<br>
   *                VXIinterp_RESULT_OUT_OF_MEMORY
   */
  VXIinterpreterResult (*Validate)(struct VXIinterpreterInterface  *pThis,
                                   const VXIchar *name);

  /**
   * In the interpreter is running and doStop == TRUE, this will cause the
   * in progress Run to return as soon as possible with 
   * VXIinterp_RESULT_STOPPED.  If doStop == FALSE, this clears any pending
   * stop request.
   *
   * NOTE: if the interpreter encounters an error before noticing the request
   * or while servicing the request, the actual return value from Run may not
   * something other than VXIinterp_RESULT_STOPPED.
   *
   * @return         VXIinterp_RESULT_SUCCESS on success<br>
   *                 VXIinterp_RESULT_INVALID_ARGUMENT
   */
  VXIinterpreterResult (*RequestStop)(struct VXIinterpreterInterface  *pThis,
                                      VXIbool doStop);

  /**
   * Trigger an event.
   *
   * NOTE: This should not be used by the integration layer to produce
   *       events in response to a call into one of the interfaces.
   *
   * @param event    [IN] VoiceXML event to generate during Run.
   * @param message  [IN] Corresponding message string; may be NULL.
   *
   * @return         VXIinterp_RESULT_SUCCESS on success<br>
   *                 VXIinterp_RESULT_INVALID_ARGUMENT
   */
  VXIinterpreterResult (*InsertEvent)(struct VXIinterpreterInterface *pThis,
                                      const VXIchar                  *event,
                                      const VXIchar                  *message);

  /**
   * Clear the event queue
   *
   * NOTE: this function must be called by the integration to clear the event(s)
   * in case of error occurs and the event(s) is no longer valid.  The interpreter
   * will not clear the queue to avoid race condition.
   *
   * @return         VXIinterp_RESULT_SUCCESS on success<br>
   *                 VXIinterp_RESULT_INVALID_ARGUMENT
   */
  VXIinterpreterResult (*ClearEventQueue)(struct VXIinterpreterInterface *pThis);

} VXIinterpreterInterface;


/**
 * Per-process initialization for VXIinterpreter.
 * This function should be called once at process startup.
 *
 * @param log            [IN] VXI Logging interface used for error/diagnostic
 *                             logging, only used for the duration of this
 *                             function call
 * @param  diagLogBase   [IN] Base tag number for diagnostic logging purposes.
 *                             All diagnostic tags for the VXI will start at
 *                             this ID and increase upwards.
 * @param  props         [IN]
 *
 * @return     VXIinterp_RESULT_SUCCESS if resources may be created.
 * @return     VXIinterp_RESULT_FAILURE if interface is unavailable.
 */
VXI_INTERPRETER
VXIinterpreterResult VXIinterpreterInit(VXIlogInterface  *log,
                                        VXIunsigned       diagLogBase,
                                        const VXIMap     *props);


/**
 * Per-process de-initialization for VXIinterpreter.
 * This function should be called once per process shutdown, after
 * all the interfaces for the process are destroyed.
 *
 * @param log [IN] VXI Logging interface used for error/diagnostic logging,
 *                  only used for the duration of this function call
 */
VXI_INTERPRETER void VXIinterpreterShutDown(VXIlogInterface  *log);


/**
 * Create an interface to the VoiceXML interpreter.
 * Create a VXI interface given an interface structure that
 * contains all the resources required for the VXI.
 *
 * @param resource [IN] A pointer to a structure containing all the
 *                       interfaces requires by the VXI
 * @param pThis    [IN] A pointer to the VXI interface that is to be
 *                       allocated.  The pointer will be set if this call
 *                       is successful.
 *
 * @return     VXIinterp_RESULT_SUCCESS if interface is available for use<br>
 *             VXIinterp_RESULT_OUT_OF_MEMORY if low memory is suspected<br>
 *             VXIinterp_RESULT_INVALID_ARGUMENT
 */
VXI_INTERPRETER VXIinterpreterResult
VXIinterpreterCreateResource(VXIresources *resource,
                             VXIinterpreterInterface ** pThis);

/**
 * Destroy and de-allocate a VXI interface.
 * 
 * Destroy an interface returned from VXIinterpreterCreateResource.
 * The pointer is set to NULL on success.
 *
 * @param pThis [IN] The pointer to the interface to be destroyed.
 */
VXI_INTERPRETER void
VXIinterpreterDestroyResource(VXIinterpreterInterface ** pThis);

/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
