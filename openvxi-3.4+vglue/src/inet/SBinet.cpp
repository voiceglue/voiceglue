
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

#ifndef _SB_USE_STD_NAMESPACE
#define _SB_USE_STD_NAMESPACE
#endif

#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#endif /* WIN32 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wctype.h>

#include "VXItypes.h"
#include "VXIvalue.h"
#include "VXIinet.h"
#define SBINET_EXPORTS
#include "SBinet.h"
#include "VXItrd.h"
#include "VXIlog.h"

#ifdef _WIN32
#undef HTTP_VERSION
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <winsock2.h>
#endif /* WIN32 */

#include "SBinetLog.h"
#include "SBinetChannel.h"

#include <SBinetString.hpp>

class SBinet: public SWIutilLogger
{
public:
  SBinet():
    SWIutilLogger(MODULE_SBINET, NULL, 0),
    _initialized(false)
  {}

  bool isInitialized() const
  {
    return _initialized;
  }

  virtual ~SBinet();

  SWIutilLogger::SetLog;

  VXIinetResult  Init(const VXIMap*    configParams);
  VXIinetResult  Terminate(VXIbool     clearLogResource);

private:
  VXIinetResult  CheckEnvironment();
  bool _initialized;
};

/*****************************************************************************
 *****************************************************************************
 * Globals
 *****************************************************************************
 *****************************************************************************
 */

// The Global inet object. Manages Event Thread, etc.
static SBinet g_SBinet;

/*****************************************************************************
 *****************************************************************************
 * SBinet C routines:  Init(), ShutDown(), CreateResource(), DestroyResource()
 *
 * NOTE: We provide both OSB and SB prefixed entry points, OSB for the
 * OpenVXI open source release, SB for the OpenSpeech Browser PIK product.
 *****************************************************************************
 *****************************************************************************
 */


SBINET_API
VXIinetResult SBinetInit( VXIlogInterface* piVXILog,
                          const VXIunsigned diagLogBase,
                          const VXIchar*   reserved1,
                          const VXIint     reserved2,
                          const VXIint     reserved3,
                          const VXIint     reserved4,
                          const VXIchar*   pszProxyServer,
                          const VXIulong   nProxyPort,
			  const VXIchar*   userAgentName,
			  const VXIMap*    extensionRules,
			  const VXIVector* reserved )
{
  g_SBinet.SetLog(piVXILog, diagLogBase);
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  SBinetLogFunc apiTrace (piVXILog, diagLogBase+ MODULE_SBINET_API_TAGID,
			  L"SBinetInit", (int *) &rc,
			  L"entering: 0x%p, %s, %d, %d, %d, %s, %lu, %s, "
			  L"0x%p, 0x%p", piVXILog, 
			  (reserved1!=NULL)?reserved1:L"",
			  reserved2, reserved3,
			  reserved4, 
			  (pszProxyServer!=NULL)?pszProxyServer:L"", 
			  nProxyPort,
			  (userAgentName!=NULL)?userAgentName:L"", 
			  extensionRules, reserved);

  VXIMap *theMap = VXIMapCreate();

  if (theMap == NULL)
  {
    return rc = VXIinet_RESULT_OUT_OF_MEMORY;
  }

  if (userAgentName && *userAgentName)
  {
    VXIMapSetProperty(theMap, SBINET_USER_AGENT_NAME, (VXIValue *) VXIStringCreate(userAgentName));
  }
  if (extensionRules)
  {
    VXIMapSetProperty(theMap, SBINET_EXTENSION_RULES, (VXIValue *) VXIMapClone(extensionRules));
  }

  if (pszProxyServer && *pszProxyServer)
  {
    VXIVector *proxyVector = VXIVectorCreate();
    if (proxyVector == NULL)
    {
      rc = VXIinet_RESULT_OUT_OF_MEMORY;
      goto cleanUp;
    }
    VXIMapSetProperty(theMap, SBINET_PROXY_RULES, (VXIValue *) proxyVector);

    SBinetString proxyRule = "|";
    proxyRule += pszProxyServer;
    proxyRule += ':';
    char tmpPort[20];
    sprintf(tmpPort, "%lu", nProxyPort);
    proxyRule += tmpPort;

    VXIVectorAddElement(proxyVector, (VXIValue *) VXIStringCreate(proxyRule.c_str()));
  }

  rc = SBinetInitEx(piVXILog, diagLogBase, theMap);

 cleanUp:
  if (theMap != NULL) VXIMapDestroy(&theMap);

  return ( rc );
}


