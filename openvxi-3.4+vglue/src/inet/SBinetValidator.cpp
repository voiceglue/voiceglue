
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

#include <wchar.h>
#include <stdio.h>

#include "SBinetValidator.h"     // For this class
#include "SBinetLog.h"           // For logging definitions
#include "HttpUtils.hpp"
#include "SBinetUtils.hpp"
#include "SBinetChannel.h"

#ifdef DEBUG_VALIDATOR
#include <iostream>
#endif

using namespace std;

/*****************************************************************************
 *****************************************************************************
 * SBinetValidator Implementation
 *****************************************************************************
 *****************************************************************************
 */

#if defined(_decunix_) || defined(_solaris_)

// On solaris,
// wsncasecmp is defined in widec.h but we don't have the right comp. flags to get the prototype.
// I'm not sure about decunix though.
//
extern "C" int wsncasecmp(const wchar_t *, const wchar_t *, size_t);
#define wcsncasecmp(s1,s2,n) wsncasecmp(s1,s2,n)

#if 0
static int my_wcsncasecmp(const wchar_t *s1, const wchar_t *s2,
			  register size_t n)
{
  register unsigned int u1, u2;

  for (; n != 0; --n) {
    u1 = (unsigned int) *s1++;
    u2 = (unsigned int) *s2++;
    if (HttpUtils::toUpper(u1) != HttpUtils::toUpper(u2)) {
      return HttpUtils::toUpper(u1) - HttpUtils::toUpper(u2);
    }
    if (u1 == '\0') {
      return 0;
    }
  }
  return 0;
}
#endif
#endif

SBinetValidator::SBinetValidator(VXIlogInterface *log, VXIunsigned diagTagBase)
  : SWIutilLogger(MODULE_SBINET, log, diagTagBase), _freshnessLifetime((time_t) -1),
    _refTime((time_t) -1), _sizeBytes(0), _eTag(NULL), _mustRevalidateF(false),
    _url(NULL), _lastModified((time_t) -1), _expiresTime((time_t) -1),
    _pragmaDirective(NULL), _cacheControlDirective(NULL), _dateTime((time_t) -1)
{}

SBinetValidator::~SBinetValidator()
{
  delete [] _eTag;
  delete [] _url;
  delete [] _cacheControlDirective;
  delete [] _pragmaDirective;
}

VXIinetResult SBinetValidator::Create(const VXIchar *filename, VXIulong sizeBytes,
                                      time_t refTime)
{
  _lastModified = refTime;
  _refTime = refTime;
  return Create(filename, sizeBytes, (time_t) -1, (VXIMap *) NULL);
}

VXIinetResult SBinetValidator::Create(const VXIchar *url,
                                      time_t requestTime,
                                      const VXIMap *streamInfo)
{
  if (url == NULL || !*url || streamInfo == NULL)
  {
    return VXIinet_RESULT_INVALID_ARGUMENT;
  }

  VXIint32 sizeBytes = 0;
  SBinetUtils::getInteger(streamInfo, INET_INFO_SIZE_BYTES, sizeBytes);

  return Create(url, (VXIulong) sizeBytes, requestTime, streamInfo);
}

