
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

#include "SBcacheInternal.h"

#include "SBcacheManager.hpp"    // for this class

#include <list>                  // for STL list template class
#include <vector>                  // for STL list template class
#include <algorithm>
#include <limits.h>              // for ULONG_MAX

#if defined(__GNUC__) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 2)
#include <strstream>
#else
#include <sstream>               // for basic_ostringstream( )
#endif

static const int CACHE_REF_COUNT_MUTEX_POOL_SIZE = 256;
static const int CACHE_MAX_DIR_ENTRIES = 256;
static const char CACHE_ENTRY_FILE_EXTENSION[] = ".sbc";

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Shut down the manager
SBcacheManager::~SBcacheManager( )
{
  if ( _cacheDir.length( ) > 0 ) {
    // Lock to be paranoid, makes sure everyone else is done
    if ( _entryTableMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
      Error (110, L"%s%s", L"mutex", L"entry table mutex");
    } else {
      // Write out the index file
      WriteIndex( );

      // Clear the cache entries
      _entryTable.erase (_entryTable.begin( ), _entryTable.end( ));
      _entryLRUList.erase(_entryLRUList.begin(), _entryLRUList.end());
      _curSizeBytes.Reset (0);

      // Clear the directory name
      _cacheDir = "";

      if ( _entryTableMutex.Unlock( ) != VXItrd_RESULT_SUCCESS )
	Error (111, L"%s%s", L"mutex", L"entry table mutex");
    }
  }
}


// Create the manager
VXIcacheResult SBcacheManager::Create(const SBcacheNString &cacheDir,
				      VXIulong              cacheMaxSizeBytes,
				      VXIulong              entryMaxSizeBytes,
				      VXIulong              entryExpTimeSec,
				      VXIbool               unlockEntries,
                                      VXIulong              cacheLowWaterBytes)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  // Avoid double initialization
  if ( _cacheDir.length( ) > 0 ) {
    Error (112, NULL);
    rc = VXIcache_RESULT_FATAL_ERROR;
  }

  // Create the path sequence number
  if (( rc == VXIcache_RESULT_SUCCESS ) &&
      ( _pathSeqNum.Create( ) != VXItrd_RESULT_SUCCESS )) {
    Error (109, L"%s%s", L"mutex", L"path seq num");
    rc = VXIcache_RESULT_SYSTEM_ERROR;
  }

  // Create the entry table mutex
  if (( rc == VXIcache_RESULT_SUCCESS ) &&
      ( _entryTableMutex.Create(L"SBcacheManager entry table mutex") !=
	VXItrd_RESULT_SUCCESS )) {
    Error (109, L"%s%s", L"mutex", L"entry table mutex");
    rc = VXIcache_RESULT_SYSTEM_ERROR;
  }

  // Create the mutex pool
  if (( rc == VXIcache_RESULT_SUCCESS ) &&
      ( _refCntMutexPool.Create(L"SBcacheManager refCnt mutex pool",
				CACHE_REF_COUNT_MUTEX_POOL_SIZE) )) {
    Error (109, L"%s%s", L"mutex", L"entry table mutex");
    rc = VXIcache_RESULT_SYSTEM_ERROR;
  }

  // Create the cache size
  if (( rc == VXIcache_RESULT_SUCCESS ) &&
      ( _curSizeBytes.Create( ) != VXItrd_RESULT_SUCCESS )) {
    Error (109, L"%s%s", L"mutex", L"cache size mutex");
    rc = VXIcache_RESULT_SYSTEM_ERROR;
  }

  // Create the cache directory if required
  if ( rc == VXIcache_RESULT_SUCCESS ) {
    SBcacheStatInfo statInfo;
    if ( SBcacheStat (cacheDir.c_str( ), &statInfo) ) {
      // Exists, make sure it is a file
      if ( ! SBcacheIsDir(statInfo) ) {
	Error (113, L"%s%S", L"cacheDirectory", cacheDir.c_str( ));
	rc = VXIcache_RESULT_FATAL_ERROR;
      } else {
	// Load the cache index file
	rc = ReadIndex (cacheDir);
      }
    } else if ( ! SBcacheMkdir (cacheDir.c_str( )) ) {
      Error (114, L"%s%S", L"cacheDirectory", cacheDir.c_str( ));
      rc = VXIcache_RESULT_FATAL_ERROR;
    }
  }

  // Update data members
  if ( rc == VXIcache_RESULT_SUCCESS ) {
    _cacheDir = cacheDir;
    _maxSizeBytes = cacheMaxSizeBytes;
    _entryMaxSizeBytes = entryMaxSizeBytes;
    _lowWaterBytes = cacheLowWaterBytes;
  }

  _entryReserve = 0;
  ReserveEntries(0); // Disable, for now.

  Diag (SBCACHE_MGR_TAGID, L"Create", L"rc = %d", rc);
  return rc;
}

