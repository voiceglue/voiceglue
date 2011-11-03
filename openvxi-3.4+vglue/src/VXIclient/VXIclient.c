
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

/* -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <wchar.h>

#include <VXIvalue.h>
#include <VXItrd.h>

#include <SBcache.h>
#include <SBinet.h>
#include <SBjsi.h>
#include <SBlog.h>
#include "SBlogListeners.h"

#include <SWIprintf.h>

/* VXIclient headers */
#include "VXIclient.h"

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>          /* For GetFullPathName() */
/* Dynamic libraries */
#ifdef NDEBUG
/* OpenVXI impl */
static const char VXIREC_LIB_NAME[]     = "VXIrec.dll";
static const char VXIPROMPT_LIB_NAME[]  = "VXIprompt.dll";
static const char VXITEL_LIB_NAME[]     = "VXItel.dll"; 
/* Vocalocity impl */
static const char QAREC_LIB_NAME[]      = "QArec.dll";
static const char QAPROMPT_LIB_NAME[]   = "QAprompt.dll";
static const char QATEL_LIB_NAME[]      = "QAtel.dll"; 
#else  /* ! NDEBUG */
/* OpenVXI impl */
static const char VXIREC_LIB_NAME[]     = "VXIrecD.dll";
static const char VXIPROMPT_LIB_NAME[]  = "VXIpromptD.dll";
static const char VXITEL_LIB_NAME[]     = "VXItelD.dll"; 
/* Vocalocity impl */
static const char QAREC_LIB_NAME[]      = "QArecD.dll";
static const char QAPROMPT_LIB_NAME[]   = "QApromptD.dll";
static const char QATEL_LIB_NAME[]      = "QAtelD.dll"; 
#endif /* NDEBUG */

#else /* !WIN32 */

#include <errno.h>
#include <string.h>
#ifdef NDEBUG
/* OpenVXI impl */
static const char VXIREC_LIB_NAME[]     = "libVXIrec.so.3";
static const char VXIPROMPT_LIB_NAME[]  = "libVXIprompt.so.3";
static const char VXITEL_LIB_NAME[]     = "libVXItel.so.3"; 
/* Vocalocity impl */
static const char QAREC_LIB_NAME[]      = "libQArec.so.3";
static const char QAPROMPT_LIB_NAME[]   = "libQAprompt.so.3";
static const char QATEL_LIB_NAME[]      = "libQAtel.so.3"; 
#else  /* ! NDEBUG */
/* OpenVXI impl */
static const char VXIREC_LIB_NAME[]     = "libVXIrecD.so.3";
static const char VXIPROMPT_LIB_NAME[]  = "libVXIpromptD.so.3";
static const char VXITEL_LIB_NAME[]     = "libVXItelD.so.3"; 
/* Vocalocity impl */
static const char QAREC_LIB_NAME[]      = "libQArecD.so.3";
static const char QAPROMPT_LIB_NAME[]   = "libQApromptD.so.3";
static const char QATEL_LIB_NAME[]      = "libQAtelD.so.3"; 
#endif /* NDEBUG */

#endif

#ifndef MODULE_PREFIX
#define MODULE_PREFIX  COMPANY_DOMAIN   L"."
#endif
#define MODULE_VXICLIENT MODULE_PREFIX  L"VXIclient"
#define MODULE_NAME                     MODULE_VXICLIENT
#define PROPERTY_MAX_LEN                0x100
#ifndef MAX_PATH
#define MAX_PATH                        4096
#endif

#define STRINGLEN                       8192

/* Globals */
static VXIbool gblPlatformInitialized = FALSE;
static VXIlogInterface *gblLog        = NULL;
static VXIunsigned gblDiagLogBase     = 0;
static SBlogListenerChannelData *gblLogData = NULL;
static const VXIchar *gblUserAgentName= NULL;
static const VXIchar *SSFT            = L"vocalocity";
static const VXIchar *OPENVXI         = L"openvxi"; 

/* data structure for Dynamic libraries */
typedef struct ResourceAPI {
  /* Recognizer's resource APIs */
  VXIrecResult (*RecInit)
  (
    VXIlogInterface *log, 
    VXIunsigned diagLogBase,
    VXIMap *args
  );
  VXIrecResult (*RecShutDown)(VXIlogInterface *log);
  VXIrecResult (*RecCreateResource)
  (
    VXIlogInterface   *log,
    VXIinetInterface   *inet,
    VXIcacheInterface  *cache,
    VXIpromptInterface *prompt,
    VXItelInterface    *tel,
    VXIrecInterface    **rec
  );
  VXIrecResult (*RecDestroyResource)(VXIrecInterface **rec);             
  
  /* Prompt's resource APIs */
  VXIpromptResult (*PromptInit)
  (
    VXIlogInterface *log,
    VXIunsigned       diagLogBase, 
    const VXIVector  *resources,
    VXIMap           *args
  );
  VXIpromptResult (*PromptShutDown)(VXIlogInterface *log);                            
  VXIpromptResult (*PromptCreateResource)
  (
    VXIlogInterface *log,
    VXIinetInterface    *inet,
    VXIcacheInterface   *cache,
    VXIpromptInterface **prompt
  ); 
  VXIpromptResult (*PromptDestroyResource)(VXIpromptInterface **prompt);
  
  /* Tel's resource APIs */
  VXItelResult (*TelInit)
  (
    VXIlogInterface *log,
    VXIunsigned       diagLogBase, 
    VXIMap           *args
  );
  VXItelResult (*TelShutDown)(VXIlogInterface *log);                            
  VXItelResult (*TelCreateResource)
  (
    VXIlogInterface *log,
    VXItelInterface **tel
  ); 
  VXItelResult (*TelDestroyResource)(VXItelInterface **prompt);

  DynLibrary *dynRecLib;    /* Rec lib */
  DynLibrary *dynPromptLib; /* Prompt lib */
  DynLibrary *dynTelLib;    /* Tel. lib */

} ResourceAPI;

static ResourceAPI gblResAPI;

static const VXIchar * AppendStringAlloc(VXIchar ** dst, const VXIchar * src)
{
  VXIchar * temp, *_dst = NULL;
  int srclen = 0, dstlen = 0;
  if( !dst ) return NULL;
  if( !src ) return *dst;
  srclen = wcslen(src) + 1;
  _dst = *dst;
  if( !_dst )
  {
    _dst = (VXIchar*)malloc(sizeof(VXIchar)*srclen);
    if( !_dst ) return NULL;
    wcscpy(_dst, src);
    *dst = _dst;
    return _dst;
  }
  else
  {
    dstlen = wcslen(_dst);
    temp = (VXIchar*)realloc(_dst, (dstlen + srclen + 1) * sizeof(VXIchar));
    if( !temp ) return NULL;
    _dst = temp;
    wcscpy(&_dst[dstlen], src);    
    *dst = _dst;
    return _dst;
  }
  /* shoud never reach here */
  return NULL;
}

static VXIplatformResult LoadImplSpecificLibs(VXIMap *configArgs)
{
  VXIplatformResult platResult;
  const VXIchar* impl = NULL;
  int isLoaded = 0; 
   
  /* Load rec lib */
  if( GetVXIString(configArgs, CLIENT_REC_IMPLEMENTATION, &impl) )
  {
    if( wcscmp(impl, SSFT) == 0 ) {
      /* Load Vocalocity Rec implementation */  
      platResult = LoadDynLibrary(QAREC_LIB_NAME, &gblResAPI.dynRecLib);
      CHECK_RESULT_RETURN(NULL, "LoadDynLibrary()", platResult);
      isLoaded = 1;
    }
    else if( wcscmp(impl, OPENVXI) == 0 ) {
      /* Load OpenVXI's implementation */
      platResult = LoadDynLibrary(VXIREC_LIB_NAME, &gblResAPI.dynRecLib);
      CHECK_RESULT_RETURN(NULL, "LoadDynLibrary()", platResult); 
      isLoaded = 1;               
    } else {
      /* Other implementation, return error for now */
      return VXIplatform_RESULT_UNSUPPORTED;       
    }
  }
  
  if( !isLoaded ) {
    /* If there is no specific implementation of rec, assume it is SSFT first */
    platResult = LoadDynLibrary(QAREC_LIB_NAME, &gblResAPI.dynRecLib);
    /* Ignore previous error in case SSFT's impl not found
       try to load OpenVXI's impl */
    if( platResult != 0 ) {
      platResult = LoadDynLibrary(VXIREC_LIB_NAME, &gblResAPI.dynRecLib);
      CHECK_RESULT_RETURN(NULL, "LoadDynLibrary()", platResult);                
    }
  }
  
  /* Load rec symbols */
  {
    platResult = LoadDynSymbol(gblResAPI.dynRecLib, "VXIrecInit", 
                              (void **)&gblResAPI.RecInit);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
    platResult = LoadDynSymbol(gblResAPI.dynRecLib, "VXIrecShutDown", 
                              (void **)&gblResAPI.RecShutDown);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
    platResult = LoadDynSymbol(gblResAPI.dynRecLib, "VXIrecCreateResource", 
                              (void **)&gblResAPI.RecCreateResource);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
    platResult = LoadDynSymbol(gblResAPI.dynRecLib, "VXIrecDestroyResource", 
                              (void **)&gblResAPI.RecDestroyResource);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
  }

  /* Load promt lib */
  isLoaded = 0;
  impl = NULL;
  if( GetVXIString(configArgs, CLIENT_PROMPT_IMPLEMENTATION, &impl) )
  {
    if( wcscmp(impl, SSFT) == 0 ) {
      /* Load Vocalocity Prompt implementation */  
      platResult = LoadDynLibrary(QAPROMPT_LIB_NAME, &gblResAPI.dynPromptLib);
      CHECK_RESULT_RETURN(NULL, "LoadDynLibrary()", platResult);
      isLoaded = 1;
    }    
    else if( wcscmp(impl, OPENVXI) == 0 ) {
      /* Load OpenVXI's implementation */
      platResult = LoadDynLibrary(VXIPROMPT_LIB_NAME, &gblResAPI.dynPromptLib);
      CHECK_RESULT_RETURN(NULL, "LoadDynLibrary()", platResult);   
      isLoaded = 1;             
    } else {
      /* Other implementation, return error for now */
      return VXIplatform_RESULT_UNSUPPORTED;       
    }
  }
  
  if( !isLoaded ) {
    /* If there is no specific implementation of prompt, assume it is SSFT */
    platResult = LoadDynLibrary(QAPROMPT_LIB_NAME, &gblResAPI.dynPromptLib);
    /* Ignore previous error in case SSFT's impl not found,
       try to load OpenVXI Prompt implementation */  
    if( platResult != 0 ) {
      platResult = LoadDynLibrary(VXIPROMPT_LIB_NAME, &gblResAPI.dynPromptLib);
      CHECK_RESULT_RETURN(NULL, "LoadDynLibrary()", platResult);                
    }
  }
  
  /* Load Prompt symbols */
  {  
    platResult = LoadDynSymbol(gblResAPI.dynPromptLib, "VXIpromptInit", 
                              (void **)&gblResAPI.PromptInit);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
    platResult = LoadDynSymbol(gblResAPI.dynPromptLib, "VXIpromptShutDown", 
                              (void **)&gblResAPI.PromptShutDown);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
    platResult = LoadDynSymbol(gblResAPI.dynPromptLib, "VXIpromptCreateResource", 
                              (void **)&gblResAPI.PromptCreateResource);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
    platResult = LoadDynSymbol(gblResAPI.dynPromptLib, "VXIpromptDestroyResource", 
                              (void **)&gblResAPI.PromptDestroyResource);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);    
  }           

  /* Load Tel lib */
  isLoaded = 0;
  impl = NULL;
  if( GetVXIString(configArgs, CLIENT_TEL_IMPLEMENTATION, &impl) )
  {
    if( wcscmp(impl, SSFT) == 0 ) {
      /* Load Vocalocity Tel implementation */  
      platResult = LoadDynLibrary(QATEL_LIB_NAME, &gblResAPI.dynTelLib);
      CHECK_RESULT_RETURN(NULL, "LoadDynLibrary()", platResult);
      isLoaded = 1;
    }    
    else if( wcscmp(impl, OPENVXI) == 0 ) {
      /* Load OpenVXI's implementation */
      platResult = LoadDynLibrary(VXITEL_LIB_NAME, &gblResAPI.dynTelLib);
      CHECK_RESULT_RETURN(NULL, "LoadDynLibrary()", platResult);                
      isLoaded = 1;
    } else {
      /* Other implementation, return error for now */
      return VXIplatform_RESULT_UNSUPPORTED;       
    }
  }
  
  if( !isLoaded ) {
    /* If there is no specific implementation of Tel, assume it is SSFT */
    platResult = LoadDynLibrary(QATEL_LIB_NAME, &gblResAPI.dynTelLib);
    /* Ignore previous error in case SSFT's impl not found,
       try to load OpenVXI Tel implementation */  
    if( platResult != 0 ) {
      platResult = LoadDynLibrary(VXITEL_LIB_NAME, &gblResAPI.dynTelLib);
      CHECK_RESULT_RETURN(NULL, "LoadDynLibrary()", platResult);                
    }
  }
    
  /* Load Tel symbols */
  {  
    platResult = LoadDynSymbol(gblResAPI.dynTelLib, "VXItelInit", 
                              (void **)&gblResAPI.TelInit);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
    platResult = LoadDynSymbol(gblResAPI.dynTelLib, "VXItelShutDown", 
                              (void **)&gblResAPI.TelShutDown);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
    platResult = LoadDynSymbol(gblResAPI.dynTelLib, "VXItelCreateResource", 
                              (void **)&gblResAPI.TelCreateResource);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);
    platResult = LoadDynSymbol(gblResAPI.dynTelLib, "VXItelDestroyResource", 
                              (void **)&gblResAPI.TelDestroyResource);
    CHECK_RESULT_RETURN(NULL, "LoadDynSymbol()", platResult);    
  }           
      
  return VXIplatform_RESULT_SUCCESS;   
}

