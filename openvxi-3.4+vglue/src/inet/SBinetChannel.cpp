
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

#define SBINET_EXPORTS
#include "VXItypes.h"
#include "VXIvalue.h"
#include "VXIinet.h"
#include "VXItrd.h"
#include "VXIlog.h"

#include <syslog.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>
#include <vglue_inet.h>

#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <SBinet.h>
#include "SBinetLog.h"

#include "SBinetHttpCacheStream.hpp"
#include "SBinetTimedStream.hpp"
#include "SBinetFileStream.hpp"
#include "SBinetChannel.h"
#include "SBinetURL.h"
#include "SBinetCookie.h"
#include "SBinetProxyMatcher.hpp"
#include "SBinetUtils.hpp"
#include "SBinetHttpConnection.hpp"
#include "SBinetSSLsocket.hpp"

#include <SWIList.hpp>
#include <SWIinputStream.hpp>
#include <SWIoutputStream.hpp>
#include <SWIdataOutputStream.hpp>
#include <SWIipAddress.hpp>

/*****************************************************************************
 *****************************************************************************
 * SBinetChannel Implementation
 *****************************************************************************
 *****************************************************************************
 */
#include <stdexcept>
#include <string>
#include <map>
 
//
// override < and == operators for SBinetMD5 type
//
inline bool operator< (const SBinetMD5 & x,
                       const SBinetMD5 & y)
{ return memcmp(x.raw, y.raw, 16 * sizeof(unsigned char)) < 0; }

inline bool operator==(const SBinetMD5 & x,
                       const SBinetMD5 & y)
{ return memcmp(x.raw, y.raw, 16 * sizeof(unsigned char)) == 0; }

class VXItrdMutexRef {
public:
  VXItrdMutexRef()
  : _refCount(1), _refMutex(NULL)
  {
    VXItrdMutexCreate(&_refMutex);
  }
  virtual ~VXItrdMutexRef()
  {
    VXItrdMutexDestroy(&_refMutex);
  }  
  
  VXItrdMutex   *_refMutex;
  VXIulong      _refCount;
  SBinetMD5     _md5Key;
};

typedef std::map<SBinetMD5, VXItrdMutexRef*> SBINETLOCKMAP;
typedef std::map<SBinetMD5, SBinetHttpCacheStream* > SBINETMD5CACHEMAP;

//Static data members
VXItrdMutex *SBinetChannel::_extensionRulesMutex = NULL;
VXIMap *SBinetChannel::_extensionRules = NULL;
SBinetNString *SBinetChannel::_userAgent = NULL;
double SBinetChannel::_freshnessFraction = SBINET_FRESHNESS_FRACTION_DEFAULT;
time_t SBinetChannel::_freshnessLifetime = (time_t) SBINET_FRESHNESS_LIFETIME_DEFAULT;
time_t SBinetChannel::_maxLifetime = (time_t) SBINET_MAX_LIFETIME_DEFAULT;
VXIint32 SBinetChannel::_pageLoadTimeout = SBINET_PAGE_LOADING_TIMEOUT_DEFAULT;
VXIint32 SBinetChannel::_postContinueTimeout = SBINET_POST_CONTINUE_TIMEOUT_DEFAULT;
SWIList SBinetChannel::_proxyMatcherList;
bool SBinetChannel::_usePersistentConnections = true;
SBinetString *SBinetChannel::_defaultMimeType = NULL;
VXItrdMutex *SBinetChannel::_globalMapMutex = NULL;
VXItrdMutex *SBinetChannel::_globalMD5Mutex = NULL;
static SBINETLOCKMAP _globalLockMap = SBINETLOCKMAP();
static SBINETMD5CACHEMAP _globalMD5CacheMap = SBINETMD5CACHEMAP();

SBinetChannel::SBinetChannel( VXIlogInterface* pVXILog,
			      VXIunsigned diagLogBase,
                              VXIcacheInterface *pVXIcache):
  SWIutilLogger(MODULE_SBINET, pVXILog, diagLogBase), _cookieList(),
  _jarChanged(true), _cookiesEnabled(false),
  _connectionCount(0), _pVXILog(pVXILog), _pVXICache(pVXIcache),
  _echoStream(NULL)
{
  Diag (MODULE_SBINET_CHANNEL_TAGID, L"SBinetChannel::SBinetChannel",
	L"0x%p", pVXILog);

  cachefile_fd = -1;
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::SBinetChannel() constructed";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  // Init interface
  VXIinetInterface::GetVersion = SBinetChannel::GetVersion;
  VXIinetInterface::GetImplementationName =
    SBinetChannel::GetImplementationName;

  VXIinetInterface::Prefetch = staticPrefetch;
  VXIinetInterface::Open = staticOpen;
  VXIinetInterface::Read = staticRead;
  VXIinetInterface::Write = staticWrite;
  VXIinetInterface::Close = staticClose;
  VXIinetInterface::SetCookieJar = staticSetCookieJar;
  VXIinetInterface::GetCookieJar = staticGetCookieJar;
  VXIinetInterface::GetAbsoluteName = staticGetAbsoluteName;

  if ((DiagIsEnabled(MODULE_SBINET_DUMP_HTTP_MSGS)) &&
      (LOG_CONTENT_METHODS_SUPPORTED(_pVXILog)))
  {
    VXIString *logKey = NULL, *logValue = NULL;
    if (_pVXILog->ContentOpen(_pVXILog, MODULE_SBINET, VXI_MIME_TEXT, &logKey,
			      &logValue, &_echoStream) ==
	VXIlog_RESULT_SUCCESS)
    {
      Diag(MODULE_SBINET_DUMP_HTTP_MSGS, NULL,
	   L"Dumping HTTP messages, %s=%s", VXIStringCStr(logKey),
	   VXIStringCStr(logValue));
      VXIStringDestroy(&logKey);
      VXIStringDestroy(&logValue);
    }
  }
}

SBinetChannel::~SBinetChannel( )
{
  Diag (MODULE_SBINET_CHANNEL_TAGID, L"SBinetChannel::~SBinetChannel", NULL);

  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::~SBinetChannel() destructed";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  closeHttpConnections();
  closeAllStreams();
  deleteAllCookies();
  if (_echoStream)
  {
    _pVXILog->ContentClose(_pVXILog, &_echoStream);
  }
  // NOP //    // NOP //    // NOP //
}

static VXIinetResult parseURL(const VXIchar *pszName,
                              const VXIMap *properties,
                              SWIutilLogger *logger,
                              SBinetURL *& url)
{
  const VXIchar *pszUrlBase = SBinetUtils::getString(properties,
                                                     INET_URL_BASE);

  VXIinetResult rc = SBinetURL::create(pszName,pszUrlBase,url);

  if (rc == VXIinet_RESULT_OUT_OF_MEMORY)
  {
    logger->Error(103, NULL);
  }
  else if (rc != VXIinet_RESULT_SUCCESS)
  {
    logger->Error(200, L"%s%s%s%s", L"Operation", L"Open",
                  L"URL", (pszName != NULL) ? pszName : L"NULL");
  }

  return rc;
}


/*
 * Prefetch: For now punt, eventually spawn thread to call Open() into /dev/null
 */
VXIinetResult
SBinetChannel::prefetch(const VXIchar*  pszModuleName,
                        const VXIchar*  pszName,
                        VXIinetOpenMode eMode,
                        VXIint32        nFlags,
                        const VXIMap*   pProperties )
{
    SBinetURL *url;
    VXIinetResult rc = ::parseURL(pszName, pProperties, this, url);
    if (rc != VXIinet_RESULT_SUCCESS)
    {
	if (voiceglue_loglevel() >= LOG_ERR)
	{
	    std::ostringstream logstring;
	    logstring << "SBinetChannel::prefetch() failed parsing URL";
	    voiceglue_log ((char) LOG_ERR, logstring);
	};
	return rc;
    }

    if (voiceglue_loglevel() >= LOG_DEBUG)
    {
	std::ostringstream logstring;
	logstring << "SBinetChannel::prefetch("
		  << VXIchar_to_Std_String(url->getAbsolute())
		  << ") called";
	voiceglue_log ((char) LOG_DEBUG, logstring);
    };

    if( url ) delete url;    

#if 0 // NOT YET IMPLEMENTED
  /*
   * Add prefetch request to prefetch queue. These requests are
   * processed when the fetch engine is idle, and the fetched
   * documents are stored in the Inet cache until retrieved by
   * a subsequent Open call. Note that in order for prefetching
   * to work, caching must be enabled, the fetched document must
   * be cachable (server must not return a 'no-cache' directive)
   * and the caching mode must not be SAFE.
   */
  Diag( MODULE_SBINET_CHANNEL_TAGID, L"SBinetChannel::Prefetch",
	    L"(%s)", pszName );
#endif
  return VXIinet_RESULT_SUCCESS;
}