// NOTE: This is disabled, see above, until SPR is fixed or NPFed.
//
// Interesting trick, we want to reserve a certain amount of memory for N
// entries, but we only have an estimate as to the size of an entry, and we
// can't make all the code use a special allocator.  
// Solution: Make an estimate(E) of how much an entry will take up, and
// grab N * E bytes from the general allocator.  Whenever we add an entry,
// we give back E bytes (if we can), whenver we remove an entry we take E
// bytes (unless we hit the maximum preallocation).  
//
void SBcacheManager::ReserveEntries(int nentries)
{
  for (int i = 0; i < nentries; ++i) {
    SBcacheEntryEstimate* es = new SBcacheEntryEstimate();
    es->next = _entryReserve;
    _entryReserve = es;
  }
  _entriesReserved = nentries;
  _maxEntriesReserved = nentries;
}

void SBcacheManager::EntryAdded()
{
  if (_entriesReserved > 0) {
    SBcacheEntryEstimate* es = _entryReserve;
    _entryReserve = es->next;
    delete es;
    _entriesReserved--;
  } else {
    static int once = 1;
    if (once) {
      once = 0;
    }
  }
}

void SBcacheManager::EntryRemoved()
{
  if (_maxEntriesReserved > _entriesReserved) {
    SBcacheEntryEstimate* es = new SBcacheEntryEstimate();
    es->next = _entryReserve;
    _entryReserve = es;
    _entriesReserved++;
  }
}

// Synchronized table and LRU list
//
bool SBcacheManager::InsertEntry(SBcacheEntry& entry)
{
  SBcacheEntryTable::value_type tableEntry (entry.GetKey(), entry);
  if ( ! _entryTable.insert (tableEntry).second )
    return false;
  _entryLRUList.push_back(entry);
  EntryAdded();
  return true;
}

void SBcacheManager::RemoveEntry(const SBcacheEntry& entry)
{
  SBcacheEntryTable::iterator te = _entryTable.find(entry.GetKey());
  _entryTable.erase(te);
  SBcacheEntryList::iterator vi = _entryLRUList.begin();
  for (; vi != _entryLRUList.end(); ++vi)
    if (entry.Equivalent(*vi)) {
      _entryLRUList.erase(vi);
      break;
    }
  EntryRemoved();
}

void SBcacheManager::TouchEntry(const SBcacheEntry& entry)
{
  SBcacheEntryList::iterator vi = _entryLRUList.begin();
  for (; vi != _entryLRUList.end(); ++vi)
    if (entry.Equivalent(*vi)) {
      _entryLRUList.erase(vi);
      _entryLRUList.push_back(entry);
      break;
    }
}


//#######################################################################
// Note about locking protocol.  THere are three locks in this code:
//   A. The entrytable lock (_entryTableMutex)
//   B. a lock per entry  (GetSizeBytes, Open, Close)
//   C. a lock of _curSizeBytes (atomic operations)
// Currently, both locks A & C are held inside of lock B because an open
// obtains a lock and holds it until the entry is closed.  Therefore,
// a B lock must never be obtained while an A or C lock is held!
//#######################################################################