static time_t computeRefTime(time_t requestTime,
                             const VXIMap* streamInfo,
                             time_t& dateTime)
{
  // The algorithm is based on the RFC section 13.2.3 but instead of computing
  // an initial age, it computes a reference time that can later be used to
  // retrieve the age.

  // The algo in the RFC to compute current-age is as follows.

  //  response_delay = response_time - request_time;
  //  corrected_initial_age = corrected_received_age + response_delay;
  //  resident_time = now - response_time;
  //  current_age   = corrected_initial_age + resident_time;

  // Which can be rewritten as follows:

  //  current_age = (corrected_received_age + response_delay) + (now - response_time)
  //              = (corrected_received_age + response_time - request_time) + (now - response_time)
  //              = corrected_received_age + now - request_time
  //              = now - (request_time - corrected_received_age)
  // This algorithm computes the ref. time to be equal to request_time - corrected_received_age.

  // Using this algorithm has two advantages:
  //
  // 1) More efficient: less computation required to compute the ref. time
  // than to compute the corrected initial age. When computing the current
  //
  // 2) Only requires to store the ref. time in the validator rather than the
  // response time and the corrected initial age.

  VXIint32 tmp;
  time_t responseTime = time(NULL);
  time_t computed_age = (time_t) 0;

  if (SBinetUtils::getInteger(streamInfo, L"Date", tmp))
  {
    dateTime = (time_t) tmp;
    computed_age = responseTime - dateTime;
    if (computed_age < (time_t) 0) computed_age = (time_t) 0;
  }
  else
  {
    dateTime = responseTime;
  }

  if (SBinetUtils::getInteger(streamInfo, L"Age", tmp))
  {
    time_t age = (time_t) tmp;
    if (age > computed_age) computed_age = age;
  }
  return requestTime - computed_age;
}

VXIinetResult SBinetValidator::Create(const VXIchar *url, VXIulong sizeBytes,
                                      time_t requestTime,
                                      const VXIMap *streamInfo)
{
  if (url == NULL || !*url)
  {
    return VXIinet_RESULT_INVALID_ARGUMENT;
  }

  if (_url)
    delete [] _url;
  _url = ::wcscpy(new VXIchar[::wcslen(url) + 1], url);

  _sizeBytes = sizeBytes;
  _freshnessLifetime = (time_t) -1;

  const VXIchar *eTag = NULL;

  delete [] _eTag;

  //   // Sanity check
  //   if (_expiresTime >= 0 && _lastModifiedTime >= _expiresTime)
  //     _expiresTime = 0;

  //   time_t now = time(NULL);
  //   if (_lastModifiedTime > now)
  //     _lastModifiedTime = now;

  if (streamInfo != NULL)
  {
    // Reference time
    _refTime = computeRefTime(requestTime, streamInfo, _dateTime);

    // Last-modified
    VXIint32 lastModified = -1;
    SBinetUtils::getInteger(streamInfo, L"Last-Modified", lastModified);
    _lastModified = (time_t) lastModified;

    // Expires
    VXIint32 expiresTime = -1;
    SBinetUtils::getInteger(streamInfo, L"Expires", expiresTime);
    _expiresTime = (time_t) expiresTime;

    // ETag
    eTag = SBinetUtils::getString(streamInfo, L"ETag");

    // Pragma
    const VXIchar* pragmaDirective = SBinetUtils::getString(streamInfo, L"Pragma");
    if (pragmaDirective)
    {
      _pragmaDirective = new VXIchar [::wcslen(pragmaDirective) + 1];
      ::wcscpy(_pragmaDirective, pragmaDirective);
    }

    // Cache-control
    const VXIchar* cacheControlDirective = SBinetUtils::getString(streamInfo, L"Cache-Control");
    if (cacheControlDirective)
    {
      _cacheControlDirective = new VXIchar [::wcslen(cacheControlDirective) + 1];
      ::wcscpy(_cacheControlDirective, cacheControlDirective);
    }

    computeFreshnessLifetime();
  }

  if (eTag == NULL || !*eTag)
  {
    _eTag = NULL;
  }
  else
  {
    _eTag = ::wcscpy(new VXIchar[::wcslen(eTag) + 1], eTag);
  }

  Log(L"SBinetValidator::Create");
  return VXIinet_RESULT_SUCCESS;
}

void SBinetValidator::computeFreshnessLifetime()
{
  // Inspect the headers for other relevant directives ('no-cache' etc.)
  _freshnessLifetime = checkPragma();
  if (_freshnessLifetime == (time_t) -1)
  {
    _freshnessLifetime = checkCacheControl();
  }

  if (_freshnessLifetime == (time_t) -1)
  {
    if (_expiresTime != (time_t) -1)
      _freshnessLifetime = ((time_t) _expiresTime) - _dateTime;
    else
      _freshnessLifetime = (time_t) 0;
  }

  if (_freshnessLifetime < (time_t) 0)
    _freshnessLifetime = (time_t) 0;
}

