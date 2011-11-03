
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

#include "SBcacheInternal.h"

#define SBCACHE_EXPORTS
#include "SBcache.h"           // Init/Shutdown/Create/Destroy

#include "SBcacheManager.hpp"  // For SBcacheManager
#include "SBcacheLog.h"        // For logging
#include "base64.h"            // base64 encoding functions
#include "md5.h"               // MD5 digest functions

#define MAX_CACHE_KEY   256

/*****************************************************************************/
// Static definitions and functions
/*****************************************************************************/

static VXIunsigned      gblDiagTagBase = 0;
static SBcacheManager  *gblCacheMgr = NULL;

// Static class to ensure the cache index file is updated even if
// SBcacheShutDown( ) is not called
class SBcacheTerminate {
public:
  SBcacheTerminate( ) { }
  ~SBcacheTerminate( );
};

static SBcacheTerminate gblCacheTerminate;

SBcacheTerminate::~SBcacheTerminate( )
{
  if (gblCacheMgr) {
    // Disable logging, the log resource is probably gone by now so
    // trying to use it as will be done at shutdown causes a crash
    gblCacheMgr->ClearLogResource( );

    // Write out the index
    gblCacheMgr->WriteIndex( );

    // NOTE: Do not delete the cache manager here, doing so may result
    // in crashes/asserts due to deleting mutexes that are active
  }
}


/*****************************************************************************/
// Cache Interface Class, one per channel
/*****************************************************************************/

class SBcacheInterface : public VXIcacheInterface, public SBinetLogger
{
public:
  SBcacheInterface(VXIlogInterface *log, VXIunsigned diagTagBase);
  virtual ~SBcacheInterface( ) { }

  /****************************************/
  // Static functions for mapping from C to C++
  /****************************************/
  static    VXIint32       GetVersion(void);

  static    const VXIchar* GetImplementationName(void);

  static    VXIcacheResult CallOpen(struct VXIcacheInterface *pThis,
				    const VXIchar *moduleName,
				    const VXIchar *key,
				    VXIcacheOpenMode mode,
				    VXIint32 flags,
				    const VXIMap *properties,
				    VXIMap *streamInfo,
				    VXIcacheStream **stream);

  static    VXIcacheResult CallClose(struct VXIcacheInterface *pThis,
				     VXIcacheStream **stream);

  static    VXIcacheResult CallUnlock(struct VXIcacheInterface *pThis,
				      const VXIchar *key);
    
  static    VXIcacheResult CallRead(struct VXIcacheInterface *pThis,
				    VXIbyte   *buffer,
				    VXIulong  buflen,
				    VXIulong  *nread,
				    VXIcacheStream *stream);
    
  static    VXIcacheResult CallWrite(struct VXIcacheInterface *pThis,
				     const VXIbyte *buffer,
				     VXIulong buflen,
				     VXIulong *nwritten,
				     VXIcacheStream *stream);

  static    VXIcacheResult CallOpenEx(struct VXIcacheInterface *pThis,
				      const VXIchar *moduleName,
				      const VXIbyte *key,
				      VXIulong keySizeBytes,
				      VXIcacheOpenMode mode,
				      VXIint32 flags,
				      const VXIMap *properties,
				      VXIMap *streamInfo,
				      VXIcacheStream **stream);

  static    VXIcacheResult CallCloseEx(struct VXIcacheInterface *pThis,
				       VXIbool keepEntry,
				       VXIcacheStream **stream);
  
  static    VXIcacheResult CallUnlockEx(struct VXIcacheInterface *pThis,
					const VXIbyte *key,
					VXIulong keySizeBytes);
    
private:
  /****************************************/
  // The real implementations
  /****************************************/
  VXIcacheResult Open(const VXIchar *moduleName,
		      const VXIchar *key,
		      VXIcacheOpenMode mode,
		      VXIint32 flags,
		      const VXIMap *properties,
		      VXIMap *streamInfo,
		      VXIcacheStream **stream);