/**
 * Initialize the platform.  
 *
 * This function initializes all the components in the OSB PIK in the
 * required order.  The order can be changed quite a bit, but logging
 * must be initialized first. Also VXI requires that all the
 * components be initialized and set before it can be created.
 */
VXIplatformResult VXIplatformInit(VXIMap *configArgs, VXIunsigned *nbChannels)
{
  VXItrdResult trdResult;
  VXIlogResult logResult;
  VXIpromptResult promptResult;
  VXIinetResult inetResult;
  VXItelResult telResult;
  VXIcacheResult cacheResult;
  VXIjsiResult jsiResult;
  VXIobjResult objResult;
  VXIrecResult recResult;
  VXIinterpreterResult interpreterResult;
  VXIplatformResult platResult;
  VXIint32 diagLogBase;
  VXIint32 tempInt = 0;

  if (gblPlatformInitialized)
    return VXIplatform_RESULT_ALREADY_INITIALIZED;

  /* initialized dynamic loading structure */
  memset(&gblResAPI, 0, sizeof(gblResAPI));
   
  /* Initialize the low-level TRD utilities library */
  {
    if( GetVXIInt(configArgs, CLIENT_TRD_THREAD_STACKSIZE, &tempInt) )
    {
      trdResult = VXItrdInit(tempInt);
      CHECK_RESULT_RETURN(NULL, "VXItrdInit()", trdResult);
    }
  }
  
  /* Initialize log */
  {
    const VXIchar *lfname = L"log.txt";
    const VXIchar *ldname = L"logContent";
    const VXIchar *lfmime = L"text/plain;charset=ISO-8859-1";
    VXIint32 lmsize = 50, dmsize = 10, logfd, loglevel;
    VXIbool logToStdout = FALSE, keepLogFileOpen = FALSE;
    VXIbool reportErrorText = FALSE;
    const VXIVector *errorMapFiles = NULL;
    VXIVector *loadedErrorMapFiles = NULL;
    VXIlogInterface *logResource = NULL;
    
    /* Get VXIClient diag log base */
    if( GetVXIInt(configArgs, CLIENT_CLIENT_DIAG_BASE, &tempInt) )      
      gblDiagLogBase = (VXIunsigned) tempInt;

    /* Initialize the logging interface */
    logResult = SBlogInit(); 
    CHECK_RESULT_RETURN(NULL, "SBlogInit()", logResult);

    /* Create a global log stream with channel number -1 which is not
       used as a real channel number. This log is used for global
       operations. */
    logResult = SBlogCreateResource(&logResource);
    CHECK_RESULT_RETURN(NULL, "SBlogCreateResource()", logResult);

    /* Load configuration parameters */
    GetVXIInt(configArgs, L"logfd", &logfd);
    GetVXIInt(configArgs, L"loglevel", &loglevel);
    GetVXIString(configArgs, CLIENT_LOG_FILE_NAME, &lfname);
    GetVXIInt(configArgs, CLIENT_LOG_MAX_SIZE_MB, &lmsize);
    GetVXIString(configArgs, CLIENT_LOG_FILE_MIME_TYPE, &lfmime);
    GetVXIString(configArgs, CLIENT_LOG_CONTENT_DIR, &ldname);
    GetVXIInt(configArgs, CLIENT_LOG_CONTENT_TOTAL_SIZE_MB, &dmsize);
    GetVXIBool(configArgs, CLIENT_LOG_LOG_TO_STDOUT, &logToStdout);
    GetVXIBool(configArgs, CLIENT_LOG_KEEP_LOG_FILE_OPEN, &keepLogFileOpen);
    GetVXIBool(configArgs, CLIENT_LOG_REPORT_ERROR_TEXT, &reportErrorText);
    GetVXIVector(configArgs, CLIENT_LOG_ERROR_MAP_FILES, &errorMapFiles);

    /* If no error mapping files loaded yet, they may be directly present
       in the top-level configuration map, scan for them */
    if (!errorMapFiles) {
      const VXIchar *key;
      VXIchar *ptr;
      const VXIValue *val;
      int prefixlen = wcslen(CLIENT_LOG_ERROR_MAP_FILE_PREFIX);
      
      VXIMapIterator *iter = VXIMapGetFirstProperty(configArgs,&key,&val);
      if (iter) {
        do {
          if (wcsncmp(key, CLIENT_LOG_ERROR_MAP_FILE_PREFIX, prefixlen)==0) {
            /* get the error map index from the key */
            VXIunsigned index = (VXIunsigned) wcstol(key + prefixlen, &ptr,10);
            if (*ptr == L'\0') {
              if (loadedErrorMapFiles == NULL) {
                loadedErrorMapFiles = VXIVectorCreate();
                CHECK_MEMALLOC_RETURN(NULL, loadedErrorMapFiles, L"SBlogInit");
              }
              
              VXIVectorAddElement(loadedErrorMapFiles, VXIValueClone(val));
            }
          }
        } while (VXIMapGetNextProperty(iter, &key, &val) == 0);
      }
      VXIMapIteratorDestroy(&iter);

      if (loadedErrorMapFiles)
        errorMapFiles = loadedErrorMapFiles;

      /* Initialize SBlogListener */
      logResult = SBlogListenerInitEx3(lfname, lfmime,
				       lmsize*1024*1000, ldname, 
                                       dmsize*1024*1000, logToStdout, 
                                       keepLogFileOpen, FALSE, reportErrorText, 
                                       errorMapFiles);
      CHECK_RESULT_RETURN(NULL, "SBlogListenerInitEx3()", logResult);
      if (loadedErrorMapFiles)
        VXIVectorDestroy(&loadedErrorMapFiles);
    }

    /* Create a global log data to register the listeners */
    gblLogData = (SBlogListenerChannelData *) 
                 malloc(sizeof(SBlogListenerChannelData));
    CHECK_MEMALLOC_RETURN(NULL, gblLogData, L"SBlogListenerInit");
    memset (gblLogData, 0, sizeof (SBlogListenerChannelData));
    gblLogData->channel = -1;  /* for global log, there's no real channel */

    /* Register listeners */
    RegisterDiagnosticListener(logResource,DiagnosticListener,gblLogData);
    RegisterErrorListener(logResource,ErrorListener,gblLogData);
    RegisterEventListener(logResource,EventListener,gblLogData);
    RegisterContentListener(logResource,ContentListener,gblLogData);
    gblLog = logResource;
  }

  /**
   * Initialize Write-back cache
   */
  {
    /* Load configuration parameters */
    const VXIchar *cachePath      = L"cache";
    VXIint32 cacheTotalSizeMB     = 200;
    VXIint32 cacheEntryMaxSizeMB  = 20;
    VXIint32 cacheEntryExpTimeSec = 3600;
    VXIint32 cacheLowWaterMark    = 180;
    VXIbool unlockEntries         = TRUE;
    diagLogBase                   = 0;

    GetVXIString(configArgs, CLIENT_CACHE_CACHE_DIR, &cachePath);
    GetVXIInt(configArgs, CLIENT_CACHE_CACHE_TOTAL_SIZE_MB, &cacheTotalSizeMB);
    GetVXIInt(configArgs, CLIENT_CACHE_CACHE_LOW_WATER_MARK_MB, &cacheLowWaterMark);
    GetVXIInt(configArgs, CLIENT_CACHE_CACHE_ENTRY_MAX_SIZE_MB, &cacheEntryMaxSizeMB);
    GetVXIInt(configArgs, CLIENT_CACHE_CACHE_ENTRY_EXP_TIME_SEC,&cacheEntryExpTimeSec);
    GetVXIBool(configArgs, CLIENT_CACHE_UNLOCK_ENTRIES, &unlockEntries);
    GetVXIInt(configArgs, CLIENT_CACHE_DIAG_BASE, &diagLogBase);    
    /* Initialize the caching interface */
    cacheResult = SBcacheInit(gblLog, (VXIunsigned) diagLogBase, 
                              cachePath, cacheTotalSizeMB, 
                              cacheEntryMaxSizeMB,
                              cacheEntryExpTimeSec, unlockEntries, cacheLowWaterMark);
    CHECK_RESULT_RETURN(NULL, "SBcacheInit()", cacheResult);
  }

  /**
   * Initialize the Internet fetch component
   */
  {
    /* Load configuration parameters */
    const VXIchar *proxyServer    = NULL;
    VXIint32 proxyPort            = 0;
    const VXIMap *extensionRules  = NULL;
    const VXIVector *proxyRules   = NULL;
    VXIMap *loadedExtensionRules  = NULL;
    VXIVector *loadedProxyRules   = NULL;
    diagLogBase                   = 0;

    GetVXIMap(configArgs, CLIENT_INET_EXTENSION_RULES, &extensionRules);
    GetVXIVector(configArgs, CLIENT_INET_PROXY_RULES, &proxyRules);
    GetVXIInt(configArgs, CLIENT_INET_DIAG_BASE, &diagLogBase);
    GetVXIString(configArgs, CLIENT_INET_USER_AGENT, &gblUserAgentName);
    
	if( !gblUserAgentName ) gblUserAgentName = L"OpenVXI/3.0";

    /* If no proxy rules loaded yet, they may be directly present in
       the top-level configuration map, scan for them */
    if (!proxyRules)
    {
      const VXIchar *key = NULL;
      const VXIValue *val = NULL;
      VXIchar *ptr = NULL;
      int prefixlen = wcslen(CLIENT_INET_PROXY_RULE_PREFIX);
      
      VXIMapIterator *iter = VXIMapGetFirstProperty(configArgs,&key,&val);
      if (iter) {
        do {
          if (wcsncmp(key, CLIENT_INET_PROXY_RULE_PREFIX, prefixlen)==0) {
            /* get the resource index and property from the key */
            VXIunsigned index = (VXIunsigned) wcstol(key + prefixlen, &ptr,10);
            if( index >= 0 ) {
              VXIString* res = NULL;
              if (loadedProxyRules == NULL) {
                loadedProxyRules = VXIVectorCreate();
                CHECK_MEMALLOC_RETURN(NULL, loadedProxyRules, L"SBinetInit");
              }
              
              // overwrite the old value if exists
              res = (VXIString*) VXIVectorGetElement(loadedProxyRules, index);
              if( res == NULL ) {
                while (VXIVectorLength(loadedProxyRules) < index + 1) {
                  res = VXIStringCreate(L"");
                  CHECK_MEMALLOC_RETURN(NULL, res, L"SBinetInit");                 
                  VXIVectorAddElement(loadedProxyRules, (VXIValue *)res);
                }
              }
              VXIStringSetValue(res, VXIStringCStr((const VXIString*)val));
            } 
          }
        } while (VXIMapGetNextProperty(iter, &key, &val) == 0);
      }
      VXIMapIteratorDestroy(&iter);     
      if (loadedProxyRules)
        proxyRules = loadedProxyRules;   
    }

    /* If no extension rules loaded yet, they may be directly present in
       the top-level configuration map, scan for them */
    if (!extensionRules) {
      const VXIchar *key;
      const VXIValue *val;
      int prefixlen = wcslen(CLIENT_INET_EXTENSION_RULE_PREFIX);
      
      VXIMapIterator *iter = VXIMapGetFirstProperty(configArgs,&key,&val);
      if (iter) {
        do {
          if (wcsncmp(key, CLIENT_INET_EXTENSION_RULE_PREFIX, prefixlen)==0) {
            if (VXIValueGetType(val) == VALUE_STRING) {
              /* get extension from the key */
              const VXIchar *ext = key + prefixlen - 1;
              if (wcslen(ext) > 1) {   
                if (loadedExtensionRules == NULL) {
                  loadedExtensionRules = VXIMapCreate();
                  CHECK_MEMALLOC_RETURN(NULL, loadedExtensionRules, L"SBinetInit");                                  
                }
                VXIMapSetProperty(loadedExtensionRules, ext, VXIValueClone(val));
              }
            }
          }
        } while (VXIMapGetNextProperty(iter, &key, &val) == 0);
      }
      VXIMapIteratorDestroy(&iter);
      if (loadedExtensionRules)
        extensionRules = loadedExtensionRules;
    }

    /* The next three are optional */
    GetVXIString(configArgs, CLIENT_INET_PROXY_SERVER, &proxyServer);
    GetVXIInt(configArgs, CLIENT_INET_PROXY_PORT, &proxyPort);

    /* Initialize the Internet component */
    inetResult = SBinetInit(gblLog, (VXIunsigned) diagLogBase, NULL, 
                            0, 0, 0, proxyServer, proxyPort,
                            gblUserAgentName, extensionRules, proxyRules);
    CHECK_RESULT_RETURN(NULL, "SBinetInit()", inetResult);

    if (loadedExtensionRules)
      VXIMapDestroy(&loadedExtensionRules);
    if (loadedProxyRules)
      VXIVectorDestroy(&loadedProxyRules);
  }

  /**
   * Initialize the ECMAScript component
   */
  {
    /* Load configuration information */
    VXIint32 runtimeSize = 163840000;
    VXIint32 contextSize = 1310720;
    VXIint32 maxBranches = 1000000;
    diagLogBase          = 0;

    GetVXIInt(configArgs, CLIENT_JSI_RUNTIME_SIZE_BYTES, &runtimeSize);
    GetVXIInt(configArgs, CLIENT_JSI_CONTEXT_SIZE_BYTES, &contextSize);
    GetVXIInt(configArgs, CLIENT_JSI_MAX_BRANCHES, &maxBranches);
    GetVXIInt(configArgs, CLIENT_JSI_DIAG_BASE, &diagLogBase);
    
    /* Initialize the ECMAScript engine */
    jsiResult = SBjsiInit(gblLog, (VXIunsigned) diagLogBase, runtimeSize,
                          contextSize, maxBranches);
    CHECK_RESULT_RETURN(NULL, "OSBjsiInit()", jsiResult);
  }

  
  /**
   *  Dynamically load rec, prompt and tel libraries based
   *  on specific implementation
   */
  {
    platResult = LoadImplSpecificLibs(configArgs);
    if( platResult != VXIplatform_RESULT_SUCCESS ) return platResult;
  }

  /**
   * Initialize the telephony interface
   */
  {
    diagLogBase = 0;
    GetVXIInt(configArgs, CLIENT_TEL_DIAG_BASE, &diagLogBase);
    telResult = gblResAPI.TelInit(gblLog, (VXIunsigned) diagLogBase, configArgs);
    CHECK_RESULT_RETURN(NULL, "TelInit()", telResult);
  }

  /**
   * Initialize the Prompt component
   */
  {
    /* Load configuration parameters */
    const VXIVector *resources = NULL;
    VXIVector *loadedPromptResources = NULL;

    GetVXIVector(configArgs, CLIENT_PROMPT_RESOURCES, &resources);
    
    /* If no prompt resources loaded yet, they may be directly present
       in the top-level configuration map, scan for them */
    if ( !resources ) {
      const VXIchar *key;
      VXIchar *ptr;
      const VXIValue *val;
      int prefixlen = wcslen(CLIENT_PROMPT_RESOURCE_PREFIX);
      
      VXIMapIterator *iter = VXIMapGetFirstProperty(configArgs,&key,&val);
      if (iter) {
        do {
          if (wcsncmp(key, CLIENT_PROMPT_RESOURCE_PREFIX, prefixlen)==0) {
            /* get the resource index and property from the key */
            VXIunsigned index = (VXIunsigned) wcstol(key + prefixlen, &ptr,10);
            if ((*ptr == L'.') && (wcslen(ptr) > 1)) {
              VXIchar property[256];
              VXIMap *res = NULL;
              wcscpy(property, L"com.vocalocity.prompt.resource.");
              wcscat(property, ptr + 1);

              if (loadedPromptResources == NULL) {
                loadedPromptResources = VXIVectorCreate();
                CHECK_MEMALLOC_RETURN(NULL, loadedPromptResources, L"VXIpromptInit");                                  
              }
              
              res = (VXIMap *)VXIVectorGetElement(loadedPromptResources,index);
              if (res == NULL) {
                while (VXIVectorLength(loadedPromptResources) < index + 1) {
                  res = VXIMapCreate();
                  CHECK_MEMALLOC_RETURN(NULL, res, L"VXIpromptInit");                                                   
                  VXIVectorAddElement(loadedPromptResources, (VXIValue *)res);
                }
              }

              VXIMapSetProperty(res, property, VXIValueClone(val));
            } 
            else {
              VXIclientError(NULL, MODULE_VXICLIENT, 104, L"Invalid prompt "
                            L"resource key suffix for configuration parameter",
                            L"%s%s", L"PARAM", key);
            }
          }
        } while (VXIMapGetNextProperty(iter, &key, &val) == 0);
      }
      VXIMapIteratorDestroy(&iter);

      if (loadedPromptResources)
        resources = loadedPromptResources;
    }

    /* Initialize prompting */
    diagLogBase = 0;
    GetVXIInt(configArgs, CLIENT_PROMPT_DIAG_BASE, &diagLogBase);
    promptResult = gblResAPI.PromptInit(gblLog, (VXIunsigned) diagLogBase, resources, configArgs);
    CHECK_RESULT_RETURN(NULL, "PromptInit()", promptResult);

    if (loadedPromptResources)
      VXIVectorDestroy(&loadedPromptResources);   
  }

  /**
   * Initialize the Recognition component
   */
  {
    diagLogBase = 0;
    GetVXIInt(configArgs, CLIENT_REC_DIAG_BASE, &diagLogBase);
    recResult = gblResAPI.RecInit(gblLog, diagLogBase, configArgs);
    CHECK_RESULT_RETURN(NULL, "RecInit()", recResult);   
  }

  /**
   * Initialize the Object component
   */
  {
    /* Load configuration information */
    diagLogBase = 0;
    GetVXIInt(configArgs, CLIENT_OBJ_DIAG_BASE, &diagLogBase);
    objResult = VXIobjectInit(gblLog, (VXIunsigned) diagLogBase);
    CHECK_RESULT_RETURN(NULL, "VXIobjectInit()", objResult);
  }
  
  /**
   * Initialize the VoiceXML interpreter
   */
  {
    /* Retrieve the VXI diagnosis base TAG ID */
    diagLogBase = 0;
    GetVXIInt(configArgs, CLIENT_VXI_DIAG_BASE, &diagLogBase);

    /* Initialize the interpreter */
    interpreterResult = VXIinterpreterInit(gblLog, (VXIunsigned) diagLogBase, NULL);
    CHECK_RESULT_RETURN(NULL, "VXIinterpreterInit()", interpreterResult);
  }
     
  gblPlatformInitialized = TRUE;
  return VXIplatform_RESULT_SUCCESS;
}