VXIinetResult SBinetValidator::Create(const VXIValue *content)
{
  if (VXIValueGetType(content) != VALUE_CONTENT)
  {
    Error(214, L"Type: %d", VXIValueGetType(content));
    return VXIinet_RESULT_INVALID_ARGUMENT;
  }

  // Get the content
  const VXIchar *contentType;
  const VXIbyte *contentData;
  VXIulong contentSizeBytes;
  if ( VXIContentValue((const VXIContent*) content,
                       &contentType, &contentData,
		       &contentSizeBytes) != VXIvalue_RESULT_SUCCESS )
  {
    Error(215, NULL);
    return VXIinet_RESULT_INVALID_ARGUMENT;

  }
  else if ( ::wcscmp(contentType, VALIDATOR_MIME_TYPE) != 0 )
  {
    Error(216, L"mimeType: %s", contentType);
    return VXIinet_RESULT_INVALID_ARGUMENT;
  }

  return Create(contentData, contentSizeBytes);
}


VXIinetResult SBinetValidator::Create(const VXIbyte *data, VXIulong contentSizeBytes)
{
  // Unpack the data
  const VXIbyte *ptr = data;

  ::memcpy(&_dateTime, ptr, sizeof _dateTime);
  ptr += sizeof _dateTime;

  ::memcpy(&_expiresTime, ptr, sizeof _expiresTime);
  ptr += sizeof _expiresTime;

  ::memcpy(&_refTime, ptr, sizeof _refTime);
  ptr += sizeof _refTime;

  ::memcpy(&_freshnessLifetime, ptr, sizeof _freshnessLifetime);
  ptr += sizeof _freshnessLifetime;

  ::memcpy(&_lastModified, ptr, sizeof _lastModified);
  ptr += sizeof _lastModified;

  ::memcpy(&_mustRevalidateF, ptr, sizeof _mustRevalidateF);
  ptr += sizeof _mustRevalidateF;

  ::memcpy(&_sizeBytes, ptr, sizeof _sizeBytes);
  ptr += sizeof _sizeBytes;

  int len;
  ::memcpy(&len, ptr, sizeof len);
  ptr += sizeof len;

  delete [] _eTag;
  if (len > 0)
  {
    int size = len * sizeof(VXIchar);
    _eTag = new VXIchar[len + 1];
    ::memcpy(_eTag, ptr, size);
    ptr += size;
    _eTag[len] = L'\0';
  }
  else
  {
    _eTag = NULL;
  }

  ::memcpy(&len, ptr, sizeof len);
  ptr += sizeof len;

  delete [] _url;
  if (len > 0)
  {
    int size = len * sizeof(VXIchar);
    _url = new VXIchar[len + 1];
    ::memcpy(_url, ptr, size);
    ptr += size;
    _url[len] = L'\0';
  }
  else
  {
    _url = NULL;
  }

  ::memcpy(&len, ptr, sizeof len);
  ptr += sizeof len;

  delete [] _pragmaDirective;
  if (len > 0)
  {
    int size = len * sizeof(VXIchar);
    _pragmaDirective = new VXIchar[len + 1];
    ::memcpy(_pragmaDirective, ptr, size);
    ptr += size;
    _pragmaDirective[len] = L'\0';
  }
  else
  {
    _pragmaDirective = NULL;
  }

  delete [] _cacheControlDirective;
  if (len > 0)
  {
    int size = len * sizeof(VXIchar);
    _cacheControlDirective = new VXIchar[len + 1];
    ::memcpy(_cacheControlDirective, ptr, size);
    ptr += size;
    _cacheControlDirective[len] = L'\0';
  }
  else
  {
    _cacheControlDirective = NULL;
  }

  Log(L"SBinetValidator::Deserialize");
  return VXIinet_RESULT_SUCCESS;
}

