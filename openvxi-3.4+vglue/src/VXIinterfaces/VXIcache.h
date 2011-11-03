
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

#ifndef _VXICACHE_H
#define _VXICACHE_H

#include "VXIvalue.h"                  /* For VXIMap, VXIString, etc. */

#include "VXIheaderPrefix.h"
#ifdef VXICACHE_EXPORTS
#define VXICACHE_API SYMBOL_EXPORT_DECL
#else
#define VXICACHE_API SYMBOL_IMPORT_DECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
struct VXIcacheStream;
#else
typedef struct VXIcacheStream { void * dummy; } VXIcacheStream;
#endif

/**
 * \defgroup VXIcache Cache Interface 
 *
 * Abstract interface for accessing caching functionality, which
 * permits writing arbitrary data into the cache with a client
 * supplied key name, then retrieving that data from the cache one or
 * more times by reading against that key name. <p>
 *
 * Normally the cache implementation can choose to discard the data
 * between the write and the read when necessary (due to running out
 * of cache space, etc.), but it is also possible for clients to lock
 * data in the cache to support built-in grammars and other data that
 * is explicitly provisioned by system administrators and thus must
 * not be removed unless by explicit system administrator command. <p>

 * The interface is a synchronous interface based on the ANSI/ISO C
 * standard file I/O interface. The client of the interface may use this in
 * an asynchronous manner by using non-blocking I/O operations,
 * creating threads, or by invoking this from a separate server
 * process. <p>
 * 
 * Typically the key name specified by clients will be the URL to the
 * source form of the data that is being written back, such as the URL
 * to the grammar text being used as the key name for the compiled
 * grammar. In the case where the VXIinet and VXIcache implementations
 * share the same underlying cache storage, it is thus necessary to
 * use prefixes or some other namespace mechanism to avoid collisions
 * with the cache entry for the original URL. <p>
 *
 * However, the key names specified by clients may be very large in
 * some cases. This is most common when writing back data generated
 * from in-memory source text, such as when writing back the compiled
 * grammar for a VoiceXML document in-line grammar.  One possible
 * VXIcache implementation approach is to use the MD5 algorithm as
 * used in HTTP message headers (specified in RFC 1321 with a full
 * implementation provided) to convert long key names to a 16 byte MD5
 * value for indexing purposes, using Base64 encoding to convert the
 * binary MD5 value to ASCII text if desired (as done in HTTP message
 * headers). <p>
 *
 * There is one cache interface per thread/line. <p> 
 */

/*@{*/
/*
 * Keys identifying properties in VXIMap for Open( ), see below for
 * valid values for the enumerated type properties.
 */
#define CACHE_CREATION_COST      L"cache.creationCost"       /* VXIInteger */

/**
 * Provides a hint on how much CPU/time went into creating the
 * object being written cache.  
 * 
 * Enumerated CACHE_CREATION_COST property values. The creation costs
 * property permits prioritizing the cache so low cost objects are
 * flushed before high cost objects (for example, try to keep compiled
 * grammars which have a high creation cost over keeping audio
 * recordings that were simply fetched). Note that implementations may
 * ignore this hint.
 */
typedef enum VXIcacheCreationCost {
  /** Extremely high */
  CACHE_CREATION_COST_EXTREME         = 40,
  /** High */
  CACHE_CREATION_COST_HIGH            = 30,
  /** Medium */
  CACHE_CREATION_COST_MEDIUM          = 20,
  /** Low */
  CACHE_CREATION_COST_LOW             = 10,
  /** Only the cost of a fetch */
  CACHE_CREATION_COST_FETCH          =   0
} VXIcacheCreationCost;

/**
 * Default cache creation cost
 */
#define CACHE_CREATION_COST_DEFAULT        CACHE_CREATION_COST_LOW

/**
 * Mode values for VXIcache::Open( )
 * Open supports ANSI/ISO C standard I/O open modes
 */
typedef enum VXIcacheOpenMode { 
  /** Open for reading */
  CACHE_MODE_READ         = 0x0,
  /** Open for writing */
  CACHE_MODE_WRITE        = 0x1,
  /** Atomic operation to open for reading if the entry exists
      (VXIcache_RESULT_SUCCESS returned), otherwise open it for
      writing (VXIcache_RESULT_ENTRY_CREATED returned). This is 
      only available as of version 1.1 of the VXIcacheInterface,
      use CACHE_MODE_READ_CREATE_SUPPORTED( ) to determine
      availability. **/
  CACHE_MODE_READ_CREATE  = 0x2
} VXIcacheOpenMode;

