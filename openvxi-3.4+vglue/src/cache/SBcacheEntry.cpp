
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


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#include "SBcacheEntry.hpp"   // For this class
#include "SBcacheMisc.hpp"    // For SBcacheReaderWriterMutex,
                              // SBcacheRefCount base class

#include "VXItrd.h"          // For VXItrcSleep()

#include <cstdio>            // For FILE, fopen( ), etc
#include <cerrno>            // For errno to report fopen( ) errors
#include <string.h>          // For strerror( )

#ifndef _win32_
#include <unistd.h>
#endif

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

// SBcacheEntryMutex, a RW mutex implemented over a single exclusive lock
// mutex.  We do this to save handles for the rw mutex.
// We cannot use a pool here, because we rely on
// cache entry locks being held in conflict with each other... a pool has
// a good chance of deadlocking.
//
class SBcacheEntryMutex {
public:
  SBcacheEntryMutex(SBcacheMutex* m) :
    _mutex(m), _nreaders(0), _writer(0), _writers_waiting(0) { }
  ~SBcacheEntryMutex() { }
  VXItrdResult Create(const wchar_t* n) { return VXItrd_RESULT_SUCCESS; }

  VXItrdResult Lock() const {
    VXItrdResult rc = _mutex->Lock();
    if (rc != VXItrd_RESULT_SUCCESS) return rc;
    ++_writers_waiting;
    while (_nreaders || _writer) {
      rc = _mutex->Unlock();
      if (rc != VXItrd_RESULT_SUCCESS) return rc;
      VXItrdThreadYield();
      rc = _mutex->Lock();
      if (rc != VXItrd_RESULT_SUCCESS) return rc;
    }
    --_writers_waiting;
    ++_writer;
    rc = _mutex->Unlock();
    if (rc != VXItrd_RESULT_SUCCESS) return rc;
    return VXItrd_RESULT_SUCCESS;
  }
  VXItrdResult Unlock() const {
    VXItrdResult rc = _mutex->Lock();
    if (rc != VXItrd_RESULT_SUCCESS) return rc;
    --_writer;
    rc = _mutex->Unlock();
    if (rc != VXItrd_RESULT_SUCCESS) return rc;
    return VXItrd_RESULT_SUCCESS;
  }

  VXItrdResult StartRead() const {
    VXItrdResult rc = _mutex->Lock();
    if (rc != VXItrd_RESULT_SUCCESS) return rc;
    // i.e. while a writer holds the lock _or_ is waiting for it
    while (_writer || _writers_waiting) {
      rc = _mutex->Unlock();
      if (rc != VXItrd_RESULT_SUCCESS) return rc;
      VXItrdThreadYield();
      rc = _mutex->Lock();
      if (rc != VXItrd_RESULT_SUCCESS) return rc;
    }
    ++_nreaders;
    rc = _mutex->Unlock();
    if (rc != VXItrd_RESULT_SUCCESS) return rc;
    return VXItrd_RESULT_SUCCESS;
  }
  VXItrdResult EndRead() const {
    VXItrdResult rc = _mutex->Lock();
    if (rc != VXItrd_RESULT_SUCCESS) return rc;
    --_nreaders;
    rc = _mutex->Unlock();
    if (rc != VXItrd_RESULT_SUCCESS) return rc;
    return VXItrd_RESULT_SUCCESS;
  }
private:
  SBcacheMutex*  _mutex;
  mutable short  _nreaders;
  mutable short  _writer;
  mutable short  _writers_waiting;
};

// SBcacheStream, implementation of VXIcacheStream
class SBcacheStream : public VXIcacheStream {
public:
  // Constructor and destructor
  SBcacheStream(VXIlogInterface *log, VXIunsigned diagTagBase,
		const SBcacheString &moduleName, const SBcacheKey &key,
		SBcacheEntry &entry, VXIcacheOpenMode mode, bool blockingIO,
		VXIulong maxSizeBytes, FILE *filePtr) :
    VXIcacheStream(log, diagTagBase, moduleName, key), _entry(entry),
    _mode(mode), _blockingIO(blockingIO), _maxSizeBytes(maxSizeBytes),
    _filePtr(filePtr), _sizeBytes(0) { }
  virtual ~SBcacheStream( ) { if ( _filePtr ) Close(false); }