VXIContent *SBinetValidator::serialize() const
{
  VXIbyte *data = NULL;
  VXIulong dataSize;
  VXIContent *content;

  if (serialize(data, dataSize))
  {
    // Create the content
    content = VXIContentCreate(VALIDATOR_MIME_TYPE, data, dataSize,
                               ContentDestroy, NULL);

    if (content == NULL)
      delete [] data;
  }

  return content;
}

bool SBinetValidator::serialize(VXIbyte*& content, VXIulong& contentSize) const
{
  // Pack the data
  int eTagLen = 0;
  int urlLen = 0;
  int pragmaDirectiveLen = 0;
  int cacheControlDirectiveLen = 0;

  contentSize = ((sizeof _dateTime) +
                 (sizeof _expiresTime) +
                 (sizeof _refTime) +
                 (sizeof _freshnessLifetime) +
                 (sizeof _lastModified) +
                 (sizeof _mustRevalidateF) +
                 (sizeof _sizeBytes) +
                 (sizeof eTagLen) +
                 (sizeof urlLen) +
                 (sizeof pragmaDirectiveLen) +
                 (sizeof cacheControlDirectiveLen));

  if (_eTag != NULL)
  {
    eTagLen = ::wcslen(_eTag);
    contentSize += eTagLen * sizeof(VXIchar);
  }

  if (_url != NULL)
  {
    urlLen = ::wcslen(_url);
    contentSize += urlLen * sizeof(VXIchar);
  }

  if (_pragmaDirective != NULL)
  {
    pragmaDirectiveLen = ::wcslen(_pragmaDirective);
    contentSize += pragmaDirectiveLen * sizeof(VXIchar);
  }

  if (_cacheControlDirective != NULL)
  {
    cacheControlDirectiveLen = ::wcslen(_cacheControlDirective);
    contentSize += cacheControlDirectiveLen * sizeof(VXIchar);
  }

  content = new VXIbyte[contentSize];
  VXIbyte *ptr = content;

  ::memcpy(ptr, &_dateTime, sizeof _refTime);
  ptr += sizeof _dateTime;

  ::memcpy(ptr, &_expiresTime, sizeof _expiresTime);
  ptr += sizeof _expiresTime;

  ::memcpy(ptr, &_refTime, sizeof _refTime);
  ptr += sizeof _refTime;

  ::memcpy(ptr, &_freshnessLifetime, sizeof _freshnessLifetime);
  ptr += sizeof _freshnessLifetime;

  ::memcpy(ptr, &_lastModified, sizeof _lastModified);
  ptr += sizeof _lastModified;

  ::memcpy(ptr, &_mustRevalidateF, sizeof _mustRevalidateF);
  ptr += sizeof _mustRevalidateF;

  ::memcpy(ptr, &_sizeBytes, sizeof _sizeBytes);
  ptr += sizeof _sizeBytes;

  ::memcpy(ptr, &eTagLen, sizeof eTagLen);
  ptr += sizeof eTagLen;
  if (eTagLen > 0)
  {
    ::memcpy(ptr, _eTag, eTagLen * sizeof(VXIchar));
    ptr += eTagLen * sizeof(VXIchar);
  }

  ::memcpy(ptr, &urlLen, sizeof urlLen);
  ptr += sizeof urlLen;
  if (urlLen > 0)
  {
    ::memcpy(ptr, _url, urlLen * sizeof(VXIchar));
    ptr += urlLen * sizeof(VXIchar);
  }

  ::memcpy(ptr, &pragmaDirectiveLen, sizeof pragmaDirectiveLen);
  ptr += sizeof pragmaDirectiveLen;
  if (pragmaDirectiveLen > 0)
  {
    ::memcpy(ptr, _pragmaDirective, pragmaDirectiveLen * sizeof(VXIchar));
    ptr += pragmaDirectiveLen * sizeof(VXIchar);
  }

  ::memcpy(ptr, &cacheControlDirectiveLen, sizeof cacheControlDirectiveLen);
  ptr += sizeof cacheControlDirectiveLen;
  if (pragmaDirectiveLen > 0)
  {
    ::memcpy(ptr, _cacheControlDirective, cacheControlDirectiveLen * sizeof(VXIchar));
    ptr += cacheControlDirectiveLen * sizeof(VXIchar);
  }

  return true;
}

