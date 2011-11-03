
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

#if _MSC_VER >= 1100    // Visual C++ 5.x
#pragma warning( disable : 4786 4503 )
#endif

#include "VXIinet.h"
#include "SBinetHttpCacheStream.hpp"
#include "SBinetURL.h"
#include "SBinetValidator.h"
#include "SBinetChannel.h"
#include "SBinetUtils.hpp"
#include "SBinetString.hpp"
#include "HttpUtils.hpp"

#include <syslog.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>

VXIinetResult
SBinetHttpCacheStream::cache2inetRc(VXIcacheResult result)
{
  switch (result)
  {
   case VXIcache_RESULT_FATAL_ERROR:
     return VXIinet_RESULT_FATAL_ERROR;
   case VXIcache_RESULT_IO_ERROR:
     return VXIinet_RESULT_IO_ERROR;
   case VXIcache_RESULT_OUT_OF_MEMORY:
     return VXIinet_RESULT_OUT_OF_MEMORY;
   case VXIcache_RESULT_SYSTEM_ERROR:
     return VXIinet_RESULT_SYSTEM_ERROR;
   case VXIcache_RESULT_PLATFORM_ERROR:
     return VXIinet_RESULT_PLATFORM_ERROR;
   case VXIcache_RESULT_BUFFER_TOO_SMALL:
     return VXIinet_RESULT_BUFFER_TOO_SMALL;
   case VXIcache_RESULT_INVALID_PROP_NAME:
     return VXIinet_RESULT_INVALID_PROP_NAME;
   case VXIcache_RESULT_INVALID_PROP_VALUE:
     return VXIinet_RESULT_INVALID_PROP_VALUE;
   case VXIcache_RESULT_INVALID_ARGUMENT:
     return VXIinet_RESULT_INVALID_ARGUMENT;
   case VXIcache_RESULT_SUCCESS:
     return VXIinet_RESULT_SUCCESS;
   case VXIcache_RESULT_FAILURE:
     return VXIinet_RESULT_FAILURE;
   case VXIcache_RESULT_NON_FATAL_ERROR:
     return VXIinet_RESULT_NON_FATAL_ERROR;
   case VXIcache_RESULT_NOT_FOUND:
     return VXIinet_RESULT_NOT_FOUND;
   case VXIcache_RESULT_WOULD_BLOCK:
     return VXIinet_RESULT_WOULD_BLOCK;
   case VXIcache_RESULT_END_OF_STREAM:
     return VXIinet_RESULT_END_OF_STREAM;
   case VXIcache_RESULT_EXCEED_MAXSIZE:
     return VXIinet_RESULT_OUT_OF_MEMORY;
   case VXIcache_RESULT_ENTRY_LOCKED:
     return VXIinet_RESULT_FETCH_TIMEOUT;
   case VXIcache_RESULT_UNSUPPORTED:
     return VXIinet_RESULT_UNSUPPORTED;
   default: {
     Error(299, L"%s%i", L"VXIcacheResult", result);
     return VXIinet_RESULT_NON_FATAL_ERROR;
    }
  }
}

// Refer to SBinetHttpCacheStream.hpp for doc.
SBinetHttpCacheStream::
SBinetHttpCacheStream(SBinetURL* url,
                      SBinetHttpStream::SubmitMethod method,
                      SBinetChannel* ch,
                      VXIcacheInterface *cache,
                      VXIlogInterface *log,
                      VXIunsigned diagLogBase):
  SBinetStoppableStream(url, SBinetStream_CACHE, log, diagLogBase),
  _httpStream(NULL),
  _channel(ch),
  _cache(cache),
  _cacheStream(NULL),
  _method(method),
  _maxAge(-1),
  _maxStale(-1),
  _httpReadCompleted(false)
{}

// SBinetHttpCacheStream::~SBinetHttpCacheStream
// Refer to SBinetHttpCacheStream.hpp for doc.
SBinetHttpCacheStream::~SBinetHttpCacheStream()
{
  Close();
}