  VXIcacheResult Open(const VXIchar *moduleName,
		      const VXIbyte *key,
		      VXIulong keySizeBytes,
		      VXIcacheOpenMode mode,
		      VXIint32 flags,
		      const VXIMap *properties,
		      VXIMap *streamInfo,
		      VXIcacheStream **stream);

  VXIcacheResult Close(VXIbool keepEntry,
		       VXIcacheStream **stream);

  VXIcacheResult Unlock(const VXIchar *key);

  VXIcacheResult Unlock(const VXIbyte *key,
			VXIulong keySizeBytes);
    
  VXIcacheResult Read(VXIbyte *buffer,
		      VXIulong buflen,
		      VXIulong *nread,
		      VXIcacheStream *stream);
    
  VXIcacheResult Write(const VXIbyte *buffer,
		       VXIulong buflen,
		       VXIulong *nwritten,
		       VXIcacheStream *stream);

  /************************************/
  // Utility methods
  /************************************/
  SBcacheKey MakeCacheKey(const VXIbyte *buf, VXIunsigned bufSizeBytes) const;
  SBcacheKey MakeCacheKey(const VXIchar *buf) const;
};


SBcacheInterface::SBcacheInterface(VXIlogInterface *log, 
				   VXIunsigned diagTagBase) :
  SBinetLogger(MODULE_SBCACHE, log, diagTagBase)
{
  VXIcacheInterface::GetVersion = SBcacheInterface::GetVersion;
  VXIcacheInterface::GetImplementationName = 
    SBcacheInterface::GetImplementationName;
  
  VXIcacheInterface::Open     = SBcacheInterface::CallOpen;
  VXIcacheInterface::Close    = SBcacheInterface::CallClose;
  VXIcacheInterface::Unlock   = SBcacheInterface::CallUnlock;
  VXIcacheInterface::Read     = SBcacheInterface::CallRead;
  VXIcacheInterface::Write    = SBcacheInterface::CallWrite;
  VXIcacheInterface::OpenEx   = SBcacheInterface::CallOpenEx;
  VXIcacheInterface::CloseEx  = SBcacheInterface::CallCloseEx;
  VXIcacheInterface::UnlockEx = SBcacheInterface::CallUnlockEx;
}


VXIcacheResult
SBcacheInterface::Open(const VXIchar *moduleName,
                       const VXIchar *key,
                       VXIcacheOpenMode mode,
                       VXIint32 flags,
                       const VXIMap *properties,
                       VXIMap *streamInfo,
                       VXIcacheStream **stream )
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  
  if (( ! key ) || ( ! key[0] ) || ( ! stream ) ||
      ( mode > CACHE_MODE_READ_CREATE )) {
    Error (200, NULL);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  } else {
    // Create the final cache key
    SBcacheKey finalKey = MakeCacheKey (key);
    if ( finalKey.length( ) == 0 ) {
      Error(100, NULL);
      rc = VXIcache_RESULT_OUT_OF_MEMORY;
    } else {
      // Open the cache entry
      SBcacheString finalModuleName;
      if ( moduleName )
	finalModuleName = moduleName;
      
      rc = gblCacheMgr->Open (GetLog( ), finalModuleName, finalKey, mode,
			      flags, properties, streamInfo, stream);
    }
  }
  
  return rc;
}


VXIcacheResult
SBcacheInterface::Open(const VXIchar *moduleName,
                       const VXIbyte *key,
		       VXIulong keySizeBytes,
                       VXIcacheOpenMode mode,
                       VXIint32 flags,
                       const VXIMap *properties,
                       VXIMap *streamInfo,
                       VXIcacheStream **stream )
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  
  if (( ! key ) || ( keySizeBytes < 1 ) || ( ! stream ) ||
      ( mode > CACHE_MODE_READ_CREATE )) {
    Error (200, NULL);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  } else {
    // Create the final cache key
    SBcacheKey finalKey = MakeCacheKey (key, keySizeBytes);
    if ( finalKey.length( ) == 0 ) {
      Error(100, NULL);
      rc = VXIcache_RESULT_OUT_OF_MEMORY;
    } else {
      // Open the cache entry
      SBcacheString finalModuleName;
      if ( moduleName )
	finalModuleName = moduleName;
      
      rc = gblCacheMgr->Open (GetLog( ), finalModuleName, finalKey, mode,
			      flags, properties, streamInfo, stream);
    }
  }
  
  return rc;
}