time_t SBinetValidator::getExpiresTime() const
{
  if (_freshnessLifetime <= (time_t) 0)
    return (time_t) 0;
  return _refTime + _freshnessLifetime;
}

bool SBinetValidator::isExpired() const
{
  return _freshnessLifetime == (time_t) 0 ||
    _freshnessLifetime < time(NULL) - _refTime;
}

bool SBinetValidator::isExpired(const VXIint maxAge,
                                const VXIint maxStale) const
{
#ifdef DEBUG_VALIDATOR
  cout << "maxAge:\t" << maxAge << endl;
  cout << "maxStale:\t" << maxStale << endl;
#endif

  if (maxAge == 0) return true;

  time_t now = time(NULL);
  time_t currentAge = now - _refTime;

#ifdef DEBUG_VALIDATOR
  cout << "now:\t" << now << endl;
  cout << "_refTime:\t" << _refTime << endl;
#endif

  if (maxAge > 0 && currentAge > maxAge) return true;

  time_t staleness = currentAge - _freshnessLifetime;

#ifdef DEBUG_VALIDATOR
  cout << "_freshnessLifetime:\t" << _freshnessLifetime << endl;
  cout << "staleness: " << staleness << endl;
  cout << "_mustRevalidateF: " << _mustRevalidateF << endl;
  cout << "_expiresTime: " << _expiresTime << endl;
#endif

  if (staleness < 0) return false;
  if (_mustRevalidateF) return true;
  if (staleness > maxStale) return true;
  if (_freshnessLifetime == (time_t) 0) return true;

  return false;
}

bool SBinetValidator::isStrong() const
{
  return (_eTag != NULL) && (wcsncasecmp(_eTag, L"w/", 2) != 0);
}

void SBinetValidator::Log(const VXIchar *name) const
{
  if (DiagIsEnabled(MODULE_SBINET_VALIDATOR_TAGID))
  {
    char refTimeStr[64], expireStr[64], lastModifiedStr[64];

    if (_refTime == (time_t) -1)
      ::strcpy(refTimeStr, "-1");
    else
      SBinetUtils::getTimeStampStr(_refTime, refTimeStr);

    SBinetUtils::getTimeStampStr(getExpiresTime(), expireStr);

    if (_lastModified == (time_t) -1)
      ::strcpy(lastModifiedStr, "-1");
    else
      SBinetUtils::getTimeStampStr(_lastModified, lastModifiedStr);

    Diag(MODULE_SBINET_VALIDATOR_TAGID, name,
         L"URL: '%s', refTime %S, last modified %S, expires %S, must revalidate %s, size %lu, etag '%s'",
         _url, refTimeStr, lastModifiedStr, expireStr,
         _mustRevalidateF ? L"true" : L"false",
         _sizeBytes, _eTag ? _eTag : L"");
  }
}


void SBinetValidator::ContentDestroy(VXIbyte **content, void *userData)
{
  if (content)
  {
    delete [] *content;
    *content = NULL;
  }
}

time_t SBinetValidator::checkPragma()
{
  if (_pragmaDirective == NULL)
  {
    Diag(MODULE_SBINET_VALIDATOR_TAGID, L"SBinetValidator::checkPragma",
         L"No Pragma directive.");
    return (time_t) -1;
  }

  Diag(MODULE_SBINET_VALIDATOR_TAGID, L"SBinetValidator::checkPragma",
       L"Pragma directive: '%s'", _pragmaDirective);

  const VXIchar *p = _pragmaDirective;
  while (*p && SBinetHttpUtils::isSpace(*p)) p++;
  if (wcsncasecmp(p, L"no-cache", 8) == 0)
  {
    Diag(MODULE_SBINET_VALIDATOR_TAGID, L"SBinetValidator::checkPragma",
         L"Got Pragma: no-cache directive.");
    return (time_t) 0;
  }
  return (time_t) -1;
}

