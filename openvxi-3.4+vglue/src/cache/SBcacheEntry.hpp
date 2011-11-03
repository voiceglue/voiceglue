
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

#ifndef _SBCACHE_ENTRY_H__
#define _SBCACHE_ENTRY_H__

#include <time.h>             // for time_t

#include "VXIcache.h"         // For VXIcacheResult, CACHE_[...]
#include "SBcacheMisc.hpp"    // For SBcacheString, SBcacheKey, SBcachePath

// Forward declarations
extern "C" struct VXIlogInterface;
class SBcacheEntryDetails;

// SBcacheEntry, cache entry
class SBcacheEntry {
public:
  // Constructor and destructor
  SBcacheEntry( ) : _details(NULL) { }
  SBcacheEntry (SBcacheEntryDetails *details);
  virtual ~SBcacheEntry( );
  
  // Create the entry
  VXIcacheResult Create(VXIlogInterface *log, 
			VXIunsigned diagTagBase,
			SBcacheMutex *refCountMutex);

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
  VXIcacheResult Unlock(VXIlogInterface     *log);

  // Accessors
  bool IsLocked( ) const;
  bool IsExpired (time_t cutoffTime, time_t *lastAccessed) const;
  const SBcacheKey & GetKey( ) const;
  const SBcachePath & GetPath( ) const;
  VXIulong GetSizeBytes (bool haveEntryOpen = false) const;

  // Error logging
  VXIlogResult LogIOError (VXIunsigned errorID) const;

  // Copy constructor and assignment operator
  SBcacheEntry(const SBcacheEntry &entry);
  SBcacheEntry &operator=(const SBcacheEntry &entry);

  // Comparison operators, smaller is defined as a preference for
  // deleting this entry first, equality is having equal preference
  bool operator< (const SBcacheEntry &entry) const;
  bool operator> (const SBcacheEntry &entry) const;
  bool operator== (const SBcacheEntry &entry) const;
  bool operator!= (const SBcacheEntry &entry) const;

  bool Equivalent(const SBcacheEntry &other) const { return _details == other._details; }

public:
  // Only for SBcacheStream use

  // Close
  VXIcacheResult Close (VXIlogInterface  *log,
			VXIcacheOpenMode  mode, 
			VXIunsigned       sizeBytes,
			bool              invalidate);

private:
  SBcacheEntryDetails  *_details;
};

#endif /* include guard */