VXIinetResult
SBinetChannel::closeAllStreams()
{
  // should interate over open stream and close gracefully
  // closeHttpConnections() already remove all open streams
  return(VXIinet_RESULT_SUCCESS);
}


VXIinetStream* SBinetChannel::createHttpStream(SBinetURL *url,
                                               const VXIMap *properties)
{
  SBinetHttpStream::SubmitMethod method = SBinetHttpStream::GET_METHOD;

  const VXIchar *methodStr = SBinetUtils::getString(properties,
                                                    INET_SUBMIT_METHOD);
  if (methodStr == NULL)
    methodStr = INET_SUBMIT_METHOD_DEFAULT;

  if (!::wcscmp(methodStr, INET_SUBMIT_METHOD_POST))
    method = SBinetHttpStream::POST_METHOD;

  SBinetStoppableStream *stream = NULL;

  if (method == SBinetHttpStream::GET_METHOD)
  {
    // Get method, append the query arguments to the URL and create a cache
    // stream.
    const VXIMap *queryArgs =
      (const VXIMap *)VXIMapGetProperty(properties, INET_URL_QUERY_ARGS);

    url->appendQueryArgsToURL(queryArgs);

    //  Because caching is broken, must not use it
    /*
    stream = new SBinetHttpCacheStream(url, method, this, getCache(),
                                       GetLog(), GetDiagBase());
    */
    stream = new SBinetHttpStream(url, method, this, NULL,
				  GetLog(), GetDiagBase());
  }
  else
  {
    // No caching for POST operations.
    stream = new SBinetHttpStream(url, method, this, NULL, GetLog(), GetDiagBase());
  }

  if (stream != NULL)
    return new SBinetTimedStream(stream, GetLog(), GetDiagBase());
  else
    return NULL;
}

VXIinetResult SBinetChannel::createStream(SBinetURL *url,
                                          const VXIMap *properties,
                                          VXIinetStream* &stream)
{
  /*
   * Note: Stream now owns url and must delete on cleanup
   */
  switch (url->getProtocol())
  {
   case SBinetURL::HTTP_PROTOCOL:
   case SBinetURL::HTTPS_PROTOCOL:
     stream = createHttpStream(url, properties);
     break;
   case SBinetURL::FILE_PROTOCOL:
     stream = new SBinetFileStream(url, this, GetLog(), GetDiagBase());
     break;

   default:
     // Can't really happen.
     stream = NULL;
     break;
  }

  if (!stream)
  {
    Error(103, NULL);
    delete url;
    return VXIinet_RESULT_OUT_OF_MEMORY;
  }

  return VXIinet_RESULT_SUCCESS;
}


/*
 * Open: Serious work here.
 *   Parse URL and combine with base
 *   Collect properties and query args
 *   Call appropriate Stream constructor base on URL scheme (http or file)
 *   Call Open() on stream
 */
