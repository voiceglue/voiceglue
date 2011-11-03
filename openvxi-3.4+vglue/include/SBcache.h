
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

#ifndef _SBCACHE_H
#define _SBCACHE_H

#include "VXIcache.h"                  /* For VXIcache base interface */
#include "VXIlog.h"                    /* For VXIlog interface */

#include "VXIheaderPrefix.h"
#ifdef SBCACHE_EXPORTS
#define SBCACHE_API SYMBOL_EXPORT_DECL
#else
#define SBCACHE_API SYMBOL_IMPORT_DECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup SBcache VXIcacheInterface Implementation
 *
 * SBcache interface, and implementation of VXIcacheInterface
 * which permits writing arbitrary data into the cache with
 * a client supplied key name, then retrieving that data from the
 * cache one or more times by reading against that key name. <p>
 *
 * Normally the cache implementation can choose to discard the data
 * between the write and the read when necessary (due to running out
 * of cache space, etc.), but it is also possible for clients to lock
 * data in the cache to support built-in grammars and other data that
 * is explicitly provisioned by system administrators and thus must
 * not be removed unless by explicit system administrator command. <p>
 *
 * See VXIcache.h for additional details.<p>
 *
 * This implementation currently does NOT factor in
 * CACHE_CREATION_COST when determining how to clean up the cache, and
 * all caching is done on disk resulting in CACHE_FLAG_LOCK_MEMORY
 * being treated as CACHE_FLAG_LOCK. In addition,
 * CACHE_FLAG_NONBLOCKING_IO is ignored, disk I/O is done in a
 * blocking manner (usually undetectable by clients since local disks
 * are used). Long key names are handled using MD5 digests as described
 * as a potential implementation choice in VXIcache.h.<p>
 *
 * There is one cache interface per thread/line.
 */

/*@{*/

/**
 * Global platform initialization of SBcache
 *
 * @param log              VXI Logging interface used for error/diagnostic 
 *                         logging, only used for the duration of this 
 *                         function call
 * @param cacheDir         Cache directory name
 * @param cacheSizeMB      Maximum size of the data in the cache directory,
 *                         in megabytes
 * @param entryMaxSizeMB   Maximum size of any individual cache entry, in
 *                         megabytes
 * @param entryExpTimeSec  Maximum amount of time any individual cache entry
 *                         will remain in the cache, in seconds
 * @param unlockEntries    TRUE to unlock locked entries on startup (from
 *                         using CACHE_FLAG_LOCK and CACHE_FLAG_LOCK_MEMORY),
 *                         FALSE to leave them locked. In most cases it is
 *                         best to pass TRUE with the platform re-adding
 *                         locked entries at startup in order to avoid
 *                         having obsolete locked entries fill up the cache.
 *                         However, for some multi-process oriented
 *                         integrations it may be necessary to pass FALSE
 *                         and do this cleanup in some other way.
 *
 * @result VXIcache_RESULT_SUCCESS on success
 */
SBCACHE_API VXIcacheResult SBcacheInit(VXIlogInterface  *log,
                                       const VXIunsigned diagLogBase,
                                       const VXIchar    *cacheDir,
                                       const int         cacheSizeMB,
                                       const int         entryMaxSizeMB,
                                       const int         entryExpTimeSec,
                                       VXIbool           unlockEntries,
                                       const int         cacheLowWaterMB);

/**
 * Global platform shutdown of SBcache
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               only used for the duration of this function call
 *
 * @result VXIcache_RESULT_SUCCESS on success
 */
SBCACHE_API VXIcacheResult SBcacheShutDown (VXIlogInterface  *log);

/**
 * Create a new cache service handle
 *
 * @param log        [IN] VXI Logging interface used for error/diagnostic 
 *                   logging, must remain a valid pointer throughout the 
 *                   lifetime of the resource (until 
 *                   SBcacheDestroyResource() is called)
 * @param cache      [IN/OUT] Will hold the created cache resource.
 *
 * @result VXIcache_RESULT_SUCCESS on success 
 */
SBCACHE_API 
VXIcacheResult SBcacheCreateResource (VXIlogInterface     *log,
                                      VXIcacheInterface   **cache);
  
/**
 * Destroy the interface and free internal resources. Once this is
 * called, the logging interface passed to SBcacheCreateResource( ) 
 * may be released as well.
 *
 * @param cache [IN/OUT] The cache resource created by SBcacheCreateResource()
 *
 * @result VXIcache_RESULT_SUCCESS on success 
 */
SBCACHE_API 
VXIcacheResult SBcacheDestroyResource (VXIcacheInterface **cache);

/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