  // Read and write
  virtual VXIcacheResult Read(VXIbyte          *buffer,
			      VXIulong          buflen,
			      VXIulong         *nread);

  virtual VXIcacheResult Write(const VXIbyte   *buffer,
			       VXIulong         buflen,
			       VXIulong        *nwritten);

  // Close
  virtual VXIcacheResult Close(bool invalidate);

private:
  SBcacheStream(const SBcacheStream &s);
  const SBcacheStream &operator=(const SBcacheStream &s);

private:
  SBcacheEntry      _entry;
  VXIcacheOpenMode  _mode;
  bool              _blockingIO;
  VXIulong          _maxSizeBytes;
  FILE             *_filePtr;
  VXIunsigned       _sizeBytes;
};


// Read from the stream
VXIcacheResult SBcacheStream::Read(VXIbyte          *buffer,
				   VXIulong          buflen,
				   VXIulong         *nread)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  *nread = 0;

  if ( _mode != CACHE_MODE_READ ) {
    Error (206, NULL);
    rc = VXIcache_RESULT_IO_ERROR;
  } else if ( ! _filePtr ) {
    Error (207, NULL);
    rc = VXIcache_RESULT_FATAL_ERROR;
  }

  // Read, for blocking I/O read in a loop. Note this is a lame
  // implementation of non-blocking I/O, without doing a fctl( ) or
  // open( ) this will almost always block, but it is very OS
  // dependant and tricky to do real non-blocking I/O.
  if ( rc == VXIcache_RESULT_SUCCESS ) {
    size_t read;
    do {
      read = fread (&buffer[*nread], 1, (size_t) (buflen - *nread), _filePtr);
      if ( read > 0 )
	*nread += read;
    } while (( _blockingIO ) && ( *nread < buflen ) &&
	     (( read > 0 ) || ( ! feof (_filePtr) )));

    if ( *nread == 0 ) {
      if ( ferror (_filePtr) ) {
	_entry.LogIOError (208);
	rc = VXIcache_RESULT_IO_ERROR;
      } else if ( feof (_filePtr) ) {
	rc = VXIcache_RESULT_END_OF_STREAM;
      } else {
	rc = VXIcache_RESULT_WOULD_BLOCK;
      }
    }
  }

  Diag (SBCACHE_STREAM_TAGID, L"Read",
	L"%s, %S: %lu bytes, %lu requested, rc = %d",
	_entry.GetKey( ).c_str( ), _entry.GetPath( ).c_str( ), buflen,
	*nread, rc);
  return rc;
}


// Write to the stream
VXIcacheResult SBcacheStream::Write(const VXIbyte   *buffer,
				    VXIulong         buflen,
				    VXIulong        *nwritten)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  *nwritten = 0;

  if ( _mode != CACHE_MODE_WRITE ) {
    Error (209, NULL);
    rc = VXIcache_RESULT_IO_ERROR;
  } else if ( ! _filePtr ) {
    Error (210, NULL);
    rc = VXIcache_RESULT_FATAL_ERROR;
  } else if ( _sizeBytes + buflen > _maxSizeBytes ) {
    rc = VXIcache_RESULT_EXCEED_MAXSIZE;
  }

  // Write, for blocking I/O write in a loop. Note this is a lame
  // implementation of non-blocking I/O, without doing a fctl( ) or
  // open( ) this will almost always block, but it is very OS
  // dependant and tricky to do real non-blocking I/O.
  if ( rc == VXIcache_RESULT_SUCCESS ) {
    size_t written;
    do {
      written = fwrite (&buffer[*nwritten], 1, (size_t) (buflen - *nwritten),
			_filePtr);
      if ( written > 0 ) {
	*nwritten += written;
	_sizeBytes += written;
      }
    } while (( _blockingIO ) && ( *nwritten < buflen ) &&
	     (( written > 0 ) || ( ! ferror (_filePtr) )));

    if ( *nwritten == 0 ) {
      if ( ferror (_filePtr) ) {
	_entry.LogIOError (212);
	rc = VXIcache_RESULT_IO_ERROR;
      } else {
	rc = VXIcache_RESULT_WOULD_BLOCK;
      }
    }
  }

  Diag (SBCACHE_STREAM_TAGID, L"Write",
	L"%s, %S: %lu bytes, %lu requested, rc = %d",
	_entry.GetKey( ).c_str( ), _entry.GetPath( ).c_str( ), buflen,
	*nwritten, rc);
  return rc;
}