VXIcacheResult
SBcacheInterface::Close (VXIbool keepEntry,
			 VXIcacheStream **stream)
{ 
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  if (( ! stream ) || ( ! *stream )) {
    Error (201, NULL);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  } else {
    VXIcacheResult rc2;

    if ( ! keepEntry ) {
      // May have deleted the entry from the table already, so ignore
      // all non-fatal errors
      rc2 = gblCacheMgr->Delete (GetLog( ), (*stream)->GetKey( ), true);
      if ( rc2 < VXIcache_RESULT_SUCCESS )
	rc = rc2;
    }
    
    rc2 = (*stream)->Close (keepEntry ? false : true);
    if ( rc2 != VXIcache_RESULT_SUCCESS )
      rc = rc2;

    delete *stream;
    *stream = NULL;
  }

  return rc;
}


VXIcacheResult
SBcacheInterface::Unlock(const VXIchar *key)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  if (( ! key ) || ( ! key[0] )) {
    Error (202, NULL);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  } else {
    // Create the final cache key
    SBcacheKey finalKey = MakeCacheKey (key);
    if ( finalKey.length( ) == 0 ) {
      Error(100, NULL);
      rc = VXIcache_RESULT_OUT_OF_MEMORY;
    } else {
      // Unlock the entry
      rc = gblCacheMgr->Unlock (GetLog( ), finalKey);
    }
  }

  return rc;
}
    

VXIcacheResult
SBcacheInterface::Unlock(const VXIbyte *key,
			 VXIulong keySizeBytes)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  if (( ! key ) || ( keySizeBytes < 1 )) {
    Error (202, NULL);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  } else {
    // Create the final cache key
    SBcacheKey finalKey = MakeCacheKey (key, keySizeBytes);
    if ( finalKey.length( ) == 0 ) {
      Error(100, NULL);
      rc = VXIcache_RESULT_OUT_OF_MEMORY;
    } else {
      // Unlock the entry
      rc = gblCacheMgr->Unlock (GetLog( ), finalKey);
    }
  }

  return rc;
}
    

VXIcacheResult
SBcacheInterface::Read(VXIbyte        *buffer,
                       VXIulong        buflen,
                       VXIulong       *nread,
                       VXIcacheStream *stream )
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  if (( ! buffer ) || ( buflen < 1 ) || ( ! nread ) || ( ! stream )) {
    Error (203, NULL);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  } else {
    rc = stream->Read (buffer, buflen, nread);
  }

  return rc;
}


VXIcacheResult
SBcacheInterface::Write(const VXIbyte  *buffer,
                        VXIulong        buflen,
                        VXIulong       *nwritten,
                        VXIcacheStream *stream )
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  if (( ! buffer ) || ( buflen < 1 ) || ( ! nwritten ) || ( ! stream )) {
    Error (204, NULL);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  } else {
    rc = stream->Write (buffer, buflen, nwritten);

    if (( rc == VXIcache_RESULT_SUCCESS ) ||
	( rc == VXIcache_RESULT_WOULD_BLOCK ))
      gblCacheMgr->WriteNotification (GetLog( ), stream->GetModuleName( ),
				      *nwritten, stream->GetKey());
  }

  return rc;
}


/**
 * Use MD5 to generate a key off binary data
 */
