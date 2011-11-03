
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

#include "DocumentStorage.hpp"
#include "md5.h"

DocumentStorageSingleton * DocumentStorageSingleton::ds = NULL;
unsigned long DocumentStorageSingleton::maxBytes = 1024*1024; 

// ------*---------*---------*---------*---------*---------*---------*---------
// About GreedyDual caching
//
// This is a simple functions with the following desirable characteristics.
//  * Recently accessed items remain for a time
//  * Frequently accessed items tend to remain in cache
//  * Very few parameters are required and the computational cost is small
// 
// Initialize the limit, L, to 0.
// For every document, d:
//
//   1. If d is in memory, set H(p) = L + c(p)
//   2. If d is not in memory
//      a. While there is not room, set L to the lowest score and delete
//         documents having that score.
//      b. Store d in memory and set H(d) = L + c(p)
//
// ------*---------*---------*---------*---------*---------*---------*---------


void DocumentStorageSingleton::Initialize(unsigned int cacheSize)
{
  DocumentStorageSingleton::ds = new DocumentStorageSingleton(); 
  DocumentStorageSingleton::maxBytes = cacheSize * 1024; // max buffer allowed for memory cache
}

void DocumentStorageSingleton::Deinitialize()
{ delete DocumentStorageSingleton::ds; }

void DocumentStorageSingleton::Store(VXMLDocument & doc,
                                     const VXIbyte * buffer,
                                     VXIulong bufSize,
                                     const VXIchar * url)
{
    //  Disable memory cache
    return;

  // Handle the case where the memory cache is disabled.
  if (bufSize > maxBytes) return;

  // Generate key.
  DocumentStorageKey key = GenerateKey(buffer, bufSize, url);

  VXItrdMutexLock(mutex);

  DOC_STORAGE::iterator i = storage.find(key);
  // Cache does not exist, try to cache it
  if (i == storage.end()) {
    while ((totalBytes + bufSize > maxBytes) && !storage.empty()) {
      // Buffer is full.

      DOC_STORAGE::iterator minIterator = storage.begin();
      unsigned long minScore = (*minIterator).second.GetScore();

      // The algorithm needed to improve, right now it just find the smallest size
      for (DOC_STORAGE::iterator i = storage.begin(); i != storage.end(); ++i) {
        if ((*i).second.GetScore() > minScore) continue;
        minScore = (*i).second.GetScore();
        minIterator = i;
      }
      
      // remove the oldest entry that is found      
      if ( minIterator != storage.end() ) {
        // adjust current total bytes
        totalBytes -= (*minIterator).second.GetSize();
        storage.erase(minIterator);
      }

      // Note: In a two level cache, this document might be written to disk before
      // being flushed from memory.
    }
  
    storage[key] = DocumentStorageRecord(doc, bufSize, GreedyDualL);
    // Add size of the cache to keep track of total size of all entries
    totalBytes += bufSize;
  }

  VXItrdMutexUnlock(mutex);
}

/*
 *  Returns cached parsed document in doc if returns true
 */
bool DocumentStorageSingleton::Retrieve(VXMLDocument & doc,
                                        const VXIbyte * buffer,
                                        VXIulong bufSize,
                                        const VXIchar * url)
{
    //  Disable memory cache
    return false;

  // Handle the case where the memory cache is disabled.
  if (bufSize > maxBytes) return false;

  bool result = false;
  VXItrdMutexLock(mutex);
  DOC_STORAGE::iterator i = storage.find(GenerateKey(buffer, bufSize, url));
  if (i != storage.end()) {
    try {
      doc = (*i).second.GetDoc(GreedyDualL);
      result = true;
    } 
    catch (...) {
      // The cached document has been corrupted, remove it 
      // and adjust current total bytes
      totalBytes -= (*i).second.GetSize();
      storage.erase(i);
      result = false;        
    }
  }
  if (result == false) {
    // Note: In a two level cache scheme, this is where the data would be read
    //       back from disk.  In this one level implementation, no action is
    //       performed.
  }
  VXItrdMutexUnlock(mutex);

  return result;
}


DocumentStorageKey DocumentStorageSingleton::GenerateKey(const VXIbyte * buf,
                                                         VXIulong bufSize,
                                                         const VXIchar * url)
{
  // Create a MD5 digest for the key
  MD5_CTX md5;
  DocumentStorageKey temp;
  
  // Generate first key
  MD5Init (&md5);
  MD5Update (&md5, buf, bufSize);
  MD5Final (temp.raw, &md5);
  
  // Generate second key if applicable
  if( url != NULL ) {
    VXIulong urlLen = wcslen(url) * sizeof(VXIchar) / sizeof(VXIbyte);
    MD5Init (&md5);
    MD5Update (&md5, reinterpret_cast<const VXIbyte*>(url), urlLen);
    MD5Final (&(temp.raw[16]), &md5);       
  }
  return temp;
}