// Close the stream
VXIcacheResult SBcacheStream::Close(bool invalidate)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  // Close the file
  if ( fclose (_filePtr) != 0 ) {
    _entry.LogIOError (300);
    rc = VXIcache_RESULT_IO_ERROR;
  }
  _filePtr = NULL;

  // Close the cache entry, do this after logging to ensure the entry
  // isn't deleted on us after the close
  VXIcacheResult rc2 = _entry.Close (GetLog( ), _mode, _sizeBytes, invalidate);
  if ( rc2 != VXIcache_RESULT_SUCCESS )
    rc = rc2;

  return rc;
}


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// SBcacheEntryDetails, real reference counted cache entry
class SBcacheEntryDetails : public SBcacheRefCount, public SBinetLogger {
public:
  // Constructor and destructor
  SBcacheEntryDetails(VXIlogInterface *log, VXIunsigned diagTagBase,
		      SBcacheMutex *refCountMutex) :
    SBcacheRefCount(refCountMutex),
    SBinetLogger(MODULE_SBCACHE, log, diagTagBase),
    _rwMutex(refCountMutex),
    _creationCost(CACHE_CREATION_COST_DEFAULT), _lastModified(0),
    _lastAccessed(0), _flags(CACHE_FLAG_NULL), _key(), _path(), _sizeBytes(0),
    _fileExists(false), _invalidated(false) { }
  virtual ~SBcacheEntryDetails( );

  // Release the cache entry
  static VXItrdResult Release (SBcacheEntryDetails **rc) {
    return SBcacheRefCount::Release (reinterpret_cast<SBcacheRefCount **>(rc)); }

  // Create the entry
  VXIcacheResult Create( );

  // Open the entry
  VXIcacheResult Open(VXIlogInterface       *log,
		      const SBcacheString   &moduleName,
		      const SBcacheKey      &key,
		      const SBcachePath     &path,
		      VXIcacheOpenMode       mode,
		      VXIint32               flags,
		      VXIulong               maxSizeBytes,
		      const VXIMap          *properties,
		      VXIMap                *streamInfo,
		      VXIcacheStream       **stream);

  // Unlock the entry
  VXIcacheResult Unlock(VXIlogInterface  *log);

  // Accessors
  bool IsLocked( ) const {
    return ((_flags & (CACHE_FLAG_LOCK | CACHE_FLAG_LOCK_MEMORY)) != 0); }

  bool IsExpired (time_t cutoffTime, time_t *lastAccessed) const {
    bool expired = (( ! IsLocked( ) ) && ( _lastAccessed < cutoffTime ));
    Diag (SBCACHE_ENTRY_TAGID, L"IsExpired", L"%s, %S, %s (%d %lu %lu)",
          _key.c_str( ), _path.c_str( ), (expired ? L"true" : L"false"),
	  (int) IsLocked( ), (unsigned long) _lastAccessed,
    	  (unsigned long) cutoffTime);
    *lastAccessed = _lastAccessed;
    return expired;
  }

  const SBcacheKey &GetKey( ) const { return _key; }
  const SBcachePath &GetPath( ) const { return _path; }

  VXIulong GetSizeBytes (bool haveEntryOpen) const {
    VXIulong size = 0;
    if (( haveEntryOpen ) ||
	( _rwMutex.StartRead( ) == VXItrd_RESULT_SUCCESS )) {
      size = _sizeBytes;
      if ( ! haveEntryOpen )
	_rwMutex.EndRead( );
    }
    return size;
  }

  // Error logging
  VXIlogResult LogIOError (VXIunsigned errorID) const {
    return Error (errorID, L"%s%S%s%S%s%d", L"path", _path.c_str( ),
		  L"errnoStr", strerror(errno), L"errno", errno);
  }
  VXIlogResult LogIOError (VXIlogInterface *log,
			   VXIunsigned errorID,
			   const SBcachePath &path) const {
    return Error (log, errorID, L"%s%S%s%S%s%d", L"path", path.c_str( ),
		  L"errnoStr", strerror(errno), L"errno", errno);
  }