time_t SBinetValidator::checkCacheControl()
{
  time_t maxAge = (time_t) -1;

  if (_cacheControlDirective == NULL)
  {
    Diag(MODULE_SBINET_VALIDATOR_TAGID, L"SBinetValidator::checkCacheControl",
         L"No Cache-Control directive.");
    return maxAge;
  }

  // We need to convert to narrow string in order to take advantage of the
  // HttpUtils function that are written in terms of narrow character.

  SBinetNString tmp = _cacheControlDirective;
  const char *content = tmp.c_str();

  Diag(MODULE_SBINET_VALIDATOR_TAGID, L"SBinetValidator::checkCacheControl",
       L"Cache-Control directive: '%S'", content);

  int priority = 0;
  static const int maxAgePriority = 0;
  static const int sMaxAgePriority = 1;
  static const int cacheControlPriority = 2;

  SBinetNString attrib;
  SBinetNString val;

  for (;;)
  {
    content = SBinetHttpUtils::getValue(content, attrib);

    if (content == NULL)
    {
      Diag(MODULE_SBINET_VALIDATOR_TAGID,
           L"SBinetValidator::checkCacheControl",
           L"Could not get value.");
      break;
    }

    content = SBinetHttpUtils::expectChar(content, "=,");

    if (content == NULL)
    {
      Diag(MODULE_SBINET_VALIDATOR_TAGID,
           L"SBinetValidator::checkCacheControl",
           L"Expecting '=' or ','.");
      break;
    }

    const char *attribute = attrib.c_str();
    const char *value = NULL;

    if (*content == '=')
    {
      content = SBinetHttpUtils::getValue(++content, val);
      if (content == NULL)
      {
        Diag(MODULE_SBINET_VALIDATOR_TAGID,
             L"SBinetValidator::checkCacheControl",
             L"Could not extract value for attribute",
             attribute);
        break;
      }
      value = val.c_str();
    }

    Diag(MODULE_SBINET_VALIDATOR_TAGID, L"SBinetValidator::checkCacheControl",
         L"attribute = '%S', value = '%S'", attribute, value);

    if (strcasecmp(attribute, "no-cache") == 0)
    {
      // Ignore no-cache="something" as we have one cookie jar per channel.
      // So they are not shared.
      if (value == NULL && priority <= cacheControlPriority)
      {
        maxAge = (time_t) 0;
        priority = cacheControlPriority;
      }
    }
    else if ((strcasecmp(attribute, "no-store") == 0 ||
              strcasecmp(attribute, "private") == 0)
             && priority <= cacheControlPriority)

    {
      maxAge = (time_t) 0;
      priority = cacheControlPriority;
    }
    else if (strcasecmp(attribute, "must-revalidate") == 0)
    {
      // This directive does not affect priority nor does it depend on a
      // priority as it does not modify the maxAge value.
      _mustRevalidateF = true;
    }
    else if (strcasecmp(attribute, "max-age") == 0)
    {
      if (value != NULL &&  priority <= maxAgePriority)
      {
        maxAge = (time_t) atoi(value);
        priority = maxAgePriority;
      }
    }
    else if (strcasecmp(attribute, "s-maxage") == 0)
    {
      if (value != NULL && priority <= sMaxAgePriority)
      {
        maxAge = atoi(value);
        priority = sMaxAgePriority;
      }
    }

    content = SBinetHttpUtils::expectChar(content, ",");

    if (content == NULL)
    {
      Diag(MODULE_SBINET_VALIDATOR_TAGID,
           L"SBinetValidator::checkCacheControl",
           L"Expecting ','.");
      break;
    }

    if (!*content)
    {
      // end of string. No error.
      break;
    }

    content++;
  }

  return maxAge;
}