SBcacheKey 
SBcacheInterface::MakeCacheKey(const VXIbyte *buf, 
			       VXIunsigned    bufSizeBytes) const
{
  // Create a MD5 digest for the key
  unsigned char digest[16];
  memset(digest, 0, 16 * sizeof(unsigned char));
  MD5_CTX md5;
  MD5Init (&md5);
  MD5Update (&md5, buf, bufSizeBytes);
  MD5Final (digest, &md5);

  // The final key is an ASCII string using a base64 encoding of the
  // binary MD5 digest
  VXIchar keyBuf[B64ELEN(16) + 1];
  keyBuf[B64ELEN(16)] = L'\0';
  wcsb64e (digest, 16, keyBuf);

  return keyBuf;
}


/**
 * Generate a key off wide character data
 */
SBcacheKey
SBcacheInterface::MakeCacheKey(const VXIchar *key) const
{
  // If the text is too long, create a MD5 based key
  size_t keyLen = ::wcslen (key);
  if ( keyLen > MAX_CACHE_KEY )
    return MakeCacheKey ((const VXIbyte *) key, keyLen * sizeof(*key));

  return key;
}


/********************************************************/
/* Map the static functions to the real implementations */
/********************************************************/
VXIint32 SBcacheInterface::GetVersion(void)
{
  return VXI_CURRENT_VERSION;
}

const VXIchar* SBcacheInterface::GetImplementationName(void)
{
  static const VXIchar IMPLEMENTATION_NAME[] = SBCACHE_IMPLEMENTATION_NAME;
  return IMPLEMENTATION_NAME;
}

VXIcacheResult
SBcacheInterface::CallOpen(struct VXIcacheInterface *pThis,
                           const VXIchar *moduleName,
                           const VXIchar *key,
                           VXIcacheOpenMode mode,
                           VXIint32 flags,
                           const VXIMap *properties,
                           VXIMap *streamInfo,
                           VXIcacheStream **stream )
{
  if (!pThis) return VXIcache_RESULT_INVALID_ARGUMENT;
  SBcacheInterface * me = static_cast<SBcacheInterface *>(pThis);

  const VXIchar *modeName;
  switch ( mode ) {
  case CACHE_MODE_READ:
    modeName = L"read";
    break;
  case CACHE_MODE_WRITE:
    modeName = L"write";
    break;
  case CACHE_MODE_READ_CREATE:
    modeName = L"readCreate";
    break;
  default:
    modeName = L"unknownMode";
    break;
  }
    
  me->Diag (SBCACHE_API_TAGID, L"Open", 
	    L"entering: 0x%p, %s, %0.256s, 0x%x [%s], 0x%x [%s%s%s], 0x%p, "
	    L"0x%p, 0x%p", pThis, moduleName, key, mode, modeName,
	    flags, ((flags & CACHE_FLAG_LOCK) ? L"lock, " : L""),
	    ((flags & CACHE_FLAG_LOCK_MEMORY) ? L"memory lock, " : L""),
	    ((flags & CACHE_FLAG_NONBLOCKING_IO) ? L"non-blocking I/O" : L""),
	    properties, streamInfo, stream);

  if (stream)
    *stream = NULL;
  VXIcacheResult rc = me->Open (moduleName, key, mode, flags, properties,
				streamInfo, stream);

  me->Diag (SBCACHE_API_TAGID, L"Open", L"exiting: %d, 0x%p", rc,
	    (stream ? *stream : NULL));
  return rc;
}


VXIcacheResult
SBcacheInterface::CallClose(struct VXIcacheInterface *pThis,
                            VXIcacheStream **stream )
{
  if(!pThis) return VXIcache_RESULT_INVALID_ARGUMENT;
  SBcacheInterface * me = static_cast<SBcacheInterface *>(pThis);
  
  me->Diag (SBCACHE_API_TAGID, L"Close", L"entering: 0x%p, 0x%p [0x%p]",
	    pThis, stream, (stream ? *stream : NULL));

  VXIcacheResult rc = me->Close (TRUE, stream);

  me->Diag (SBCACHE_API_TAGID, L"Close", L"exiting: %d", rc);
  return rc;
}