  // Comparison operators, smaller is defined as a preference for
  // deleting this entry first, equality is having equal preference
  bool operator< (const SBcacheEntryDetails &entry) const {
    return ((( (! IsLocked( )) && (entry.IsLocked( )) ) ||
	     ( _creationCost < entry._creationCost ) ||
	     ( _lastAccessed < entry._lastAccessed ) ||
	     ( _sizeBytes < entry._sizeBytes )) ? true : false);
  }
  bool operator> (const SBcacheEntryDetails &entry) const {
    return ((( (IsLocked( )) && (! entry.IsLocked( )) ) ||
	     ( _creationCost > entry._creationCost ) ||
	     ( _lastAccessed > entry._lastAccessed ) ||
	     ( _sizeBytes > entry._sizeBytes )) ? true : false);
  }
  bool operator== (const SBcacheEntryDetails &entry) const {
    return ((( IsLocked( ) == entry.IsLocked( ) ) &&
	     ( _creationCost == entry._creationCost ) &&
	     ( _lastAccessed == entry._lastAccessed ) &&
	     ( _sizeBytes == entry._sizeBytes )) ? true : false);
  }
  bool operator!= (const SBcacheEntryDetails &entry) const {
    return ((( IsLocked( ) != entry.IsLocked( ) ) ||
	     ( _creationCost != entry._creationCost ) ||
	     ( _lastAccessed != entry._lastAccessed ) ||
	     ( _sizeBytes != entry._sizeBytes )) ? true : false);
  }

public:
  // Only for SBcacheStream use

  // Close
  VXIcacheResult Close (VXIlogInterface  *log,
			VXIcacheOpenMode  mode,
			VXIunsigned       sizeBytes,
			bool              invalidate);

private:
  // Delete the on-disk entry, internal routine, no mutex held
  VXIcacheResult DeleteFile( );

  // Disable the copy constructor and assignment operator
  SBcacheEntryDetails(const SBcacheEntryDetails &entry);
  const SBcacheEntryDetails &operator=(const SBcacheEntryDetails &entry);

private:
  // Est base bytes: 76 + (wcslen(modname) + wcslen(key) + wcslen(path)) * 2
  //                 76 + (10 + 32 + 40) * 2 = 76 + 164 = 240
  //                 Linux: 76 + (*4)328 = 404
  //                 404 + 8(refcnt) + (4 + 4 + (12 * 10) (logger)) = 540
  SBcacheEntryMutex        _rwMutex;         // (12)
  VXIcacheCreationCost     _creationCost;    // (4)
  time_t                   _lastModified;    // (4)
  time_t                   _lastAccessed;    // (4)
  VXIint32                 _flags;           // (4)
  SBcacheKey               _key;             // (12 + wcslen(key) * 2)
  SBcachePath              _path;            // (12 + strlen(path))
  VXIunsigned              _sizeBytes;       // (4)
  bool                     _fileExists;      // (2?)
  bool                     _invalidated;     // (2?)
};


// Destructor
SBcacheEntryDetails::~SBcacheEntryDetails( )
{
  // Delete the on-disk file if appropriate
  // TBD make this conditional by removing the #if 0 once
  // persistant caching across process restarts (saving/loading
  // the index file) is implemented
#if 0
  if ( _invalidated )
#endif
    DeleteFile( );
}


// Create the entry
VXIcacheResult SBcacheEntryDetails::Create( )
{
  // Create the mutex
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  if ( _rwMutex.Create(L"SBcacheEntry rw mutex") != VXItrd_RESULT_SUCCESS ) {
    Error (109, L"%s%s", L"mutex", L"cache entry rw mutex");
    rc = VXIcache_RESULT_SYSTEM_ERROR;
  }

  return rc;
}