SBINET_API
VXIinetResult SBinetInitEx(VXIlogInterface* piVXILog,
                           const VXIunsigned diagLogBase,
                           const VXIMap*    configParams)
{
  g_SBinet.SetLog(piVXILog, diagLogBase);

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  SBinetLogFunc apiTrace (piVXILog, diagLogBase+ MODULE_SBINET_API_TAGID,
			  L"SBinetInitEx", (int *) &rc,
			  L"entering: 0x%p, %p",
			  piVXILog, configParams);

  rc = g_SBinet.Init(configParams);

  return ( rc );
}


SBINET_API
VXIinetResult SBinetShutDown( VXIlogInterface* piVXILog )
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (piVXILog, g_SBinet.GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetShutdown", (int *) &rc,
			  L"entering: 0x%p", piVXILog);


  return rc = g_SBinet.Terminate(FALSE);
}


SBINET_API
VXIinetResult SBinetCreateResource(VXIlogInterface* piVXILog,
                                   VXIcacheInterface* piVXICache,
                                   VXIinetInterface**     ppiVXIInet )
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  SBinetLogFunc apiTrace (piVXILog, g_SBinet.GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetCreateResource", (int *) &rc,
			  L"entering: 0x%p, 0x%p",
			  piVXILog, ppiVXIInet);

  if (! g_SBinet.isInitialized())
    return VXIinet_RESULT_FATAL_ERROR;

  if (!piVXILog || !ppiVXIInet)
  {
    SWIutilLogger::Error(piVXILog,MODULE_SBINET,102,NULL);
    return (rc = VXIinet_RESULT_INVALID_ARGUMENT);
  }

  SBinetChannel* pInter = new SBinetChannel(piVXILog,
                                            g_SBinet.GetDiagBase(),
                                            piVXICache);
  if (!pInter)
  {
    SWIutilLogger::Error(piVXILog,MODULE_SBINET,103,NULL);
    return (rc = VXIinet_RESULT_OUT_OF_MEMORY);
  }
  // Could add to Channel/Interface table for validation but...

  *ppiVXIInet = (VXIinetInterface*)pInter; // Downcast...

  return (rc);
}


SBINET_API
VXIinetResult SBinetDestroyResource( VXIinetInterface** ppiVXIInet )
{
  if (! g_SBinet.isInitialized())
    return VXIinet_RESULT_FATAL_ERROR;

  VXIlogInterface* piVXILog = g_SBinet.GetLog();
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (piVXILog, g_SBinet.GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetDestroyResource", (int *) &rc,
			  L"entering: 0x%p (0x%p)", ppiVXIInet,
			  (ppiVXIInet ? *ppiVXIInet : NULL));

  if (! ppiVXIInet) {
    g_SBinet.Error(102,NULL);
    return (rc = VXIinet_RESULT_INVALID_ARGUMENT);
  }
  if (! *ppiVXIInet) {
    g_SBinet.Error(102,NULL);
    return (rc = VXIinet_RESULT_INVALID_ARGUMENT);
  }
  SBinetChannel* pSBinet = static_cast<SBinetChannel*>(*ppiVXIInet);
  if (!pSBinet)
  {
    g_SBinet.Error(102,NULL);
    return (rc = VXIinet_RESULT_INVALID_ARGUMENT);
  }
  // Could delete to Channel/Interface table for validation but...

  // Delete it, destructor cleans up
  delete pSBinet;
  *ppiVXIInet = static_cast<VXIinetInterface*>(NULL);

  return (rc);

}


/****************************************************************************
 ****************************************************************************
 * SBinet methods: Initializations
 *
 ****************************************************************************
 ****************************************************************************
 */


SBinet::~SBinet()
{
  Terminate(TRUE);
}