VXIinetResult SBinetHttpCacheStream::Open(VXIint flags,
                                          const VXIMap* properties,
                                          VXIMap* streamInfo)
{
  //  Log to voiceglue
    /*
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBinetHttpCacheStream::Open(): getCacheInfo()";
      voiceglue_log ((char) LOG_INFO, logstring);
  };
    */

  getCacheInfo(properties, _maxAge, _maxStale);

  SBinetValidator cachedValidator(GetLog(), GetDiagBase());
  bool cachedValidatorPresent = false;
  std::basic_string<VXIchar> oldCachedUri;
  
  //  Log to voiceglue
  /*
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBinetHttpCacheStream::Open(): readValidatorFromCache()";
      voiceglue_log ((char) LOG_INFO, logstring);
  };
  */

  // Read the cached validator for this resource.
  SBinetURL* ultimateURL = NULL;
  bool isCacheExpired = false;
  switch(readValidatorFromCache(ultimateURL, cachedValidator, isCacheExpired))
  {
    case VXIcache_RESULT_SUCCESS:
      // All is well, we successfully read the validator.
      cachedValidatorPresent = true;
      break;
    case VXIcache_RESULT_ENTRY_LOCKED:
      Error(234, L"%s%s", L"URL", _url->getAbsolute());
      delete ultimateURL;
      return VXIinet_RESULT_FETCH_TIMEOUT;
    default:
      cachedValidatorPresent = isCacheExpired;
      break;
  }

  //  Log to voiceglue
  /*
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBinetHttpCacheStream::Open(): externalValidator()";
      voiceglue_log ((char) LOG_INFO, logstring);
  };
  */

  // Determine if this is a conditional open with a validator,
  // irrelevant if caching is disabled
  bool externalValidatorPresent = false;
  const VXIValue* validatorVal =
    VXIMapGetProperty(properties, INET_OPEN_IF_MODIFIED);;
  SBinetValidator externalValidator(GetLog(), GetDiagBase());
  if (validatorVal != NULL)
  {
      //  Log to voiceglue
      /*
      if (voiceglue_loglevel() >= LOG_INFO)
      {
	  std::ostringstream logstring;
	  logstring << "SBinetHttpCacheStream::Open(): validatorVal != NULL";
	  voiceglue_log ((char) LOG_INFO, logstring);
      };
      */

    externalValidatorPresent = true;
    externalValidator.Create(validatorVal);

    if (!externalValidator.isExpired(_maxAge, _maxStale))
    {
      delete ultimateURL;

      //  Log to voiceglue
      /*
      if (voiceglue_loglevel() >= LOG_INFO)
      {
	  std::ostringstream logstring;
	  logstring << "SBinetHttpCacheStream::Open(): !externalValidator.isExpired()";
	  voiceglue_log ((char) LOG_INFO, logstring);
      };
      */

      // Update the external validator information with the latest cached
      // validator for this document.  If there is no cached validator,
      // just return the external validator that was passed in.
      if (streamInfo)
      {
	  //  Log to voiceglue
	  /*
	  if (voiceglue_loglevel() >= LOG_INFO)
	  {
	      std::ostringstream logstring;
	      logstring << "SBinetHttpCacheStream::Open(): setValidatorInfo()";
	      voiceglue_log ((char) LOG_INFO, logstring);
	  };
	  */

        SBinetValidator* pValidatorToReturn = &cachedValidator;
        if (!pValidatorToReturn)
          pValidatorToReturn = &externalValidator;
        if (!SBinetUtils::setValidatorInfo(streamInfo, pValidatorToReturn))
        {
          Error(103, NULL);
          return VXIinet_RESULT_OUT_OF_MEMORY;
        }
        // Set CACHE data source information   
        VXIMapSetProperty(streamInfo, INET_INFO_DATA_SOURCE, 
                         (VXIValue*) VXIIntegerCreate(INET_DATA_SOURCE_CACHE));
      }
      return VXIinet_RESULT_NOT_MODIFIED;
    }
  }

  // Check whether we can read this document from the cache.
  if (_maxAge != 0)
  {
      //  Log to voiceglue
      /*
      if (voiceglue_loglevel() >= LOG_INFO)
      {
	  std::ostringstream logstring;
	  logstring << "SBinetHttpCacheStream::Open(): _maxAge != 0";
	  voiceglue_log ((char) LOG_INFO, logstring);
      };
      */

    // Possible scenarios:
    // 1) There was no cached validator -> continue with http fetch
    // 2) Validator is expired -> continue with http fetch
    // 3) Error reading document from cache -> continue with http fetch
    // 4) Locked document cache entry -> return fetch timeout
    // 5) Success -> return not modified
    if (cachedValidatorPresent)
    {
      if (!cachedValidator.isExpired(_maxAge, _maxStale))
      {
	  //  Log to voiceglue
	  /*
	  if (voiceglue_loglevel() >= LOG_INFO)
	  {
	      std::ostringstream logstring;
	      logstring << "SBinetHttpCacheStream::Open(): openCacheRead()";
	      voiceglue_log ((char) LOG_INFO, logstring);
	  };
	  */

        // The cache is available for retreiving.
        switch (openCacheRead(ultimateURL->getAbsolute(), streamInfo))
        {
          case VXIcache_RESULT_SUCCESS:

	      //  Log to voiceglue
	      /*
	      if (voiceglue_loglevel() >= LOG_INFO)
	      {
		  std::ostringstream logstring;
		  logstring << "SBinetHttpCacheStream::Open(): VXIcache_RESULT_SUCCESS";
		  voiceglue_log ((char) LOG_INFO, logstring);
	      };
	      */

            delete ultimateURL;
            if( streamInfo ) {
              // Set CACHE data source information   
              VXIMapSetProperty(streamInfo, INET_INFO_DATA_SOURCE, 
                        (VXIValue*) VXIIntegerCreate(INET_DATA_SOURCE_CACHE));
              
            }
            // Remove the HTTP status property (there was no HTTP status for this fetch).
            VXIMapDeleteProperty(streamInfo, INET_INFO_HTTP_STATUS);

            // Return the cached validator to the user.
            SBinetUtils::setValidatorInfo(streamInfo, &cachedValidator);

            // If an external validator was passed in and it matches the
            // cached validator for this resource, return NOT_MODIFIED.
            // Otherwise, return SUCCESS.
            if (externalValidatorPresent && (externalValidator == cachedValidator))
            {
              Close();
              return VXIinet_RESULT_NOT_MODIFIED;
            }
            else
            {
              // Document cache entry is not closed.
              // Subsequent calls to Read() are possible.
              return VXIinet_RESULT_SUCCESS;
            }
          case VXIcache_RESULT_ENTRY_LOCKED:

	      //  Log to voiceglue
	      /*
	      if (voiceglue_loglevel() >= LOG_INFO)
	      {
		  std::ostringstream logstring;
		  logstring << "SBinetHttpCacheStream::Open(): VXIcache_RESULT_ENTRY_LOCKED";
		  voiceglue_log ((char) LOG_INFO, logstring);
	      };
	      */

            Error(234, L"%s%s", L"URL", _url->getAbsolute());
            delete ultimateURL;
            return VXIinet_RESULT_FETCH_TIMEOUT;
          default:

	      //  Log to voiceglue
	      /*
	      if (voiceglue_loglevel() >= LOG_INFO)
	      {
		  std::ostringstream logstring;
		  logstring << "SBinetHttpCacheStream::Open(): VXIcache_RESULT default";
		  voiceglue_log ((char) LOG_INFO, logstring);
	      };
	      */

            // Unable to read the document from the cache.  Perhaps the cache
            // entry no longer exists.  Continue with http fetch.

            // Disregard the cached validator.
            cachedValidator.Clear();
            break;
        }
      }
      else
      {
	  //  Log to voiceglue
	  /*
	  if (voiceglue_loglevel() >= LOG_INFO)
	  {
	      std::ostringstream logstring;
	      logstring << "SBinetHttpCacheStream::Open(): validator expired";
	      voiceglue_log ((char) LOG_INFO, logstring);
	  };
	  */

        Diag(MODULE_SBINET_CACHE_TAGID, L"SBinetHttpCacheStream::Open",
             L"%s%s", L"Validator expired: ", ultimateURL->getAbsolute());     
        oldCachedUri = ultimateURL->getAbsolute();
        isCacheExpired = true;
        // Continue on with the HTTP fetch.
      }
    }
  }

  delete ultimateURL;
  SBinetURL *url = new SBinetURL(*_url);

  if (!_cache && externalValidatorPresent)
  {
      //  Log to voiceglue
      /*
      if (voiceglue_loglevel() >= LOG_INFO)
      {
	  std::ostringstream logstring;
	  logstring << "SBinetHttpCacheStream::Open(): new SBinetHttpStream(externalValidator)";
	  voiceglue_log ((char) LOG_INFO, logstring);
      };
      */

    _httpStream =
      new SBinetHttpStream(url, _method, _channel, &externalValidator, GetLog(), GetDiagBase());
  }
  else
  {
      //  Log to voiceglue
      /*
      if (voiceglue_loglevel() >= LOG_INFO)
      {
	  std::ostringstream logstring;
	  logstring << "SBinetHttpCacheStream::Open(): new SBinetHttpStream(cachedValidator)";
	  voiceglue_log ((char) LOG_INFO, logstring);
      };
      */

    // If there is a cache, use the cached validator.  If there is no cached
    // validator, use the default cached validator which is empty.
    _httpStream =
      new SBinetHttpStream(url, _method, _channel, &cachedValidator, GetLog(), GetDiagBase());
  }

  _httpReadCompleted = false;

  SWITimeStamp timeout;
  getTimeOut(&timeout);
  _httpStream->setTimeOut(&timeout);

  //  Log to voiceglue
  /*
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBinetHttpCacheStream::Open(): _httpStream->Open()";
      voiceglue_log ((char) LOG_INFO, logstring);
  };
  */

  // Fetch the document using HTTP.
  VXIinetResult rc = _httpStream->Open(flags, properties, streamInfo);
  SBinetValidator *newValidator = (SBinetValidator*)_httpStream->getValidator();

  switch (rc)
  {
   case VXIinet_RESULT_SUCCESS: {

       //  Log to voiceglue
       /*
       if (voiceglue_loglevel() >= LOG_INFO)
       {
	   std::ostringstream logstring;
	   logstring << "SBinetHttpCacheStream::Open(): openCacheEntry()";
	   voiceglue_log ((char) LOG_INFO, logstring);
       };
       */

     VXIcacheResult crc;    
     if( isCacheExpired && !oldCachedUri.empty() ) { 
        // Remove the document cache entry associated with this validator.
        // if the cache expired and changed from the webserver
        VXIMap * dummy = VXIMapCreate();
        crc = openCacheEntry(oldCachedUri.c_str(), CACHE_MODE_WRITE, dummy);
        if (crc == VXIcache_RESULT_SUCCESS)
        {
          crc = _cache->CloseEx(_cache, FALSE, &_cacheStream);
          if (crc != VXIcache_RESULT_SUCCESS)
            Error(239, L"%s%s%s%i", L"URL", oldCachedUri.c_str(), L"rc", crc);

          _cacheStream = NULL;
        }
        if( dummy ) VXIMapDestroy(&dummy);
     }
      
     if (newValidator != NULL)
     {
       //  Log to voiceglue
	 /*
       if (voiceglue_loglevel() >= LOG_INFO)
       {
	   std::ostringstream logstring;
	   logstring << "SBinetHttpCacheStream::Open(): writeValidatorToCache()";
	   voiceglue_log ((char) LOG_INFO, logstring);
       };
	 */

       // Only write to cache if no other thread/channel is writting the
       // same cache
       crc = writeValidatorToCache(url, *newValidator);
       if (crc != VXIcache_RESULT_SUCCESS) {
         return cache2inetRc(crc);
       } 

       //  Log to voiceglue
       /*
       if (voiceglue_loglevel() >= LOG_INFO)
       {
	   std::ostringstream logstring;
	   logstring << "SBinetHttpCacheStream::Open(): openCacheWrite()";
	   voiceglue_log ((char) LOG_INFO, logstring);
       };
       */

       // Prepare the cache for writing (as the document is fetched during
       // Read(), it is written to the cache).
       rc = openCacheWrite(url->getAbsolute(), streamInfo);

       // NOTE: In this case, the validator to return in the streamInfo map
       // has already been set by the HTTP stream.
     }
   } break;
   case VXIinet_RESULT_NOT_MODIFIED:
     {
       // Update the new validator return by the HTTP stream with any
       // information it does not contain from the previously cached validator.
       // This is an assumption we make to make sure validator data is
       // properly progated when it is not returned by an HTTP server.
       if (newValidator != NULL)
       {
	   //  Log to voiceglue
	   /*
	   if (voiceglue_loglevel() >= LOG_INFO)
	   {
	       std::ostringstream logstring;
	       logstring << "SBinetHttpCacheStream::Open(): propagateDataFrom()";
	       voiceglue_log ((char) LOG_INFO, logstring);
	   };
	   */

         bool changed = newValidator->propagateDataFrom(cachedValidator);
         if (changed)
         {
	   //  Log to voiceglue
	     /*
	   if (voiceglue_loglevel() >= LOG_INFO)
	   {
	       std::ostringstream logstring;
	       logstring << "SBinetHttpCacheStream::Open(): setValidatorInfo()";
	       voiceglue_log ((char) LOG_INFO, logstring);
	   };
	     */

           // If the new validator has changed, update the validator in the
           // streamInfo map.
           if (!SBinetUtils::setValidatorInfo(streamInfo, newValidator))
           {
             Error(103, NULL);
             rc = VXIinet_RESULT_OUT_OF_MEMORY;
           }
         }
       }

       if( streamInfo ) {
         // Set VALIDATED CACHE data source information   
         VXIMapSetProperty(streamInfo, INET_INFO_DATA_SOURCE, 
                       (VXIValue*) VXIIntegerCreate(INET_DATA_SOURCE_VALIDATED));
       }

       // Write the updated validator to the cache.  This validator contains
       // all original validator information (propagated from the original
       // validator) and contains any new information returned by the server
       // in the Not-Modified response.
       if (newValidator != NULL)
       {
	   //  Log to voiceglue
	   /*
	   if (voiceglue_loglevel() >= LOG_INFO)
	   {
	       std::ostringstream logstring;
	       logstring << "SBinetHttpCacheStream::Open(): writeValidatorToCache()";
	       voiceglue_log ((char) LOG_INFO, logstring);
	   };
	   */

         // Make sure only ONE thread/channel write this info
         VXIcacheResult crc = writeValidatorToCache(url, *newValidator);
         // let other thread/channel to write validator info
         if (crc != VXIcache_RESULT_SUCCESS) {
           return cache2inetRc(crc);
         }
       }

       // Cases for non-error return values when HTTP stream returns NOT_MODIFIED:
       // 1) No external validator was passed to INET -> SUCCESS
       // 2) An external validator was passed to INET and it did not match the
       //    cached validator -> SUCCESS;
       // 3) An external validator was passed to INET and it matched the cached
       //    validator -> NOT_MODIFIED
       // NOTE: The case where an external validator was passed to INET but
       //       there is no cached validator results in SUCCESS since the HTTP
       //       fetch will have occurred (no conditional fetch will be
       //       performed).
       if (externalValidatorPresent && (externalValidator == cachedValidator))
       {
         rc = Close();
         if (rc != VXIinet_RESULT_SUCCESS)
           return rc;

         return VXIinet_RESULT_NOT_MODIFIED;
       }
       else
       {
         // Reset return code, if error occurs it will be set accordingly
         rc = VXIinet_RESULT_SUCCESS;
         
         // We no longer need the http stream.  We must close and delete it.
         // Otherwise, subsequent calls to Read() will attempt to read from
         // it rather than the cache.
         // It is important to do this after we are done using the stream's
         // validator.
         if (_httpStream != NULL)
         {
	   //  Log to voiceglue
	     /*
	   if (voiceglue_loglevel() >= LOG_INFO)
	   {
	       std::ostringstream logstring;
	       logstring << "SBinetHttpCacheStream::Open(): _httpStream->Close()";
	       voiceglue_log ((char) LOG_INFO, logstring);
	   };
	     */

           VXIinetResult nrc = _httpStream->Close();
           if (nrc != VXIinet_RESULT_SUCCESS)
             return nrc;

           delete _httpStream;
           _httpStream = NULL;
           newValidator = NULL;
         }

         // Prepare the document for reading so that subsequent calls to
         // Read() will succeed.
         if (_cache)
         {
	   //  Log to voiceglue
	     /*
	   if (voiceglue_loglevel() >= LOG_INFO)
	   {
	       std::ostringstream logstring;
	       logstring << "SBinetHttpCacheStream::Open(): openCacheRead()";
	       voiceglue_log ((char) LOG_INFO, logstring);
	   };
	     */

           VXIcacheResult crc = openCacheRead(_url->getAbsolute(), streamInfo);
           // The cache entry may have been deleted, fetch the content from webserver
           // Do not cache this page
           if (crc != VXIcache_RESULT_SUCCESS) {

	       //  Log to voiceglue
	       /*
	       if (voiceglue_loglevel() >= LOG_INFO)
	       {
		   std::ostringstream logstring;
		   logstring << "SBinetHttpCacheStream::Open(): cachedValidator.Clear()";
		   voiceglue_log ((char) LOG_INFO, logstring);
	       };
	       */

             // (1) Close all streams and clear validator
             Close();
             cachedValidator.Clear();
             
	       //  Log to voiceglue
	     /*
	       if (voiceglue_loglevel() >= LOG_INFO)
	       {
		   std::ostringstream logstring;
		   logstring << "SBinetHttpCacheStream::Open(): new SBinetHttpStream()";
		   voiceglue_log ((char) LOG_INFO, logstring);
	       };
	     */

             // (2) Create new HTTP stream, no matter external or interal validator, the client
             //     must read from the HTTP stream, and obtains new stream info!
             SBinetURL *tmpUrl = new SBinetURL(*_url);
             _httpStream =
               new SBinetHttpStream(tmpUrl, _method, _channel, &cachedValidator, GetLog(), GetDiagBase());

	     //  Log to voiceglue
	     /*
	     if (voiceglue_loglevel() >= LOG_INFO)
	     {
		 std::ostringstream logstring;
		 logstring << "SBinetHttpCacheStream::Open(): getTimeOut()";
		 voiceglue_log ((char) LOG_INFO, logstring);
	     };
	     */
	     
             // (3) Setup timeout
             _httpReadCompleted = false;
             getTimeOut(&timeout);
             _httpStream->setTimeOut(&timeout);
             
	     //  Log to voiceglue
	     /*
	     if (voiceglue_loglevel() >= LOG_INFO)
	     {
		 std::ostringstream logstring;
		 logstring << "SBinetHttpCacheStream::Open(): _httpStream->Open()";
		 voiceglue_log ((char) LOG_INFO, logstring);
	     };
	     */
	     
             // (4) Connect to webserver for non-conditional fetch
             rc = _httpStream->Open(flags, properties, streamInfo);
             
	     //  Log to voiceglue
	     /*
	     if (voiceglue_loglevel() >= LOG_INFO)
	     {
		 std::ostringstream logstring;
		 logstring << "SBinetHttpCacheStream::Open(): Close()";
		 voiceglue_log ((char) LOG_INFO, logstring);
	     };
	     */
	     
             // (5) Cleanup if fetch failed!
             if( rc != VXIinet_RESULT_SUCCESS )
               Close();                        
           }              
         }
       }
     }
     break;
   default:
     Close();
     break;
  }

  //  Log to voiceglue
  /*
  if (voiceglue_loglevel() >= LOG_INFO)
  {
      std::ostringstream logstring;
      logstring << "SBinetHttpCacheStream::Open(): finished";
      voiceglue_log ((char) LOG_INFO, logstring);
  };
  */
	     
  return rc;
}