VXIinetResult
SBinetChannel::open(const VXIchar*   pszModuleName,
		    const  VXIchar*  pszName,
                    VXIinetOpenMode  eMode,
                    VXIint32         nFlags,
                    const VXIMap*    pProperties,
                    VXIMap*          pmapStreamInfo,
                    VXIinetStream**  ppStream)
{
    if ((eMode != INET_MODE_READ) && (eMode != INET_MODE_FILE))
	return(VXIinet_RESULT_UNSUPPORTED);

  SBinetURL *url;
  VXIinetResult rc = ::parseURL(pszName, pProperties, this, url);
  if (rc != VXIinet_RESULT_SUCCESS)
    return rc;

  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::open("
		<< VXIchar_to_Std_String(url->getAbsolute())
		<< ") called with mode "
		<< (eMode == INET_MODE_READ ? "READ" : "FILE")
		<< " and with properties: "
		<< VXIValue_to_Std_String((const VXIValue *) pProperties);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  //  Get arguments to voiceglue_http_get
  const VXIchar *methodStr =
      SBinetUtils::getString(pProperties, INET_SUBMIT_METHOD);
  if (methodStr == NULL) {methodStr = INET_SUBMIT_METHOD_DEFAULT;};
  const VXIMap *postdata_map =
      (const VXIMap *) VXIMapGetProperty (pProperties, INET_URL_QUERY_ARGS);

  //  Use voiceglue httpcache
  std::string cachefile;
  std::string content_type;
  std::string parse_tree_addr;
  voiceglue_http_get
      (methodStr, url->getAbsolute(), postdata_map,
       ((eMode == INET_MODE_FILE) ? 1 : 0),
       cachefile, content_type, parse_tree_addr);

  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::open() voiceglue_http_get("
		<< VXIchar_to_Std_String(url->getAbsolute())
		<< ") returns cachefile=\"" << cachefile << "\""
		<< " content_type=\"" << content_type << "\""
		<< " parse_tree_addr="
		<< parse_tree_addr;
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  if (cachefile.length() < 2)
  {
      if( url ) delete url;    
      return VXIinet_RESULT_INVALID_ARGUMENT;
  };

  //  Get content size
  struct stat statinfo;
  int stat_return;
  stat_return = ::stat (cachefile.c_str(), &statinfo);
  if (stat_return == -1)
  {
      if (voiceglue_loglevel() >= LOG_ERR)
      {
	  std::ostringstream logstring;
	  logstring << "SBinetChannel::open() failed to stat file \""
		    << cachefile
		    << "\" : errno=" << errno;
	  voiceglue_log ((char) LOG_ERR, logstring);
      };
      if( url ) delete url;    
      return VXIinet_RESULT_INVALID_ARGUMENT;
  };

  //  Populate info about the result:

  // Set the MIME type property
  VXIMapSetProperty(pmapStreamInfo, INET_INFO_MIME_TYPE,
		    (VXIValue*)VXIStringCreate
		    (Std_String_to_vxistring(content_type).c_str()));
  // Set the absolute URL property
  VXIMapSetProperty(pmapStreamInfo, INET_INFO_ABSOLUTE_NAME,
                    (VXIValue*)VXIStringCreate(url->getAbsolute()));
  // Set the document size property
  VXIMapSetProperty(pmapStreamInfo, INET_INFO_SIZE_BYTES,
                    (VXIValue*)VXIIntegerCreate(statinfo.st_size));

  if (eMode == INET_MODE_FILE)
  {
      //  Don't open the file, just return its path
      vxistring wide_path = Std_String_to_vxistring (cachefile);
      vxistring wide_parse_tree = Std_String_to_vxistring (parse_tree_addr);
      VXIMapSetProperty(pmapStreamInfo, INET_INFO_LOCAL_FILE_PATH,
			(VXIValue*)VXIStringCreate(wide_path.c_str()));
      VXIMapSetProperty(pmapStreamInfo, INET_INFO_PARSE_TREE,
			(VXIValue*)VXIStringCreate(wide_parse_tree.c_str()));
  }
  else
  {
      //  Open fd to the file for reading
      cachefile_fd = ::open(cachefile.c_str(), O_RDONLY);
      if (cachefile_fd == -1)
      {
	  if (voiceglue_loglevel() >= LOG_ERR)
	  {
	      std::ostringstream logstring;
	      logstring << "SBinetChannel::open() failed opening file \""
			<< cachefile
			<< "\" for reading: " << errno;
	      voiceglue_log ((char) LOG_ERR, logstring);
	  };
	  if( url ) delete url;    
	  return VXIinet_RESULT_INVALID_ARGUMENT;
      };
  };

  if( url ) delete url;    
  return VXIinet_RESULT_SUCCESS;

  /*
  if(!ppStream)
  {
    Error(200, L"%s%s%s%s", L"Operation", L"Open",
          L"URL", (pszName != NULL) ? pszName : L"NULL");
    return(VXIinet_RESULT_INVALID_ARGUMENT);
  }

  Diag (MODULE_SBINET_CHANNEL_TAGID, L"SBinetChannel::Open",
	L"(%s)", pszName != NULL ? pszName : L"NULL");

  rc = createStream(url, pProperties, *ppStream);
  if (rc != VXIinet_RESULT_SUCCESS)
    return rc;

  //  Log to voiceglue
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::open:createStream("
		<< VXIchar_to_Std_String(url->getAbsolute())
		<< ") returns VXIinetStream="
		<< Pointer_to_Std_String(*ppStream)
		<< ", calling its VXIinetStream::Open()";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  rc = (*ppStream)->Open(nFlags, pProperties, pmapStreamInfo);

  //  Log to voiceglue
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::open("
		<< VXIchar_to_Std_String(url->getAbsolute())
		<< ") finished";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  switch (rc)
  {
   case VXIinet_RESULT_SUCCESS:
   case VXIinet_RESULT_NOT_MODIFIED:
     return rc;
     break;
   case VXIinet_RESULT_LOCAL_FILE:
     // no logging to perform.
     break;
   case VXIinet_RESULT_FETCH_TIMEOUT:
     Error(228, L"%s%s", L"URL", url->getAbsolute());
     // no break: intentional
   default:
     Error(204, L"%s%s%s%d",
           L"URL", url->getAbsolute(),
           L"rc",rc);
     break;
  }

  delete (*ppStream);
  *ppStream = NULL;

  return rc;
  */
}


VXIinetResult
SBinetChannel::close(VXIinetStream**  ppStream)
{
  //  Log to voiceglue
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::close("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };
  if (cachefile_fd != -1)
  {
      ::close (cachefile_fd);
  };
  return VXIinet_RESULT_SUCCESS;

  /*
  if (ppStream == NULL)
  {
    Error(200, L"%s%s", L"Operation", L"Close");
    return(VXIinet_RESULT_INVALID_ARGUMENT);
  }
  VXIinetStream* st = *ppStream;

  VXIinetResult err;
  if(st != NULL)
  {
    err = st->Close();
    delete st;
    st = NULL;
  }
  else
  {
    Error(200, L"%s%s", L"Operation", L"Close");
    err = VXIinet_RESULT_INVALID_ARGUMENT;
  }

  *ppStream = NULL;
  return(err);
  */
}


VXIinetResult SBinetChannel::setCookieJar( const VXIVector* pJar )
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::setCookieJar("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };
  return VXIinet_RESULT_SUCCESS;

  /*
  deleteAllCookies();

  if (pJar == NULL)
  {
    // NULL jar means 'disable cookie usage'
    _jarChanged = false;
    _cookiesEnabled = false;
    return VXIinet_RESULT_SUCCESS;
  }

  _cookiesEnabled = true; // Enable cookie usage

  VXIunsigned numElements = VXIVectorLength(pJar);

  for (VXIunsigned i = 0; i < numElements; i++)
  {
    const VXIMap *cookie_map = (const VXIMap *) VXIVectorGetElement(pJar, i);
    if (cookie_map == NULL)
    {
      Error (212, NULL);
      continue;
    }

    if (VXIValueGetType((const VXIValue *) cookie_map) != VALUE_MAP)
    {
      Error (213, L"%s%d", L"VXIValueType",
             VXIValueGetType((const VXIValue *) cookie_map));
      continue;
    }

    VXIint32 tempInt = 0;
    SBinetUtils::getInteger(cookie_map, INET_COOKIE_EXPIRES, tempInt);
    time_t expires = (time_t) tempInt;

    // Check if the cookie is expired, if so don't add it, zero
    // expiration time means the cookie is not persistent and should not be added.
    // For now, add a cookie whose expiration time is 0.
    if(expires != 0 && expires < time(0))
      continue;

    // Get the name
    const VXIchar *tempStr = SBinetUtils::getString(cookie_map, INET_COOKIE_NAME);

    if (tempStr == NULL)
    {
      Error(106, NULL);
      continue;
    }

    // convert to narrow string.
    SBinetNString name = tempStr;

    // Get the domain
    tempStr = SBinetUtils::getString(cookie_map, INET_COOKIE_DOMAIN);

    SBinetNString domain;
    if(tempStr != NULL)
    {
      domain = tempStr;
    }

    // Get the path
    tempStr = SBinetUtils::getString(cookie_map, INET_COOKIE_PATH);
    SBinetNString path;
    if(tempStr != NULL)
    {
      path = tempStr;
    }

    // Get the secure flag
    bool secure = SBinetUtils::getInteger(cookie_map, INET_COOKIE_SECURE, tempInt) && tempInt;

    // AC: Why would it be an error to have a cookie withouth the secure attribute?
    //        if(tempInt == NULL)
    //        {
    //          Error(200, NULL);
    //          continue;
    //        }

    // Get the value
    tempStr = SBinetUtils::getString(cookie_map,INET_COOKIE_VALUE);
    SBinetNString value;
    if(tempStr != NULL)
    {
      value = tempStr;
    }

    // Create the cookie
    SBinetCookie* pSBinetCookie = new SBinetCookie(domain.c_str(), path.c_str(), name.c_str(),
                                                   value.c_str(), expires, secure);
    // Add the cookie to the channel's list
    if (pSBinetCookie  && !addCookie(pSBinetCookie))
      delete pSBinetCookie; // Could not add
  }

  _jarChanged = false;
  return VXIinet_RESULT_SUCCESS;
  */
}


VXIinetResult SBinetChannel::getCookieJar( VXIVector** ppJar,
                                           VXIbool* ppfChanged )
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::getCookieJar("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };
  return VXIinet_RESULT_SUCCESS;

  /*
  time_t now = time(NULL);

  if (ppJar == NULL)
  {
    Error(200, L"%s%s", L"Operation", L"GetCookieJar");
    return VXIinet_RESULT_INVALID_ARGUMENT;
  }

  *ppJar = VXIVectorCreate();
  if(*ppJar == NULL)
  {
    Error(103, NULL);
    return VXIinet_RESULT_OUT_OF_MEMORY;
  }

  // Parse the channel's cookie list
  SWIList::Iterator iter(_cookieList);
  while (iter.hasNext())
  {
    SBinetCookie *cookie = (SBinetCookie *) iter.next();

    // Get session specific cookies as well as cookies that haven't
    // expired.
    if (cookie->getExpires() != 0 && cookie->getExpires() < now )
      continue;

    VXIMap *cookie_map = VXIMapCreate();
    if(cookie_map == NULL)
    {
      VXIVectorDestroy(ppJar);
      Error(103, NULL);
      return VXIinet_RESULT_OUT_OF_MEMORY;
    }

    // Set the domain
    SBinetString tmpStr = cookie->getDomain();
    VXIMapSetProperty(cookie_map, INET_COOKIE_DOMAIN,
                      (VXIValue *)VXIStringCreate(tmpStr.c_str()));

    // Set the path
    tmpStr = cookie->getPath();
    VXIMapSetProperty(cookie_map, INET_COOKIE_PATH,
                      (VXIValue *)VXIStringCreate(tmpStr.c_str()));

    // Set the expiration date
    VXIMapSetProperty(cookie_map, INET_COOKIE_EXPIRES,
                      (VXIValue *)VXIIntegerCreate(cookie->getExpires()));

    // Set the secure flag
    VXIMapSetProperty(cookie_map, INET_COOKIE_SECURE,
                      (VXIValue *)VXIIntegerCreate(cookie->getSecure()));

    // Set the value
    tmpStr = cookie->getValue();
    VXIMapSetProperty(cookie_map, INET_COOKIE_VALUE,
                      (VXIValue *)VXIStringCreate(tmpStr.c_str()));

    // Set the name
    tmpStr = cookie->getName();
    VXIMapSetProperty(cookie_map, INET_COOKIE_NAME,
		      (VXIValue *)VXIStringCreate(tmpStr.c_str()));

    // Set the version, TBD currently a hack, should get the version
    // from the incoming cookie
    VXIMapSetProperty(cookie_map, INET_COOKIE_VERSION,
		      (VXIValue *)VXIIntegerCreate(1));

    // Add the cookie to the jar
    VXIVectorAddElement(*ppJar, (VXIValue *)cookie_map);
  }

  if (ppfChanged != NULL)
  {
    *ppfChanged = _jarChanged;
  }

  return VXIinet_RESULT_SUCCESS;
  */
}


void
SBinetChannel::deleteAllCookies()
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::deleteAllCookies("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };
  return;

  /*
  SBinetCookie *cookie;

  while ((cookie = (SBinetCookie *) _cookieList.removeFirst()) != NULL)
  {
    delete cookie;
  }
  _jarChanged = true;
  */
}


bool SBinetChannel::updateCookie(SBinetCookie *newCookie)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::updateCookie("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };
  return true;

  /*
  if (newCookie == NULL)
    return true;

  time_t now = time(NULL);

  SWIList::Iterator iter(_cookieList);
  while (iter.hasNext())
  {
    SBinetCookie *cookie = (SBinetCookie *) iter.next();
    if (cookie->matchExact(newCookie))
    {
      // If the newCookie is expired, we actually remove both cookies
      // from the jar.
      time_t expires = newCookie->getExpires();
      if (expires != (time_t) 0 && expires < now)
      {
        delete newCookie;
        iter.removeBack();
      }
      else
        iter.set(newCookie);

      _jarChanged = true;
      delete cookie;
      return true;
    }
  }

  // If we get here, it means the cookie was not already in the list.
  // Just add it.
  return addCookie(newCookie);

  */
}


VXIint SBinetChannel::cleanCookies()
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::cleanCookies("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };
  return 0;

      /*
  VXIint deleted = 0;
  time_t now = time(NULL);

  // Delete expired cookies first
  SWIList::Iterator iter(_cookieList);

  while (iter.hasNext())
  {
    SBinetCookie *cookie = (SBinetCookie *) iter.next();

    // remove expired cookie.
    time_t expires = cookie->getExpires();
    if (expires != (time_t) 0 && expires < now)
    {
      iter.removeBack();
      delete cookie;
      deleted++;
      _jarChanged = true;
    }
  }

  if (deleted > 0) return deleted;

  // No cookies were expired, so delete the least recently used
  time_t oldestDate = 0;

  // Find the oldest timestamp
  iter.setPos(SWIList::Iterator::BEG);
  while (iter.hasNext())
  {
    SBinetCookie *cookie = (SBinetCookie *) iter.next();

    if (oldestDate == 0 || cookie->getTimeStamp() < oldestDate)
      oldestDate = cookie->getTimeStamp();
  }

  // Now delete all 'old' cookies
  iter.setPos(SWIList::Iterator::BEG);
  while (iter.hasNext())
  {
    SBinetCookie *cookie = (SBinetCookie *) iter.next();
    if (cookie->getTimeStamp() == oldestDate)
    {
      iter.removeBack();
      delete cookie;
      deleted++;
      _jarChanged = true;
    }
  }

  return deleted;
      */
}


// Minimum from Cookie spec is 300
#define MAX_COOKIES 500

bool SBinetChannel::addCookie(SBinetCookie* cookie)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::addCookie("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };
  return true;

  /*
  if (cookie != NULL)
  {
    if (_cookieList.size() < MAX_COOKIES)
    {
      _cookieList.addLast(cookie);
      _jarChanged = true;
    }
    else
    {
      cleanCookies(); // Delete old cookies
      if (_cookieList.size() >= MAX_COOKIES)
      {
        Error(210, NULL);
        return false;
      }
      else
      {
        _cookieList.addLast(cookie);
        _jarChanged = true;
      }
    }
  }
  return true;
  */
}



void SBinetChannel::collectCookies(SWIoutputStream* output,
                                   const char *domain,
                                   const char *path)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::collectCookies("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };
  return;

  /*
  time_t now = time(NULL);

  SWIList::Iterator iter(_cookieList);

  while (iter.hasNext())
  {
    SBinetCookie *cookie = (SBinetCookie *) iter.next();

    time_t expires = cookie->getExpires();

    if ((expires == (time_t) 0 || expires >= now) &&
        cookie->matchRequest(domain, path, this))
    {
      writeString(output, "Cookie: ");

      writeString(output, "$Version=1; ");

      writeString(output, cookie->getName());
      writeString(output, "=");
      writeString(output, cookie->getValue());
      //writeString(output, "\"");

      const char *tmp = cookie->getPath();
      if (tmp && *tmp)
      {
        writeString(output, "; $Path=");
        writeString(output, tmp);
        //writeString(output, "\"");
      }

      tmp = cookie->getDomain();
      if (tmp && *tmp)
      {
        writeString(output, "; $Domain=");
        writeString(output, tmp);
        //writeString(output, "\"");
      }

      writeString(output, "\r\n");
      cookie->updateTimeStamp();
    }
  }
  */
}

/*
 * GetAbsoluteName
 */
typedef std::basic_string<VXIchar> vxistring;

static VXIinetResult getAbsUri
(	
  const VXIchar*   pszName,
  const VXIMap*    pProperties,
  vxistring & finalUri
)
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  const VXIValue * val = NULL; 
  
  if( pszName && !pProperties ) {
    // Just return the name since there is no additional
    // information to calculate absolute uri
    finalUri = pszName;
    return VXIinet_RESULT_SUCCESS;
  }
  else {
    if( pProperties ) 
    {
      val = VXIMapGetProperty(pProperties, INET_INFO_ABSOLUTE_NAME);
      // If the absolute URI does not exist, retrieve the base url
      if( !val && (!pszName || *pszName == L'\0') )
        return VXIinet_RESULT_FAILURE;
      if( !val )        
        val = VXIMapGetProperty(pProperties, INET_URL_BASE);
    }
  
    if( (!pszName || *pszName == L'\0') && val ) 
    {
      // The user wants to get the existing absolute URI
      // None of the information is available to get absolute url
      if( !val || VXIValueGetType(val) != VALUE_STRING)
        return VXIinet_RESULT_FAILURE;
      // We got one
      const VXIchar * uri = VXIStringCStr((const VXIString*)val);
      // Bad uri
      if( !uri || *uri == L'\0' ) 
        return VXIinet_RESULT_FAILURE;
              
      // remove query args if exists
      vxistring uristr = uri;
      vxistring::size_type pos = uristr.find(L"?");
      if( pos != vxistring::npos )
        uristr.erase(pos);  
      finalUri = uristr;
      return VXIinet_RESULT_SUCCESS;
    }
  }  
  
  // Otherwise retrieve base url to combine with relative url
  const VXIchar* baseUri = NULL;
  SBinetURL * url = NULL;
  if( val && VXIValueGetType(val) == VALUE_STRING )
    baseUri = VXIStringCStr((const VXIString*)val);
    
  rc = SBinetURL::create(pszName, baseUri, url);  
  if( rc == VXIinet_RESULT_SUCCESS )
    finalUri = url->getAbsolute();
  if( url ) delete url;    
  return rc;  
}

VXIinetResult SBinetChannel::getAbsoluteName
(
  const VXIchar*   pszModuleName,
	const VXIchar*   pszName,
  const VXIMap*    pProperties,
  VXIString**      absoluteName
)
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  
  if(!absoluteName || ((!pszName || *pszName == L'\0') && !pProperties))
  {
    Error(200, L"%s%s%s%s", L"Operation", L"Open",
          L"URL", (pszName != NULL) ? pszName : L"NULL");
    return(VXIinet_RESULT_INVALID_ARGUMENT);
  }

  Diag (MODULE_SBINET_CHANNEL_TAGID, L"SBinetChannel::GetAbsoluteName",
	L"(%s)", pszName != NULL ? pszName : L"NULL");

  vxistring finalUrl;
  *absoluteName = NULL;
  
  rc = getAbsUri(pszName, pProperties, finalUrl);
  if( rc == VXIinet_RESULT_SUCCESS ) {
    // Remove file:// part if it is local file
    vxistring::size_type pos = finalUrl.find(L"file://");
    if( pos != vxistring::npos )
      finalUrl.erase(0, strlen("file://"));  
    *absoluteName = VXIStringCreate(finalUrl.c_str());         
  }
  return rc;
}


SWIstream::Result SBinetChannel::writeString(SWIoutputStream* output, const char *data)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::writeString("
		<< data
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  SWIstream::Result rc = output->printString(data);
  if (rc == SWIstream::CONNECTION_ABORTED)
    throw std::runtime_error("CONNECTION_ABORTED");
  else if (rc == SWIstream::WRITE_ERROR)
    throw std::runtime_error("SOCKET_ERROR");

  if (_echoStream)
  {
    echoStreamWrite(data, strlen(data));
  }

  return rc;
}

SWIstream::Result SBinetChannel::writeInt(SWIoutputStream* output, int value)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::writeInt("
		<< value
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  SWIstream::Result rc = output->printInt(value);
  if (rc == SWIstream::CONNECTION_ABORTED)
    throw std::runtime_error("CONNECTION_ABORTED");
  else if (rc == SWIstream::WRITE_ERROR)
    throw std::runtime_error("SOCKET_ERROR");
    
  if (_echoStream)
  {
    char valStr[20];
    sprintf(valStr, "%d", value);
    echoStreamWrite(valStr, strlen(valStr));
  }

  return rc;
}

int SBinetChannel::writeData(SWIoutputStream* output,
                             const void *data, int dataSize)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::writeData("
		<< dataSize
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  int rc = output->writeBytes(data, dataSize);
  if (rc == SWIstream::CONNECTION_ABORTED)
    throw std::runtime_error("CONNECTION_ABORTED");
  else if (rc == SWIstream::WRITE_ERROR)
    throw std::runtime_error("SOCKET_ERROR");

  if (_echoStream)
  {
    echoStreamWrite(data, rc);
  }

  return rc;
}

// Strictly for writing debug info to the log
void SBinetChannel::writeDebugString(const char *data)
{
  if (_echoStream)
  {
    echoStreamWrite(data, strlen(data));
  }
}

int SBinetChannel::readChar(SWIinputStream* input)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::readChar("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  int k = input->read();
  if (k >= 0)
  {
    char c = (char) k;
    if (_echoStream) echoStreamWrite(&c, 1);
  }

  return k;
}

SWIstream::Result SBinetChannel::readLine(SWIinputStream* input, SWIdataOutputStream* line)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::readLine("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  line->reset();
  int rc = input->readLine(*line);
  if (rc < 0) return SWIstream::Result(rc);

  if (_echoStream)
  {
    echoStreamWrite(line->getString(), line->getSize());
    echoStreamWrite("\r\n", 2);
  }

  return SWIstream::SUCCESS;
}

int SBinetChannel::readData(SWIinputStream* input, void *buffer, int toRead)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::readData("
		<< toRead
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  //  Log to voiceglue
    /*
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::readData("
		<< toRead
		<< ") started";
      voiceglue_log ((char) LOG_INFO, logstring);
  };
    */

  int rc = input->readBytes(buffer, toRead);
  if (rc >= 0 && _echoStream)
    echoStreamWrite(buffer, rc);

  //  Log to voiceglue
  /*
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::readData("
		<< toRead
		<< ") finished";
      voiceglue_log ((char) LOG_INFO, logstring);
  };
  */

  return rc;
}

/*
 * Call Channel method
 */

VXIint32 SBinetChannel::GetVersion()
{
  return VXI_CURRENT_VERSION;
}


const VXIchar* SBinetChannel::GetImplementationName()
{
  return SBINET_IMPLEMENTATION_NAME;
}


VXIinetResult
SBinetChannel::staticPrefetch(/* [IN]  */ VXIinetInterface*      pThis,
                              /* [IN]  */ const VXIchar*   pszModuleName,
                              /* [IN]  */ const VXIchar*   pszName,
                              /* [IN]  */ VXIinetOpenMode  eMode,
                              /* [IN]  */ VXIint32         nFlags,
                              /* [IN]  */ const VXIMap*    pProperties  )
{
  SBinetChannel* chan = static_cast<SBinetChannel *>(pThis);
  if (!chan)
  {
    return (VXIinet_RESULT_INVALID_ARGUMENT);
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (chan->GetLog(), chan->GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetChannel::Prefetch", (int *) &rc,
			  L"entering: 0x%p, %s, %s, 0x%x, 0x%x, 0x%p",
			  pThis, pszModuleName, pszName, eMode, nFlags,
			  pProperties);

  rc = chan->prefetch(pszModuleName, pszName, eMode, nFlags, pProperties);
  if (rc != VXIinet_RESULT_SUCCESS)
  {
    chan->Error(203,L"%s%d",L"rc",rc);
  }
  return (rc);
}

/*
 * Call Channel method
 */
VXIinetResult
SBinetChannel::staticOpen(/* [IN]  */ VXIinetInterface*      pThis,
                          /* [IN]  */ const VXIchar*   pszModuleName,
                          /* [IN]  */ const  VXIchar*  pszName,
                          /* [IN]  */ VXIinetOpenMode  eMode,
                          /* [IN]  */ VXIint32         nFlags,
                          /* [IN]  */ const VXIMap*    pProperties,
                          /* [OUT] */ VXIMap*          pmapStreamInfo,
                          /* [OUT] */ VXIinetStream**  ppStream     )
{
  SBinetChannel* chan = static_cast<SBinetChannel *>(pThis);

  if (!chan)
  {
    return (VXIinet_RESULT_INVALID_ARGUMENT);
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  SBinetLogFunc apiTrace (chan->GetLog(), chan->GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetChannel::Open", (int *) &rc,
			  L"entering: 0x%p, %s, %s, 0x%x, 0x%x, 0x%p, 0x%p, 0x%p",
			  pThis, pszModuleName, pszName, eMode, nFlags,
			  pProperties, pmapStreamInfo, ppStream);


  rc = chan->open(pszModuleName, pszName, eMode, nFlags,
                  pProperties, pmapStreamInfo, ppStream);

  return rc;
}

/*
 * We must call channel method so we can GC the stream
 */
VXIinetResult
SBinetChannel::staticClose(/* [IN]  */ VXIinetInterface* pThis,
                           /* [IN]  */ VXIinetStream** ppStream     )
{
  SBinetChannel* chan = static_cast<SBinetChannel*>(pThis);

  if (!chan)
  {
    return (VXIinet_RESULT_INVALID_ARGUMENT);
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (chan->GetLog(), chan->GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetChannel::Close", (int *) &rc,
			  L"entering: 0x%p, 0x%p (0x%p)", pThis, ppStream,
			  (ppStream ? *ppStream : NULL));

  rc = chan->close(ppStream);

  if (rc != VXIinet_RESULT_SUCCESS)
  {
    chan->Error(205,L"%s%d",L"rc",rc);
  }
  return (rc);
}

/*
 * Actually, we are simply going to go and call Stream routine directly here
 */
VXIinetResult
SBinetChannel::staticRead(/* [IN]  */ VXIinetInterface*      pThis,
                          /* [OUT] */ VXIbyte*         pBuffer,
                          /* [IN]  */ VXIulong         nBuflen,
                          /* [OUT] */ VXIulong*        pnRead,
                          /* [IN]  */ VXIinetStream*   pStream      )
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::staticRead("
		<< nBuflen
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  SBinetChannel* chan = static_cast<SBinetChannel*>(pThis);
  if (!chan || VXIlong(nBuflen) < 0 || !pBuffer)
  {
    return VXIinet_RESULT_INVALID_ARGUMENT;
  }

  if (chan->cachefile_fd == -1)
  {
      return VXIinet_RESULT_INVALID_ARGUMENT;
  };

  int read_byte_count = ::read (chan->cachefile_fd, pBuffer, nBuflen);
  if (read_byte_count == -1)
  {
      if (voiceglue_loglevel() >= LOG_ERR)
      {
	  std::ostringstream logstring;
	  logstring << "SBinetChannel::staticRead() failed to read file \""
		    << chan->cachefile
		    << "\" : errno=" << errno;
	  voiceglue_log ((char) LOG_ERR, logstring);
      };
  };

  *pnRead = read_byte_count;

  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::staticRead("
		<< nBuflen << ") returns " << *pnRead;
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  if (read_byte_count == 0)
  {
      return VXIinet_RESULT_END_OF_STREAM;
  };
  return VXIinet_RESULT_SUCCESS;

  /*
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (chan->GetLog(), chan->GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetChannel::Read", (int *) &rc,
			  L"entering: 0x%p, 0x%p, %lu, %lu, 0x%p",
			  pThis, pBuffer, nBuflen, pnRead, pStream);

  if (pStream == NULL)
  {
    chan->Error(200, L"%s%s", L"Operation", L"Read");
    return (rc = VXIinet_RESULT_INVALID_ARGUMENT);
  }

  //  Log to voiceglue
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::staticRead("
		<< Pointer_to_Std_String(pStream)
		<< ","
		<< nBuflen
		<< ") started";
      voiceglue_log ((char) LOG_INFO, logstring);
  };

  rc = (pStream->Read)(pBuffer,nBuflen, pnRead);

  switch (rc)
  {
   case VXIinet_RESULT_SUCCESS:
   case VXIinet_RESULT_WOULD_BLOCK:
   case VXIinet_RESULT_END_OF_STREAM:
     // no logging to perform.
     break;
   case VXIinet_RESULT_FETCH_TIMEOUT:
     chan->Error(228, NULL);
     // no break: intentional
   default:
     chan->Error(206, L"%s%d", L"rc", rc);
     break;
  }

  //  Log to voiceglue
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::staticRead("
		<< Pointer_to_Std_String(pStream)
		<< ","
		<< nBuflen
		<< ") finished";
      voiceglue_log ((char) LOG_INFO, logstring);
  };

  return (rc);
  */
}


VXIinetResult
SBinetChannel::staticWrite(/* [IN]  */ VXIinetInterface*      pThis,
                           /* [OUT] */ const VXIbyte*   pBuffer,
                           /* [IN]  */ VXIulong         nBuflen,
                           /* [OUT] */ VXIulong*        pnWritten,
                           /* [IN]  */ VXIinetStream*   pStream      )
{
  SBinetChannel* chan = static_cast<SBinetChannel*>(pThis);
  if (!chan || VXIlong(nBuflen) < 0 || !pBuffer)
  {
    return (VXIinet_RESULT_INVALID_ARGUMENT);
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  SBinetLogFunc apiTrace(chan->GetLog(),
                         chan->GetDiagBase() + MODULE_SBINET_API_TAGID,
                         L"SBinetChannel::Write", (int *) &rc,
                         L"entering: 0x%p, 0x%p, %lu, 0x%p, 0x%p",
                         pThis, pBuffer, nBuflen, pnWritten, pStream);

  if (pStream == NULL)
  {
    chan->Error(200, L"%s%s", L"Operation", L"Write");
    return (rc = VXIinet_RESULT_INVALID_ARGUMENT);
  }

  rc = (pStream->Write)(pBuffer, nBuflen, pnWritten);

  if ((rc != VXIinet_RESULT_SUCCESS) &&
      (rc != VXIinet_RESULT_WOULD_BLOCK))
  {
    chan->Error(207,L"%s%d",L"rc",rc);
  }

  return rc;
}

/*
 * Call Channel method
 */
VXIinetResult
SBinetChannel::staticSetCookieJar( /* [IN]  */ VXIinetInterface*      pThis,
                                   /* [IN]  */ const VXIVector*       pJar )
{
  SBinetChannel* chan = static_cast<SBinetChannel*>(pThis);
  if (!chan)
  {
    return (VXIinet_RESULT_INVALID_ARGUMENT);
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (chan->GetLog(), chan->GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetChannel::SetCookieJar", (int *) &rc,
			  L"entering: 0x%p, 0x%p", pThis, pJar);

  rc = chan->setCookieJar(pJar);
  if (rc != VXIinet_RESULT_SUCCESS) {
    chan->Error(208,L"%s%d",L"rc",rc);
  }
  return (rc);
}

/*
 * Call Channel method
 */
VXIinetResult
SBinetChannel::staticGetCookieJar( /* [IN]  */ VXIinetInterface*      pThis,
                                   /* [OUT] */ VXIVector**            ppJar,
                                   /* [OUT] */ VXIbool*               pfChanged   )
{
  SBinetChannel* chan = static_cast<SBinetChannel*>(pThis);
  if (!chan)
  {
    return (VXIinet_RESULT_INVALID_ARGUMENT);
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (chan->GetLog(), chan->GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetChannel::GetCookieJar", (int *) &rc,
			  L"entering: 0x%p, 0x%p, 0x%p", pThis, ppJar,
			  pfChanged);

  rc = chan->getCookieJar(ppJar,pfChanged);
  if (rc != VXIinet_RESULT_SUCCESS)
  {
    chan->Error(209, L"%s%d", L"rc", rc);
  }
  return (rc);
}

/*
 * Call Channel method
 */

VXIinetResult SBinetChannel::staticGetAbsoluteName
(
  /* [IN]  */ VXIinetInterface* pThis,
  /* [IN]  */ const VXIchar *pszModuleName,
  /* [IN]  */ const VXIchar *pszName,
  /* [IN]  */ const VXIMap  *properties,
  /* [OUT] */ VXIString     **absoluteName
)  				
{
  SBinetChannel* chan = static_cast<SBinetChannel *>(pThis);

  if (!chan)
  {
    return (VXIinet_RESULT_INVALID_ARGUMENT);
  }

  VXIinetResult rc = VXIinet_RESULT_SUCCESS;

  SBinetLogFunc apiTrace (chan->GetLog(), chan->GetDiagBase() +
			  MODULE_SBINET_API_TAGID,
			  L"SBinetChannel::GetAbsoluteName", (int *) &rc,
			  L"entering: 0x%p, %s, %s, 0x%x, 0x%x",
			  pThis, pszModuleName, pszName, properties, absoluteName);

  rc = chan->getAbsoluteName(pszModuleName, pszName, properties, absoluteName);
  if (rc != VXIinet_RESULT_SUCCESS)
  {
    chan->Error(220, L"%s%d", L"rc", rc);
  }
  return (rc);
}

// Init class at startup
VXIinetResult SBinetChannel::init(const VXIMap *configParams, const SWIutilLogger *logger)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::init("
		<< ") called with map: "
		<< VXIValue_to_Std_String((const VXIValue *) configParams);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  // User agent
  const VXIchar *userAgent = SBinetUtils::getString(configParams, SBINET_USER_AGENT_NAME);

  if (!userAgent || !*userAgent)
  {
    // Have to parse the user agent name
#ifdef OPENVXI
     userAgent = OSBINET_USER_AGENT_NAME_DEFAULT;
#else
     userAgent = SBINET_USER_AGENT_NAME_DEFAULT;
#endif
  }
  if (!_userAgent)
    _userAgent = new SBinetNString(userAgent);
  else
    _userAgent->operator=(userAgent);

  // Proxy rules.
  const VXIValue * tmp = VXIMapGetProperty(configParams, SBINET_PROXY_RULES);
  if (tmp != NULL && VXIValueGetType(tmp) == VALUE_VECTOR)
  {
    const VXIVector *proxyRules = (const VXIVector *) tmp;
    VXIunsigned n = VXIVectorLength(proxyRules);
    for (VXIunsigned i = 0; i < n; i++)
    {
      tmp = VXIVectorGetElement(proxyRules, i);
      if (tmp != NULL && VXIValueGetType(tmp) == VALUE_STRING)
      {
        SBinetNString rule = VXIStringCStr((const VXIString *) tmp);
        SBinetProxyMatcher *p = SBinetProxyMatcher::createMatcher(rule.c_str(), logger);
        if (p != NULL)
        {
          _proxyMatcherList.addLast(p);
        }
      }
    }
  }
  
  // Global Map Mutex
  if( !_globalMapMutex ) VXItrdMutexCreate(&_globalMapMutex);

  // Create global MD5 mutex
  if( !_globalMD5Mutex ) VXItrdMutexCreate(&_globalMD5Mutex);

  // Extension rules
  if (! _extensionRulesMutex)
    VXItrdMutexCreate(&_extensionRulesMutex);

  if (VXItrdMutexLock(_extensionRulesMutex) == VXItrd_RESULT_SUCCESS)
  {
    if (_extensionRules)
      VXIMapDestroy(&_extensionRules);

    const VXIValue *tmp = VXIMapGetProperty(configParams, SBINET_EXTENSION_RULES);
    const VXIMap *extensionRules = NULL;
    if (tmp != NULL && VXIValueGetType(tmp) == VALUE_MAP)
      extensionRules = (const VXIMap *) tmp;

    if (extensionRules)
      _extensionRules = VXIMapClone(extensionRules);
    else
      _extensionRules = NULL;

    VXItrdMutexUnlock(_extensionRulesMutex);
  }

  VXIint32 itmp;

  // page load time out.
  itmp = SBINET_PAGE_LOADING_TIMEOUT_DEFAULT;
  SBinetUtils::getInteger(configParams, SBINET_PAGE_LOADING_TIMEOUT, itmp);
  setPageLoadTimeout(itmp);

  // freshness fraction
  VXIflt32 ftmp = (VXIflt32) SBINET_FRESHNESS_FRACTION_DEFAULT;
  SBinetUtils::getFloat(configParams, SBINET_FRESHNESS_FRACTION, ftmp);
  setFreshnessFraction(ftmp);

  // freshness lifetime
  itmp = SBINET_FRESHNESS_LIFETIME_DEFAULT;
  SBinetUtils::getInteger(configParams, SBINET_FRESHNESS_LIFETIME, itmp);
  setFreshnessLifetime((time_t) itmp);

  // max lifetime
  itmp = SBINET_MAX_LIFETIME_DEFAULT;
  SBinetUtils::getInteger(configParams, SBINET_MAX_LIFETIME, itmp);
  setMaxLifetime((time_t) itmp);

  // post continue timeout.
  itmp = SBINET_POST_CONTINUE_TIMEOUT_DEFAULT;
  SBinetUtils::getInteger(configParams, SBINET_POST_CONTINUE_TIMEOUT, itmp);
  setPostContinueTimeout(itmp);

  const VXIchar* defaultMimeType = SBinetUtils::getString(configParams, SBINET_DEFAULT_MIME_TYPE);
  if (!defaultMimeType)
    defaultMimeType = L"application/octet-stream";

  if (!_defaultMimeType)
    _defaultMimeType = new SBinetString(defaultMimeType);
  else
    _defaultMimeType->operator=(defaultMimeType);

  itmp = 1;
  SBinetUtils::getInteger(configParams, SBINET_PERSISTENT_CONNECTIONS, itmp);
  setUsePersistentConnections(itmp != 0);

  if (SBinetSSLsocket::initialize() == 0)
    return VXIinet_RESULT_SUCCESS;
  else
    return VXIinet_RESULT_PLATFORM_ERROR;
}


// Shutdown class
void SBinetChannel::shutdown()
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::shutdown("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  delete _userAgent;
  _userAgent = NULL;

  if (VXItrdMutexLock(_extensionRulesMutex) == VXItrd_RESULT_SUCCESS)
  {
    if (_extensionRules)
      VXIMapDestroy(&_extensionRules);

    VXItrdMutexUnlock(_extensionRulesMutex);
    VXItrdMutexDestroy(&_extensionRulesMutex);
  }

  // Global Map Mutex
  if( VXItrdMutexLock(_globalMapMutex ) == VXItrd_RESULT_SUCCESS ) 
  {
    // Destroy any left-over lock
    if( !_globalLockMap.empty() ) {
      for( SBINETLOCKMAP::iterator i = _globalLockMap.begin();
           i != _globalLockMap.end(); ++i)
      {
        delete ((*i).second);
      }
      _globalLockMap.clear();
    }
    VXItrdMutexUnlock(_globalMapMutex);
    VXItrdMutexDestroy(&_globalMapMutex);
  }

  // Destroy global MD5 mutex
  if( _globalMD5Mutex ) VXItrdMutexDestroy(&_globalMD5Mutex);
  _globalMD5Mutex = NULL;
  
  SBinetSSLsocket::shutdown();
}

bool SBinetChannel::getCacheExistence(const SBinetMD5 & md5k)
{
    return false;

    /*
  bool rc = false;
  // Gain access to global mutex
  if( VXItrdMutexLock(_globalMD5Mutex) == VXItrd_RESULT_SUCCESS ) {
    // Search for this md5
    SBINETMD5CACHEMAP::iterator i = _globalMD5CacheMap.find(md5k);
    // If NOT found, the cache is existed
    // Otherwise the cache is not existed yet
    if( i == _globalMD5CacheMap.end() )
      rc = true;
    
    VXItrdMutexUnlock(_globalMD5Mutex);
    // Release global mutex  
  }
  return rc;
    */
}

bool SBinetChannel::setCacheExistence(const SBinetMD5 & md5k, 
                                      SBinetHttpCacheStream* p, bool done)
{
    return true;

    /*
  bool rc = false;
  // Gain access to global mutex
  if( VXItrdMutexLock(_globalMD5Mutex) == VXItrd_RESULT_SUCCESS ) {
    // The first thread/channel that is going to write the cache
    // will get hold to this mutex first.  An md5 will be created
    // for the uri to indicate that its
    // cache is going to be avail. once this thread/channel fetch successfully
    // Search for this md5
    SBINETMD5CACHEMAP::iterator i = _globalMD5CacheMap.find(md5k);
    
    if( i == _globalMD5CacheMap.end() ) {
      // If NOT found, and done == false, meaning this channe/thread
      // start to write its cache, make it the first to write cache
      if( !done ) {
        _globalMD5CacheMap[md5k] = p;
        rc = true;
      }
      else {
        // This should not happen!!! maybe the function is called incorrectly
        rc = false;  
      }
    }
    else {
      // If FOUND, and done == true, this thread/channel finishes writing
      // cache succesfully.  Remove the previously stored md5 to let other
      // thread/channel to read directly from its cache
      if( done && (*i).second == p ) {
        _globalMD5CacheMap.erase(i);
        rc = true;
      }
      else {
        // Should never happen!!! maybe race-condition, ignore!
        rc = false;    
      }        
    }
    
    VXItrdMutexUnlock(_globalMD5Mutex);
    // Release global mutex  
  }  
  return rc;  
    */
}

//VXItrdMutexRef* SBinetChannel::acquireEntryLock(const SBinetMD5 & md5k)
VXItrdMutexRef* SBinetChannel::acquireEntryLock(const VXIchar* absurl, 
                                                SWIutilLogger *logger)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::acquireEntryLock("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  const VXIchar* fnname = L"SBinetChannel::acquireEntryLock";
  VXItrdMutexRef* e_lock = NULL;
  if( !logger || !absurl || *absurl == L'\0' ) return NULL;
  
  // Generate MD5 key for this absolute url
  SBinetMD5 md5k(absurl);

  logger->Diag(MODULE_SBINET_CHANNEL_TAGID, fnname, L"%s%s", 
               L"Acquiring Lock: ", absurl);
  
  // Lock the global map
  VXItrdMutexLock(_globalMapMutex);
  // Search the global map for the lock of the given URI
  SBINETLOCKMAP::iterator i = _globalLockMap.find(md5k);
  if( i != _globalLockMap.end() )
  {
    e_lock = (*i).second;  
    ++e_lock->_refCount;
  }
  else {
    // If the lock does not exist, create one
    e_lock = new VXItrdMutexRef();
    if( e_lock ) {
      e_lock->_md5Key = md5k; // make a copy of this md5
      _globalLockMap[md5k] = e_lock;    
    }
  }

  // Unlock 
  VXItrdMutexUnlock(_globalMapMutex);

  // If the lock is found, Lock the entry now!
  if( e_lock ) VXItrdMutexLock(e_lock->_refMutex);
  logger->Diag(MODULE_SBINET_CHANNEL_TAGID, fnname, L"%s%s%s%p%s%d", 
               L"Got Lock: ", absurl, L", ", e_lock, L", ref: ", 
               e_lock->_refCount);
  return e_lock;
}

void SBinetChannel::releaseEntryLock(VXItrdMutexRef* e_lock,
                                     SWIutilLogger *logger)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::releaseEntryLock("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  const VXIchar* fnname = L"SBinetChannel::releaseEntryLock";
  // Unlock the URL entry to allow other channel to gain access  
  if( e_lock ) {
    logger->Diag(MODULE_SBINET_CHANNEL_TAGID, fnname, L"%s%p%s%d", 
               L"Releasing Lock: ", e_lock, L", ref: ", e_lock->_refCount);              
    VXItrdMutexUnlock(e_lock->_refMutex);
    logger->Diag(MODULE_SBINET_CHANNEL_TAGID, fnname, L"%s%p", 
               L"Lock Released: ", e_lock);              
    
    // Lock the global map and remove the mutex ref
    VXItrdMutexLock(_globalMapMutex);
    if( --e_lock->_refCount == 0 ) {
      SBINETLOCKMAP::iterator i = _globalLockMap.find(e_lock->_md5Key);
      if( i != _globalLockMap.end() ) {
        _globalLockMap.erase(i);
      }
      delete e_lock;
    }
    //Unlock
    VXItrdMutexUnlock(_globalMapMutex);
  }
}

// --------------------------------------------------

void SBinetChannel::addExtensionRule(const VXIchar* ext,
				     const VXIchar* mimeType)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::addExtensionRule("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  if (! mimeType) return;

  if (VXItrdMutexLock(_extensionRulesMutex) == VXItrd_RESULT_SUCCESS) {
    if (! _extensionRules)
      _extensionRules = VXIMapCreate();

    if (_extensionRules)
      VXIMapSetProperty(_extensionRules, ext,
			(VXIValue *) VXIStringCreate(mimeType));
    VXItrdMutexUnlock(_extensionRulesMutex);
  }
}


const VXIchar *SBinetChannel::mapExtension(const VXIchar* ext)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::mapExtension("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  if (ext == NULL) return NULL;

  const VXIchar *ret = NULL;
  if (VXItrdMutexLock(_extensionRulesMutex) == VXItrd_RESULT_SUCCESS) {
    if (_extensionRules)
      ret = SBinetUtils::getString(_extensionRules, ext);
    VXItrdMutexUnlock(_extensionRulesMutex);
  }

  return ret;
}

const char *SBinetChannel::getUserAgent()
{
  return _userAgent ? _userAgent->c_str() : "";
}

const VXIchar *SBinetChannel::getDefaultMimeType()
{
  return _defaultMimeType ? _defaultMimeType->c_str() : L"";
}

VXIlogResult SBinetChannel::echoStreamWrite(const void *buffer, size_t buflen)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;

  VXIulong totWritten = 0;
  while ((rc == VXIlog_RESULT_SUCCESS) && (totWritten < buflen)) {
    VXIulong nWritten = 0;
    rc = _pVXILog->ContentWrite(_pVXILog,
                                reinterpret_cast<const VXIbyte *>(buffer) + totWritten,
                                buflen - totWritten, &nWritten, _echoStream);
    totWritten += nWritten;
  }

  return rc;
}

// void SBinetChannel::eraseCookie(SBinetChannel::CookieList::iterator &vi)
// {
// #if defined(__GNUC__) && ((__GNUC__ <= 2) || (__GLIBCPP__ <= 20011023)) || (_decunix_)
//   // Old STL has erase return void
//   CookieList::iterator vi2 = vi;
//   vi2++;
//   _cookieList.erase (vi);
//   vi = vi2;
// #else
//   // ISO C++ standard has erase return an iterator to the next
//   // item
//   vi = _cookieList.erase (vi);
// #endif
// }

void SBinetChannel::setUsePersistentConnections(bool flag)
{
  _usePersistentConnections = flag;
}

bool SBinetChannel::getUsePersistentConnections()
{
  return _usePersistentConnections;
}

bool SBinetChannel::setPageLoadTimeout(VXIint32 timeout)
{
  if (timeout <= 0)
    return false;

  _pageLoadTimeout = timeout;
  return true;
}


VXIint32 SBinetChannel::getPageLoadTimeout()
{
  return _pageLoadTimeout;
}

bool SBinetChannel::setPostContinueTimeout(VXIint32 timeout)
{
  if (timeout <= 0)
    return false;

  _postContinueTimeout = timeout;
  return true;
}


VXIint32 SBinetChannel::getPostContinueTimeout()
{
  return _postContinueTimeout;
}

time_t SBinetChannel::getFreshnessLifetime()
{
  return _freshnessLifetime;
}

bool SBinetChannel::setFreshnessLifetime(time_t freshnessLifetime)
{
  if (freshnessLifetime >= (time_t) 0)
  {
    _freshnessLifetime = freshnessLifetime;
    return true;
  }
  return false;
}

double SBinetChannel::getFreshnessFraction()
{
  return _freshnessFraction;
}

bool SBinetChannel::setFreshnessFraction(double freshnessFraction)
{
  if (freshnessFraction >= 0.0 && freshnessFraction <= 1.0)
  {
    _freshnessFraction = freshnessFraction;
    return true;
  }
  return false;
}

time_t SBinetChannel::getMaxLifetime()
{
  return _maxLifetime;
}

bool SBinetChannel::setMaxLifetime(time_t maxLifetime)
{
  if (maxLifetime >= (time_t) 0)
  {
    _maxLifetime = maxLifetime;
    return true;
  }
  return false;
}

SBinetProxyMatcher *SBinetChannel::getProxyMatcher(const char *domain,
                                                   const char *path)
{
  SWIList::Iterator iter(_proxyMatcherList);

  while (iter.hasNext())
  {
    SBinetProxyMatcher *p = static_cast<SBinetProxyMatcher *>(iter.next());
    if (p->matches(domain, path))
    {
      return p;
    }
  }

  return NULL;
}

SBinetHttpConnection *SBinetChannel::getHttpConnection(const SBinetURL *url,
                                                       const VXIMap *properties)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::getHttpConnection("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  VXIint32 newConnection = !getUsePersistentConnections();

  const char *hostname = url->getNHost();
  const char *path = url->getNPath();

  int port = url->getPort();

  SBinetUtils::getInteger(properties, INET_NEW_CONNECTION, newConnection);

  const SBinetProxyMatcher *proxyMatcher = getProxyMatcher(hostname, path);

  const char *proxyServer = NULL;
  if (proxyMatcher != NULL)
    proxyServer = proxyMatcher->getProxyServer();

  // Ignore empty proxy server.
  if (proxyServer && !*proxyServer)
    proxyServer = NULL;

  bool usesProxy;
  if (proxyServer)
  {
    hostname = proxyServer;
    port = proxyMatcher->getProxyPort();
    usesProxy = true;
  }
  else
    usesProxy = false;

  SWIipAddress remoteAddress(hostname, port, this);

  // FIXME: This code should be protected by a Mutex once we have the cleaning
  // thread
  SBinetHttpConnection *conn = (SBinetHttpConnection *) _connectionMap.remove(remoteAddress);

  if (newConnection && conn)
  {
    delete conn;
    conn = NULL;
  }
  // FIXME: End of code section that should be protected.

  if (!conn)
  {
    char buf[10];
    sprintf(buf, "%d", _connectionCount++);
    conn = new SBinetHttpConnection(url->getProtocol(), remoteAddress,
                                    usesProxy, this, buf);
  }

  return conn;
}

void SBinetChannel::putHttpConnection(SBinetHttpConnection *connection)
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::putHttpConnection("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  // Ideally, we should check whether we own the connection.

  // FIXME: This code should be protected by a Mutex once we have the cleaning  thread
  const SWIipAddress *addr = connection->getRemoteAddress();

  SBinetHttpConnection *oldconn = (SBinetHttpConnection *)
    _connectionMap.putValue(*addr, connection);
  // FIXME: End of code section that should be protected.

  delete oldconn;
}

void SBinetChannel::closeHttpConnections()
{
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "SBinetChannel::closeHttpConnections("
		<< ") called";
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  // Iterate saved connections and dispose of them all
  SWIHashMap::Iterator it(_connectionMap);
  while (it.hasNext()) {
    const SBinetHttpConnection *conn = static_cast<const SBinetHttpConnection*>(it.next()->getValue());
    delete conn;
  }
}
