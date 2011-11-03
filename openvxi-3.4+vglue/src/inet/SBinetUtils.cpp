
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

#include "SBinetInternal.h"

#include "SBinetUtils.hpp"
#include "SBinetValidator.h"
#include "md5.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>                  // for ftime( )/_ftime( )

bool SBinetUtils::getInteger(const VXIMap *theMap, const VXIchar *prop,
			     VXIint32& val)
{
  const VXIValue *value = VXIMapGetProperty(theMap, prop);
  if (value != NULL && VXIValueGetType(value) == VALUE_INTEGER)
  {
    val = VXIIntegerValue((const VXIInteger *) value);
    return true;
  }
#if (VXI_CURRENT_VERSION >= 0x00030000)
  if (value != NULL && VXIValueGetType(value) == VALUE_ULONG)
  {
    val = (VXIint32)VXIULongValue((const VXIULong *) value);
    return true;
  }
#endif  
  return false;
}

bool SBinetUtils::getFloat(const VXIMap *theMap, const VXIchar *prop,
                           VXIflt32& val)
{
  const VXIValue *value = VXIMapGetProperty(theMap, prop);
  if (value != NULL && VXIValueGetType(value) == VALUE_FLOAT)
  {
    val = VXIFloatValue((const VXIFloat *) value);
    return true;
  }
#if (VXI_CURRENT_VERSION >= 0x00030000)
  if (value != NULL && VXIValueGetType(value) == VALUE_DOUBLE)
  {
    val = (VXIflt32)VXIDoubleValue((const VXIDouble *) value);
    return true;
  }
#endif  
  return false;
}

const VXIchar *SBinetUtils::getString(const VXIMap *theMap,
                                      const VXIchar *prop)
{
  const VXIValue *value = VXIMapGetProperty(theMap, prop);
  if (value != NULL && VXIValueGetType(value) == VALUE_STRING)
  {
    return VXIStringCStr((const VXIString *) value);
  }
  return NULL;
}

bool SBinetUtils::getContent(const VXIMap *theMap,
                             const VXIchar *prop,
                             const char *&content,
                             VXIulong &contentLength)
{
  const VXIValue *value = VXIMapGetProperty(theMap, prop);
  if (value != NULL && VXIValueGetType(value) == VALUE_CONTENT)
  {
    const VXIchar *mimeType;

    VXIvalueResult rc = VXIContentValue((const VXIContent *) value, &mimeType,
                                        (const VXIbyte **) &content,
					&contentLength);

    if (rc == VXIvalue_RESULT_SUCCESS && content != NULL)
    {
      // FIXME: should check for text/plain mimetype!
      return true;
    }
  }

  return false;
}

static void DestroyContent(VXIbyte **content, void *userdata)
{
  if (content)
  {
    delete [] (char *)(*content);
    *content = NULL;
  }
}

VXIContent *SBinetUtils::createContent(const char *str)
{
  int len = ::strlen(str);
  return VXIContentCreate(L"text/plain",
                          (VXIbyte *) ::strcpy(new char[len +1], str), len,
                          DestroyContent, NULL);
}

char *SBinetUtils::getTimeStampStr(time_t  timestamp,
				   char   *timestampStr)
{
#ifdef WIN32
  char *timeStr = ctime(&timestamp);
  if (timeStr)
    ::strcpy(timestampStr, timeStr);
  else
    ::strcpy(timestampStr, "<bad timestamp>");
#else
  char *timeStr = ctime_r(&timestamp, timestampStr);
  if (! timeStr)
    ::strcpy(timestampStr, "<bad timestamp>");
#endif

  return timestampStr;
}

char *SBinetUtils::getTimeStampMsecStr(char  *timestampStr)
{
#ifdef WIN32
  struct _timeb tbuf;
  _ftime(&tbuf);
  char *timeStr = ctime(&tbuf.time);
#else
  struct timeb tbuf;
  ftime(&tbuf);
  char timeStr_r[64] = "";
  char *timeStr = ctime_r(&tbuf.time, timeStr_r);
#endif

  if (timeStr) {
    // Strip the weekday name from the front, year from the end,
    // append hundredths of a second (the thousandths position is
    // inaccurate, remains constant across entire runs of the process)
    ::strncpy(timestampStr, &timeStr[4], 15);
    ::sprintf(&timestampStr[15], ".%02u", tbuf.millitm / 10);
  } else {
    ::strcpy(timestampStr, "<bad timestamp>");
  }

  return timestampStr;
}

bool SBinetUtils::setValidatorInfo(VXIMap *streamInfo, const SBinetValidator *validator)
{
  // Build caching information required to put in the streamInfo map.  This
  // consist of three properties:
  //
  // 1) INET_INFO_EXPIRES_HINT: indicates when a new request should be made to
  // the remote server.
  //
  // 2) INET_INFO_VALIDATOR_STRONG: indicates that the ETag provided by the
  // remote server is a strong one.
  //
  // 3) INET_INFO_VALIDATOR: the actual validator information as returned by
  // the remote server.

  if (validator->isStrong())
  {
    // We have a strong validator.
    if (VXIMapSetProperty(streamInfo, INET_INFO_VALIDATOR_STRONG, (VXIValue *) VXIIntegerCreate(TRUE)) !=
        VXIvalue_RESULT_SUCCESS)
      return false;
  }
  else
  {
    VXIMapDeleteProperty(streamInfo, INET_INFO_VALIDATOR_STRONG);
  }

  time_t expiresTimeStamp = validator->getExpiresTime();
  if (expiresTimeStamp < (time_t) 0) expiresTimeStamp = (time_t) 0;
  if (VXIMapSetProperty(streamInfo, INET_INFO_EXPIRES_HINT, (VXIValue *) VXIIntegerCreate(expiresTimeStamp)) !=
      VXIvalue_RESULT_SUCCESS)
    return false;


  return VXIMapSetProperty(streamInfo, INET_INFO_VALIDATOR, (VXIValue *) validator->serialize()) ==
    VXIvalue_RESULT_SUCCESS;
}


//
// MD5 genaration implementation
//
SBinetMD5::SBinetMD5()
{
  memset(raw, 0, 16 * sizeof(unsigned char));  
}

SBinetMD5::SBinetMD5(const VXIchar* url)
{
  memset(raw, 0, 16 * sizeof(unsigned char));  
  GenMD5Key(url);
}
  
SBinetMD5::SBinetMD5(const SBinetMD5 & x)
{
  memcpy(raw, x.raw, 16 * sizeof(unsigned char));   
}

SBinetMD5 & SBinetMD5::operator=(const SBinetMD5 & x)
{
  if (this != &x) 
  { 
    memcpy(raw, x.raw, 16 * sizeof(unsigned char)); 
  }
  return *this;  
}
  
bool SBinetMD5::GenMD5Key(const VXIchar* content)
{
  if( !content ) return false;  
  // Generate MD5 key
  MD5_CTX md5;
  VXIulong cntLen = wcslen(content) * sizeof(VXIchar) / sizeof(VXIbyte);
  MD5Init (&md5);
  MD5Update (&md5, reinterpret_cast<const VXIbyte*>(content), cntLen);
  MD5Final (raw, &md5);           
  return true;  
}

void SBinetMD5::Clear()
{
  memset(raw, 0, 16 * sizeof(unsigned char));  
}

bool SBinetMD5::IsClear() const
{
  for(int i = 0; i <= 16; ++i)
    if( raw[i] != 0 ) return false;
  return true;  
}