/**
 * Shutdown the platform. 
 *
 * VXIplatformShutdown can only be called when all the resources have
 * been destroyed.  
 */
VXIplatformResult VXIplatformShutdown()
{
  VXItrdResult trdResult;
  VXIlogResult logResult;
  VXItelResult telResult;
  VXIpromptResult promptResult;
  VXIinetResult inetResult;
  VXIcacheResult cacheResult;
  VXIjsiResult jsiResult;
  VXIobjResult objResult;
  VXIrecResult recResult;
  
  /* Error if not already initialized */
  if (!gblPlatformInitialized)
    return VXIplatform_RESULT_NOT_INITIALIZED;

  /* Do everything in reverse.  Shutdown the VXI first */
  VXIinterpreterShutDown(gblLog);

  /* Now shutdown Object */
  objResult = VXIobjectShutDown(gblLog);
  CHECK_RESULT_RETURN(NULL, "VXIobjectShutDown()", objResult); 

  /* Now shutdown ECMAScript */
  jsiResult = SBjsiShutDown(gblLog);
  CHECK_RESULT_RETURN(NULL, "SBjsiShutDown()", jsiResult);

  /* Shutdown Rec */
  recResult = gblResAPI.RecShutDown(gblLog);
  CHECK_RESULT_RETURN(NULL, "RecShutDown()", recResult);  

  /* Shutdown Prompt */
  promptResult = gblResAPI.PromptShutDown(gblLog);
  CHECK_RESULT_RETURN(NULL, "PromptShutDown()", recResult);  

  /* Shutdown Tel */
  telResult = gblResAPI.TelShutDown(gblLog);
  CHECK_RESULT_RETURN(NULL, "TelShutDown()", recResult);  

  /* Shutdown the internet fetching component */
  inetResult = SBinetShutDown(gblLog);
  CHECK_RESULT_RETURN(NULL, "SBinetShutDown()", inetResult);

  /* Shutdown the caching component */
  cacheResult = SBcacheShutDown(gblLog);
  CHECK_RESULT_RETURN(NULL, "SBcacheShutDown()", cacheResult);

  /* unregister all listners */
  UnregisterDiagnosticListener(gblLog,DiagnosticListener,gblLogData);
  UnregisterErrorListener(gblLog,ErrorListener,gblLogData);
  UnregisterEventListener(gblLog,EventListener,gblLogData);
  UnregisterContentListener(gblLog,ContentListener,gblLogData);
  free(gblLogData);
  logResult = SBlogDestroyResource(&gblLog);
  CHECK_RESULT_RETURN(NULL, "SBlogDestroyResource()", logResult);

  /* Shutdown the logging API. */
  logResult = SBlogShutDown();
  CHECK_RESULT_RETURN(NULL, "SBlogShutDown()", logResult);

  /* Shutdown the TRD library. At this point the system should just
     be returning and no OpenVXI PIK component functions should be run. */
  trdResult = VXItrdShutDown();
  CHECK_RESULT_RETURN(NULL, "VXItrdShutDown()", trdResult);

  /* Finally close the loaded dynamic libraries */
  CloseDynLibrary(&gblResAPI.dynRecLib);
  CloseDynLibrary(&gblResAPI.dynPromptLib);
  CloseDynLibrary(&gblResAPI.dynTelLib);
  
  gblPlatformInitialized = FALSE;
  return VXIplatform_RESULT_SUCCESS;
}


