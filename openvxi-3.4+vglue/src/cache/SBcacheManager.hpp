
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

#ifndef _SBCACHE_MANAGER_H__
#define _SBCACHE_MANAGER_H__

#include <time.h>             // For time_t
#include <map>                // For STL map template class
#include <list>

#include "SBinetLogger.hpp"   // For SBinetLogger
#include "SBcacheLog.h"       // For logging defines
#include "SBcacheMisc.hpp"    // For SBcacheString, SBcacheKey, SBcachePath,
                              // SBcacheReaderWriterMutex
#include "SBcacheCounter.hpp" // For SBcacheCounter, thread safe counters
#include "SBcacheEntry.hpp"   // For SBcacheEntry, entry in the table

class SBcacheManager : public SBinetLogger {
public:
  // Constructor and destructor
  SBcacheManager(VXIlogInterface *log, VXIunsigned diagTagBase) : 
    SBinetLogger(MODULE_SBCACHE, log, diagTagBase), 
    _cacheDir(), _curSizeBytes(0), _maxSizeBytes(0), _entryMaxSizeBytes(0),
    _pathSeqNum(1), 
    _entryTableMutex(log, diagTagBase + SBCACHE_ET_MUTEX_TAGID), 
    _refCntMutexPool(), _entryTable() { }
  virtual ~SBcacheManager( );
  
  // Create the manager
  VXIcacheResult Create(const SBcacheNString &cacheDir,
			VXIulong              cacheMaxSizeBytes,
			VXIulong              entryMaxSizeBytes,
			VXIulong              entryExpTimeSec,
			VXIbool               unlockEntries,
                        VXIulong              cacheLowWaterBytes);

  // Open an entry
  VXIcacheResult Open(VXIlogInterface       *log,
		      const SBcacheString   &moduleName,
		      const SBcacheKey      &key,
		      VXIcacheOpenMode       mode,
		      VXIint32               flags,
		      const VXIMap          *properties,
		      VXIMap                *streamInfo,
		      VXIcacheStream       **stream);

  // Notification of data writes
  VXIcacheResult WriteNotification (VXIlogInterface     *log,
				    const SBcacheString &moduleName,
				    VXIulong             nwritten,
                                    const SBcacheKey    &key);

  // Unlock an entry
  VXIcacheResult Unlock(VXIlogInterface       *log,
			const SBcacheKey      &key);

  // Delete an entry
  VXIcacheResult Delete(VXIlogInterface       *log,
			const SBcacheKey      &key,
			bool                   haveEntryOpen = false);

  // Write out the index file, used to handle abnormal termination
  VXIcacheResult WriteIndex( );

  // Clear log resource to avoid crash during caught abnormal termination
  void ClearLogResource( ) { SetLog (NULL, GetDiagBase( )); }

private:
  // Read the index file, used at startup
  VXIcacheResult ReadIndex(const SBcacheNString  &cacheDir);

  // Get a new path for an entry
  SBcachePath GetNewEntryPath(const SBcacheString &moduleName,
			      const SBcacheKey    &key);

  // Clean up the cache to eliminate expired entries and if neccessary
  // delete other entries to remain within the allocated size
  VXIcacheResult Cleanup (bool forcedCleanup, const SBcacheKey& writingKey);

  // Disable the copy constructor and assignment operator
  SBcacheManager(const SBcacheManager &manager);
  const SBcacheManager &operator=(const SBcacheManager &manager);

  // Preallocation
  void ReserveEntries(int nentries);
  void EntryAdded();
  void EntryRemoved();

  // Synchronize LRU list and lookup map
  bool InsertEntry(SBcacheEntry& entry);
  void RemoveEntry(const SBcacheEntry& entry);
  void TouchEntry(const SBcacheEntry& entry);

private:
  SBcacheNString           _cacheDir;
  SBcacheCounter           _curSizeBytes;
  VXIulong                 _maxSizeBytes;
  VXIulong                 _entryMaxSizeBytes;
  VXIulong                 _lowWaterBytes;

  SBcacheCounter           _pathSeqNum;

  SBcacheReaderWriterMutex _entryTableMutex;
  SBcacheMutexPool         _refCntMutexPool;
  typedef std::map<SBcacheKey, SBcacheEntry> SBcacheEntryTable;
  typedef std::list<SBcacheEntry> SBcacheEntryList;
  SBcacheEntryTable        _entryTable;
  SBcacheEntryList          _entryLRUList;

  // Reverse memory allocator, see note in .cpp file.
  struct SBcacheEntryEstimate { SBcacheEntryEstimate* next; char dummy[2048]; };
  SBcacheEntryEstimate* _entryReserve;
  int                      _entriesReserved;
  int                      _maxEntriesReserved;
};

#endif /* include guard */