//
// Initialize SBinet
//
VXIinetResult SBinet::Init(const VXIMap *configParams)
{
  if (_initialized)
  {
    return VXIinet_RESULT_SUCCESS;
  }

  // We need to initialize winsock on Windows.
#ifdef _WIN32

  WSADATA wsaData;

  int err = WSAStartup( MAKEWORD(1,1), &wsaData );
  if (err != 0)
  {
    Error(109,NULL);
    return   VXIinet_RESULT_PLATFORM_ERROR;
  }

  //    if (LOBYTE(wsaData.wVersion) != 1 && HIBYTE(wsaData.wVersion) != 1) {
  //      WSACleanup();
  //      return -1;
  //    }
#endif

  // Check if the environment is correct for supporting SBinet
  VXIinetResult rc = CheckEnvironment();
  if (rc != VXIinet_RESULT_SUCCESS)
    return rc;

  rc = SBinetChannel::init(configParams, this);
  if (rc != VXIinet_RESULT_SUCCESS)
    return rc;

  _initialized = true;
  return rc;
}


//
// Terminate SBinet
//
VXIinetResult SBinet::Terminate(VXIbool clearLogResource)
{
  if (!_initialized)
  {
    return VXIinet_RESULT_NON_FATAL_ERROR;
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  if (clearLogResource)
    SetLog(NULL, GetDiagBase());

  SBinetChannel::shutdown();

  // TODO: Scan Instance and Stream maps and delete outstanding objects

  // Shutdown winsock on WIN32
#ifdef _WIN32
  WSACleanup();
#endif

  _initialized = false;
  return rc;
}


//
// Check if the environment properly supports SBinet
//
VXIinetResult SBinet::CheckEnvironment()
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

#ifdef _WIN32
  // Get the Microsoft Internet Explorer version, must have version
  // 5.0 or later otherwise InternetCrackUrl() rejects various
  // file:// URL formats that we need to support. Base this off the
  // version of the DLL that implements the browser as documented at
  // http://support.microsoft.com/support/kb/articles/q164/5/39.asp,
  // the IE 5 GA release = 5.00.2014.213
  static const wchar_t IE_DLL_NAME[] = L"shdocvw.dll";
  static const int IE_DLL_VER_1 = 5;
  static const int IE_DLL_VER_2 = 0;
  static const int IE_DLL_VER_3 = 2014;
  static const int IE_DLL_VER_4 = 213;

  VXIint infoSize = GetFileVersionInfoSize((wchar_t *) IE_DLL_NAME, 0);
  if (infoSize <= 0) {
    // Could not find the DLL or version info not available
    Error(105, NULL);
    rc = VXIinet_RESULT_PLATFORM_ERROR;
  } else {
    VXIbyte *infoData = new VXIbyte [infoSize];
    if (infoData == NULL) {
      Error(103, NULL);
      rc = VXIinet_RESULT_OUT_OF_MEMORY;
    } else {
      LPDWORD dwResource;
      UINT cnSize;

      if ((GetFileVersionInfo((wchar_t*)IE_DLL_NAME, 0,infoSize,infoData)==0)||
	 (VerQueryValue(infoData, L"\\", (LPVOID*)(&dwResource), &cnSize)==0)){
	// Version info not available
	Error(105, NULL);
	rc = VXIinet_RESULT_PLATFORM_ERROR;
      } else {
	Diag(MODULE_SBINET_API_TAGID, NULL,
	     L"Microsoft Internet Explorer %2d.%02d.%04d.%d is active",
	     HIWORD(dwResource[2]), LOWORD(dwResource[2]),
	     HIWORD(dwResource[3]), LOWORD(dwResource[3]));

        // Check if the DLL's version is OK for us
	bool badVersion = false;
        if (HIWORD(dwResource[2]) < IE_DLL_VER_1)
          badVersion = true;
        else if (HIWORD(dwResource[2]) == IE_DLL_VER_1) {
          if (LOWORD(dwResource[2]) < IE_DLL_VER_2)
            badVersion = true;
          else if (LOWORD(dwResource[2]) == IE_DLL_VER_2) {
            if (HIWORD(dwResource[3]) < IE_DLL_VER_3)
              badVersion = true;
            else if (HIWORD(dwResource[3]) == IE_DLL_VER_3) {
              if (LOWORD(dwResource[3]) < IE_DLL_VER_4)
                badVersion = true;
            }
          }
        }

	if (badVersion) {
	  Error(104, L"%s%d%s%d%s%d%s%d",
		L"IEMajor", HIWORD(dwResource[2]),
		L"IEMinor", LOWORD(dwResource[2]),
		L"IEBuild", HIWORD(dwResource[3]),
		L"IEPatch", LOWORD(dwResource[3]));
	  rc = VXIinet_RESULT_PLATFORM_ERROR;
	}
      }

      delete [] infoData;
    }
  }
#endif

  return rc;
}