// This method writes the message body to the cache.
VXIinetResult SBinetHttpCacheStream::Read(/* [OUT] */ VXIbyte*         pBuffer,
                                          /* [IN]  */ VXIulong         nBuflen,
                                          /* [OUT] */ VXIulong*        pnRead )
{
  VXIinetResult rc = VXIinet_RESULT_NON_FATAL_ERROR;
  bool cleanCache = false;

  if (_httpStream != NULL)
  {
    VXIulong nbRead;
    rc = _httpStream->Read(pBuffer, nBuflen, &nbRead);
    if (pnRead != NULL) *pnRead = nbRead;

    if (_cacheStream != NULL)
    {
      switch (rc)
      {
       case VXIinet_RESULT_END_OF_STREAM:
         _httpReadCompleted = true;
         // no break: intentional
       case VXIinet_RESULT_SUCCESS:
         if (nbRead > 0)
         {
           VXIulong nbWritten = 0;
           // Now write the read data (message body) to the cache
           VXIcacheResult rcc = _cache->Write(_cache, pBuffer, nbRead,
                                              &nbWritten, _cacheStream);

           if ((rcc != VXIcache_RESULT_SUCCESS) || (nbWritten != nbRead))
           {
             if (rcc == VXIcache_RESULT_EXCEED_MAXSIZE)
             {
               Error(305, L"%s%s", L"URL", _url->getAbsolute());
               rc = VXIinet_RESULT_SUCCESS;
             }
             else
             {
               Error(232, L"%s%s%s%i%s%i%s%i", L"URL", _url->getAbsolute(),
                     L"rc", rcc, L"written", nbWritten, L"expected", nbRead);
               rc = VXIinet_RESULT_NON_FATAL_ERROR;
             }
             cleanCache = true;
           }
         }
         break;
       default:
         cleanCache = true;
         break;
      }
    }    
  }
  else if (_cacheStream != NULL)
  {
    if (hasTimedOut())
    {
      Error(251, L"%s%s", L"URL", _url->getAbsolute());
      rc = VXIinet_RESULT_FETCH_TIMEOUT;
      cleanCache = true;
    }
    else
    {
      VXIcacheResult rcc = _cache->Read(_cache, pBuffer, nBuflen, pnRead, _cacheStream);
      if ((rcc != VXIcache_RESULT_SUCCESS) && (rcc != VXIcache_RESULT_END_OF_STREAM))
      {
         Error(231, L"%s%s%s%i%s%i%s%i", L"URL", _url->getAbsolute(),
               L"rc", rcc, L"read", *pnRead, L"expected", nBuflen);
         cleanCache = true;
      }
      rc = cache2inetRc(rcc);
    }
  }
  else
  {
    Error(218, L"%s%s", L"URL", _url->getAbsolute());
  }

  if (cleanCache)
  {
    VXIcacheResult rcc = _cache->CloseEx(_cache, FALSE, &_cacheStream);
    if (rcc != VXIcache_RESULT_SUCCESS)
      Error(239, L"%s%s%s%i", L"URL", _url->getAbsolute(), L"rc", rcc);
    _cacheStream = NULL;
  }

  return rc;
}


