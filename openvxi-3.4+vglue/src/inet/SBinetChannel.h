#ifndef SBINETCHANNEL_H                   /* Allows multiple inclusions */
#define SBINETCHANNEL_H

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

#include "VXItypes.h"
#include "VXIvalue.h"
#include "VXIinet.h"
#include "VXItrd.h"
#include "VXIcache.h"
#include "SBinetLog.h"
#include <SWIutilLogger.hpp>
#include "SBinetString.hpp"
#include <SWIHashMap.hpp>
#include <SWIList.hpp>

#include <SWIstream.hpp>

#include <string>

class SBinetCookie;
class SBinetURL;
class SBinetProxyMatcher;
class SWIoutputStream;
class SWIinputStream;
class SWIdataOutputStream;
class SBinetHttpConnection;
class SBinetHttpCacheStream;
class SBinetMD5;
class VXItrdMutexRef;

class SBinetChannel : public VXIinetInterface, public SWIutilLogger
{
  public:
  SBinetChannel(VXIlogInterface* log,
                VXIunsigned diagLogBase,
                VXIcacheInterface* pVXIcache);
  ~SBinetChannel();

  // Adds a cookie without verifying whether the cookie already exists.
  bool addCookie(SBinetCookie* cookie);

  // If a corresponding cookie already exists. Replaces it, otherwise adds it.
  bool updateCookie(SBinetCookie* cookie);

  VXIint cleanCookies();
  void deleteAllCookies();

  void collectCookies(SWIoutputStream* output,
                      const char *domain,
                      const char *path);

  bool updateCookieIfExists( const char*   pszDomain,
                             const char*   pszPath,
                             const char*   pszName,
                             const char*   pszValue,
                             time_t  nExpires,
                             bool fSecure );

  bool cookiesEnabled() { return _cookiesEnabled; }

  SBinetHttpConnection *getHttpConnection(const SBinetURL *url,
                                          const VXIMap *properties);

  void putHttpConnection(SBinetHttpConnection *connection);

  void closeHttpConnections();

 public:

  // Initialize the class at startup, shutdown later
  static VXIinetResult init(const VXIMap *configParams, const SWIutilLogger *logger);

  static void shutdown();

  static const VXIchar* mapExtension(const VXIchar* ext);
  static void addExtensionRule(const VXIchar* ext, const VXIchar* mimeType);

  static VXIint32 getPostContinueTimeout();
  static bool setPostContinueTimeout(VXIint32 timeout);

  static VXIint32 getPageLoadTimeout();
  static bool setPageLoadTimeout(VXIint32 timeout);

  static time_t getFreshnessLifetime();
  static bool setFreshnessLifetime(time_t freshnessLifetime);

  static double getFreshnessFraction();
  static bool setFreshnessFraction(double freshnessFraction);

  static time_t getMaxLifetime();
  static bool setMaxLifetime(time_t maxLifetime);

  static bool getUsePersistentConnections();
  static void setUsePersistentConnections(bool f);

  VXIinetResult closeAllStreams();

  VXIinetResult prefetch(/* [IN]  */ const VXIchar*   pszModuleName,
                         /* [IN]  */ const VXIchar*   pszName,
			 /* [IN]  */ VXIinetOpenMode  eMode,
			 /* [IN]  */ VXIint32         nFlags,
			 /* [IN]  */ const VXIMap*    pProperties  );

  VXIinetResult open(/* [IN]  */ const VXIchar*   pszModuleName,
                     /* [IN]  */ const  VXIchar*  pszName,
                     /* [IN]  */ VXIinetOpenMode  eMode,
                     /* [IN]  */ VXIint32         nFlags,
                     /* [IN]  */ const VXIMap*    pProperties,
                     /* [OUT] */ VXIMap*          pmapStreamInfo,
                     /* [OUT] */ VXIinetStream**  ppStream     );

  VXIinetResult close(/* [IN]  */ VXIinetStream**  ppStream     );

  VXIinetResult read(/* [OUT] */ VXIbyte*         pBuffer,
                     /* [IN]  */ VXIulong         nBuflen,
                     /* [OUT] */ VXIulong*        pnRead,
                     /* [IN]  */ VXIinetStream*   pStream      );

  VXIinetResult write(/* [OUT] */ const VXIbyte*   pBuffer,
                      /* [IN]  */ VXIulong         nBuflen,
                      /* [OUT] */ VXIulong*        pnWritten,
                      /* [IN]  */ VXIinetStream*   pStream      );

  VXIinetResult setCookieJar( /* [IN]  */ const VXIVector*    pJar );

  VXIinetResult getCookieJar( /* [OUT] */ VXIVector**      ppJar,
                              /* [OUT] */ VXIbool*         pfChanged    );

  VXIinetResult getAbsoluteName(/* [IN]  */ const VXIchar *moduleName,
                                /* [IN]  */ const VXIchar *name,
                                /* [IN]  */ const VXIMap  *properties,
                                /* [OUT] */ VXIString     **absoluteName);  				

  SWIstream::Result writeString(SWIoutputStream* output, const char *data);
  int writeData(SWIoutputStream* output, const void *data, int dataSize);
  SWIstream::Result writeInt(SWIoutputStream* output, int value);
  void writeDebugString(const char *data);

