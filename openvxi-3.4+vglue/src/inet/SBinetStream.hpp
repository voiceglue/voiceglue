
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

#ifndef SBINETSTREAM_H
#define SBINETSTREAM_H

#include "VXIinetStream.hpp"
#include "SWIutilLogger.hpp"
#include "SBinetURL.h"

typedef enum SBinetStreamType
{
  SBinetStream_FILE   = 0,
  SBinetStream_HTTP   = 1,
  SBinetStream_CACHE  = 2
} SBinetStreamType;

class SBinetStream: public VXIinetStream, public SWIutilLogger
{
 public:
  SBinetStream(SBinetURL *url, SBinetStreamType type, VXIlogInterface *log, VXIunsigned diagLogBase);

 public:
  virtual ~SBinetStream();

 public:
  const SBinetURL *getURL() const
  {
    return _url;
  }
  
  const SBinetStreamType getType() const
  {
    return _type;
  }

 public:
  virtual VXIinetResult Open(VXIint flags,
                             const VXIMap* properties,
                             VXIMap* streamInfo) = 0;

  virtual VXIinetResult Read(/* [OUT] */ VXIbyte*         pBuffer,
                             /* [IN]  */ VXIulong         nBuflen,
                             /* [OUT] */ VXIulong*        pnRead ) = 0;

  virtual VXIinetResult Write(/* [IN] */ const VXIbyte*   pBuffer,
                              /* [IN]  */ VXIulong         nBuflen,
                              /* [OUT] */ VXIulong*        pnWritten) = 0;

  virtual VXIinetResult Close() = 0;

 protected:
  void getCacheInfo(const VXIMap* properties, VXIint32& maxAge, VXIint32& maxStale);
  void getModInfo(const VXIMap *properties, time_t& lastModified, SBinetNString &etag);
  SBinetURL *_url;
  SBinetStreamType _type;
};

#endif