// Propagates data from an old validator to a new validator.
// For example, if an HTTP fetch returns 304 (Not-Modified), the
// validator constructed from the data returned may not contain
// all the validation data necessary.  Some of this data may need
// to be propagated from the old validator that was stored in the
// cache.
bool SBinetValidator::propagateDataFrom(const SBinetValidator& validator)
{
  bool changed = false;

  // Document Size in Bytes
  // When a 304 response is received from an HTTP server, we don't know what
  // the actual size of the resource/document is.  So, we set it to -1 knowing
  // that when we propagate values from the cached validator, this value gets
  // updated properly.  I don't like this, but the original coupled design
  // of INET leaves us no choice for now.
  if (_sizeBytes == -1)
  {
    _sizeBytes = validator._sizeBytes;
    changed = true;
  }

  // Pragma
  if ((_pragmaDirective == NULL) && (validator._pragmaDirective != NULL))
  {
    delete [] _pragmaDirective;
    _pragmaDirective = new VXIchar [::wcslen(validator._pragmaDirective) + 1];
    ::wcscpy(_pragmaDirective, validator._pragmaDirective);

    // Pragma directive overrides other directives, we can just return.
    computeFreshnessLifetime();
    return true;
  }

  // Cache-Control
  if ((_cacheControlDirective == NULL) && (validator._cacheControlDirective != NULL))
  {
    delete [] _cacheControlDirective;
    _cacheControlDirective = new VXIchar [::wcslen(validator._cacheControlDirective) + 1];
    ::wcscpy(_cacheControlDirective, validator._cacheControlDirective);
    changed = true;
  }

  // Expires
  if ((_expiresTime == (time_t) -1) && (validator._expiresTime != (time_t) -1))
  {
    _expiresTime = validator._expiresTime;
    // If a new expiration time is not set, and there was an old expiration time,
    // our new date time should be the same as the old date time.
    _dateTime = validator._dateTime;
    changed = true;
  }

  // Last-Modified
  if ((_lastModified == (time_t) -1) && (validator._lastModified != (time_t) -1))
  {
    _lastModified = validator._lastModified;
    changed = true;
  }

  // ETag
  if ((_eTag == NULL) && (validator._eTag != NULL))
  {
    delete [] _eTag;
    _eTag = new VXIchar [::wcslen(validator._eTag) + 1];
    ::wcscpy(_eTag, validator._eTag);
    changed = true;
  }

  // If some data was propagated from the other validator,
  // compute the new freshness lifetime based on the new data.
  if (changed)
  {
    computeFreshnessLifetime();
    return true;
  }

  return false;
}

// Checks whether the current validator's validator info matches that of
// the specified validator.  This is used when an external validator is used
// to verify whether that external validator matches the cached validator in
// cases where a conditional fetch on an HTTP server returns NOT_MODIFIED.
// In such cases, the cached validator says the current cached version of the
// document is up-to-date.  When the external validator does not match the
// cached validator, it means that the externally cached version of the
// document does not match the currently cached version.
bool SBinetValidator::operator==(const SBinetValidator& validator)
{
  if (_sizeBytes != validator._sizeBytes)
    return false;

  if (_lastModified != validator._lastModified)
    return false;

  if (!_eTag && validator._eTag)
    return false;
  if (_eTag && !validator._eTag)
    return false;
  if (_eTag && validator._eTag && wcscmp(_eTag, validator._eTag))
    return false;

  return true;
}

void SBinetValidator::Clear()
{
  if (_url)
  {
    delete [] _url;
    _url = NULL;
  }

  if (_eTag)
  {
    delete [] _eTag;
    _eTag = NULL;
  }

  if (_pragmaDirective)
  {
    delete [] _pragmaDirective;
    _pragmaDirective = NULL;
  }

  if (_cacheControlDirective)
  {
    delete [] _cacheControlDirective;
    _cacheControlDirective = NULL;
  }

  _sizeBytes = 0;
  _dateTime = _expiresTime = _lastModified = _refTime = _freshnessLifetime = (time_t)-1;
  _mustRevalidateF = false;
}