// Open the entry
VXIcacheResult SBcacheEntryDetails::Open(VXIlogInterface       *log,
					 const SBcacheString   &moduleName,
					 const SBcacheKey      &key,
					 const SBcachePath     &path,
					 VXIcacheOpenMode       mode,
					 VXIint32               flags,
					 VXIulong               maxSizeBytes,
					 const VXIMap          *properties,
					 VXIMap                *streamInfo,
					 VXIcacheStream       **stream)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  FILE *filePtr = NULL;

  // Lock and copy input data
  switch ( mode ) {
  case CACHE_MODE_WRITE:
    if ( _rwMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 110, L"%s%s", L"mutex", L"cache entry rw mutex, writer");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    } else {
      // If the open fails due to directory not existing, call
      // path.CreateDirectories( ) to create the dir tree then open
      // again
      if ( ! _invalidated ) {
	filePtr = fopen (path.c_str( ), "wb");
        int retries = 10; // work around possible race condition w/ rmdir
	while ((retries-- > 0) &&
            ( ! filePtr ) &&
            ( path.CreateDirectories( ) == VXIcache_RESULT_SUCCESS ))
	  filePtr = fopen (path.c_str( ), "wb");
      }

      if ( ! filePtr ) {
	if ( _invalidated ) {
	  // Invalidated or another thread attempted an open for write
	  // and failed, we only got here due to a race condition
	  // where we were waiting on the mutex to start our write
	  // before the failure/invalidation happened by the previous
	  // writer
	  rc = VXIcache_RESULT_FAILURE;
	} else {
	  // File open error
	  LogIOError (log, 213, path);
	  rc = VXIcache_RESULT_IO_ERROR;
	}

	_fileExists = false;
	_rwMutex.Unlock( );
      } else {
	_fileExists = true;
	_creationCost = CACHE_CREATION_COST_DEFAULT;
	_flags = flags;
	_sizeBytes = 0;

        // Only set the path and key if they differ
        if (path != _path || key != _key) {
          _path = path;
          _key = key;
        }

	// Load properties
	if ( properties ) {
	  const VXIValue *value =
	    VXIMapGetProperty (properties, CACHE_CREATION_COST);
	  if ( value ) {
	    if ( VXIValueGetType(value) == VALUE_INTEGER )
	      _creationCost = (VXIcacheCreationCost)
		VXIIntegerValue ((const VXIInteger *) value);
	    else
	      Error (log, 301, L"%s%d", L"VXIValueType",
		     VXIValueGetType(value));
	  }
	}
      }
    }
    break;

  case CACHE_MODE_READ:
    if ( _rwMutex.StartRead( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 110, L"%s%s", L"mutex", L"cache entry rw mutex, reader");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    } else {
      if (( _fileExists ) && ( ! _invalidated )) {
	filePtr = fopen (_path.c_str( ), "rb");
	if ( ! filePtr ) {
	  _fileExists = false;
	  LogIOError (log, 214, _path);
	  _rwMutex.EndRead( );
	  rc = VXIcache_RESULT_IO_ERROR;
	}
      } else {
	// Invalidated or another thread attempted an open for write
	// and failed, we only got here due to a race condition where
	// we were waiting on the mutex to start our read before the
	// failure/invalidation happened by the writer
	_rwMutex.EndRead( );
	rc = VXIcache_RESULT_FAILURE;
      }
    }
    break;

  default:
    Error (log, 215, L"%s%d", L"mode", mode);
    rc = VXIcache_RESULT_UNSUPPORTED;
  }

  if ( rc == VXIcache_RESULT_SUCCESS ) {
    // Update the accessed time
    _lastAccessed = time(0);
    if (mode == CACHE_MODE_WRITE)
      _lastModified = _lastAccessed;

    // Return stream information
    if ( streamInfo ) {
      if (( VXIMapSetProperty (streamInfo, CACHE_INFO_LAST_MODIFIED,
			       (VXIValue *) VXIIntegerCreate(_lastModified)) !=
	    VXIvalue_RESULT_SUCCESS ) ||
	  ( VXIMapSetProperty (streamInfo, CACHE_INFO_SIZE_BYTES,
			       (VXIValue *) VXIIntegerCreate (_sizeBytes)) !=
	    VXIvalue_RESULT_SUCCESS )) {
	Error (log, 100, NULL);
	rc = VXIcache_RESULT_OUT_OF_MEMORY;
      }
    }
  }

  // Return the stream
  if ( rc == VXIcache_RESULT_SUCCESS ) {
    SBcacheEntry entry(this);
    *stream = new SBcacheStream (log, GetDiagBase( ), moduleName, key,
				 entry, mode,
				 ((flags & CACHE_FLAG_NONBLOCKING_IO) == 0),
				 maxSizeBytes, filePtr);
    if ( ! *stream ) {
      Error (log, 100, NULL);
      fclose (filePtr);
      Close (log, mode, 0, true);
      rc = VXIcache_RESULT_OUT_OF_MEMORY;
    }
  }

  Diag (log, SBCACHE_ENTRY_TAGID, L"Open", L"%s, %S, 0x%x, 0x%x: rc = %d",
	key.c_str( ), path.c_str( ), mode, flags, rc);
  return rc;
}