  int readData(SWIinputStream* input, void *buffer, int bufferSize);
  int readChar(SWIinputStream* input);
  SWIstream::Result readLine(SWIinputStream* input, SWIdataOutputStream* line);

  VXIcacheInterface *getCache()
  {
    return _pVXICache;
  }

  static const char *getUserAgent();
  static const VXIchar *getDefaultMimeType();

  static SBinetProxyMatcher *getProxyMatcher(const char *domain, const char *path);

 private:
  static VXIint32 GetVersion(void);

  static const VXIchar* GetImplementationName(void);

  static VXIinetResult staticPrefetch(/* [IN]  */ VXIinetInterface*      pThis,
                                      /* [IN]  */ const VXIchar*   pszModuleName,
                                      /* [IN]  */ const VXIchar*   pszName,
                                      /* [IN]  */ VXIinetOpenMode  eMode,
                                      /* [IN]  */ VXIint32         nFlags,
                                      /* [IN]  */ const VXIMap*    pProperties  );


  static VXIinetResult staticOpen(/* [IN]  */ VXIinetInterface*      pThis,
                                  /* [IN]  */ const VXIchar*   pszModuleName,
                                  /* [IN]  */ const  VXIchar*  pszName,
                                  /* [IN]  */ VXIinetOpenMode  eMode,
                                  /* [IN]  */ VXIint32         nFlags,
                                  /* [IN]  */ const VXIMap*    pProperties,
                                  /* [OUT] */ VXIMap*          pmapStreamInfo,
                                  /* [OUT] */ VXIinetStream**  ppStream     );

  static VXIinetResult staticClose(/* [IN]  */ VXIinetInterface*      pThis,
                                   /* [IN]  */ VXIinetStream**  ppStream     );

  static VXIinetResult staticRead(/* [IN]  */ VXIinetInterface*      pThis,
                                  /* [OUT] */ VXIbyte*         pBuffer,
                                  /* [IN]  */ VXIulong         nBuflen,
                                  /* [OUT] */ VXIulong*        pnRead,
                                  /* [IN]  */ VXIinetStream*   pStream      );

  static VXIinetResult staticWrite(/* [IN]  */ VXIinetInterface*      pThis,
                                   /* [IN]  */ const VXIbyte*   pBuffer,
                                   /* [IN]  */ VXIulong         nBuflen,
                                   /* [OUT] */ VXIulong*        pnWritten,
                                   /* [IN]  */ VXIinetStream*   pStream      );

  static VXIinetResult staticSetCookieJar(/* [IN] */ VXIinetInterface* pThis,
                                          /* [IN] */ const VXIVector* pJar );

  static VXIinetResult staticGetCookieJar(/* [IN]  */ VXIinetInterface* pThis,
                                          /* [OUT] */ VXIVector** ppJar,
                                          /* [OUT] */ VXIbool* pfChanged );

  static VXIinetResult staticGetAbsoluteName
  (
    /* [IN]  */ VXIinetInterface* pThis,
    /* [IN]  */ const VXIchar *moduleName,
    /* [IN]  */ const VXIchar *name,
    /* [IN]  */ const VXIMap  *properties,
    /* [OUT] */ VXIString     **absoluteName);  				

  VXIinetResult createStream(SBinetURL *url,
                             const VXIMap *properties,
                             VXIinetStream* &stream);

  VXIinetStream* createHttpStream(SBinetURL *url, const VXIMap *properties);

  VXIlogResult echoStreamWrite(const void *buffer, size_t buflen);

  //void eraseCookie(CookieList::iterator &vi);

 private:
  SWIList _cookieList;
  static SWIList _proxyMatcherList;
  bool _jarChanged; // For GetCookieJar()
  bool _cookiesEnabled; // Enable or diable cookie usage

  VXIlogInterface *_pVXILog;
  VXIcacheInterface *_pVXICache;

  VXIlogStream *_echoStream;

  SWIHashMap _connectionMap;
  int _connectionCount;

  std::string cachefile;
  int cachefile_fd;

 private:
  static double _freshnessFraction;
  static time_t _freshnessLifetime;
  static time_t _maxLifetime;
  static VXIint32 _pageLoadTimeout;
  static VXIint32 _postContinueTimeout;
  static VXItrdMutex *_extensionRulesMutex;
  static VXIMap *_extensionRules;
  static SBinetNString *_userAgent;
  static bool _usePersistentConnections;
  static SBinetString *_defaultMimeType;
  
 private:  
  static VXItrdMutex *_globalMD5Mutex;
  static VXItrdMutex *_globalMapMutex; 
    
 public:
  //static VXItrdMutexRef* acquireEntryLock(const SBinetMD5 & md5k);
  static VXItrdMutexRef* acquireEntryLock(const VXIchar* absurl, SWIutilLogger *logger);
  static void releaseEntryLock(VXItrdMutexRef* e_lock, SWIutilLogger *logger);
  
  static bool setCacheExistence(const SBinetMD5 & md5k, 
                                SBinetHttpCacheStream* p, bool done);
  static bool getCacheExistence(const SBinetMD5 & md5k);    
};

#endif