VXIinetResult SBinetHttpCacheStream::Write(/* [IN] */ const VXIbyte*   pBuffer,
                                           /* [IN]  */ VXIulong         nBuflen,
                                           /* [OUT] */ VXIulong*        pnWritten)
{
  if (_httpStream != NULL)
  {
    return _httpStream->Write(pBuffer, nBuflen, pnWritten);
  }
  return VXIinet_RESULT_PLATFORM_ERROR;
}


VXIinetResult SBinetHttpCacheStream::Close()
{
  VXIinetResult rc1 = VXIinet_RESULT_SUCCESS;
  VXIinetResult rc2 = VXIinet_RESULT_SUCCESS;

  VXIbool keepEntry = TRUE;
  
  if (_httpStream != NULL)
  {
    rc1 = _httpStream->Close();
    delete _httpStream;
    _httpStream = NULL;
    keepEntry = _httpReadCompleted;
  }

  if (_cacheStream != NULL)
  {
    VXIcacheResult rcc = _cache->CloseEx(_cache, keepEntry, &_cacheStream);

    if (rcc != VXIcache_RESULT_SUCCESS)
    {
      Error(239, L"%s%s%s%i", L"URL", _url->getAbsolute(), L"rc", rcc);
      rc2 = VXIinet_RESULT_FAILURE;
    }
    _cacheStream = NULL;
  }

  // If any of the close fails, return that one.
  return rc1 != VXIinet_RESULT_SUCCESS ? rc1 : rc2;
}

