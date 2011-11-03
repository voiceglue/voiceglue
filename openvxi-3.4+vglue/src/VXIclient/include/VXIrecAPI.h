
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

#ifndef _VXIREC_API_H
#define _VXIREC_API_H

#include <VXIheaderPrefix.h>
#include <VXIrec.h>                 /* For VXIrec base interface */
#include <VXIlog.h>                 /* For VXIlog interface */
#include <VXIinet.h>                   /* For VXIinet interface */
#include <VXIcache.h>
#include <VXIprompt.h>
#include <VXItel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name VXIrec
 * @memo VXIrec Interface
 * @doc
 * VXIrec provides a simulator implementation of the VXIrec abstract interface
 * for recognition functionality.   Recognition is done with all
 * blocking calls, because VoiceXML is essentially a blocking
 * protocol.  One VXIrec interface should be constructed per line.
 */

/*@{*/

/**
 * Global platform initialization of VXIrec
 *
 * @param log            VXI Logging interface used for error/diagnostic 
 *                       logging, only used for the duration of this 
 *                       function call
 * @param diagLogBase    Base tag number for diagnostic logging purposes.
 *                       All diagnostic tags for VXIrec will start at this
 *                       ID and increase upwards.
 * 
 * @result VXIrec_RESULT_SUCCESS on success
 */
VXIREC_API VXIrecResult VXIrecInit(VXIlogInterface *log,
				   VXIunsigned      diagLogBase,
				   VXIMap           *args);

/**
 * Global platform shutdown of Recognizer
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               only used for the duration of this function call
 *
 * @result VXIrec_RESULT_SUCCESS on success
 */
VXIREC_API VXIrecResult VXIrecShutDown(VXIlogInterface *log);

/**
 * Create a new recognizer service handle
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               must remain a valid pointer throughout the lifetime of
 *               the resource (until VXIrecDestroyResource( ) is called)
 * @param inet   VXI Internet interface used for URL fetches,
 *               must remain a valid pointer throughout the lifetime of
 *               the resource (until VXIrecDestroyResource( ) is called)
 * @param rec    VXIrecInterface pointer that will be allocated within this
 *               function. Call VXIrecDestroyResource( ) to delete the 
 *               resource.
 *
 * @result VXIrec_RESULT_SUCCESS on success 
 */
VXIREC_API VXIrecResult VXIrecCreateResource(VXIlogInterface   *log,
					     VXIinetInterface  *inet,
  					   VXIcacheInterface *cache,
	  				   VXIpromptInterface *prompt,
		  			   VXItelInterface *tel,		     
					     VXIrecInterface  **rec);

/**
 * Destroy the interface and free internal resources. Once this is
 *  called, the logging and Internet interfaces passed to
 *  VXIrecognizerCreateResource( ) may be released as well.
 *
 * @param rec    VXIrecInterface pointer that will be deallocated.  It will
 *               be set to NULL when deallocated.
 *
 * @result VXIrec_RESULT_SUCCESS on success 
 */
VXIREC_API VXIrecResult VXIrecDestroyResource(VXIrecInterface **rec);

/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
