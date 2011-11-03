
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

#ifndef _VXIPROMPT_API_H
#define _VXIPROMPT_API_H

#include <VXIheaderPrefix.h>
#include <VXIvalue.h>                  /* For VXIprompt base interface */
#include <VXIlog.h>                    /* For VXIlog interface */
#include <VXIinet.h>                   /* For VXIinet interface */
#include <VXIcache.h>                  /* For VXIcache interface */
#include <VXIprompt.h>                 /* For VXIprompt base interface */

#ifdef __cplusplus
extern "C" {
#endif

#define SIM_INPUT_TYPE   L"simulator.input.type"

/**
 * @name VXIprompt
 * @memo VXIprompt Interface
 * @doc
 * VXIprompt interface, a simulator implementation of the VXIprompt abstract
 * interface for Prompting functionality. Prompts are constructed as a
 * series of audio segments, where segments may be in-memory audio
 * samples, paths to on-disk audio files, URLs to remote audio, text
 * for playback via a Text-to-Speech engine, or text for playback
 * using concatenative audio routines (123 played as "1.ulaw 2.ulaw
 * 3.ulaw"). This implementation merely logs what a real VXIprompt
 * implementation should do to the diagnostic log. <p>
 *
 * There is one prompt interface per thread/line.
 */

  /*@{*/

/**
 * VXIprompt interface, extends the VXIprompt interface to add a stop for
 * call method.
 */
typedef struct VXIpromptInterfaceEx
{
  /* Base interface, must be the first member */
  VXIpromptInterface vxiprompt;
   
  /**
  * Stop playing prompt
  *
  * VXIprompt interface provides no Stop() functionality, therefore add an API
  *  extension to access the Stop() from real prompt implementation
  *
  * @param prompt     VXIpromptInterface pointer that will be deallocated.  
  *                   
  * @result VXIprompt_RESULT_SUCCESS on success 
  */
  VXIpromptResult (*Stop) (struct VXIpromptInterfaceEx *prompt);

  /**
  * Return the delta-time for simulated play.  An SSFT's extension
  * that may not be needed by other platforms!  Also an illustration of
  * adding extension to VXIprompt interface
  *
  * @param prompt     VXIpromptInterface pointer that will be deallocated.  
  *
  * @param delta      delta time (could be negative or positive)  
  *
  * @result VXIprompt_RESULT_SUCCESS on success 
  */
  VXIpromptResult (*GetDeltaTime) (struct VXIpromptInterfaceEx *prompt, VXIlong *delta);

} VXIpromptInterfaceEx;


/**
 * Global platform initialization of VXIprompt
 *
 * @param log            VXI Logging interface used for error/diagnostic 
 *                       logging, only used for the duration of this 
 *                       function call
 * @param diagLogBase    Base tag number for diagnostic logging purposes.
 *                       All diagnostic tags for VXIprompt will start at this
 *                       ID and increase upwards.
 *
 * @result VXIprompt_RESULT_SUCCESS on success
 */
VXIPROMPT_API VXIpromptResult VXIpromptInit 
(
  VXIlogInterface  *log,
	VXIunsigned       diagLogBase,
	const VXIVector  *resources,
	VXIMap           *args
);

/**
 * Global platform shutdown of VXIprompt
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               only used for the duration of this function call
 *
 * @result VXIprompt_RESULT_SUCCESS on success
 */
VXIPROMPT_API VXIpromptResult VXIpromptShutDown (VXIlogInterface  *log);

/**
 * Create a new prompt service handle
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               must remain a valid pointer throughout the lifetime of
 *               the resource (until VXIpromptDestroyResource( ) is called)
 * @param inet   VXI Internet interface used for URL fetches,
 *               must remain a valid pointer throughout the lifetime of
 *               the resource (until VXIpromptDestroyResource( ) is called)
 * @param prompt VXIpromptInterface pointer that will be allocated within this
 *               function. Call VXIpromptDestroyResource( ) to delete the 
 *               resource.
 *
 * @result VXIprompt_RESULT_SUCCESS on success 
 */
VXIPROMPT_API 
VXIpromptResult VXIpromptCreateResource (
  VXIlogInterface     *log,
  VXIinetInterface    *inet,
  VXIcacheInterface   *cache,
  VXIpromptInterface **prompt);

/**
 * Destroy the interface and free internal resources. Once this is
 *  called, the logging and Internet interfaces passed to
 *  VXIpromptCreateResource( ) may be released as well.
 *
 * @param prompt VXIpromptInterface pointer that will be deallocated.  It will
 *               be set to NULL when deallocated.
 *
 * @result VXIprompt_RESULT_SUCCESS on success 
 */
VXIPROMPT_API 
VXIpromptResult VXIpromptDestroyResource (VXIpromptInterface **prompt);

/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
