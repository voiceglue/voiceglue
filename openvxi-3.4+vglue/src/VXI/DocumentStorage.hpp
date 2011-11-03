
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

#include "VXMLDocument.hpp"          // for VXMLDocument and document model
#include "VXItrd.h"                  // for ThreadMutex
#include <map>
#include <cstring>

class DocumentStorageKey {
public:
  DocumentStorageKey()
  { memset(raw, 0, 32 * sizeof(unsigned char)); }

  DocumentStorageKey(const DocumentStorageKey & x)
  { memcpy(raw, x.raw, 32 * sizeof(unsigned char)); }

  DocumentStorageKey & operator=(const DocumentStorageKey & x)
  { if (this != &x) { memcpy(raw, x.raw, 32 * sizeof(unsigned char)); }
    return *this; }

  unsigned char raw[32];
};


inline bool operator< (const DocumentStorageKey & x,
                       const DocumentStorageKey & y)
{ return memcmp(x.raw, y.raw, 32 * sizeof(unsigned char)) < 0; }

inline bool operator==(const DocumentStorageKey & x,
                       const DocumentStorageKey & y)
{ return memcmp(x.raw, y.raw, 32 * sizeof(unsigned char)) == 0; }

// ------*---------*---------*---------*---------*---------*---------*---------

class DocumentStorageRecord {
private:
  VXMLDocument  doc;   // compiled form of VXML document
  unsigned int  size;  // size of the cache
  unsigned long score; // cache score to keep track of the priority

public:
  VXMLDocument & GetDoc(unsigned long L)    { score = L + size; return doc; }
  unsigned long GetScore() const            { return score; }
  unsigned int  GetSize()  const            { return size; }

  DocumentStorageRecord(VXMLDocument & d, unsigned int s, unsigned long L)
    : doc(d), size(s), score(L + s) { }

  DocumentStorageRecord() : doc(0), size(0), score(0) { }

  DocumentStorageRecord(const DocumentStorageRecord & x)
  { doc = x.doc; size = x.size; score = x.score; }

  DocumentStorageRecord & operator=(const DocumentStorageRecord & x)
  { if (this != &x) { doc = x.doc; size = x.size; score = x.score; }
    return *this; }
};

// ------*---------*---------*---------*---------*---------*---------*---------

class DocumentStorageSingleton {
public:
  static void Initialize(unsigned int cacheSize);
  static void Deinitialize();

  static DocumentStorageSingleton * Instance()
  { return DocumentStorageSingleton::ds; }

  // when storing and retrieving the cache, we must take the abs. url into 
  // consideration because documents that has same content residing on different
  // ports should have their own cache.
  // i.e: http://abc/test.vxml and http://abc:8080/test.vxml
  // The issue is the base url is entirely different between 
  // "http://abc/" and "http://abc:8080/" 
  bool Retrieve(VXMLDocument & doc, const VXIbyte * buffer, VXIulong bufSize, const VXIchar * url);
  void Store(VXMLDocument & doc, const VXIbyte * buffer, VXIulong bufSize, const VXIchar * url);

private:
  DocumentStorageKey GenerateKey(const VXIbyte * buffer, VXIulong bufSize, const VXIchar * url);

private:
  typedef std::map<DocumentStorageKey, DocumentStorageRecord> DOC_STORAGE;
  DOC_STORAGE storage;
  unsigned long GreedyDualL;

  DocumentStorageSingleton()   { VXItrdMutexCreate(&mutex);  totalBytes = 0; 
                                 GreedyDualL = 0; }
  ~DocumentStorageSingleton()  { VXItrdMutexDestroy(&mutex); }

  VXItrdMutex * mutex;
  unsigned int totalBytes; // total size of all entries, must be less than maxBytes

  static unsigned long maxBytes; // max limit of memory cache, cannot exceed this limit
  static DocumentStorageSingleton * ds;
};
