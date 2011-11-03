
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

#ifndef _VXICLIENT_H
#define _VXICLIENT_H

#include <VXIlog.h>
#include <VXIjsi.h>
#include <VXIinet.h>
#include <VXIcache.h>
#include <VXItel.h>
#include <VXIrec.h>
#include <VXIprompt.h>
#include <VXIobject.h>
#include <VXIinterpreter.h>
#include <VXItrd.h>
#include <vglue_ipc_c.h>

#include "SBlogListeners.h"
#include "VXIclientUtils.h"
#include "VXIclientConfig.h"    /* For CLIENT_[...] configuration properties */
#include "VXIobjectAPI.h"       /* An SSFT's implementation sample of VXIobject */
#include "VXItelAPI.h"

#define CLIENT_API_TAG         (gblDiagLogBase + 0)
#define CLIENT_COMPONENT_TAG   (gblDiagLogBase + 1)
#define CLIENT_GEN_TAG         (gblDiagLogBase + 2)

#ifdef __cplusplus
extern "C" {
#endif

 /** 
  * @name OSBclient
  * @memo OSBclient implementation of VXIplatform
  * @doc
  * Provides an implementation of the VXIplatform abstract interface
  * for creating and destroying the VXI resources required by the
  * browser and to isolate the main from the actual implementation
  * details of the resource management.  This set of convenience
  * functions is used by the reference application manager.  
  */

/*@{*/

/**
 * @memo OSBclient defined VXIplatform structure, returned upon initialization 
 * by VXIplatformCreateResources()
 */
struct VXIplatform {
  VXItelInterface           *VXItel;
  VXIlogInterface           *VXIlog;
  VXIinetInterface          *VXIinet;
  VXIcacheInterface         *VXIcache;
  VXIpromptInterface        *VXIprompt;
  VXIrecInterface           *VXIrec;
  VXIjsiInterface           *VXIjsi;
  VXIobjectInterface        *VXIobject;
  VXIinterpreterInterface   *VXIinterpreter;

  VXIMap                    *telephonyProps;
  VXIchar                   *connectionPropScript;
  const VXIchar             *globalScript;
  VXIresources               resources;
  VXIbool                    acceptCookies;
  VXIunsigned                channelNum;
  VXIulong                   numCall;
  SBlogListenerChannelData  *logData; /* Use SSFT VXIlog's listener implementation */
  VXIobjectResources         objectResources;
};

/* Convenience macros */
#define CHECK_RESULT_RETURN(_platform, _func, _res)                       \
  if ((VXIint)(_res) != 0) {                                              \
    VXIclientError((_platform), MODULE_NAME, 200, L"Function call failed",\
      L"%s%S%s%d%s%S%s%d", L"FUNC", _func, L"RESULT", (_res),             \
      L"FILE", __FILE__, L"LINE", __LINE__);                              \
    return VXIplatform_RESULT_FATAL_ERROR;                                \
  }

#define CHECK_RESULT_NO_RETURN(_platform, _func, _res)                    \
  if ((VXIint)(_res) != 0)                                                \
    VXIclientError((_platform), MODULE_NAME, 200, L"Function call failed",\
      L"%s%S%s%d%s%S%s%d", L"FUNC", _func, L"RESULT", (_res),             \
      L"FILE", __FILE__, L"LINE", __LINE__);

#define CHECK_MEMALLOC_RETURN(_platform, _buf, _location) \
    if (_buf == NULL) { \
      VXIclientError((_platform), MODULE_NAME, 107, L"Out of memory",     \
                    L"%s%S%s%S%s%d", L"LOCATION", _location,              \
                    L"FILE", __FILE__, L"LINE", __LINE__);                \
      return VXIplatform_RESULT_OUT_OF_MEMORY;                            \
    }

#define CLIENT_LOG_IMPLEMENTATION(_platform, _name, _intf)                \
    VXIclientDiag((_platform), CLIENT_COMPONENT_TAG, NULL,                \
                 _name L" implementation: %s, interface version %d.%d",   \
                 _intf->GetImplementationName(),                          \
                 VXI_MAJOR_VERSION(_intf->GetVersion()),                  \
                 VXI_MINOR_VERSION(_intf->GetVersion()));

/* Error and diagnostic logging, must be defined by each VXIplatform
   implementation */
VXIlogResult 
VXIclientError
(
  VXIplatform *platform, 
  const VXIchar *moduleName,
  VXIunsigned errorID, 
  const VXIchar *errorIDText,
  const VXIchar *format, ...
);

VXIlogResult 
VXIclientDiag
(
  VXIplatform *platform, 
  VXIunsigned tag, 
  const VXIchar *subtag, 
  const VXIchar *format, ...
);

VXIlogResult 
VXIclientErrorToConsole
(
  const VXIchar *moduleName,
  VXIunsigned errorID,
  const VXIchar *errorIDText
);

VXIlogResult 
VXIclientVErrorToConsole
(
  const VXIchar *moduleName, 
  VXIunsigned errorID,
  const VXIchar *errorIDText, 
  const VXIchar *format,
  va_list arguments
);

VXIplatformResult 
VXIclientControliagnosticTags
(
  const VXIMap *configArgs,
  VXIplatform  *platform
);

VXIplatformResult ConvertInterpreterResult(VXIinterpreterResult result);

#ifdef __cplusplus
}
#endif

/*@}*/

#endif /* include guard */
