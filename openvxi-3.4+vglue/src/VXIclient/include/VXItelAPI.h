
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

#ifndef _VXITEL_API_H
#define _VXITEL_API_H

#include <VXIheaderPrefix.h>
#include <VXIlog.h>
#include <VXItel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name VXItel
 * @memo VXItel Interface
 * @doc
 * VXItel provides a simulator implementation of the VXItel abstract interface
 * for call control functionality.   All calls are blocking with the
 * results returned by the implementation being used to determine the
 * outcome of transers. One VXItel interface should be constructed per line.
 */

/*@{*/

/**
 * VXItel interface, extends the VXItel interface to add a wait for
 * call method.
 */
typedef struct VXItelInterfaceEx
{
  /* Base interface, must be the first member */
  VXItelInterface vxitel;

  /**
   * EnableCall
   *
   * Re-arms the hardware interface to allow it to accept a call. The
   * telephony interface starts up in an out-of-service (OOS) state.
   * In this state, calls will not be excepted. The best
   * implementation is to arrange for the line to be out of service
   * back to the telephony switch so that failover to the next line
   * occurs at the switch level. For some protocols this can be done
   * by setting the line to a busy state.  This call is blocking. When
   * it returns the interface will be ready to accept calls.
   *
   * A hangup or error on the line will place the hardware back into
   * the out-of-service state so that no calls come into the line
   * until it is explictly re-armed.
   *
   * @return VXItel_RESULT_SUCCESS if sucess, different value on failure 
   */
  VXItelResult (*EnableCall)(struct VXItelInterfaceEx  *pThis);

  /**
   * WaitForCall
   *
   * Wait for and answer a call, EnableCall( ) must be called prior
   * to this to enable calls on the channel.
   *
   * @param telephonyProps [OUT] Used to return the telephony properties
   *                             that should be made available to the
   *                             application as "session.telephone.*" 
   *                             variables, such as the called and calling
   *                             party telephone numbers. The platform is
   *                             responsible for destroying these on
   *                             success.
   *
   * @return VXItel_RESULT_SUCCESS if sucess, different value on failure 
   */
  VXItelResult (*WaitForCall)(struct VXItelInterfaceEx  *pThis,
			      VXIMap                 **telephonyProps);

} VXItelInterfaceEx;


/**
 * Initializes an VXItel implementation of the VXItel interface
 *
 * @param log              VXI Logging interface used for error/diagnostic 
 *                         logging, only used for the duration of this 
 *                         function call
 * @param diagLogBase      Logging base for diagnostic logging. All 
 *                         diagnostics are based on this offset.
 *
 * @return VXItel_RESULT_SUCCESS if sucess, different value on failure
 */
VXITEL_API VXItelResult VXItelInit (VXIlogInterface  *log,
				    VXIunsigned       diagLogBase,
				    VXIMap            *args);

/**
 * Shutdown an VXItel implementation of the VXItel interface
 *
 * @param log              VXI Logging interface used for error/diagnostic 
 *                         logging, only used for the duration of this 
 *                         function call
 *
 * @return VXItel_RESULT_SUCCESS if sucess, different value on failure
 */
VXITEL_API VXItelResult VXItelShutDown (VXIlogInterface  *log);

/**
 * Creates an VXItel implementation of the VXItel interface
 *
 * @param log            [IN]  VXI Logging interface used for error/diagnostic
 *                             logging, must remain a valid pointer throughout
 *                             the lifetime of the resource (until 
 *                             VXIpromptDestroyResource( ) is called)
 * @param tel            [OUT] Pointer that returns the newly created VXItel
 *                             interface.
 *
 * @return VXItel_RESULT_SUCCESS if sucess, different value on failure
 */
VXITEL_API VXItelResult VXItelCreateResource(VXIlogInterface *log,
					     VXItelInterface **tel);

/**
 * Destroys the specified VXItel implementation
 *
 * @param tel  [IN/OUT] Pointer to the VXItel object to be destroyed.  Set
 *                      to NULL on return.
 *
 * @return VXItel_RESULT_SUCCESS if sucess, different value on failure
 */
VXITEL_API VXItelResult VXItelDestroyResource(VXItelInterface **tel);

/*@}*/
#include <VXIheaderSuffix.h>

#ifdef __cplusplus
}
#endif

#endif  /* include guard */