/** 
 * @name Cache Flags
 * Set of flags which control the caching behavior of the data
 * put into the cache.<p>
 * The Open() call takes  bitwise or of cache flags which control
 * the caching behavior of data which is written or read from the
 * cache.
 */
/*@{*/

/** Null flag.  This causes the cache to use its default behavior on cache
      locking and management */
#define CACHE_FLAG_NULL            0x0

/** Never flush from the cache.  This locks the file on disk for the
      duration of the process.  This flag may be bitwise combined through OR 
      with the other CACHE_FLAG defines in the call to Open(). */
#define CACHE_FLAG_LOCK            0x2     

/** Never flush from memory.  This locks the data in memory. This lock
      option is only a hint, some implementations may ignore this and
      interpret it as CACHE_FLAG_LOCK. */
#define CACHE_FLAG_LOCK_MEMORY     0x4  

/** Non-blocking reads/writes. Do all I/O using non-blocking operations. */
#define CACHE_FLAG_NONBLOCKING_IO  0x8     

/*@}*/

/** 
  @name Cache Open Properties
  Set of properties that are returned on the Open() of an
  existing cache entry.<p>
  The Open() call returns a VXIMap which contains a set of
  key/value properties.  This set of defines provides the definition
  of the properties that must be supported by an implementation of Open().
  */
/*@{*/

/** Final key name used for storing the cache entry. This may differ 
    from the original key name specified for Open( ) in cases where that 
    original key name was very large and the implementation used a MD5 
    digest or other mechanism to compress it.  While the user may still
    obtain access to the entry using the original key name, it may opt
    to use this final key name instead in order to reduce its own memory
    use.  This is returned as a VXIString.
  */
#define CACHE_INFO_FINAL_KEY      L"cache.info.finalKey"

/** Last Modifed time.  This is a property in the VXIMap used to
    return cache information on Open(). It returns the last modified date 
    and time for the entry. This is returned as a VXIInteger. 
  */
#define CACHE_INFO_LAST_MODIFIED  L"cache.info.lastModified"

/** Size in bytes.  This property of the VXIMap is used to return
    cache information on the size of the cache entry in bytes as a 
    VXIInteger.
  */
#define CACHE_INFO_SIZE_BYTES     L"cache.info.sizeBytes"

/*@}*/

/*
 * Macros to determine the availability of new methods
 */
#define CACHE_OPENEX_SUPPORTED(cacheIntf) \
  (cacheIntf->GetVersion( ) >= 0x00010001)
#define CACHE_CLOSEEX_SUPPORTED(cacheIntf) \
  (cacheIntf->GetVersion( ) >= 0x00010001)
#define CACHE_UNLOCKEX_SUPPORTED(cacheIntf) \
  (cacheIntf->GetVersion( ) >= 0x00010001)
#define CACHE_MODE_READ_CREATE_SUPPORTED(cacheIntf) \
  (cacheIntf->GetVersion( ) >= 0x00010001)

/**
 * VXIcache return codes.
 *
 * Result codes less then zero are severe errors (likely to be
 * platform faults), those greater then zero are warnings (likely to
 * be application issues) 
 */
