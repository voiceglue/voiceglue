
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

#include "SBinetStream.hpp"
#include "SBinetUtils.hpp"
#include "SBinetValidator.h"
#include "SBinetLog.h"

SBinetStream::SBinetStream(SBinetURL *url, SBinetStreamType type, VXIlogInterface *log, VXIunsigned diagLogBase):
  SWIutilLogger(MODULE_SBINET, log, diagLogBase),
  _url(url), _type(type)
{}

SBinetStream::~SBinetStream()
{
  delete _url;
}

void SBinetStream::getCacheInfo(const VXIMap* properties, VXIint32& maxAge, VXIint32& maxStale)
{
  // Determine whether safe caching was requested (for proxy use)
  const VXIchar* cMode = SBinetUtils::getString(properties, INET_CACHING);

  if (cMode != NULL && ::wcscmp(cMode, INET_CACHING_SAFE) == 0)
  {
    maxStale = maxAge = 0;
  }
  else
  {
    maxStale = maxAge = -1;
    SBinetUtils::getInteger(properties,
                            INET_CACHE_CONTROL_MAX_AGE,
                            maxAge);

    SBinetUtils::getInteger(properties,
                            INET_CACHE_CONTROL_MAX_STALE,
                            maxStale);
  }
}

void SBinetStream::getModInfo(const VXIMap *properties,
                              time_t& lastModified, SBinetNString &etag)
{
  lastModified = (time_t) -1;
  etag.clear();

  // Determine attributes for conditional fetching.
  const VXIValue *val = VXIMapGetProperty(properties, INET_OPEN_IF_MODIFIED);

  if (val != NULL)
  {
    SBinetValidator validator(GetLog(), GetDiagBase());

    if (validator.Create(val) == VXIinet_RESULT_SUCCESS)
    {
      // Only keep the Etag if it is not a weak one.
      if (validator.isStrong())
        etag = validator.getETAG();

      lastModified = validator.getLastModified();
    }
  }
}