/**
 * Create resources for the platform.
 *
 * This function creates all the resources needed by the VXI to
 * execute.  This includes both APIs that the VXI calls directly and
 * the APIS that those components call.  All components in the OpenVXI PIK are
 * exposed here.<p>
 *
 * The resources will be written into the platform pointer which is
 * allocated in this function.  
 */
VXIplatformResult VXIplatformCreateResources(VXIunsigned channelNum, 
                                             VXIMap *configArgs,
                                             VXIplatform **platform)
{
  VXIplatform *newPlatform;
  VXItelResult telResult;
  VXIlogResult logResult;
  VXIcacheResult cacheResult;
  VXIinetResult inetResult;
  VXIpromptResult promptResult;
  VXIjsiResult jsiResult;
  VXIobjResult objResult;
  VXIrecResult recResult;
  VXIinterpreterResult interpreterResult;
  VXIchar *connectionPropScript = NULL;
  VXIchar tempScript[STRINGLEN];

  if (!gblPlatformInitialized) {
    return VXIplatform_RESULT_NOT_INITIALIZED;
  }
  if (platform == NULL) {
    return VXIplatform_RESULT_INVALID_ARGUMENT;
  }
  
  /* initialize variables */
  memset(tempScript, 0, sizeof(tempScript));

  /* set platform to NULL and then initialize it to all NULL
     components once it is created */
  *platform = NULL; 
  newPlatform = (VXIplatform *) malloc(sizeof(VXIplatform));
  CHECK_MEMALLOC_RETURN(NULL, newPlatform, L"Platform Allocation");
  memset(newPlatform, 0, sizeof(VXIplatform)); 
  newPlatform->channelNum = channelNum;

  /* Create a channel logging resource.  A logging interface, per
     channel, is required to do the logging.  The channel number is
     bound at the creation time and will appear in any log message
     generated through this interface instance. */
  logResult = SBlogCreateResource(&newPlatform->VXIlog);
  CHECK_RESULT_RETURN(NULL, "SBlogCreateResource()", logResult);

  /* Create a channel specific logging user data */
  newPlatform->logData = (SBlogListenerChannelData *)
                          malloc(sizeof(SBlogListenerChannelData));
  CHECK_MEMALLOC_RETURN(NULL, newPlatform->logData, L"Channel Log-data Allocation");                                                   
  memset (newPlatform->logData, 0, sizeof (SBlogListenerChannelData));
  newPlatform->logData->channel = channelNum;
  
  /* Register logging listeners for this channel */
  RegisterDiagnosticListener(newPlatform->VXIlog, DiagnosticListener,
                             newPlatform->logData);
  RegisterErrorListener(newPlatform->VXIlog,ErrorListener,
                        newPlatform->logData);
  RegisterEventListener(newPlatform->VXIlog,EventListener,
                        newPlatform->logData);
  RegisterContentListener(newPlatform->VXIlog,ContentListener,
                          newPlatform->logData);

  /* Turn on/off diagnostic tags based on the configuration settings,
     we have to log the implementation name after this otherwise
     we'll never see the message */
  VXIclientControliagnosticTags (configArgs, newPlatform);
  CLIENT_LOG_IMPLEMENTATION(newPlatform, L"VXIlog", newPlatform->VXIlog);
  
  /* Store properties for session connection default values.  The values are
     platform-specific and therefore platform's responsibility. 
     They can be referenced as connection.* (technically
     session.connection.* but since the session scope is the global
     scope, connection.* will be found, and the connection.* form is
     what is universally used to query this information for HTML
     browsers). */
  {
    const VXIchar* scString = NULL;
    /* create connection object */
    AppendStringAlloc(&connectionPropScript, L"var connection = new Object();\n");
    
    /* local uri */
    GetVXIString(configArgs, CLIENT_SESSION_CONNECTION_LOCAL_URI, &scString);
    if( scString ) 
    {
      SWIswprintf(tempScript, STRINGLEN, L"connection.local = new Object();\n"
                                       L"connection.local.uri = '%s'\n",
                                       scString);
      AppendStringAlloc(&connectionPropScript, tempScript);
    }
    
    // remote uri
    scString = NULL;
    GetVXIString(configArgs, CLIENT_SESSION_CONNECTION_REMOTE_URI, &scString);
    if( scString )
    {
      SWIswprintf(tempScript, STRINGLEN, L"connection.remote = new Object();\n"
                                       L"connection.remote.uri = '%s';\n",
                                       scString);
      AppendStringAlloc(&connectionPropScript, tempScript);
    }
    
    // protocol name
    scString = NULL;
    GetVXIString(configArgs, CLIENT_SESSION_CONNECTION_PROTOCOL_NAME, &scString);
    if( scString )
    {
      SWIswprintf(tempScript, STRINGLEN, L"connection.protocol = new Object();\n"
                                       L"connection.protocol.name = '%s';\n"
                                       L"connection.protocol.%s = new Object();\n"
                                       L"connection.protocol.%s.uui = 'User-to-User';\n",
                                       scString, scString, scString);
      AppendStringAlloc(&connectionPropScript, tempScript);
      // protocol version
      scString = NULL;
      GetVXIString(configArgs, CLIENT_SESSION_CONNECTION_PROTOCOL_VERSION, &scString);
      if( scString )
      {        
        SWIswprintf(tempScript, STRINGLEN, L"connection.protocol.version = '%s';\n", scString);
        AppendStringAlloc(&connectionPropScript, tempScript);
      }                         
    }

    // application-to-application information
    scString = NULL;
    GetVXIString(configArgs, CLIENT_SESSION_CONNECTION_AAI, &scString);
    if( scString )
    { 
      SWIswprintf(tempScript, STRINGLEN, L"connection.aai = '%s';\n", scString);
      AppendStringAlloc(&connectionPropScript, tempScript);
    }

    // originator 
    scString = NULL;
    GetVXIString(configArgs, CLIENT_SESSION_CONNECTION_ORIGINATOR, &scString);
    if( scString )
    {        
      SWIswprintf(tempScript, STRINGLEN, L"connection.originator = %s;\n", scString);
      AppendStringAlloc(&connectionPropScript, tempScript);
    }

    // Add the channel number
    SWIswprintf(tempScript, STRINGLEN, L"connection.channel = %d;\n", channelNum);
    AppendStringAlloc(&connectionPropScript, tempScript);

    // redirect: a bit tricky to get the array of redirect,
    // first we build a vector of redirect then using the vector to
    // create the ECMAScript
    scString = NULL;
    {
      VXIVector *loadedRedirectResources = NULL;
      const VXIchar *key;
      VXIchar *ptr;
      const VXIValue *val;
      int prefixlen = wcslen(CLIENT_SESSION_CONNECTION_REDIRECT_DOT);
  
      VXIMapIterator *iter = VXIMapGetFirstProperty(configArgs,&key,&val);
      if (iter) 
      {
        do 
        {
          if( wcsncmp(key, CLIENT_SESSION_CONNECTION_REDIRECT_DOT, prefixlen) == 0 )
          {
            /* get the resource index and property from the key */
            VXIunsigned index = (VXIunsigned) wcstol(key + prefixlen, &ptr,10);
            if ((*ptr == L'.') && (wcslen(ptr) > 1)) 
            {
              VXIchar property[256];
              VXIMap *res = NULL;
              wcscpy(property, ptr+1);
              if (loadedRedirectResources == NULL) 
              {
                loadedRedirectResources = VXIVectorCreate();
                CHECK_MEMALLOC_RETURN(NULL, loadedRedirectResources, "Redirect");
              }
              
              res = (VXIMap *)VXIVectorGetElement(loadedRedirectResources,index);
              if (res == NULL) 
              {
                while (VXIVectorLength(loadedRedirectResources) < index + 1) 
                {
                  res = VXIMapCreate();
                  CHECK_MEMALLOC_RETURN(newPlatform, res, L"Construction Redirect Session Vars.");                                                   
                  VXIVectorAddElement(loadedRedirectResources, (VXIValue *)res);
                }
              }

              VXIMapSetProperty(res, property, VXIValueClone(val));
            } 
            else 
            {
              VXIclientError(newPlatform, MODULE_VXICLIENT, 104, L"Invalid redirect "
                            L"property key suffix for configuration parameter",
                            L"%s%s", L"PARAM", key);
            }
          }

        } while (VXIMapGetNextProperty(iter, &key, &val) == 0);
      }
      VXIMapIteratorDestroy(&iter);
      /* Use the vector to create ECMAScript */
      if( loadedRedirectResources )
      {
        int idx , vlen = VXIVectorLength(loadedRedirectResources);
        /* create redirect array object */
        AppendStringAlloc(&connectionPropScript, L"connection.redirect = new Array();\n");
        for( idx = 0; idx < vlen; idx++)
        {
          const VXIMap *v = (const VXIMap*) VXIVectorGetElement (loadedRedirectResources, idx);
          if( v )
          {
            VXIvalueResult ret = VXIvalue_RESULT_SUCCESS;
            const wchar_t* key;
            const VXIValue *value;
            VXIMapIterator *it = VXIMapGetFirstProperty(v, &key, &value);
            /* create redirect[i] object */
            SWIswprintf(tempScript, STRINGLEN, 
                        L"connection.redirect[%d] = new Object();\n", idx);
            AppendStringAlloc(&connectionPropScript, tempScript);
  
            /* create redirect[i].* (.uri, .pi, .si, .reason) */
            while( ret == VXIvalue_RESULT_SUCCESS)
            {
              if( key && value && VXIValueGetType(value) == VALUE_STRING)
              {
                SWIswprintf(tempScript, STRINGLEN, 
                            L"connection.redirect[%d].%s = '%s';\n",
                            idx, key, VXIStringCStr((const VXIString*)value));
                AppendStringAlloc(&connectionPropScript, tempScript);
              }
              ret = VXIMapGetNextProperty(it, &key, &value);
            }
            VXIMapIteratorDestroy(&it);            

          } /* end-if */
        } /* end-for */
        
        /* destroy the vector */
        VXIVectorDestroy(&loadedRedirectResources);
      }
    } // end-scope Redirect   
  }  
  
  /* Create the cache resource.  The cache resource will be used by
     the recognizer and prompting components for caching of computed
     data like compiled grammars and text-to-speech prompts. */
  cacheResult = SBcacheCreateResource(newPlatform->VXIlog, &newPlatform->VXIcache);
  CHECK_RESULT_RETURN(newPlatform, "SBcacheCreateResource()", cacheResult);
  CLIENT_LOG_IMPLEMENTATION(newPlatform, L"VXIcache", newPlatform->VXIcache);
  
  /* Lookup whether cookies should be accepted */
  GetVXIBool(configArgs, CLIENT_INET_ACCEPT_COOKIES, &newPlatform->acceptCookies);
  
  /* Create the internet component for fetching URLs */
  inetResult = SBinetCreateResource(newPlatform->VXIlog, 
                                   newPlatform->VXIcache,
                                   &newPlatform->VXIinet);
  CHECK_RESULT_RETURN(newPlatform, "SBinetCreateResource()", inetResult);
  CLIENT_LOG_IMPLEMENTATION(newPlatform, L"VXIinet", newPlatform->VXIinet);

  /* Now create the jsi resource.  This is used for storing and
     computing ECMAScript results by the VXI. Internal component
     required by VXI */
  jsiResult = SBjsiCreateResource(newPlatform->VXIlog, &newPlatform->VXIjsi);
  CHECK_RESULT_RETURN(newPlatform, "SBjsiCreateResource()", jsiResult);
  CLIENT_LOG_IMPLEMENTATION(newPlatform, L"VXIjsi", newPlatform->VXIjsi);

  /* Now loading ECMAScript file if specified in the config file */
  {
    const VXIchar * jsFile = NULL;
    GetVXIString(configArgs, CLIENT_JSI_GLOBAL_SCRIPT_FILE, &jsFile);
    if( jsFile ) {
      /* Use inet to open this file */
      VXIinetResult res;
      VXIinetStream *stream;
      VXIulong nread, wcsize = 0;
      VXIint totalRead = 0;
      VXIbyte buffer[STRINGLEN];  /* Hopefully the buffer won't overflow */
      VXIinetInterface *inet = NULL;
      /* Create the internet component for fetching URLs */
      inetResult = SBinetCreateResource(newPlatform->VXIlog, newPlatform->VXIcache, &inet);
      CHECK_RESULT_RETURN(newPlatform, "SBinetCreateResource()", inetResult);
      res = inet->Open(inet,
                       MODULE_VXICLIENT, jsFile,
                       INET_MODE_READ, 0, NULL, 
                       NULL, &stream);      

      // successfully open the uri, now try to read the content
      if( res == VXIinet_RESULT_SUCCESS ) {
        while(res == VXIinet_RESULT_SUCCESS) {
          nread = 0;
          res = inet->Read(inet, buffer, sizeof(buffer), &nread, stream);
          totalRead += nread;
        }
        tempScript[0] = L'\0';
        buffer[totalRead] = L'\0';
        if (char2wchar(tempScript, (const char*)buffer, STRINGLEN)
            == VXIplatform_RESULT_BUFFER_TOO_SMALL )
        {
          VXIclientError(newPlatform, L"testClient", 9999, L"", L"%s%s", L"buffer", L"too small");
        }
        // append to session script
        AppendStringAlloc(&connectionPropScript, tempScript);
      } 
      else {
       VXIclientError(newPlatform, MODULE_VXICLIENT, 9999, L"", L"%s%s", L"uri not found", jsFile);
      }
      // Close Inet Stream
      inet->Close(inet, &stream);            
      // Destroy inet
      SBinetDestroyResource(&inet);        
    }
  }

  // save the connection properties
  if( connectionPropScript )
    newPlatform->connectionPropScript = connectionPropScript;

  /* Create the VXItel implementation resource.  This is required by
     the VXI for handling telephony functions. The telephony resource
     uses the session control resource to perform its functions so it
     must be passed in. */
  telResult = gblResAPI.TelCreateResource(newPlatform->VXIlog, &newPlatform->VXItel);
  CHECK_RESULT_RETURN(newPlatform, "VXItelCreateResource()", telResult);
  CLIENT_LOG_IMPLEMENTATION(newPlatform, L"VXItel", newPlatform->VXItel);
  
  /* Now create the prompt resource. The prompting resource takes a
     logging interface for logging and the inet interface for getting
     prompts from URIs. */
  promptResult = gblResAPI.PromptCreateResource(newPlatform->VXIlog, 
                                                newPlatform->VXIinet,
                                                newPlatform->VXIcache,
                                                &newPlatform->VXIprompt);
  CHECK_RESULT_RETURN(newPlatform, "VXIpromptCreateResource()", promptResult);
  CLIENT_LOG_IMPLEMENTATION(newPlatform, L"VXIprompt", newPlatform->VXIprompt);

  /* Now create the recognizer resource. The recognizer resource takes
     a logging interface for logging and the inet interface for
     getting grammars from URIs. */
  recResult = gblResAPI.RecCreateResource(newPlatform->VXIlog,
                                          newPlatform->VXIinet,
                                          newPlatform->VXIcache,
                                          newPlatform->VXIprompt,
                                          newPlatform->VXItel,
                                          &newPlatform->VXIrec);
  CHECK_RESULT_RETURN(newPlatform, "VXIrecCreateResource()", recResult);
  CLIENT_LOG_IMPLEMENTATION(newPlatform, L"VXIrec", newPlatform->VXIrec);

  /* Now create the object resource.  This is used for executing
     VoiceXML <object> elements */
  newPlatform->objectResources.log     = newPlatform->VXIlog;
  newPlatform->objectResources.inet    = newPlatform->VXIinet;
  newPlatform->objectResources.jsi     = newPlatform->VXIjsi;
  newPlatform->objectResources.rec     = newPlatform->VXIrec;
  newPlatform->objectResources.prompt  = newPlatform->VXIprompt;
  newPlatform->objectResources.tel     = newPlatform->VXItel;
  newPlatform->objectResources.platform = newPlatform;

  objResult = VXIobjectCreateResource(&newPlatform->objectResources,
                                      &newPlatform->VXIobject);
  CHECK_RESULT_RETURN(newPlatform, "VXIobjectCreateResource()", objResult);
  CLIENT_LOG_IMPLEMENTATION(newPlatform, L"VXIobject", newPlatform->VXIobject);

  /* Now copy the components required by the VXI into its resource
     structure for the VXI initialization */
  newPlatform->resources.log     = newPlatform->VXIlog;
  newPlatform->resources.inet    = newPlatform->VXIinet;
  newPlatform->resources.jsi     = newPlatform->VXIjsi;
  newPlatform->resources.rec     = newPlatform->VXIrec;
  newPlatform->resources.prompt  = newPlatform->VXIprompt;
  newPlatform->resources.tel     = newPlatform->VXItel;
  newPlatform->resources.object  = newPlatform->VXIobject;
  newPlatform->resources.cache   = newPlatform->VXIcache;

  interpreterResult = VXIinterpreterCreateResource(&newPlatform->resources,
                                                   &newPlatform->VXIinterpreter);
  CHECK_RESULT_RETURN(newPlatform, "VXIinterpreterCreateResource()", interpreterResult);
  CLIENT_LOG_IMPLEMENTATION(newPlatform, L"VXIinterpreter", newPlatform->VXIinterpreter);

  /* Set interpreter properties. */
  {
    const VXIchar *vxiBeepURI = NULL;
    const VXIchar *vxiDefaultsURI = NULL;

    VXIMap *vxiProperties = VXIMapCreate();
    CHECK_MEMALLOC_RETURN(newPlatform, vxiProperties, "VXI properties");

    GetVXIString(configArgs, CLIENT_VXI_BEEP_URI, &vxiBeepURI);
    if (vxiBeepURI != NULL) {
      VXIString * val = VXIStringCreate(vxiBeepURI);
      CHECK_MEMALLOC_RETURN(newPlatform, val, "VXI properties beep URI");
      VXIMapSetProperty(vxiProperties, VXI_BEEP_AUDIO, (VXIValue *) val);
    }

    GetVXIString(configArgs, CLIENT_VXI_DEFAULTS_URI, &vxiDefaultsURI);
    if (vxiDefaultsURI != NULL) {
      VXIString * val = VXIStringCreate(vxiDefaultsURI);
      CHECK_MEMALLOC_RETURN(newPlatform, val, "VXI properties beep URI");
      VXIMapSetProperty(vxiProperties, VXI_PLATFORM_DEFAULTS, (VXIValue*) val);
    }

    if (VXIMapNumProperties(vxiProperties) != 0)
      newPlatform->VXIinterpreter->SetProperties(newPlatform->VXIinterpreter,
                                                 vxiProperties);
    VXIMapDestroy(&vxiProperties);
  }

  VXIclientDiag(newPlatform, CLIENT_API_TAG, L"VXIplatformCreateResources",
                L"exiting: rc = %d, 0x%p",
                VXIplatform_RESULT_SUCCESS, newPlatform);

  /* Return the platform resources that have been created */
  *platform = newPlatform;  
  return VXIplatform_RESULT_SUCCESS;
}


