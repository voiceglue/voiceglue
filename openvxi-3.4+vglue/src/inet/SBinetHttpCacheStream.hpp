
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

#ifndef SBINETHTTPCACHESTREAM_HPP
#define SBINETHTTPCACHESTREAM_HPP

/**
 * This class is responsible for reading information from a cache if
 * available and non-expired.  Otherwise, it uses an SBinetHttpStream
 * to fetch information and then caches it.
 *
 * @doc <p>
 **/

#include "SBinetStoppableStream.hpp"
#include "SBinetHttpStream.hpp"
#include "VXIcache.h"

class SBinetChannel;
class SBinetValidator;

class SBinetHttpCacheStream: public SBinetStoppableStream
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.
   **/
 public:
  SBinetHttpCacheStream(SBinetURL* url,
                        SBinetHttpStream::SubmitMethod method,
                        SBinetChannel* ch,
                        VXIcacheInterface *cache,
                        VXIlogInterface *log, VXIunsigned diagLogBase);

  /**
   * Destructor.
   **/
 public:
  virtual ~SBinetHttpCacheStream();

  virtual VXIinetResult Open(VXIint flags,
                             const VXIMap* properties,
                             VXIMap* streamInfo);

  virtual VXIinetResult Read(/* [OUT] */ VXIbyte*         pBuffer,
                             /* [IN]  */ VXIulong         nBuflen,
                             /* [OUT] */ VXIulong*        pnRead );

  virtual VXIinetResult Write(/* [IN] */ const VXIbyte*   pBuffer,
                              /* [IN]  */ VXIulong         nBuflen,
                              /* [OUT] */ VXIulong*        pnWritten);

  virtual VXIinetResult Close();

  virtual void setTimeOut(const SWITimeStamp *timeout);

  /**
    * Disabled copy constructor.
   **/
 private:
  SBinetHttpCacheStream(const SBinetHttpCacheStream&);

  /**
    * Disabled assignment operator.
   **/
 private:
  SBinetHttpCacheStream& operator=(const SBinetHttpCacheStream&);

 private:
  void setProperties(const VXIMap* properties);
  VXIcacheResult openCacheRead(const VXIchar *absoluteURL, VXIMap *streamInfo);
  VXIinetResult openCacheWrite(const VXIchar *absoluteURL, const VXIMap *streamInfo);
  VXIcacheResult openCacheEntry(const VXIchar *absoluteURL, VXIcacheOpenMode mode, VXIMap *cacheStreamInfo);

  VXIcacheResult readValidatorFromCache(SBinetURL* &ultimateURL, SBinetValidator& validator, bool &expired);
  VXIcacheResult writeValidatorToCache(const SBinetURL *ultimateURL, const SBinetValidator& validator);

  VXIinetResult cache2inetRc(VXIcacheResult result);

 private:
  bool _httpReadCompleted;
  VXIint32 _maxAge;
  VXIint32 _maxStale;
  SBinetHttpStream *_httpStream;
  SBinetChannel *_channel;
  VXIcacheInterface *_cache;
  VXIcacheStream *_cacheStream;
  SBinetHttpStream::SubmitMethod _method;
};
#endif