// Close the entry
VXIcacheResult SBcacheEntryDetails::Close(VXIlogInterface   *log,
					  VXIcacheOpenMode   mode,
					  VXIunsigned        sizeBytes,
					  bool               invalidate)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  Diag (log, SBCACHE_ENTRY_TAGID, L"Close", L"entering: %s, %S, 0x%x, %u, %d",
	_key.c_str( ), _path.c_str( ), mode, sizeBytes, invalidate);

  // Invalidate the entry if requested to do so, we just set a flag so
  // when the entry is deleted we remove the on-disk copy. Can't just
  // do "_invalidated = invalidate" as there may be multiple readers
  // at once, if one reader thinks we're good (don't invalidate) and
  // another thinks we're bad (invalidate) don't want us to have race
  // conditions where we ignore the invalidate request.
  if ( invalidate )
    _invalidated = true;

  // Unlock the entry, note that we should avoid doing anything after
  // the unlock (including logging ) because this object may get
  // deleted as soon as we do so
  switch ( mode ) {
  case CACHE_MODE_WRITE:
    _sizeBytes = sizeBytes;

    if ( _rwMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 111, L"%s%s", L"mutex", L"cache entry rw mutex, writer");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    }
    break;

  case CACHE_MODE_READ:
    if ( _rwMutex.EndRead( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 111, L"%s%s", L"mutex", L"cache entry rw mutex, reader");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    }
    break;

  default:
    Error (log, 211, L"%s%d", L"mode", mode);
    rc = VXIcache_RESULT_FATAL_ERROR;
  }

  Diag (log, SBCACHE_ENTRY_TAGID, L"Close", L"exiting: rc = %d", rc);
  return rc;
}


// Unlock the entry
VXIcacheResult SBcacheEntryDetails::Unlock(VXIlogInterface       *log)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  if ( _rwMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
    Error (log, 110, L"%s%s", L"mutex", L"cache entry rw mutex, writer");
    rc = VXIcache_RESULT_SYSTEM_ERROR;
  } else {
    if ( _flags & CACHE_FLAG_LOCK )
      _flags = (_flags ^ CACHE_FLAG_LOCK);
    if ( _flags & CACHE_FLAG_LOCK_MEMORY )
      _flags = (_flags ^ CACHE_FLAG_LOCK_MEMORY);

    if ( _rwMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 111, L"%s%s", L"mutex", L"cache entry rw mutex, writer");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    }
  }

  Diag (log, SBCACHE_ENTRY_TAGID, L"Unlock", L"%s, %S: rc = %d",
  	_key.c_str( ), _path.c_str( ), rc);
  return rc;
}


// Delete the on-disk entry, internal routine, no mutex held
VXIcacheResult SBcacheEntryDetails::DeleteFile( )
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  // Delete the file
  if  ( _fileExists ) {
    if ( remove (_path.c_str( )) != 0 )
      LogIOError (216);

    // Remove directory
    SBcacheNString head(_path.c_str());
    SBcacheNString::size_type pos = head.find_last_of("/\\");
    if (pos != SBcacheNString::npos) {
      head = head.substr(0, pos);
      if (!head.empty()) {
#ifdef _win32_
        RemoveDirectory(head.c_str());
#else
        rmdir(head.c_str());
#endif
      }
    }

    _fileExists = false;
  }

  Diag (SBCACHE_ENTRY_TAGID, L"DeleteFile", L"%s, %S: rc = %d",
	_key.c_str( ), _path.c_str( ), rc);
  return rc;
}


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Construct a SBcacheEntry from details
SBcacheEntry::SBcacheEntry (SBcacheEntryDetails *details) : _details(details)
{
  if ( SBcacheEntryDetails::AddRef (_details) != VXItrd_RESULT_SUCCESS )
    _details = NULL;
}