void SBinetHttpCacheStream::setTimeOut(const SWITimeStamp *timeOfDay)
{
  if (_httpStream != NULL)
  {
    _httpStream->setTimeOut(timeOfDay);
  }
  SBinetStoppableStream::setTimeOut(timeOfDay);
}

VXIcacheResult
SBinetHttpCacheStream::openCacheEntry(const VXIchar *absoluteURL,
                                      VXIcacheOpenMode mode,
                                      VXIMap *cacheStreamInfo)
{
  VXIcacheResult rc;
  long delay;
  const long maxSleepTime = 20;

  static const VXIchar *moduleName = L"swi:" MODULE_SBINET;

  SBinetString cacheKey = moduleName;
  cacheKey += L':';
  cacheKey += absoluteURL;

  VXIMapHolder cacheProp; // will destroy the map for us
  VXIMapSetProperty(cacheProp.GetValue(), CACHE_CREATION_COST,
		    (VXIValue *) VXIIntegerCreate(CACHE_CREATION_COST_FETCH));

  while ((rc = _cache->Open(_cache, MODULE_SBINET,
                            cacheKey.c_str(),
                            mode, CACHE_FLAG_NULL,
                            cacheProp.GetValue(), cacheStreamInfo,
			    &_cacheStream))
         == VXIcache_RESULT_ENTRY_LOCKED)
  {
    //VXItrdThreadYield();
    delay = getDelay();

    if (delay == 0)
      break;
    else if (delay < 0 || delay > maxSleepTime)
      delay = maxSleepTime;

    SBinetHttpUtils::usleep(delay * 1000);
  }

  if (rc != VXIcache_RESULT_SUCCESS)
  {
    if (rc == VXIcache_RESULT_NOT_FOUND)
      Diag(MODULE_SBINET_CACHE_TAGID, L"SBinetHttpCacheStream",
           L"Not found in cache: %s", absoluteURL);
    else
      Error(230, L"%s%s%s%i", L"URL", absoluteURL, L"rc", rc);
  }

  return rc;
}