typedef enum VXIcacheResult {
  /** Fatal error, terminate call    */
  VXIcache_RESULT_FATAL_ERROR       =  -100, 
  /** I/O error                      */
  VXIcache_RESULT_IO_ERROR           =   -8,
  /** Out of memory                  */
  VXIcache_RESULT_OUT_OF_MEMORY      =   -7, 
  /** System error, out of service   */
  VXIcache_RESULT_SYSTEM_ERROR       =   -6, 
  /** Errors from platform services  */
  VXIcache_RESULT_PLATFORM_ERROR     =   -5, 
  /** Return buffer too small        */
  VXIcache_RESULT_BUFFER_TOO_SMALL   =   -4, 
  /** Property name is not valid    */
  VXIcache_RESULT_INVALID_PROP_NAME  =   -3, 
  /** Property value is not valid   */
  VXIcache_RESULT_INVALID_PROP_VALUE =   -2, 
  /** Invalid function argument      */
  VXIcache_RESULT_INVALID_ARGUMENT   =   -1, 
  /** Success                        */
  VXIcache_RESULT_SUCCESS            =    0,
  /** Normal failure, nothing logged */
  VXIcache_RESULT_FAILURE            =    1,
  /** Non-fatal non-specific error   */
  VXIcache_RESULT_NON_FATAL_ERROR    =    2, 
  /** Named data not found           */
  VXIcache_RESULT_NOT_FOUND          =   50, 
  /** I/O operation would block      */
  VXIcache_RESULT_WOULD_BLOCK        =   53,
  /** End of stream                  */
  VXIcache_RESULT_END_OF_STREAM      =   54,
  /** Buffer exceeds maximum size    */
  VXIcache_RESULT_EXCEED_MAXSIZE     =   55,
  /** Entry in the cache in currently in used and is locked   */
  VXIcache_RESULT_ENTRY_LOCKED       =   56,
  /** CACHE_MODE_READ_WRITE requested and the entry did not exist so a
      write stream to the new entry is being returned */
  VXIcache_RESULT_ENTRY_CREATED      =   57,
  /** Operation is not supported     */
  VXIcache_RESULT_UNSUPPORTED        =  100
} VXIcacheResult;

/**
 * Abstract interface for accessing caching functionality.
 *
 * Permits writing arbitrary data into the cache with a client
 * supplied key name, then retrieving that data from the cache one or
 * more times by reading against that key name. <p>
 */
