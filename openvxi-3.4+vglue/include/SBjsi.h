/*****************************************************************************
 *****************************************************************************
 *
 *
 * SBjsi JavaScript (ECMAScript) Engine Interface
 *
 * SBjsi interface, an implementation of the VXIjsi abstract interface
 * for interacting with a JavaScript (ECMAScript) engine.  This
 * provides functionality for creating JavaScript execution contexts,
 * manipulating JavaScript scopes, manipulating variables within those
 * scopes, and evaluating JavaScript expressions/scripts.
 *
 * There is one JavaScript interface per thread/line.
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

#ifndef _SBJSI_H
#define _SBJSI_H

#include "VXIjsi.h"                    /* For VXIjsi base interface */
#include "VXIlog.h"                    /* For VXIlog interface */

#include "VXIheaderPrefix.h"
#ifdef SBJSI_EXPORTS
#define SBJSI_API SYMBOL_EXPORT_DECL
#else
#define SBJSI_API SYMBOL_IMPORT_DECL
#endif

  /* -- start docme interface -- */

/**
 * @name SBjsi
 * @memo SBjsi implementation of VXIjsi
 * @doc
 * SBjsi interface, an implementation of the VXIjsi interface for
 * interacting with a ECMAScript (JavaScript) engine.  This provides
 * functionality for creating ECMAScript execution contexts,
 * manipulating ECMAScript scopes, manipulating variables within those
 * scopes, and evaluating ECMAScript expressions/scripts. <p>
 *
 * There is one VXIjsi interface per thread/line.  
 */

  /*@{*/


/* Recommended defaults for SBjsiInit */
#define JSI_RUNTIME_SIZE_DEFAULT     (1024 * 1024 * 16)
#define JSI_CONTEXT_SIZE_DEFAULT     (1024 * 128)
#define JSI_MAX_BRANCHES_DEFAULT     100000

/**
 * Global platform initialization of JavaScript
 *
 * @param log             VXI Logging interface used for error/diagnostic 
 *                        logging, only used for the duration of this 
 *                        function call
 * @param  diagLogBase    Base tag number for diagnostic logging purposes.
 *                        All diagnostic tags for SBjsi will start at this
 *                        ID and increase upwards.
 * @param  runtimeSize    Size of the JavaScript runtime environment, in 
 *                        bytes. There is one runtime per process. See 
 *                        above for a recommended default.
 * @param  contextSize    Size of each JavaScript context, in bytes. There
 *                        may be multiple contexts per channel, although
 *                        the VXI typically only uses one per channel.
 *                        See above for a recommended default.
 * @param  maxBranches    Maximum number of JavaScript branches for each 
 *                        JavaScript evaluation, used to interrupt infinite
 *                        loops from (possibly malicious) scripts
 *
 * @result VXIjsiResult 0 on success
 */
SBJSI_API VXIjsiResult SBjsiInit (VXIlogInterface  *log,
				  VXIunsigned       diagLogBase,
				  VXIlong           runtimeSize,
				  VXIlong           contextSize,
				  VXIlong           maxBranches);

/**
 * Global platform shutdown of JavaScript
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               only used for the duration of this function call
 *
 * @result VXIjsiResult 0 on success
 */
SBJSI_API VXIjsiResult SBjsiShutDown (VXIlogInterface  *log);

/**
 * Create a new JavaScript service handle
 *
 * @param log    VXI Logging interface used for error/diagnostic 
 *               logging, must remain a valid pointer throughout the 
 *               lifetime of the resource (until SBjsiDestroyResource( )
 *               is called)
 *
 * @result VXIjsiResult 0 on success 
 */
SBJSI_API VXIjsiResult SBjsiCreateResource(VXIlogInterface   *log,
					   VXIjsiInterface  **jsi);

/**
 * Destroy the interface and free internal resources. Once this is
 *  called, the logging interface passed to SBjsiCreateResource( ) may
 *  be released as well.
 *
 * @result VXIjsiResult 0 on success 
 */
SBJSI_API VXIjsiResult SBjsiDestroyResource(VXIjsiInterface **jsi);

  /* -- end docme interface -- */
/*@}*/
#include "VXIheaderSuffix.h"

#endif  /* include guard */
