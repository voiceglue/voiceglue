
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

#ifndef __SBVALIDATOR_H_                   /* Allows multiple inclusions */
#define __SBVALIDATOR_H_

#include <cstdlib>

#include <time.h>    // For time( )

#include "VXIinet.h"          // For VXIinetResult
#include "SWIutilLogger.hpp"   // Base class
#include "SBinetString.hpp"   // For SBinetString

#define VALIDATOR_MIME_TYPE L"application/x-vxi-SBinet-validator"

class SBinetValidator : protected SWIutilLogger
{
public:
  SBinetValidator(VXIlogInterface *log, VXIunsigned diagTagBase);
  virtual ~SBinetValidator();

  // Creation
  VXIinetResult Create(const VXIchar *url, time_t requestTime, const VXIMap *streamInfo);
  VXIinetResult Create(const VXIchar *url, VXIulong sizeBytes,
                       time_t requestTime, const VXIMap *streamInfo);
  VXIinetResult Create(const VXIchar *filename, VXIulong sizeBytes, time_t refTime);
  VXIinetResult Create(const VXIValue *value);

  VXIinetResult Create(const VXIbyte *content, VXIulong contentSize);

  // Serialization
  VXIContent *serialize() const;
  bool serialize(VXIbyte *&content, VXIulong& contentSize) const;

  // Determine if it is expired or modified
  bool isExpired() const;

  bool isModified(time_t lastModified, VXIulong sizeBytes) const
  {
    return (lastModified != _lastModified) || (sizeBytes != _sizeBytes);
  }

  bool isExpired(const VXIint maxAge, const VXIint maxStale) const;

  time_t getLastModified() const
  {
    return _lastModified;
  }

  bool getMustRevalidateF() const
  {
    return _mustRevalidateF;
  }

  time_t getRefTime() const
  {
    return _refTime;
  }

  time_t getExpiresTime() const;

  time_t getCurrentAge() const
  {
    return time(NULL) - _refTime;
  }


  time_t getFreshnessLifetime() const
  {
    return _freshnessLifetime;
  }

  VXIulong getSize() const
  {
    return _sizeBytes;
  }

  const VXIchar *getETAG() const
  {
    return _eTag;
  }

  bool isStrong() const;

  const VXIchar *getURL() const
  {
    return _url;
  }

  bool propagateDataFrom(const SBinetValidator& validator);

  bool operator==(const SBinetValidator& validator);

  void Clear();

 private:
  // Log the validator to the diagnostic log
  void Log(const VXIchar *name) const;

  // VXIContent destructor
  static void ContentDestroy(VXIbyte **content, void *userData);

 private:
  time_t checkPragma();
  time_t checkCacheControl();

  void computeFreshnessLifetime();

  VXIchar *_url;
  VXIulong _sizeBytes;

  time_t _dateTime;
  time_t _expiresTime;
  time_t _lastModified;
  VXIchar *_eTag;
  bool _mustRevalidateF;
  VXIchar *_pragmaDirective;
  VXIchar *_cacheControlDirective;

  time_t _refTime;
  time_t _freshnessLifetime;
};

#endif // include guard