VXIcacheResult
SBcacheInterface::CallUnlock(struct VXIcacheInterface *pThis,
                             const VXIchar *key)
{
  if(!pThis) return VXIcache_RESULT_INVALID_ARGUMENT;
  SBcacheInterface * me = static_cast<SBcacheInterface *>(pThis);

  me->Diag (SBCACHE_API_TAGID, L"Unlock", L"entering: 0x%p, %0.256s",
	    pThis, key);

  VXIcacheResult rc = me->Unlock (key);

  me->Diag (SBCACHE_API_TAGID, L"Unlock", L"exiting: %d", rc);
  return rc;
}

VXIcacheResult
SBcacheInterface::CallRead(struct VXIcacheInterface *pThis,
                           VXIbyte   *buffer,
                           VXIulong   buflen,
                           VXIulong  *nread,
                           VXIcacheStream *stream )
{
  if(!pThis) return VXIcache_RESULT_INVALID_ARGUMENT;
  SBcacheInterface * me = static_cast<SBcacheInterface *>(pThis);
  
  me->Diag (SBCACHE_API_TAGID, L"Read", 
	    L"entering: 0x%p, 0x%p, %lu, 0x%p, 0x%p",
	    pThis, buffer, buflen, nread, stream);

  VXIcacheResult rc = me->Read(buffer, buflen, nread, stream );

  me->Diag (SBCACHE_API_TAGID, L"Read", L"exiting: %d, %lu", rc,
	    (nread ? *nread : 0));
  return rc;
}
    
VXIcacheResult
SBcacheInterface::CallWrite(struct VXIcacheInterface *pThis,
                            const VXIbyte *buffer,
                            VXIulong buflen,
                            VXIulong *nwritten,
                            VXIcacheStream *stream )
{
  if(!pThis) return VXIcache_RESULT_INVALID_ARGUMENT;
  SBcacheInterface * me = static_cast<SBcacheInterface *>(pThis);

  me->Diag (SBCACHE_API_TAGID, L"Write", 
	    L"entering: 0x%p, 0x%p, %lu, 0x%p, 0x%p",
	    pThis, buffer, buflen, nwritten, stream);
  
  VXIcacheResult rc = me->Write (buffer, buflen, nwritten, stream );

  me->Diag (SBCACHE_API_TAGID, L"Write", L"exiting: %d, %lu", rc,
	    (nwritten ? *nwritten : 0));
  return rc;
}

VXIcacheResult
SBcacheInterface::CallOpenEx(struct VXIcacheInterface *pThis,
			     const VXIchar *moduleName,
			     const VXIbyte *key,
			     VXIulong keySizeBytes,
			     VXIcacheOpenMode mode,
			     VXIint32 flags,
			     const VXIMap *properties,
			     VXIMap *streamInfo,
			     VXIcacheStream **stream )
{
  if (!pThis) return VXIcache_RESULT_INVALID_ARGUMENT;
  SBcacheInterface * me = static_cast<SBcacheInterface *>(pThis);

  me->Diag (SBCACHE_API_TAGID, L"OpenEx", 
	    L"entering: 0x%p, %s, 0x%p, %lu, 0x%x [%s], 0x%x [%s%s%s], 0x%p, "
	    L"0x%p, 0x%p", pThis, moduleName, key, keySizeBytes, mode, 
	    (mode == CACHE_MODE_READ ? L"read" :
	     (mode == CACHE_MODE_WRITE ? L"write" : L"<unknown>")),
	    flags, ((flags & CACHE_FLAG_LOCK) ? L"lock, " : L""),
	    ((flags & CACHE_FLAG_LOCK_MEMORY) ? L"memory lock, " : L""),
	    ((flags & CACHE_FLAG_NONBLOCKING_IO) ? L"non-blocking I/O" : L""),
	    properties, streamInfo, stream);
  
  VXIcacheResult rc = me->Open (moduleName, key, keySizeBytes, mode, flags,
				properties, streamInfo, stream);

  me->Diag (SBCACHE_API_TAGID, L"OpenEx", L"exiting: %d, 0x%p", rc,
	    (stream ? *stream : NULL));
  return rc;
}


