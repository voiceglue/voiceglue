
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

/***********************************************************************
 *
 * Routines to handle caching of documents and resources.
 *
 ***********************************************************************/

#include "VXICacheStream.hpp"


VXICacheStream::VXICacheStream(VXIcacheInterface * cache,
                               const VXIbyte * buffer,
                               VXIulong bufferSize)
  : writable(false), bad(false), stream(NULL), cache(NULL)
{
  if (cache == NULL || buffer == NULL || bufferSize == 0) {
    bad = true;
    return;
  }

  VXIcacheStream * stream = NULL;
  VXIcacheResult rc = cache->OpenEx(cache, L"VXI", buffer, bufferSize,
                                    CACHE_MODE_READ_CREATE, CACHE_FLAG_NULL,
                                    NULL, NULL, &stream);

  switch (rc) {
  case VXIcache_RESULT_ENTRY_CREATED: // We have write permission.
    writable = true;
    break;
  case VXIcache_RESULT_SUCCESS: // Already in cache, just read it in.
    writable = false;
    break;
  default:
    bad = true;
    return;
  }

  this->cache = cache;
  this->stream = stream;
  return;
}


void VXICacheStream::CommitEntry()
{
  if (cache != NULL && stream != NULL && writable) {
    cache->CloseEx(cache, true, &stream);
    stream = NULL; // just in case
  }
}


void VXICacheStream::DiscardEntry()
{
  if (cache != NULL && stream != NULL && writable) {
    cache->CloseEx(cache, false, &stream);
    stream = NULL; // just in case
  }
}


VXICacheStream::~VXICacheStream()
{
  if (cache == NULL || stream == NULL) return;
  cache->CloseEx(cache, !writable, &stream);
}