/**
 * Destroy resources for the platform.
 *
 * This function destroys all the resources needed by the VXI to
 * execute.  This includes both APIs that the VXI calls directly and
 * the APIS that those components call.  All components in the OpenVXI PIK are
 * exposed here.<p>
 *
 * The resources will be freed from the platform pointer which is
 * deleted and set to NULL in this function.  
 */
VXIplatformResult VXIplatformDestroyResources(VXIplatform **platform)
{
  
  VXItelResult telResult;
  VXIlogResult logResult;
  VXIpromptResult promptResult;
  VXIjsiResult jsiResult;
  VXIrecResult recResult;
  VXIcacheResult cacheResult;
  VXIinetResult inetResult;
  VXIobjResult objResult;
  VXIplatform *pPlatform = NULL;

  if ((platform == NULL) || (*platform == NULL)) {
    return VXIplatform_RESULT_INVALID_ARGUMENT;
  }
  if (!gblPlatformInitialized) {
    return VXIplatform_RESULT_NOT_INITIALIZED;
  }

  pPlatform = *platform;
  
  VXIclientDiag(pPlatform, CLIENT_API_TAG, L"VXIplatformDestroyResources",
                L"entering: 0x%p", platform);
  
  /* Destroy the VoiceXML interpreter */
  VXIinterpreterDestroyResource(&pPlatform->VXIinterpreter);

  /* Destroy the object handler */
  objResult = VXIobjectDestroyResource(&pPlatform->VXIobject);
  CHECK_RESULT_RETURN(pPlatform, "VXIobjectDestroyResource()", objResult);

  /* Destroy the recognizer */
  recResult = gblResAPI.RecDestroyResource(&pPlatform->VXIrec);
  CHECK_RESULT_RETURN(pPlatform, "VXIrecDestroyResource()", recResult);

  /* Destroy the prompt engine */
  promptResult = gblResAPI.PromptDestroyResource(&pPlatform->VXIprompt);
  CHECK_RESULT_RETURN(pPlatform, "VXIpromptDestroyResource()", promptResult);

  /* Destroy the telephony component */
  telResult = gblResAPI.TelDestroyResource(&pPlatform->VXItel);
  CHECK_RESULT_RETURN(pPlatform, "VXItelDestroyResource()", telResult);

  /* Destroy the ECMAScript interpreter */
  jsiResult = SBjsiDestroyResource(&pPlatform->VXIjsi);
  CHECK_RESULT_RETURN(pPlatform, "SBjsiDestroyResource()", jsiResult);

  /* Destroy the internet component */
  inetResult = SBinetDestroyResource(&pPlatform->VXIinet);
  CHECK_RESULT_RETURN(pPlatform, "SBinetDestroyResource()", inetResult);
  
  /* Destroy the cache component */
  cacheResult = SBcacheDestroyResource(&pPlatform->VXIcache);
  CHECK_RESULT_RETURN(pPlatform, "SBcacheDestroyResource()", cacheResult);

  /* unregister all listners */
  UnregisterDiagnosticListener(pPlatform->VXIlog,DiagnosticListener,pPlatform->logData);
  UnregisterErrorListener(pPlatform->VXIlog,ErrorListener,pPlatform->logData);
  UnregisterEventListener(pPlatform->VXIlog,EventListener,pPlatform->logData);
  UnregisterContentListener(pPlatform->VXIlog,ContentListener,pPlatform->logData);
  if( pPlatform->logData ) free(pPlatform->logData);
  pPlatform->logData = NULL;

  /* Destroy the log component */
  logResult = SBlogDestroyResource(&pPlatform->VXIlog);
  CHECK_RESULT_RETURN(NULL, "SBlogDestroyResource()", logResult);

  /* release session connection ECMAScript */
  if( pPlatform->connectionPropScript )
  {
    free(pPlatform->connectionPropScript);
    pPlatform->connectionPropScript = NULL;
  }

  /* Release the platform resource handle */
  free(*platform);
  *platform = NULL;

  VXIclientDiag(NULL, CLIENT_API_TAG, L"VXIplatformDestroyResources",
                L"exiting: rc = %d", VXIplatform_RESULT_SUCCESS);
  
  return VXIplatform_RESULT_SUCCESS;
}