// Open an entry
VXIcacheResult SBcacheManager::Open(VXIlogInterface       *log,
				    const SBcacheString   &moduleName,
				    const SBcacheKey      &key,
				    VXIcacheOpenMode       mode,
				    VXIint32               flags,
				    const VXIMap          *properties,
				    VXIMap                *streamInfo,
				    VXIcacheStream       **stream)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  // Big loop where we attempt to open and re-open the cache entry for
  // as long as we get recoverable errors
  VXIcacheOpenMode finalMode;
  do {
    finalMode = mode;
    rc = VXIcache_RESULT_SUCCESS;

    if ( _entryTableMutex.StartRead( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 110, L"%s%s", L"mutex", L"entry table mutex");
      return VXIcache_RESULT_SYSTEM_ERROR;
    }

    // Find the entry and open it, only need read permission
    bool entryOpened = false;
    SBcacheEntry entry;
    SBcacheEntryTable::iterator vi = _entryTable.find (key);
    if ( vi == _entryTable.end( ) ) {
      rc = VXIcache_RESULT_NOT_FOUND;
    } else {
      entry = (*vi).second;
      finalMode = (mode == CACHE_MODE_READ_CREATE ? CACHE_MODE_READ : mode);
    }

    if ( _entryTableMutex.EndRead( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 111, L"%s%s", L"mutex", L"entry table mutex");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    }


    // For write and read/create mode, create the entry if not found
    if (( rc == VXIcache_RESULT_NOT_FOUND ) && ( mode != CACHE_MODE_READ )) {
      rc = VXIcache_RESULT_SUCCESS;

      // Get write permission
      if ( _entryTableMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
	Error (log, 110, L"%s%s", L"mutex", L"entry table mutex");
	rc = VXIcache_RESULT_SYSTEM_ERROR;
      } else {
	// Try to find the entry again, it may have been created by now
	vi = _entryTable.find (key);
	if ( vi != _entryTable.end( ) ) {
	  // Found it this time
	  entry = (*vi).second;
	  finalMode = (mode == CACHE_MODE_READ_CREATE ? CACHE_MODE_READ : mode);
	} else {
	  // Create and open the entry for write
	  rc = entry.Create (GetLog( ), GetDiagBase( ),
			     _refCntMutexPool.GetMutex( ));

	  if ( rc == VXIcache_RESULT_SUCCESS ) {
	    finalMode = (mode == CACHE_MODE_READ_CREATE ? CACHE_MODE_WRITE :
			 mode);

            // Note entry lock obtained while entrytable lock is held! We
            // can get away with this, because the lock isn't visible to
            // anyone else yet.
	    rc = entry.Open (log, moduleName, key,
			     GetNewEntryPath (moduleName, key),
			     finalMode, flags, _entryMaxSizeBytes, properties,
			     streamInfo, stream);
	  }

	  // Insert the entry
	  if ( rc == VXIcache_RESULT_SUCCESS ) {
	    entryOpened = true;

	    SBcacheEntryTable::value_type tableEntry (key, entry);
	    if ( !InsertEntry(entry)) {
	      Error (log, 100, NULL);
	      (*stream)->Close (true);
	      *stream = NULL;
	      rc = VXIcache_RESULT_OUT_OF_MEMORY;
	    } 
	  }
	}

	if ( _entryTableMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
	  Error (log, 111, L"%s%s", L"mutex", L"entry table mutex");
	  rc = VXIcache_RESULT_SYSTEM_ERROR;
	}
      }
    }


    // Open pre-existing entry. Note that this may return
    // VXIcache_RESULT_FAILURE in some cases where we then need to
    // retry the entire process of opening the entry -- that occurs
    // when a writer opens the entry, we attempt to open for read
    // access here before the writer finishes writing the entry, then
    // the writer invalidates the entry on us. When that happens, if
    // we're attempting MODE_READ_CREATE we want to get that invalid
    // entry out of the table and create the entry ourselves returning
    // in WRITE mode, for MODE_READ we just want to see if another
    // writer has created a new entry that is valid for us to use
    // otherwise we bail out with the normal RESULT_NOT_FOUND.
    if (( rc == VXIcache_RESULT_SUCCESS ) && ( ! entryOpened )) {
      // If this is a write over an existing entry, wipe the previous entry
      // accounting from the cache.
      if (finalMode == CACHE_MODE_WRITE)
        _curSizeBytes.Decrement(entry.GetSizeBytes(false));

      rc = entry.Open (log, moduleName, key, entry.GetPath( ), finalMode,
		       flags, _entryMaxSizeBytes, properties, streamInfo,
		       stream);

      // Don't let a corrupt entry persist
      if (( rc != VXIcache_RESULT_SUCCESS ) &&
	  ( rc != VXIcache_RESULT_FAILURE ))
	Delete (log, key);
    }

    // Accessed, so move it to the top of the LRU list
    if ( _entryTableMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 110, L"%s%s", L"mutex", L"entry table mutex");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    } else {
      if (rc == VXIcache_RESULT_SUCCESS)
        TouchEntry(entry);
      if ( _entryTableMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
        Error (log, 111, L"%s%s", L"mutex", L"entry table mutex");
        rc = VXIcache_RESULT_SYSTEM_ERROR;
      }
    }

  } while ( rc == VXIcache_RESULT_FAILURE );

  // Finish up returning the stream information
  if (( rc == VXIcache_RESULT_SUCCESS ) && ( streamInfo ) &&
      ( VXIMapSetProperty (streamInfo, CACHE_INFO_FINAL_KEY,
			   (VXIValue *) VXIStringCreate(key.c_str( ))) !=
	VXIvalue_RESULT_SUCCESS )) {
    Error (log, 100, NULL);
    (*stream)->Close (true);
    *stream = NULL;
    rc = VXIcache_RESULT_OUT_OF_MEMORY;
  }

  if (( rc == VXIcache_RESULT_SUCCESS ) &&
      ( mode == CACHE_MODE_READ_CREATE ) && ( finalMode == CACHE_MODE_WRITE ))
    rc = VXIcache_RESULT_ENTRY_CREATED;

  // Maybe do cleanup, ignore all but fatal errors
  VXIcacheResult rc2 = Cleanup (false, key);
  if ( rc2 < VXIcache_RESULT_SUCCESS ) {
    (*stream)->Close (true);
    *stream = NULL;
    rc = rc2;
  }

  Diag (log, SBCACHE_MGR_TAGID, L"Open", L"%s: rc = %d", key.c_str( ), rc);
  return rc;
}