typedef struct VXIcacheInterface {
  /**
   * Get the VXI interface version implemented
   *
   * @return  VXIint32 for the version number. The high high word is 
   *          the major version number, the low word is the minor version 
   *          number, using the native CPU/OS byte order. The current
   *          version is VXI_CURRENT_VERSION as defined in VXItypes.h.
   */ 
  VXIint32 (*GetVersion)(void);
  
  /**
   * Get the name of the implementation
   *
   * @return  Implementation defined string that must be different from
   *          all other implementations. The recommended name is one
   *          where the interface name is prefixed by the implementator's
   *          Internet address in reverse order, such as com.xyz.rec for
   *          VXIrec from xyz.com. This is similar to how VoiceXML 1.0
   *          recommends defining application specific error types.
   */
  const VXIchar* (*GetImplementationName)(void);
  
  /**
   * Open a stream for reading or writing given a wide character key
   *
   * NOTE: OpenEx( ) is the same but supports binary keys
   *
   * If the cache entry is currently in use and a stream cannot be returned
   * because this use locks the entry, Open should return 
   * VXIcache_RESULT_ENTRY_LOCKED. <p>
   *
   * The behavior of opening a cache entry for reading during a write
   * operation is implementation defined. <p>
   *
   * @param moduleName   [IN] Name of the software module that is 
   *                       writing or reading the data. See the top of 
   *                       VXIlog.h for moduleName allocation rules.
   * @param key          [IN] Key name of the data to access, NULL
   *                       terminated VXIchar based string that may be
   *                       of an arbitrary length
   * @param mode         [IN] Mode to open the data with, a CACHE_MODE
   *                       value as defined above
   * @param flags        [IN] Flags to control the open:
   *                     CACHE_FLAG_LOCK, lock retrieved data in the
   *                       cache so it is not flushed (although may be
   *                       flushed from the memory cache to the disk cache)
   *                     CACHE_FLAG_LOCK_MEMORY, lock retrieved data
   *                       in the memory cache so it is not flushed
   *                       (not even to the disk cache), note that some
   *                       implementations may ignore this and simply
   *                       treat this as CACHE_FLAG_LOCK.
   *                     CACHE_FLAG_NONBLOCKING_IO, non-blocking reads and
   *                       writes, although the open and close is still
   *                       blocking
   * @param properties   [IN] Properties to control the open, as listed
   *                       above. May be NULL.
   * @param streamInfo   [OUT] (Optional, pass NULL if not required.) Map
   *                       that will be populated with information about 
   *                       the stream, the CACHE_INFO_[...] keys listed
   *                       above are mandatory with the implementation 
   *                       possibly setting other keys.  This may not be 
   *                       populated on an open for WRITE since that is 
   *                       creating a new file.
   * @param stream       [OUT] Handle to the opened stream
   *
   * @return VXIcache_RESULT_SUCCESS on success
   * @return VXIcache_RESULT_ENTRY_LOCKED if the entry is in used and 
   *           cannot be returned. Returned if a writer has the entry open.
   * @return VXIcache_RESULT_NULL_STREAM if the stream that was passed in 
   *           or the interface is NULL 
   */
  VXIcacheResult (*Open)(struct VXIcacheInterface  *pThis,
			 const VXIchar             *moduleName,
			 const VXIchar             *key,
			 VXIcacheOpenMode           mode,
			 VXIint32                   flags,
			 const VXIMap              *properties,
			 VXIMap                    *streamInfo,
			 VXIcacheStream           **stream);

  /**
   * Close a previously opened stream
   *
   * NOTE: CloseEx( ) is the same but supports invalidating the entry
   *
   * Close a previously opened stream.  If Close is called on 
   * a NULL stream or a previously closed stream an error will be
   * returned
   *
   * @param stream       [IN/OUT] Stream to close, set to NULL on
   *                       success
   *
   * @return VXIcache_RESULT_SUCCESS on success
   */
  VXIcacheResult (*Close)(struct VXIcacheInterface  *pThis,
			  VXIcacheStream           **stream);
  
  /**
   * Unlock an entry that was previously locked into the cache given
   * a wide character key
   *
   * NOTE: UnlockEx( ) is the same but supports binary keys
   *
   * This releases a cache lock on the indicated data. Note that it
   * is up to the implementation to decide when to flush the data
   * from the cache, it may choose to do so immediately or may
   * do so at a later time.
   *
   * @param key          [IN] Key name of the data to access, NULL
   *                       terminated VXIchar based string that may be
   *                       of an arbitrary length
   *
   * @return VXIcache_RESULT_SUCCESS on success
   */
  VXIcacheResult (*Unlock)(struct VXIcacheInterface *pThis,
			   const VXIchar            *key);

  /**
   * Read from a stream
   *
   * This may or not block, as determined by the flags used when
   * opening the stream. When in non-blocking mode, partial buffers
   * may be returned instead of blocking, or an
   * VXIcache_RESULT_WOULD_BLOCK error is returned if no data is
   * available at all.
   *
   * @param buffer   [OUT] Buffer that will receive data from the stream
   * @param buflen   [IN] Length of buffer, in bytes
   * @param nread    [OUT] Number of bytes actual read, may be less then
   *                   buflen if the end of the stream was found, or if
   *                   using non-blocking I/O and there is currently no
   *                   more data available to read
   * @param stream   [IN] Handle to the stream to read from
   *
   * @return VXIcache_RESULT_SUCCESS on success 
   */
  VXIcacheResult (*Read)(struct VXIcacheInterface *pThis,
			 VXIbyte                  *buffer,
			 VXIulong                  buflen,
			 VXIulong                 *nread,
			 VXIcacheStream           *stream);
  
  /**
   * Write to a stream
   *
   * This may or not block, as determined by the flags used when
   * opening the stream. When in non-blocking mode, partial writes may
   * occur instead of blocking, or an VXIcache_RESULT_WOULD_BLOCK
   * error is returned if no data could be written at all.
   *
   * @param buffer   [OUT] Buffer of data to write to the stream
   * @param buflen   [IN] Number of bytes to write
   * @param nread    [OUT] Number of bytes actual written, may be less then
   *                   buflen if an error is returned, or if using 
   *                   non-blocking I/O and the write operation would block
   * @param stream   [IN] Handle to the stream to write to
   *
   * @return VXIcache_RESULT_SUCCESS on success 
   */
  VXIcacheResult (*Write)(struct VXIcacheInterface *pThis,
			  const VXIbyte            *buffer,
			  VXIulong                  buflen,
			  VXIulong                 *nwritten,
			  VXIcacheStream           *stream);

  /**
   * Open a stream for reading or writing given a binary key.
   *
   * NOTE: Same as Open( ) but supports binary keys. This is only 
   *       available as of version 1.1 of the VXIcacheInterface, use
   *       CACHE_OPENEX_SUPPORTED( ) to determine availability.<p>
   *
   * If the cache entry is currently in use and a stream cannot be returned
   * because this use locks the entry, Open should return 
   * VXIcache_RESULT_ENTRY_LOCKED. <p>
   *
   * The behavior of opening a cache entry for reading during a write
   * operation is implementation defined. <p>
   *
   * @param moduleName   [IN] Name of the software module that is 
   *                       writing or reading the data. See the top of 
   *                       VXIlog.h for moduleName allocation rules.
   * @param key          [IN] Key name of the data to access, binary
   *                       data (raw bytes) of an arbitrary length
   * @param keySizeBytes [IN] Size of the key name in bytes
   * @param mode         [IN] Mode to open the data with, a CACHE_MODE
   *                       value as defined above
   * @param flags        [IN] Flags to control the open:
   *                     CACHE_FLAG_LOCK, lock retrieved data in the
   *                       cache so it is not flushed (although may be
   *                       flushed from the memory cache to the disk cache)
   *                     CACHE_FLAG_LOCK_MEMORY, lock retrieved data
   *                       in the memory cache so it is not flushed
   *                       (not even to the disk cache), note that some
   *                       implementations may ignore this and simply
   *                       treat this as CACHE_FLAG_LOCK.
   *                     CACHE_FLAG_NONBLOCKING_IO, non-blocking reads and
   *                       writes, although the open and close is still
   *                       blocking
   * @param properties   [IN] Properties to control the open, as listed
   *                       above. May be NULL.
   * @param streamInfo   [OUT] (Optional, pass NULL if not required.) Map
   *                       that will be populated with information about 
   *                       the stream, the CACHE_INFO_[...] keys listed
   *                       above are mandatory with the implementation 
   *                       possibly setting other keys.  This may not be 
   *                       populated on an open for WRITE since that is 
   *                       creating a new file.
   * @param stream       [OUT] Handle to the opened stream
   *
   * @return VXIcache_RESULT_SUCCESS on success
   * @return VXIcache_RESULT_ENTRY_LOCKED if the entry is in used and 
   *           cannot be returned. Returned if a writer has the entry open.
   * @return VXIcache_RESULT_NULL_STREAM if the stream that was passed in 
   *           or the interface is NULL 
   */
  VXIcacheResult (*OpenEx)(struct VXIcacheInterface  *pThis,
			   const VXIchar             *moduleName,
			   const VXIbyte             *key,
			   VXIulong                   keySizeBytes,
			   VXIcacheOpenMode           mode,
			   VXIint32                   flags,
			   const VXIMap              *properties,
			   VXIMap                    *streamInfo,
			   VXIcacheStream           **stream);

  /**
   * Close a previously opened stream
   *
   * NOTE: Same as Close( ) but supports invalidating the entry. This
   *       is only available as of version 1.1 of the VXIcacheInterface,
   *       use CACHE_CLOSEEX_SUPPORTED( ) to determine availability.
   *
   * Close a previously opened stream.  If Close is called on 
   * a NULL stream or a previously closed stream an error will be
   * returned
   *
   * @param keepEntry    [IN] TRUE to indiate the write was a success
   *                       and the cache entry should be retained,
   *                       FALSE to indicate the write failed
   *                       (typically due to a data source read
   *                       unexpectedly failing)
   * @param stream       [IN/OUT] Stream to close, set to NULL on
   *                       success
   *
   * @return VXIcache_RESULT_SUCCESS on success
   */
  VXIcacheResult (*CloseEx)(struct VXIcacheInterface  *pThis,
			    VXIbool                    keepEntry,
			    VXIcacheStream           **stream);
  
  /**
   * Unlock an entry that was previously locked into the cache
   * given a binary key
   *
   * NOTE: Same as Unlock( ) but supports binary keys. This is only 
   *       available as of version 1.1 of the VXIcacheInterface, use
   *       CACHE_UNLOCKEX_SUPPORTED( ) to determine availability.
   *
   * This releases a cache lock on the indicated data. Note that it
   * is up to the implementation to decide when to flush the data
   * from the cache, it may choose to do so immediately or may
   * do so at a later time.
   *
   * @param key          [IN] Key name of the data to access, binary
   *                       data (raw bytes) of an arbitrary length
   * @param keySizeBytes [IN] Size of the key name in bytes
   *
   * @return VXIcache_RESULT_SUCCESS on success
   */
  VXIcacheResult (*UnlockEx)(struct VXIcacheInterface  *pThis,
			     const VXIbyte             *key,
			     VXIulong                   keySizeBytes);

} VXIcacheInterface;

/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