/**
 * Set platform diagnostic tracing on or off
 *
 * Turns tracing on or off (depending on parameter
 */
void VXIplatformSetTrace (int state)
{
    VXIbool boolState;

    if (state)
    {
	boolState = TRUE;
    }
    else
    {
	boolState = FALSE;
    };

    ControlDiagnosticTag (gblLog, 2000, boolState);
    ControlDiagnosticTag (gblLog, 3000, boolState);
    ControlDiagnosticTag (gblLog, 4000, boolState);
    ControlDiagnosticTag (gblLog, 5000, boolState);
    ControlDiagnosticTag (gblLog, 5001, boolState);
    ControlDiagnosticTag (gblLog, 5002, boolState);
    ControlDiagnosticTag (gblLog, 6000, boolState);
    ControlDiagnosticTag (gblLog, 7000, boolState);
    ControlDiagnosticTag (gblLog, 9000, boolState);
    ControlDiagnosticTag (gblLog, 8000, boolState);
    ControlDiagnosticTag (gblLog, 8001, boolState);
    ControlDiagnosticTag (gblLog, 8002, boolState);
    ControlDiagnosticTag (gblLog, 10000, boolState);
    ControlDiagnosticTag (gblLog, 10001, boolState);
    ControlDiagnosticTag (gblLog, 10002, boolState);
};


/**
 * Enables the hardware to wait for a call
 *
 * This function enables the hardware to wait for a call using the
 * resources specified by the passed platform pointer. It blocks until
 * the hardware is enabled and then returns
 * VXIplatform_RESULT_SUCCESS.  This must be calld before
 * VXIplatformWaitForCall.
 */
VXIplatformResult VXIplatformEnableCall(VXIplatform *platform)
{
  VXItelResult telResult;
  VXItelInterfaceEx *telIntfEx;  /* extension of VXItel API */

  VXIclientDiag(platform, CLIENT_API_TAG, L"VXIplatformEnableCall",
                L"entering: 0x%p", platform);

  if (!gblPlatformInitialized)
    return VXIplatform_RESULT_NOT_INITIALIZED;

  if (platform == NULL)
    return VXIplatform_RESULT_INVALID_ARGUMENT;

  VXIclientDiag(platform, CLIENT_GEN_TAG, L"Entering EnableCall", NULL);

  /* Enable calls using the telephony interface */
  if (wcsstr(platform->VXItel->GetImplementationName( ), L".VXItel")) {
    telIntfEx = (VXItelInterfaceEx*) platform->VXItel;
    telResult = telIntfEx->EnableCall(telIntfEx);
  } else {
    telResult = VXItel_RESULT_UNSUPPORTED;
  }
  if (telResult != VXItel_RESULT_SUCCESS){
    VXIclientError(platform, MODULE_VXICLIENT, 100, L"VXItel EnableCall failed",
                   L"%s%d", L"rc", telResult);
    return (telResult > 0 ? VXIplatform_RESULT_FAILURE : 
                            VXIplatform_RESULT_TEL_ERROR);
  }

  VXIclientDiag(platform, CLIENT_API_TAG, L"VXIplatformEnableCall",
                L"exiting: rc = %d", VXIplatform_RESULT_SUCCESS);
  VXIclientDiag(platform, CLIENT_GEN_TAG, L"Leaving EnableCall", NULL);
  return VXIplatform_RESULT_SUCCESS;
}

/**
 * Wait for a call.
 *
 * This function waits for a call and then answers it using the
 * resources specified by the passed platform pointer. It blocks until
 * a call is recieved and then returns VXIplatform_RESULT_SUCCESS.
 */