// Notification of data writes
VXIcacheResult
SBcacheManager::WriteNotification (VXIlogInterface     *log,
				   const SBcacheString &moduleName,
				   VXIulong             nwritten,
                                   const SBcacheKey    &key)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  // moduleName is reserved for future use, may want to allow
  // configuring the cache with specific cache allocations on a
  // per-module basis

  // Note that _curSizeBytes is a thread-safe object, intentionally
  // ignore cleanup errors unless fatal
  if ( _curSizeBytes.IncrementTest (nwritten, _maxSizeBytes) > 0 ) {
    VXIcacheResult rc2 = Cleanup (true, key);
    if ( rc2 < VXIcache_RESULT_SUCCESS )
      rc = rc2;
  }

  return rc;
}


// Unlock an entry
VXIcacheResult SBcacheManager::Unlock(VXIlogInterface   *log,
				      const SBcacheKey  &key)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  if ( _entryTableMutex.StartRead( ) != VXItrd_RESULT_SUCCESS ) {
    Error (log, 110, L"%s%s", L"mutex", L"entry table mutex");
    rc = VXIcache_RESULT_SYSTEM_ERROR;
  } else {
    // Find the entry and unlock it, only need read permission
    SBcacheEntryTable::iterator vi = _entryTable.find (key);
    if ( vi != _entryTable.end( ) ) {
      rc = (*vi).second.Unlock (log);
    } else {
      rc = VXIcache_RESULT_NOT_FOUND;
    }

    if ( _entryTableMutex.EndRead( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 111, L"%s%s", L"mutex", L"entry table mutex");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    }
  }

  Diag (log, SBCACHE_MGR_TAGID, L"Unlock", L"%s: rc = %d", key.c_str( ), rc);
  return rc;
}