// This method reads the document mime type from the cache.
// Sets the INET_INFO_SIZE_BYTES and INET_INFO_MIME_TYPE info values
// in this stream's streamInfo map.
// The actual reading of the document is done inside Read().
VXIcacheResult SBinetHttpCacheStream::openCacheRead(const VXIchar* absoluteURL, VXIMap *streamInfo)
{
  VXIMap *cacheStreamInfo = VXIMapCreate();
  VXIcacheResult rc;
  VXIint32 entrySizeBytes = 0;
  VXIchar* mimeType = NULL;
  VXIulong bytesRead = 0;
  VXIulong dataSize = 0;
  VXIulong dataSizeBytes = 0;

  if (_cache == NULL)
    return VXIcache_RESULT_FAILURE;

  rc = openCacheEntry(absoluteURL, CACHE_MODE_READ, cacheStreamInfo);
  if (rc != VXIcache_RESULT_SUCCESS)
    goto failure;

  // Retrieve the total size of the cache entry
  if (!SBinetUtils::getInteger(cacheStreamInfo,
                               CACHE_INFO_SIZE_BYTES, entrySizeBytes))
  {
    Error(233, L"%s%s%", L"URL", absoluteURL);
    rc = VXIcache_RESULT_FAILURE;
    goto failure;
  }

  // Read the MIME type size (number of VXIchar), followed by the actual MIME
  // type
  bytesRead = 0;
  rc = _cache->Read(_cache, (VXIbyte *) &dataSize, sizeof(dataSize), &bytesRead, _cacheStream);
  if ((rc != VXIcache_RESULT_SUCCESS) || (bytesRead != sizeof(dataSize)))
  {
    Error(231, L"%s%s%s%i%s%i%s%i", "URL", absoluteURL, L"rc", rc,
          L"read", bytesRead, L"expected", sizeof(dataSize));
    rc = VXIcache_RESULT_FAILURE;
    goto failure;
  }
  // Adjust the actual size
  entrySizeBytes -= bytesRead;

  // Now read mime type.
  mimeType = new VXIchar[dataSize + 1];
  dataSizeBytes = dataSize * sizeof(VXIchar);

  rc = _cache->Read(_cache, (VXIbyte *) mimeType, dataSizeBytes, &bytesRead, _cacheStream);
  if ((rc != VXIcache_RESULT_SUCCESS) || (bytesRead != dataSizeBytes))
  {
    Error(231, L"%s%s%s%i%s%i%s%i", L"URL", absoluteURL, L"rc", rc,
          L"read", bytesRead, L"expected", dataSizeBytes);
    rc = VXIcache_RESULT_FAILURE;
    goto failure;
  }
  mimeType[dataSize] = L'\0';
  // Adjust the actual size
  entrySizeBytes -= bytesRead;

  // Set the document size property
  VXIMapSetProperty(streamInfo, INET_INFO_SIZE_BYTES,
                    (VXIValue*)VXIIntegerCreate(entrySizeBytes));

  // Set the MIME type property
  VXIMapSetProperty(streamInfo, INET_INFO_MIME_TYPE,
                    (VXIValue*)VXIStringCreate(mimeType));

  // Set the absolute URL property
  VXIMapSetProperty(streamInfo, INET_INFO_ABSOLUTE_NAME,
                    (VXIValue*)VXIStringCreate(absoluteURL));

  goto end;

 failure:
  if (_cacheStream)
  {
    VXIcacheResult rcc = _cache->CloseEx(_cache, TRUE, &_cacheStream);
    if (rcc != VXIcache_RESULT_SUCCESS)
      Error(239, L"%s%s%s%i", L"URL", absoluteURL, L"rc", rcc);
  }
  _cacheStream = NULL;

 end:
  if (mimeType)
    delete [] mimeType;
  if (cacheStreamInfo)
    VXIMapDestroy(&cacheStreamInfo);

  return rc;
}