VXIplatformResult VXIplatformWaitForCall(VXIplatform *platform)
{
  VXItelResult telResult;
  VXItelInterfaceEx *telIntfEx;  /* extension of VXItel API */

  VXIclientDiag(platform, CLIENT_API_TAG, L"VXIplatformWaitForCall",
                L"entering: 0x%p", platform);

  if (platform == NULL)
    return VXIplatform_RESULT_INVALID_ARGUMENT;

  /* Clean up from prior calls if necessary */
  if (platform->telephonyProps) {
    VXIMapDestroy(&platform->telephonyProps);
    platform->telephonyProps = NULL;
  }

  /* Wait for calls using the telephony interface */
  if (wcsstr(platform->VXItel->GetImplementationName( ), L".VXItel")) {
    telIntfEx = (VXItelInterfaceEx *) platform->VXItel;
    telResult = telIntfEx->WaitForCall(telIntfEx, &platform->telephonyProps);
  } else {
    telResult = VXItel_RESULT_UNSUPPORTED;
  }
  if (telResult != VXItel_RESULT_SUCCESS){
    VXIclientError(platform, MODULE_VXICLIENT, 100, 
                   L"VXItel WaitForCall failed", L"%s%d", L"rc", telResult);
    return (telResult > 0 ? VXIplatform_RESULT_FAILURE : 
                            VXIplatform_RESULT_TEL_ERROR);
  }

  VXIclientDiag(platform, CLIENT_API_TAG, L"VXIplatformWaitForCall",
                L"exiting: rc = %d", VXIplatform_RESULT_SUCCESS);
  VXIclientDiag(platform, CLIENT_GEN_TAG, L"Leaving WaitForCall", NULL);
  return VXIplatform_RESULT_SUCCESS;
}


/**
 * Process a VoiceXML document.
 *
 * This function processes a VoiceXML document using the resources
 * specified by the passed platform pointer. It blocks until the
 * processing is complete and then returns VXIplatform_RESULT_SUCCESS.
 */
VXIplatformResult VXIplatformProcessDocument(const VXIchar *url,
					     VXIMap *callInfo,
					     VXIchar **sessionScript,
					     VXIValue **documentResult,
					     VXIplatform *platform)
{
  VXIinterpreterResult interpreterResult;
  VXIinetResult inetResult;
  VXItelResult telResult;
  VXIpromptResult promptResult;
  VXIrecResult recResult;
  VXIchar *allocatedUrl = NULL;
  const VXIchar *finalUrl = NULL;
  VXIVector *cookieJar = NULL;
  VXIchar tempScript[STRINGLEN];
  
  if (!gblPlatformInitialized) {
    return VXIplatform_RESULT_NOT_INITIALIZED;
  }

  VXIclientDiag(platform, CLIENT_API_TAG, L"VXIplatformProcessDocument",
                L"entering: %s, 0x%p, 0x%p", 
                url, documentResult, platform);
  /* initialization of variables */
  memset(tempScript, 0, sizeof(tempScript));
  
  /* If the URL is really a local file path, change it to be a full
     path so relative URL references in it can be resolved */
  finalUrl = url;
  if (wcschr(url, L':') == NULL) {
    allocatedUrl = (VXIchar *) calloc(MAX_PATH + 1, sizeof(VXIchar));
    CHECK_MEMALLOC_RETURN(platform, allocatedUrl, "Allocated URL");
#ifdef WIN32
    /* Win32 version */
    {
      
#ifdef UNICODE
      VXIchar *ignored;
      if (!allocatedUrl)
        CHECK_RESULT_RETURN(platform, "full path allocation", -1);      
      if (GetFullPathName(url, MAX_PATH, allocatedUrl, &ignored) <= 0)
        CHECK_RESULT_RETURN(platform, "Win32 GetFullPathName()", -1);
#else
      int len = wcslen(url);
      char *ignored;
      char fpath[MAX_PATH+1];
      char *turl = (char*)malloc(sizeof(char*)*(len+1));
      CHECK_MEMALLOC_RETURN(platform, turl, "Allocated temp. URL");
      wchar2char(turl, url, len+1);
      if (GetFullPathName(turl, MAX_PATH, fpath, &ignored) <= 0)
        CHECK_RESULT_RETURN(platform, "Win32 GetFullPathName()", -1);
      char2wchar(allocatedUrl, fpath, MAX_PATH);  
      free( turl );    
#endif

    }
#else
    {
      /* Unix version */
      char cwd[MAX_PATH + 1];
      cwd[0] = '\0';
      
      getcwd (cwd, MAX_PATH);
      if (!cwd[0])
        CHECK_RESULT_RETURN(platform, "getcwd()", -1);      
      if (strlen(cwd) + wcslen(url) + 1 > MAX_PATH)
        CHECK_RESULT_RETURN(platform, 
          "MAX_PATH exceeded for getting full path", -1);      
      char2wchar(allocatedUrl, cwd, MAX_PATH + 1);
      wcscat (allocatedUrl, L"/");
      wcscat (allocatedUrl, url);
    }
#endif
    finalUrl = allocatedUrl;
  }

  /* Set up SBinet for a new call by enabling or disabling cookies as
     configured for the resource. When cookies are enabled, this
     implementation establishes a new cookie jar for each new call,
     and destroys it at the end of the call. This makes it so cookies
     do not persist across calls, meaning applications can use cookies
     to keep track of in-call state but not track state and callers
     across calls.

     Fully persistant cookies require a highly accurate mechanism for
     identifying individual callers (caller ID is not usually
     sufficient), a database for storing the cookie jar for each
     caller, and a way for individual callers to establish their
     privacy policy (web site or VoiceXML pages that allow them to
     configure whether cookies are enabled and cookie filters). Once
     in place, merely store the cookie jar at the end of the call,
     then once the caller is identified set the cookie jar to the
     previously stored one.  
  */
  if (platform->acceptCookies) {
    cookieJar = VXIVectorCreate();
    CHECK_MEMALLOC_RETURN(platform, cookieJar, "VXIinet cookie jar");
  }
  inetResult = platform->VXIinet->SetCookieJar (platform->VXIinet, cookieJar);
  CHECK_RESULT_RETURN(platform, "VXIinet->SetCookieJar()", inetResult);
  VXIVectorDestroy (&cookieJar);

  /* Set up the telephony interface for a new call. Begin session has to 
     be called at the start of every call.  End session has to be
     called at the end of every call. */
  telResult = platform->VXItel->BeginSession(platform->VXItel, NULL);
  CHECK_RESULT_RETURN(platform, "VXItel->BeginSession()", telResult);

  /* Set up the prompt interface for a new call. Begin session has to 
     be called at the start of every call.  End session has to be
     called at the end of every call. */
  {     
    VXIMap * pProps = VXIMapCreate();
    // Create call num property
    VXIMapSetProperty(pProps, CLIENT_CURRENT_NUMCALL, 
                      (VXIValue*)VXIULongCreate(platform->numCall));
    promptResult = platform->VXIprompt->BeginSession(platform->VXIprompt, pProps);
    VXIMapDestroy(&pProps);
    CHECK_RESULT_RETURN(platform, "VXIprompt->BeginSession()", promptResult);
  }
  /* Set up the recognizer for a new call. Begin session has to 
     be called at the start of every call.  End session has to be
     called at the end of every call. */
  recResult = platform->VXIrec->BeginSession(platform->VXIrec, NULL);
  CHECK_RESULT_RETURN(platform, "VXIrec->BeginSession()", recResult);

  /* Set session connection properties */
  /* the connection ECMAScript has been created at platform creation */
  AppendStringAlloc(sessionScript, platform->connectionPropScript);

  /* Put call-specific values into the session ECMAScript */
  if (platform->telephonyProps) {
    /* Create a ECMAScript for telephony and call properties in session scope */
    SWIswprintf
	(tempScript, STRINGLEN, 
	 L"var telephone = new Object();\n"
	 L"telephone.ani = '%s';\n"
	 L"telephone.dnis = '%s';\n"
	 L"connection.remote.uri = '%s';\n"
	 L"connection.local.uri = '%s';\n"
	 L"connection.vgid = %d;\n%s",
	 VXIStringCStr((const VXIString *) VXIMapGetProperty(callInfo,L"ani")),
	 VXIStringCStr((const VXIString *) VXIMapGetProperty(callInfo,L"dnis")),
	 VXIStringCStr((const VXIString *) VXIMapGetProperty(callInfo,L"ani")),
	 VXIStringCStr((const VXIString *) VXIMapGetProperty(callInfo,L"dnis")),
	 VXIULongValue((const VXIULong *) VXIMapGetProperty(callInfo,L"vgid")),
	 VXIStringCStr((const VXIString *)
		       VXIMapGetProperty(callInfo,L"javascript_init"))
	    );
    /* Append to sessionScript */
    AppendStringAlloc(sessionScript, tempScript);                            
    VXIMapDestroy(&platform->telephonyProps);                              
    platform->telephonyProps = NULL;
  }
  
  /* Store properties for querying the browser implementation in the
     session ECMAScript, they can be referenced as navigator.* (technically
     session.navigator.* but since the session scope is the global
     scope, navigator.* will be found, and the navigator.* form is
     what is universally used to query this information for HTML
     browsers). */
  {
    /* User agent name is configured */
    const VXIchar *start, *end;
    VXIchar ac[128], av[128];

    /* Code name is the portion of the user agent up to the slash */
    end = wcschr(gblUserAgentName, L'/');
    if ((end) && (*end)) {
      memset(ac, 0, sizeof(ac));
      wcsncpy(ac, gblUserAgentName, end - gblUserAgentName);
    }
    else
      CHECK_RESULT_RETURN(platform, "User Agent Name parse",
                          VXIplatform_RESULT_INVALID_ARGUMENT);

    /* Version is the portion of the user agent after the slash and up
       through the alphanumeric sequence that follows it */
    start = end + 1;
    while (ISWSPACE(*start)) start++;
    end = start;
    while ((ISWALPHA(*end)) || (ISWDIGIT(*end)) ||
           (*end == L'.')) end++;    
    if( start && *start )
    {
      memset(av, 0, sizeof(av));
      wcsncpy(av, start, end-start);
    }
    /* Create a ECMAScript for navigator properties in session scope */
    SWIswprintf(tempScript, STRINGLEN, 
                L"var navigator = new Object();\n"
                L"navigator.appName = 'Vocalocity OpenVXI';\n"
                L"navigator.userAgent = '%s';\n"
                L"navigator.appCodeName = '%s';\n"
                L"navigator.appVersion = '%s';\n",
                gblUserAgentName, ac, av );
    AppendStringAlloc(sessionScript, tempScript);  
  }  

  /* Ready to run the VXI.  This will return with a result of one of
     the following:

     a) a VoiceXML page hits an <exit> tag. In this case result will
        contain the value of the exit.
     b) A page simply has no where to go.  The application falls out
        of VoiceXML and ends. result will be NULL in this case.
     c) An error occurs.  result will be NULL in this case.

     Note that the session arguments contain all the information that
     the platform wants to put into the ECMAScript variable session at
     channel startup.
     
     The initial URL will be fetched with a POST sending the session
     arguments in the initial POST.  
  */
  interpreterResult = platform->VXIinterpreter->Run(platform->VXIinterpreter,
                                                    finalUrl, *sessionScript,
                                                    documentResult);
  CHECK_RESULT_NO_RETURN(platform, "VXIinterpreter->Run()", interpreterResult);
  if (allocatedUrl)
    free(allocatedUrl);

  /* Now end the recognizer, prompt, and tel session */
  recResult = platform->VXIrec->EndSession(platform->VXIrec, NULL);
  CHECK_RESULT_RETURN(platform, "VXIrec->EndSession()", recResult);

  promptResult = platform->VXIprompt->EndSession(platform->VXIprompt, NULL);
  CHECK_RESULT_RETURN(platform, "VXIprompt->EndSession()", promptResult);

  telResult = platform->VXItel->EndSession(platform->VXItel, NULL);
  CHECK_RESULT_RETURN(platform, "VXItel->EndSession()", telResult);

  VXIclientDiag(platform, CLIENT_API_TAG, L"VXIplatformProcessDocument",
                L"exiting: rc = %d, 0x%p", interpreterResult, 
                (documentResult ? *documentResult : NULL));
          
  /* Do a map of the interpreter result to a platform result */
  return ConvertInterpreterResult (0);
}