// Delete an entry
VXIcacheResult SBcacheManager::Delete(VXIlogInterface   *log,
				      const SBcacheKey  &key,
				      bool               haveEntryOpen)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  // Find the entry and delete it, only need read permission. Note
  // that we need to hold a reference to the entry in order to make
  // sure it only gets deleted after we release our lock here (that
  // may be a costly operation). The extra braces ensure the actual
  // deletion is done prior to logging our exit.
  {
    SBcacheEntry entry;

    if ( _entryTableMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
      Error (log, 110, L"%s%s", L"mutex", L"entry table mutex");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    } else {
      SBcacheEntryTable::iterator vi = _entryTable.find (key);
      if ( vi != _entryTable.end( ) ) {
	entry = (*vi).second;
        RemoveEntry(entry);
      } else {
	rc = VXIcache_RESULT_NOT_FOUND;
      }

      if ( _entryTableMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
	Error (log, 111, L"%s%s", L"mutex", L"entry table mutex");
	rc = VXIcache_RESULT_SYSTEM_ERROR;
      }
    }

    // Note that even getting the size can take time since we
    // may lock on a write mutex until some write is done, and note that
    // erasing the entries may take time since that involves deleting
    // on-disk files in the usual case where there are no active streams
    // for the entry.
    if ( rc == VXIcache_RESULT_SUCCESS )
      _curSizeBytes.Decrement (entry.GetSizeBytes (haveEntryOpen));
  }

  Diag (log, SBCACHE_MGR_TAGID, L"Delete", L"%s: rc = %d", key.c_str( ), rc);
  return rc;
}


// Write out the index file, used to handle abnormal termination
VXIcacheResult SBcacheManager::WriteIndex( )
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  Diag (SBCACHE_MGR_TAGID, L"WriteIndex", L"entering");

  // TBD write the index from the entry table, schedule periodically?

  Diag (SBCACHE_MGR_TAGID, L"WriteIndex", L"exiting: rc = %d", rc);
  return rc;
}


// Read the index file, used at startup
VXIcacheResult SBcacheManager::ReadIndex(const SBcacheNString  &cacheDir)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;
  Diag (SBCACHE_MGR_TAGID, L"ReadIndex", L"entering: %S", cacheDir.c_str( ));

  // TBD, read the index to create the entry table

  // TBD, purge files with our extension that aren't in the index but
  // are in the cache, warn about those without our extension

  Diag (SBCACHE_MGR_TAGID, L"ReadIndex", L"exiting: rc = %d", rc);
  return rc;
}


// Get a new path for an entry
SBcachePath SBcacheManager::GetNewEntryPath(const SBcacheString &moduleName,
					    const SBcacheKey    &key)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  // Get the index number, note that _pathSeqNum is a thread-safe object
  VXIulong index = _pathSeqNum.IncrementSeqNum( );

  // Construct the path, return an empty string on failure. The path
  // is a directory tree where there is a subdirectory for each module
  // name, then subdirectories underneath each module to limit each
  // subdirectory to no more then 256 entries in order to avoid OS
  // limitations on files per directory for some older OSes

#if defined(__GNUC__) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 2)
  std::ostrstream pathStream;
#else
  std::basic_ostringstream<char> pathStream;
#endif

  const wchar_t *ptr = moduleName.c_str( );
  if (( ptr ) && ( *ptr )) {
    while (*ptr != L'\0') {
      if ((( *ptr >= L'0' ) && ( *ptr <= L'9' )) ||
	  (( *ptr >= L'A' ) && ( *ptr <= L'Z' )) ||
	  (( *ptr >= L'a' ) && ( *ptr <= L'z' )) ||
	  ( *ptr == L'.' ))
	pathStream << static_cast<char>(*ptr);
      else
	pathStream << '_';

      ptr++;
    }
  } else {
    pathStream << "unknown_module";
  }

  pathStream << SBcachePath::PATH_SEPARATOR
	     << index / CACHE_MAX_DIR_ENTRIES << SBcachePath::PATH_SEPARATOR
	     << index % CACHE_MAX_DIR_ENTRIES << CACHE_ENTRY_FILE_EXTENSION;