// This method reads the validator from the validator cache entry.
VXIcacheResult
SBinetHttpCacheStream::readValidatorFromCache(SBinetURL *&ultimateURL,
                                              SBinetValidator& validator,
                                              bool &expired)
{
  if (_cache == NULL)
    return VXIcache_RESULT_FAILURE;

  VXIMap *cacheStreamInfo = VXIMapCreate();
  VXIbyte *validatorData = NULL;
  VXIcacheResult rc;
  VXIulong maxDataSize = 0;
  VXIulong bytesRead = 0;
  VXIulong dataSize = 0;
  SBinetURL *tmpURL = NULL;
  const VXIchar *absoluteURL = NULL;

  ultimateURL = new SBinetURL(*_url);
  expired = false;
  
  for (;;)
  {
    absoluteURL = ultimateURL->getAbsolute();
    VXIchar* urlKey = new VXIchar [wcslen(absoluteURL) + 12];
    wcscpy(urlKey, absoluteURL);
    wcscat(urlKey, L"_validator");

    rc = openCacheEntry(urlKey, CACHE_MODE_READ, cacheStreamInfo);

    delete [] urlKey;

    if (rc != VXIcache_RESULT_SUCCESS)
      break;

    // The document is in cache.
    Diag(MODULE_SBINET_CACHE_TAGID, L"SBinetHttpCacheStream",
         L"Found in cache: %s", absoluteURL);

    // Read the size of the validator info.
    rc = _cache->Read(_cache, (VXIbyte *) &dataSize,
                      sizeof(dataSize), &bytesRead, _cacheStream);

    if (rc != VXIcache_RESULT_SUCCESS || bytesRead != sizeof(dataSize))
    {
      Error(231, L"%s%s%s%i%s%i%s%i", L"URL", absoluteURL, L"rc", rc,
            L"read", bytesRead, L"expected", sizeof(dataSize));
      rc = VXIcache_RESULT_FAILURE;
      break;
    }

    if (dataSize > maxDataSize)
    {
      delete [] validatorData;
      validatorData = new VXIbyte[dataSize];
      maxDataSize = dataSize;
    }

    if (validatorData == NULL)
    {
      Error(103, NULL);
      rc = VXIcache_RESULT_FAILURE;
      break;
    }

    rc = _cache->Read(_cache, validatorData, dataSize, &bytesRead, _cacheStream);
    if (rc != VXIcache_RESULT_SUCCESS || bytesRead != dataSize)
    {
      Error(231, L"%s%s%s%i%s%i%s%i", L"URL", absoluteURL, L"rc", rc,
            L"read", bytesRead, L"expected", dataSize);
      rc = VXIcache_RESULT_FAILURE;
      break;
    }

    if (validator.Create(validatorData, dataSize) != VXIinet_RESULT_SUCCESS)
    {
      Error(215, L"%s%s", NULL);
      rc = VXIcache_RESULT_FAILURE;
      break;
    }

    if (validator.isExpired(_maxAge, _maxStale))
    {
      expired = true;
      Diag(MODULE_SBINET_CACHE_TAGID, L"SBinetHttpCacheStream",
           L"Validator expired: %s", absoluteURL);
      rc = VXIcache_RESULT_FAILURE;      
      break;
    }

    // If the validator's URL is not equals to the current ultimate URL, then
    // this is a redirect.
    absoluteURL = validator.getURL();
    if (tmpURL == NULL)
    {
      if (SBinetURL::create(absoluteURL, NULL, tmpURL) != VXIinet_RESULT_SUCCESS)
        rc = VXIcache_RESULT_FAILURE;
    }
    else
    {
      if (tmpURL->parse(absoluteURL, NULL) != VXIinet_RESULT_SUCCESS)
        rc = VXIcache_RESULT_FAILURE;
    }
    if (rc != VXIcache_RESULT_SUCCESS)
    {
      Diag(MODULE_SBINET_CACHE_TAGID,
           L"SBinetHttpCacheStream::OpenCacheRead",
           L"Invalid URL: <%s>", absoluteURL);
      break;
    }

    // Compare URLs.
    if (*ultimateURL == *tmpURL)
    {
      // No redirection.
      break;
    }

    // Redirection: continue.
    Diag(MODULE_SBINET_CACHE_TAGID, L"SBinetHttpCacheStream::OpenCacheRead",
         L"Redirection detected, from %s to %s", ultimateURL->getAbsolute(), absoluteURL);
    *ultimateURL = *tmpURL;

    // Close the current cache entry and continue with the redirection.
    VXIcacheResult rcc = _cache->CloseEx(_cache, TRUE, &_cacheStream);
    if (rcc != VXIcache_RESULT_SUCCESS)
      Error(239, L"%s%s%s%i", L"URL", absoluteURL, L"rc", rcc);
    _cacheStream = NULL;
  }

  delete [] validatorData;
  delete tmpURL;

  // Close the cache entry no matter what.  It will not be reused.
  if (_cacheStream != NULL)
  {
    VXIcacheResult rcc = _cache->CloseEx(_cache, TRUE, &_cacheStream);
    if (rcc != VXIcache_RESULT_SUCCESS)
      Error(239, L"%s%s%s%i", L"URL", absoluteURL, L"rc", rcc);
    _cacheStream = NULL;
  }

  if (cacheStreamInfo)
    VXIMapDestroy(&cacheStreamInfo);

  if (rc != VXIcache_RESULT_SUCCESS && !expired)
  {
    delete ultimateURL;
    ultimateURL = NULL;
  }

  return rc;
}