/**
 * Enable/disable diagnostic tags based on the passed configuration data
 */
VXIplatformResult VXIclientControliagnosticTags
(
  const VXIMap *configArgs,
  VXIplatform  *platform
)
{
  VXIplatformResult rc = VXIplatform_RESULT_SUCCESS;
  const VXIchar *key;
  const VXIValue *val;
  int prefixlen = wcslen(CLIENT_LOG_DIAG_TAG_KEY_PREFIX);
  VXIMapIterator *iter;
  VXIlogInterface *log = NULL;
  SBlogInterface *sbLog = NULL;

  if (!configArgs)
    rc = VXIplatform_RESULT_INVALID_ARGUMENT;

  if ((platform) && (platform->VXIlog))
    log = platform->VXIlog;
  else if (gblLog)
    log = gblLog;
  else
    return VXIlog_RESULT_NON_FATAL_ERROR;

  /* Determine the implementation */
  if (wcsstr(log->GetImplementationName( ), L".SBlog")) {
    sbLog = (SBlogInterface *) log;
  } else {
    VXIclientError(platform, MODULE_VXICLIENT, 105,
      L"Unknown log implementation"
      L" for for enabling diagnostic tags", L"%s%s", 
      L"implementationName", log->GetImplementationName( ));
    return VXIlog_RESULT_FAILURE;
  }

  /* Configure the diagnostic tags */
  iter = VXIMapGetFirstProperty(configArgs,&key,&val);
  if (iter) {
    do {
      if (wcsncmp(key, CLIENT_LOG_DIAG_TAG_KEY_PREFIX, prefixlen)==0) {
        if (VXIValueGetType(val) == VALUE_INTEGER) {
          VXIint tflag = VXIIntegerValue((const VXIInteger*)val);
          /* get suffix TAG ID from the key */
          VXIchar *ptr;
          VXIunsigned tagID = (VXIunsigned) wcstol (key + prefixlen,&ptr,10);
          sbLog->ControlDiagnosticTag(sbLog, tagID, tflag ? TRUE : FALSE);
        }
        else {
          VXIclientError(platform, MODULE_VXICLIENT, 103, L"Invalid type for "
                        L"configuration parameter, VXIInteger required", 
                        L"%s%s", L"PARAM", key);
        }
      }
    } while (VXIMapGetNextProperty(iter, &key, &val) == 0);
  }
  VXIMapIteratorDestroy(&iter);
  return rc;
}

/**
 * Convert VXIinterpreterResult codes to VXIplatformResult codes
 */
VXIplatformResult ConvertInterpreterResult(VXIinterpreterResult result)
{
  VXIplatformResult rc;

  switch (result) {
  case VXIinterp_RESULT_FATAL_ERROR:
    rc = VXIplatform_RESULT_INTERPRETER_ERROR;
    break;
  case VXIinterp_RESULT_OUT_OF_MEMORY:
    rc = VXIplatform_RESULT_OUT_OF_MEMORY;
    break;
  case VXIinterp_RESULT_PLATFORM_ERROR:
    rc = VXIplatform_RESULT_PLATFORM_ERROR;
    break;
  case VXIinterp_RESULT_INVALID_PROP_NAME:
    rc = VXIplatform_RESULT_INVALID_PROP_NAME;
    break;
  case VXIinterp_RESULT_INVALID_PROP_VALUE:
    rc = VXIplatform_RESULT_INVALID_PROP_VALUE;
    break;
  case VXIinterp_RESULT_INVALID_ARGUMENT:
    rc = VXIplatform_RESULT_INVALID_ARGUMENT;
    break;
  case VXIinterp_RESULT_SUCCESS:
  case VXIinterp_RESULT_STOPPED:
    rc = VXIplatform_RESULT_SUCCESS;
    break;
  case VXIinterp_RESULT_FAILURE:
    rc = VXIplatform_RESULT_FAILURE;
    break;
  case VXIinterp_RESULT_FETCH_TIMEOUT:
  case VXIinterp_RESULT_FETCH_ERROR:
  case VXIinterp_RESULT_INVALID_DOCUMENT:
    rc = VXIplatform_RESULT_NON_FATAL_ERROR;
    break;
  case VXIinterp_RESULT_UNSUPPORTED:
    rc = VXIplatform_RESULT_UNSUPPORTED;
    break;
  default:
    if (result > 0)
      rc = VXIplatform_RESULT_NON_FATAL_ERROR;
    else
      rc = VXIplatform_RESULT_INTERPRETER_ERROR;
  }  
  return rc;
}

/**
 * Log an error
 */
VXIlogResult VXIclientError
(
  VXIplatform *platform, 
  const VXIchar *moduleName,
  VXIunsigned errorID, 
  const VXIchar *errorIDText,
  const VXIchar *format, ...
)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  VXIlogInterface *log = NULL;
  va_list arguments;

  if ((platform) && (platform->VXIlog))
    log = platform->VXIlog;

  if (format) {
    va_start(arguments, format);
    if ( log )
      rc = (*log->VError)(log, moduleName, errorID, format, arguments);
    else
      rc = VXIclientVErrorToConsole(moduleName, errorID, errorIDText, format,
              arguments);
    va_end(arguments);
  } else {
    if ( log )
      rc = (*log->Error)(log, moduleName, errorID, NULL);
    else
      rc = VXIclientErrorToConsole(moduleName, errorID, errorIDText);
  }
  return rc;
}


/**
 * Log a diagnostic message
 */
VXIlogResult 
VXIclientDiag
(
  VXIplatform *platform, 
  VXIunsigned tag, 
  const VXIchar *subtag, 
  const VXIchar *format, ...
)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  VXIlogInterface *log = NULL;
  va_list arguments;

  if ((platform) && (platform->VXIlog))
    log = platform->VXIlog;

  if (format) {
    va_start(arguments, format);
    if( log )
      rc = (*log->VDiagnostic)(log, tag, subtag, format, arguments);
    va_end(arguments);
  } else {
    if( log )
      rc = (*log->Diagnostic)(log, tag, subtag, NULL);
  }
  return rc;
}

/**
 * Log errors to the console, only used for errors that occur prior
 * to initializing the log subsystem
 */
VXIlogResult VXIclientErrorToConsole
(
  const VXIchar *moduleName,
  VXIunsigned errorID,
  const VXIchar *errorIDText
)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;

  /* Get the severity */
  const VXIchar *severity;
  if (errorID < 200)
    severity = L"CRITICAL";
  else if (errorID < 300)
    severity = L"SEVERE";
  else
    severity = L"WARNING";

  /* OS-dependent routines to handle wide chars. */
#ifdef WIN32
  fwprintf(stderr, L"%ls: %ls|%u|%ls\n", severity, moduleName, errorID,
     errorIDText);
#else
  fprintf(stderr, "%ls: %ls|%u|%ls\n", severity, moduleName, errorID,
    errorIDText);
#endif
  return rc;
}


VXIlogResult 
VXIclientVErrorToConsole
(
  const VXIchar *moduleName, 
  VXIunsigned errorID,
  const VXIchar *errorIDText, 
  const VXIchar *format,
  va_list arguments
)
{
  int argCount;
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  const VXIchar *in;
  VXIchar *out, tempFormat[256];

  /* Get the severity */
  const VXIchar *severity;
  if (errorID < 200)
    severity = L"CRITICAL";
  else if (errorID < 300)
    severity = L"SEVERE";
  else
    severity = L"WARNING";

  /* Properly delimit the format arguments for human consumption */
  argCount = 0;
  if (format) {
    for (in = format, out = tempFormat; *in != L'\0'; in++, out++) {
      if (*in == L'%') {
        argCount++;
        *out = (argCount % 2 == 0 ? L'=' : L'|');
        out++;
      }
      *out = *in;
    }
    *out = L'\n';
    out++;
    *out = L'\0';

#ifdef WIN32
    fwprintf(stderr, L"%ls: %ls|%u|%ls", severity, moduleName, errorID,
       errorIDText);
    vfwprintf(stderr, tempFormat, arguments);
#else
    /* VXIvswprintf( ) handles the format conversion if necessary
       from Microsoft Visual C++ run-time library/GNU gcc 2.x C
       library notation to GNU gcc 3.x C library (and most other UNIX
       C library) notation: %s -> %ls, %S -> %s. Unfortunately there
       is no single format string representation that can be used
       universally. */
    {
      VXIchar tempBuf[4096];
      VXIvswprintf(tempBuf, 4096, tempFormat, arguments);
      fprintf(stderr, "%ls: %ls|%u|%ls%ls\n", severity, moduleName, errorID,
        errorIDText, tempBuf);
    }
#endif
  } else {
#ifdef WIN32
    fwprintf(stderr, L"%ls: %ls|%u|%ls\n", severity, moduleName, errorID,
       errorIDText);
#else
    fprintf(stderr, "%ls: %ls|%u|%ls\n", severity, moduleName, errorID,
      errorIDText);
#endif
  }
  
  return rc;
}