VXIcacheResult
SBcacheInterface::CallCloseEx(struct VXIcacheInterface *pThis,
			      VXIbool keepEntry,
			      VXIcacheStream **stream )
{
  if(!pThis) return VXIcache_RESULT_INVALID_ARGUMENT;
  SBcacheInterface * me = static_cast<SBcacheInterface *>(pThis);
  
  me->Diag (SBCACHE_API_TAGID, L"CloseEx", L"entering: 0x%p, %s, 0x%p [0x%p]",
	    pThis, (keepEntry ? L"TRUE" : L"FALSE"), stream, 
	    (stream ? *stream : NULL));

  VXIcacheResult rc = me->Close (keepEntry, stream);

  me->Diag (SBCACHE_API_TAGID, L"CloseEx", L"exiting: %d", rc);
  return rc;
}

VXIcacheResult
SBcacheInterface::CallUnlockEx(struct VXIcacheInterface *pThis,
			       const VXIbyte *key,
			       VXIulong keySizeBytes)
{
  if(!pThis) return VXIcache_RESULT_INVALID_ARGUMENT;
  SBcacheInterface * me = static_cast<SBcacheInterface *>(pThis);

  me->Diag (SBCACHE_API_TAGID, L"UnlockEx", L"entering: 0x%p, 0x%p, %lu",
	    pThis, key, keySizeBytes);

  VXIcacheResult rc = me->Unlock (key, keySizeBytes);

  me->Diag (SBCACHE_API_TAGID, L"UnlockEx", L"exiting: %d", rc);
  return rc;
}


/********************************************/
/* Public Interface DLL Functions           */
/********************************************/

SBCACHE_API
VXIcacheResult SBcacheInit( VXIlogInterface    *log,
                            const VXIunsigned   diagTagBase,
                            const VXIchar      *pCacheFolder,
                            const VXIint        nCacheTotalSizeMB,
                            const VXIint        nCacheEntryMaxSizeMB,
                            const VXIint        nCacheEntryExpTimeSec,
                            VXIbool             fUnlockEntries,
                            const VXIint        nCacheLowWaterMB)
{
  if (log == NULL)
    return VXIcache_RESULT_INVALID_ARGUMENT;

  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (log, diagTagBase + SBCACHE_API_TAGID, 
			  L"SBcacheInit", (int *) &rc, 
			  L"entering: 0x%p, %u, %s, %d, %d, %d, %d, %d",
			  log, diagTagBase, pCacheFolder, 
			  nCacheTotalSizeMB, nCacheEntryMaxSizeMB,
			  nCacheEntryExpTimeSec, fUnlockEntries,
			  nCacheLowWaterMB);
  
  // Avoid double initialization
  if ( gblCacheMgr ) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 101, NULL);
    return (rc = VXIcache_RESULT_SUCCESS);
  }

  // Copy over globals
  gblDiagTagBase = diagTagBase;

  // Check the remaining arguments
  if ((! pCacheFolder) || (! pCacheFolder[0])) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 102, NULL);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  } else {
    // Ensure we only have 8 bit characters for the folder name
    int i;
    for (i = 0; 
	 (pCacheFolder[i] != L'\0') && ((pCacheFolder[i] & 0xff00) == 0); i++)
      ; /* keep going */
    
    if ( pCacheFolder[i] != L'\0' ) {
      SBinetLogger::Error (log, MODULE_SBCACHE, 103, L"%s%s",
			   L"configured", pCacheFolder);
      rc = VXIcache_RESULT_INVALID_ARGUMENT;
    }
  }

  if ( nCacheTotalSizeMB < 0 ) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 104, L"%s%d", L"configured",
			 nCacheTotalSizeMB);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  }

  if ( nCacheEntryMaxSizeMB < 0 ) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 105, L"%s%d", L"configured",
			 nCacheEntryMaxSizeMB);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  }

  if ( nCacheEntryExpTimeSec <= 0 ) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 106, L"%s%d", L"configured",
			 nCacheEntryExpTimeSec);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  }

  if ( nCacheEntryMaxSizeMB > nCacheTotalSizeMB ) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 107, L"%s%d%s%d", 
			 L"entryMaxSizeMB", nCacheEntryMaxSizeMB, 
			 L"totalSizeMB", nCacheTotalSizeMB);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  }

  if ( nCacheLowWaterMB > nCacheTotalSizeMB ) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 107, L"%s%d%s%d", 
			 L"cacheLowWaterMB", nCacheLowWaterMB,
			 L"totalSizeMB", nCacheTotalSizeMB);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  }

  // Create the cache manager
  if ( rc == VXIcache_RESULT_SUCCESS ) {
    gblCacheMgr = new SBcacheManager (log, diagTagBase);
    if (! gblCacheMgr) {
      SBinetLogger::Error (log, MODULE_SBCACHE, 100, NULL);    
      rc = VXIcache_RESULT_OUT_OF_MEMORY;
    } else {
      SBcacheNString nCacheFolderStr;
      for (int i = 0; pCacheFolder[i] != L'\0'; i++)
	nCacheFolderStr += SBinetW2C (pCacheFolder[i]);
      
      rc = gblCacheMgr->Create (nCacheFolderStr, nCacheTotalSizeMB * 1000000, 
				nCacheEntryMaxSizeMB * 1000000, 
				nCacheEntryExpTimeSec, fUnlockEntries,
                                nCacheLowWaterMB * 1000000);
      
      if ( rc != VXIcache_RESULT_SUCCESS ) {
	// Error already reported
	delete gblCacheMgr;
	gblCacheMgr = NULL;
      }
    }
  }

  return rc;
}