#if defined(__GNUC__) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 2) 
  pathStream << ends; 
#endif 

  SBcachePath path (_cacheDir, pathStream.str( ));

#if defined(__GNUC__) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 2)
  // str() freezes the stream, we unfreeze it here to allow pathStream to
  // cleanup after itself.
  pathStream.freeze(false);
#else
  // Not required in earlier GCC or windows?
#endif

  return path;
}

// Clean up the cache to eliminate expired entries and if neccessary
// delete other entries to remain within the allocated size
VXIcacheResult SBcacheManager::Cleanup (bool forcedCleanup, 
    const SBcacheKey& writingKey)
{
  VXIcacheResult rc = VXIcache_RESULT_SUCCESS;

  if (_curSizeBytes.Get() <= _maxSizeBytes)
    return rc;

  // Log to Mgr and Cleanup logs, and emit an warning the first time
  // cleanup is entered so integrations know if they're filling it too
  // rapidly.
  Diag (SBCACHE_MGR_TAGID, L"Cleanup",
	L"cleanup starting: %lu bytes in cache, %lu allocation",
	_curSizeBytes.Get( ), _maxSizeBytes);
  Diag (SBCACHE_CLEANUP_TAGID, L"Cleanup",
	L"cleanup starting: %lu bytes in cache, %lu allocation",
	_curSizeBytes.Get( ), _maxSizeBytes);
  { static int once = 0;
    if (once == 0) { 
      once = 1;
      Error(302, L"%s%s", L"Cleanup", L"One-time notification: Disk cache "
          L"filled to maximum for first time, performing cleanup.");
    } 
  }

  int bytesToExpire = _curSizeBytes.Get() - _lowWaterBytes;
  time_t now = time(0);
  SBcacheEntryTable::iterator vi;
  SBcacheEntryList::iterator li;
  SBcacheEntryList expiredEntries;
  VXIulong totalBytesFreed = 0;
  time_t oldestAccessed = now;
  int entriesFreed = 0;

  // Lock cache and transfer enough entries from the cache to our temporary
  // list to get us below the cache low water mark.
  // 
  if ( _entryTableMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
    Error (110, L"%s%s", L"mutex", L"entry table mutex");
    rc = VXIcache_RESULT_SYSTEM_ERROR;
  } else {
    // Copy entries to our temporary list
    for (li = _entryLRUList.begin(); li != _entryLRUList.end(); ++li) {
      if (bytesToExpire <= 0) break;
      int entrySize = (*li).GetSizeBytes(true);
      if ((*li).GetKey() != writingKey && 
          0 < entrySize &&
          (*li).IsExpired(now, &oldestAccessed)) {
        expiredEntries.push_back(*li);
        totalBytesFreed += entrySize;
        bytesToExpire -= entrySize;
        entriesFreed++;
      }
    }

    // Remove them from cache
    for (li = expiredEntries.begin(); li != expiredEntries.end(); ++li)
      RemoveEntry(*li);

    if ( _entryTableMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
      Error (111, L"%s%s", L"mutex", L"entry table mutex");
      rc = VXIcache_RESULT_SYSTEM_ERROR;
    }
  }

  // Now that we're outside the cache lock, actually remove all the
  // entries by removing the last reference to them in our list.
  expiredEntries.erase(expiredEntries.begin(), expiredEntries.end());
  _curSizeBytes.Decrement(totalBytesFreed);

  // Log prior to setting the next cleanup time to ensure we log our
  // completion prior to another cleanup starting
  Diag (SBCACHE_MGR_TAGID, L"Cleanup",
      L"exiting: rc = %d, released %u entries, %lu bytes, %lu bytes in cache",
	rc, entriesFreed, totalBytesFreed, _curSizeBytes.Get( ));
  Diag (SBCACHE_CLEANUP_TAGID, L"Cleanup",
      L"exiting: rc = %d, released %u entries, %lu bytes, %lu bytes in cache",
	rc, entriesFreed, totalBytesFreed, _curSizeBytes.Get( ));

  return rc;
}
