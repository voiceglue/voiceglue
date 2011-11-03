
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

#ifndef _VXIOBJECT_API_H
#define _VXIOBJECT_API_H

#include "VXIobject.h"                 /* For VXIobject base interface */
#include "VXIheaderPrefix.h"

#define VXIOBJECT_API_EX               /* Place holder for export symbol
                                          since we don't build DLL for object */

#ifdef __cplusplus

struct VXIlogInterface;
struct VXIinetInterface;
struct VXIjsiInterface;
struct VXIrecInterface;
struct VXIpromptInterface;
struct VXItelInterface;
struct VXIplatform;

extern "C" {
#else

#include "VXIlog.h"
#include "VXIinet.h"
#include "VXIjsi.h"
#include "VXIrec.h"
#include "VXIprompt.h"
#include "VXItel.h"
#include "VXIplatform.h"
#endif

 /** 
  * @name VXIobject
  * @memo VXIobject implementation of VXIobject
  * @doc
  * Provides an implementation of the VXIobject abstract interface for
  * VoiceXML object functionality that allows integrators to define
  * VoiceXML language extensions that can be executed by applications
  * through the VoiceXML object element.  These objects can provide
  * almost any extended functionality that is desired.
  *
  * There is one object interface per thread/line.  
  */

/*@{*/

/**
 * @name VXIobjectResources
 * @memo Structure containing all the interfaces required for Objects
 * @doc This structure must be allocated and all the pointers filled
 * with created and initialized resources before creating the Object
 * interface.
 */
typedef struct VXIobjectResources {
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
  /** Platform interface */
  VXIplatform        * platform;
} VXIobjectResources;


/**
 * Global platform initialization of VXIobject
 *
 * @param log            VXI Logging interface used for error/diagnostic 
 *                       logging, only used for the duration of this 
 *                       function call
 * @param  diagLogBase   Base tag number for diagnostic logging purposes.
 *                       All diagnostic tags for VXIobject will start at this
 *                       ID and increase upwards.
 *
 * @result VXIobj_RESULT_SUCCESS on success
 */
VXIOBJECT_API_EX 
VXIobjResult VXIobjectInit (VXIlogInterface  *log,
                            VXIunsigned       diagLogBase);

/**
 * Global platform shutdown of VXIobject
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               only used for the duration of this function call
 *
 * @result VXIobj_RESULT_SUCCESS on success
 */
VXIOBJECT_API_EX 
VXIobjResult VXIobjectShutDown (VXIlogInterface  *log);

/**
 * Create a new object service handle
 *
 * @param resources  A pointer to a structure containing all the interfaces
 *                   that may be required by the object resource
 *
 * @result VXIobj_RESULT_SUCCESS on success 
 */
VXIOBJECT_API_EX 
VXIobjResult VXIobjectCreateResource (VXIobjectResources   *resources,
                                      VXIobjectInterface **object);

/**
 * Destroy the interface and free internal resources. Once this is
 *  called, the resource interfaces passed to VXIobjectCreateResource( )
 *  may be released as well.
 *
 * @result VXIobj_RESULT_SUCCESS on success 
 */
VXIOBJECT_API_EX 
VXIobjResult VXIobjectDestroyResource (VXIobjectInterface **object);

/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