SBCACHE_API
VXIcacheResult SBcacheShutDown( VXIlogInterface *log )
{
  if ( !log )
    return VXIcache_RESULT_INVALID_ARGUMENT;

  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (log, gblDiagTagBase + SBCACHE_API_TAGID, 
			  L"SBcacheShutDown", (int *) &rc, 
			  L"entering: 0x%p", log);

  // Destroy the cache manager
  if (! gblCacheMgr) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 205, NULL);
    rc = VXIcache_RESULT_NON_FATAL_ERROR;
  } else {
    delete gblCacheMgr;
    gblCacheMgr = NULL;
  }

  return rc;
}


SBCACHE_API
VXIcacheResult SBcacheCreateResource( VXIlogInterface    *log,
                                      VXIcacheInterface **ppVXIcache)  
{
  if ( !log )
    return VXIcache_RESULT_INVALID_ARGUMENT;

  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (log, gblDiagTagBase + SBCACHE_API_TAGID, 
			  L"SBcacheCreateResource", (int *) &rc, 
			  L"entering: 0x%p, 0x%p", log, ppVXIcache);
  
  // Create the cache interface
  if ( ! ppVXIcache ) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 108, NULL);
    rc = VXIcache_RESULT_INVALID_ARGUMENT;
  } else {
    *ppVXIcache = new SBcacheInterface (log, gblDiagTagBase);
    if ( ! *ppVXIcache ) {
      SBinetLogger::Error (log, MODULE_SBCACHE, 100, NULL);
      rc = VXIcache_RESULT_OUT_OF_MEMORY;
    }
  }

  return rc;
}


SBCACHE_API
VXIcacheResult SBcacheDestroyResource( VXIcacheInterface **ppVXIcache )
{
  if (( ! ppVXIcache ) || ( ! *ppVXIcache ))
    return VXIcache_RESULT_INVALID_ARGUMENT;

  SBcacheInterface * me = static_cast<SBcacheInterface *>(*ppVXIcache);
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  SBinetLogFunc apiTrace (me->GetLog( ), 
			  me->GetDiagBase( ) + SBCACHE_API_TAGID,
			  L"Read", (int *) &rc, L"entering: 0x%p [0x%p]", 
			  ppVXIcache, *ppVXIcache);

  // Destroy the cache interface
  delete me;
  *ppVXIcache = NULL;
  
  return rc;
}