// Write the document to cache. This method only stores
// MIME type in the cache. The real document (message body) is
// written later in Read().
VXIinetResult
SBinetHttpCacheStream::openCacheWrite(const VXIchar *absoluteURL,
                                      const VXIMap *streamInfo)
{
  if (_cache == NULL)
    return VXIinet_RESULT_SUCCESS;

  // Check whether we already cached this URL.
  VXIcacheResult rc = openCacheEntry(absoluteURL, CACHE_MODE_WRITE, NULL);
  if (rc != VXIcache_RESULT_SUCCESS)
    return VXIinet_RESULT_SUCCESS;

  const VXIchar* mimeType = SBinetUtils::getString(streamInfo, INET_INFO_MIME_TYPE);
  VXIulong dataSize = (mimeType == NULL)? 0 : ::wcslen(mimeType);
  VXIulong dataSizeBytes = 0;
  VXIulong bytesWritten = 0;

  // Write the size of the of the mime-type (number of VXIchar).
  rc = _cache->Write(_cache, (VXIbyte *) &dataSize, sizeof(dataSize), &bytesWritten, _cacheStream);
  if ((rc != VXIcache_RESULT_SUCCESS) || (bytesWritten != sizeof(dataSize)))
  {
    Error(232, L"%s%s%s%i%s%i%s%i", L"URL", absoluteURL, L"rc", rc,
          L"written", bytesWritten, L"expected", sizeof(dataSize));
    goto failure;
  }

  // Now write the mime-type.
  bytesWritten = 0;
  dataSizeBytes = dataSize * sizeof(VXIchar);
  rc = _cache->Write(_cache, (const VXIbyte *) mimeType, dataSizeBytes,
		     &bytesWritten, _cacheStream);
  if ((rc != VXIcache_RESULT_SUCCESS) || (bytesWritten != dataSizeBytes))
  {
    Error(232, L"%s%s%s%i%s%i%s%i", L"URL", absoluteURL, L"rc", rc,
          L"written", bytesWritten, L"expected", dataSizeBytes);
    goto failure;
  }

  goto end;

 failure:
  if (_cacheStream)
  {
    VXIcacheResult rcc = _cache->CloseEx(_cache, FALSE, &_cacheStream);
    if (rcc != VXIcache_RESULT_SUCCESS)
      Error(239, L"%s%s%s%i", L"URL", absoluteURL, L"rc", rcc);
  }
  _cacheStream = NULL;

 end:
  if (rc == VXIcache_RESULT_ENTRY_LOCKED)
  {
    // This is because of a timeout on trying to lock the cache entry.  We just
    // return a a fetch timeout to cause our calling function to fail.
    Error(235, L"%s%s", L"URL", absoluteURL);
    return VXIinet_RESULT_FETCH_TIMEOUT;
  }
  else
  {
    // Something wrong occured while trying to write data to the cache entry.
    // We will assume success, but there will be no caching.
    return VXIinet_RESULT_SUCCESS;
  }
}

// This method writes the validator to the cache.
VXIcacheResult
SBinetHttpCacheStream::writeValidatorToCache(const SBinetURL *ultimateURL,
                                             const SBinetValidator& validator)
{
  if (_cache == NULL)
    return VXIcache_RESULT_SUCCESS;

  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  const VXIchar* absoluteURL = NULL;
  // Take care of redirection.
  if (*_url != *ultimateURL)
  {
    Diag(MODULE_SBINET_CACHE_TAGID, L"SBinetHttpCacheStream::OpenCacheWrite",
         L"Redirection detected, from %s to %s", absoluteURL, ultimateURL->getAbsolute());

    absoluteURL = ultimateURL->getAbsolute();
  }
  else
  {
    absoluteURL = _url->getAbsolute();
  }

  VXIbyte *validatorData = NULL;
  VXIulong bytesWritten = 0;
  VXIulong dataSize = 0;

  // Construct the key for this validator cache entry.
  VXIchar* urlKey = new VXIchar [wcslen(absoluteURL) + 12];
  wcscpy(urlKey, absoluteURL);
  wcscat(urlKey, L"_validator");

  // Check whether we already cached the validator for this URL.
  rc = openCacheEntry(urlKey, CACHE_MODE_WRITE, NULL);
  delete [] urlKey;
  if (rc != VXIcache_RESULT_SUCCESS)
    goto failure;

  if (!validator.serialize(validatorData, dataSize))
  {
    Error(215, L"%s%s", L"URL", absoluteURL);
    rc = VXIcache_RESULT_FAILURE;
    goto failure;
  }

  // Write the size of the validator.
  rc = _cache->Write(_cache, (VXIbyte *) &dataSize, sizeof(dataSize), &bytesWritten, _cacheStream);
  if (rc != VXIcache_RESULT_SUCCESS || (bytesWritten != sizeof(dataSize)))
  {
    if (rc == VXIcache_RESULT_EXCEED_MAXSIZE)
      Error(305, L"%s%s", L"URL", absoluteURL);
    else
      Error(232, L"%s%s%s%i%s%i%s%i", L"URL", absoluteURL, L"rc", rc,
            L"written", bytesWritten, L"expected", sizeof(dataSize));
    rc = VXIcache_RESULT_FAILURE;
    goto failure;
  }

  // Write the validator.
  bytesWritten = 0;
  rc = _cache->Write(_cache, validatorData, dataSize, &bytesWritten, _cacheStream);
  if (rc != VXIcache_RESULT_SUCCESS || bytesWritten != dataSize)
  {
    Error(232, L"%s%s%s%i%s%i%s%i", L"URL", absoluteURL, L"rc", rc,
          L"written", bytesWritten, L"expected", dataSize);
    rc = VXIcache_RESULT_FAILURE;
    goto failure;
  }

 failure:
  delete [] validatorData;

  // Close the validator cache stream once done.
  if (_cacheStream)
  {
    VXIcacheResult rcc = _cache->CloseEx(_cache, TRUE, &_cacheStream);
    if (rcc != VXIcache_RESULT_SUCCESS)
      Error(239, L"%s%s%s%i", L"URL", absoluteURL, L"rc", rcc);
  }
  _cacheStream = NULL;

  return rc;
}