// Destructor
SBcacheEntry::~SBcacheEntry( )
{
  if ( _details )
    SBcacheEntryDetails::Release (&_details);
}


// Create the entry
VXIcacheResult SBcacheEntry::Create(VXIlogInterface *log,
				    VXIunsigned diagTagBase,
				    SBcacheMutex *refCountMutex)
{
  if ( _details )
    return VXIcache_RESULT_FATAL_ERROR;

  VXIcacheResult rc;
  _details = new SBcacheEntryDetails (log, diagTagBase, refCountMutex);
  if ( ! _details ) {
    SBinetLogger::Error (log, MODULE_SBCACHE, 100, NULL);
    rc = VXIcache_RESULT_OUT_OF_MEMORY;
  } else {
    rc = _details->Create( );
  }

  return rc;
}


// Open the entry
VXIcacheResult SBcacheEntry::Open(VXIlogInterface       *log,
				  const SBcacheString   &moduleName,
				  const SBcacheKey      &key,
				  const SBcachePath     &path,
				  VXIcacheOpenMode       mode,
				  VXIint32               flags,
				  VXIulong               maxSizeBytes,
				  const VXIMap          *properties,
				  VXIMap                *streamInfo,
				  VXIcacheStream       **stream)
{
  return (_details == NULL ? VXIcache_RESULT_FATAL_ERROR :
	  _details->Open (log, moduleName, key, path, mode, flags,
			  maxSizeBytes, properties, streamInfo, stream));
}


// Close the entry
VXIcacheResult SBcacheEntry::Close(VXIlogInterface  *log,
				   VXIcacheOpenMode  mode,
				   VXIunsigned       sizeBytes,
				   bool              invalidate)
{
  return (_details == NULL ? VXIcache_RESULT_FATAL_ERROR :
	  _details->Close (log, mode, sizeBytes, invalidate));
}


// Unlock the entry
VXIcacheResult SBcacheEntry::Unlock(VXIlogInterface       *log)
{
  return (_details == NULL ? VXIcache_RESULT_FATAL_ERROR :
	  _details->Unlock (log));
}


// Accessors
bool SBcacheEntry::IsLocked( ) const
{
  return _details->IsLocked( );
}

bool SBcacheEntry::IsExpired (time_t  cutoffTime,
			      time_t *lastAccessed) const
{
  return _details->IsExpired (cutoffTime, lastAccessed);
}

const SBcacheKey & SBcacheEntry::GetKey( ) const
{
  return _details->GetKey( );
}

const SBcachePath & SBcacheEntry::GetPath( ) const
{
  return _details->GetPath( );
}

VXIulong SBcacheEntry::GetSizeBytes (bool haveEntryOpen) const
{
  return _details->GetSizeBytes (haveEntryOpen);
}


// Error logging
VXIlogResult SBcacheEntry::LogIOError (VXIunsigned errorID) const
{
  return _details->LogIOError (errorID);
}


// Copy constructor and assignment operator that implement reference
// counting of the held SBcacheEntryData
SBcacheEntry::SBcacheEntry (const SBcacheEntry & e) : _details(e._details)
{
  if ( SBcacheEntryDetails::AddRef (_details) != VXItrd_RESULT_SUCCESS )
    _details = NULL;
}

SBcacheEntry & SBcacheEntry::operator= (const SBcacheEntry & e)
{
  if ( &e != this ) {
    SBcacheEntryDetails::Release (&_details);
    if ( SBcacheEntryDetails::AddRef (e._details) == VXItrd_RESULT_SUCCESS )
      _details = e._details;
  }
  return *this;
}


// Comparison operators, smaller is defined as a preference for
// deleting this entry first, equality is having equal preference
bool SBcacheEntry::operator< (const SBcacheEntry &entry) const
{
  return _details->operator< (*(entry._details));
}

bool SBcacheEntry::operator> (const SBcacheEntry &entry) const
{
  return _details->operator> (*(entry._details));
}

bool SBcacheEntry::operator== (const SBcacheEntry &entry) const
{
  return _details->operator== (*(entry._details));
}

bool SBcacheEntry::operator!= (const SBcacheEntry &entry) const
{
  return _details->operator!= (*(entry._details));
}
