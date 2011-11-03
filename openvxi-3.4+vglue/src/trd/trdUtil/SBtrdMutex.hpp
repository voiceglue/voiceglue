/* SBtrdMutex, classes for managing various types of mutexes */

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

#ifndef _SBTRD_MUTEX_H__
#define _SBTRD_MUTEX_H__

#include "VXIheaderPrefix.h"          // For SYMBOL_EXPORT_CPP_DECL
#include "VXItrd.h"                   // For return codes
#include <cwchar>

#ifdef SBTRDUTIL_EXPORTS
#define SBTRDUTIL_API_CLASS SYMBOL_EXPORT_CPP_CLASS_DECL
#else
#define SBTRDUTIL_API_CLASS SYMBOL_IMPORT_CPP_CLASS_DECL
#endif

extern "C" struct VXIlogInterface;

// Basic mutex class, simple wrapper around VXItrdMutex where each
// mutex that gets created points at a different underlying mutex
class SBTRDUTIL_API_CLASS SBtrdMutex {
 public:
  // Constructor and destructor
  SBtrdMutex ( ) : _name(0), _mutex(0) { }
  virtual ~SBtrdMutex( );

  // Creation method, the name is simply for logging purposes
  virtual VXItrdResult Create (const wchar_t *name);

  // Provide access to the name for logging purposes
  const wchar_t *GetName( ) { return _name; }
  
  // Lock and unlock the mutex. Declared as const so that users can
  // lock/unlock for read access within const methods.
  virtual VXItrdResult Lock( ) const;
  virtual VXItrdResult Unlock( ) const;

 private:
  // Disable the copy constructor and equality operator to catch making
  // copies at compile or link time, neither is really implemented
  SBtrdMutex (const SBtrdMutex &);
  SBtrdMutex & operator= (const SBtrdMutex &);

 private:
  wchar_t      *_name;
  VXItrdMutex  *_mutex;
};


// Reader/writer mutex class, mutex class that permits any number of
// readers and no writers, or one writer with no readers, with a
// strong writer preference to prevent starvation. This is implemented
// using the classic algorithm, see
// http://faculty.juniata.edu/rhodes/os/ch5d.htm
class SBTRDUTIL_API_CLASS SBtrdReaderWriterMutex : public SBtrdMutex {
 public:
  // Constructor and destructor
  SBtrdReaderWriterMutex(VXIlogInterface *log = NULL, 
			 VXIunsigned diagTagBase = 0) : 
    SBtrdMutex( ), _diagTagBase(diagTagBase), _log(log), _readerCount(0),
    _writerCount(0), _readerMutex( ), _writerMutex( ), _readerCountMutex( ), 
    _writerCountMutex( ) { }
  virtual ~SBtrdReaderWriterMutex( ) { }

  // Creation method, the name is simply for logging purposes
  virtual VXItrdResult Create (const wchar_t *name);

  // Obtain/release write access
  virtual VXItrdResult Lock( ) const;
  virtual VXItrdResult Unlock( ) const;

  // Obtain/release read access. Declared as const so that users can
  // lock/unlock for read access within const methods.
  VXItrdResult StartRead( ) const;
  VXItrdResult EndRead( ) const;

 private:
  // Error logging
  void Error (VXIunsigned errorID, const VXIchar *format, ...) const;

  // Diagnostic logging
  void Diag (VXIunsigned tag, const VXIchar *subtag, 
	     const VXIchar *format, ...) const;

  // Disable the copy constructor and equality operator to catch making
  // copies at compile or link time, neither is really implemented
  SBtrdReaderWriterMutex (const SBtrdReaderWriterMutex &);
  SBtrdReaderWriterMutex & operator= (const SBtrdReaderWriterMutex &);

 private:
  // Internal class that provides a mutex which can be locked by one
  // thread, unlocked by another, and if locked by one thread and that
  // same thread comes back and locks it again, that thread will wait
  // (behave like any other thread that does a double lock). The
  // entire algorithm of the reader/writer mutex requires this
  // independance.
  class SBTRDUTIL_API_CLASS CrossThreadMutex : public SBtrdMutex {
   public:
    // Constructor and destructor
    CrossThreadMutex( ) : SBtrdMutex( ), _locked(false), _timer(NULL) {}
    virtual ~CrossThreadMutex( ) {
      if (_timer) VXItrdTimerDestroy (&_timer); }

    // Creation method, the name is simply for logging purposes
    virtual VXItrdResult Create (const wchar_t *name);

    // Obtain/release write access
    virtual VXItrdResult Lock( ) const;
    virtual VXItrdResult Unlock( ) const;

   private:
    volatile bool   _locked;
    VXItrdTimer    *_timer;
  };

 private:
  VXIunsigned      _diagTagBase;
  VXIlogInterface *_log;
  unsigned long    _readerCount, _writerCount;
  CrossThreadMutex _readerMutex, _writerMutex;
  SBtrdMutex       _readerCountMutex, _writerCountMutex;
};


// Mutex pool mutex class, manages a mutex pool of a specified size
// where you can then request mutexes out of the pool (allocated in a
// round robin fashion) for shared use by multiple data instances.
class SBTRDUTIL_API_CLASS SBtrdMutexPool {
 public:
  // Constructor and destructor
  SBtrdMutexPool( ) : _size(0), _curIndex(0), _pool(NULL) { }
  ~SBtrdMutexPool( ) { if ( _pool ) delete [] _pool; }

  // Creation method, the name is simply for logging purposes
  VXItrdResult Create (const wchar_t *name, unsigned int size = 50);

  // Obtain a mutex out of the pool, the mutex pool continues to own
  // this mutex so you must not destroy it, and this mutex may be
  // returned multiple times. This is thread safe.
  SBtrdMutex *GetMutex( );

 private:
  // Disable the copy constructor and equality operator to catch making
  // copies at compile or link time, neither is really implemented
  SBtrdMutexPool (const SBtrdMutexPool &);
  SBtrdMutexPool & operator= (const SBtrdMutexPool &);
 
 private:
  unsigned int   _size;
  unsigned int   _curIndex;
  SBtrdMutex    *_pool;
};

// Reader/writer mutex pool class
class SBTRDUTIL_API_CLASS SBtrdReaderWriterMutexPool {
 public:
  // Constructor and destructor
  SBtrdReaderWriterMutexPool( ) : _size(0), _curIndex(0), 
    _allocationMutex(NULL), _pool(NULL) { }
  ~SBtrdReaderWriterMutexPool( ) { 
    if ( _allocationMutex ) delete _allocationMutex;
    if ( _pool ) delete [] _pool; }

  // Creation method, the name is simply for logging purposes
  VXItrdResult Create (const wchar_t *name, unsigned int size = 50);

  // Obtain a mutex out of the pool, the mutex pool continues to own
  // this mutex so you must not destroy it, and this mutex may be
  // returned multiple times. This is thread safe.
  SBtrdReaderWriterMutex *GetMutex( );

 private:
  // Disable the copy constructor and equality operator to catch making
  // copies at compile or link time, neither is really implemented
  SBtrdReaderWriterMutexPool (const SBtrdReaderWriterMutexPool &);
  SBtrdReaderWriterMutexPool & operator= (const SBtrdReaderWriterMutexPool &);
 
 private:
  unsigned int             _size;
  unsigned int             _curIndex;
  SBtrdMutex              *_allocationMutex;
  SBtrdReaderWriterMutex  *_pool;
};

#include "VXIheaderSuffix.h"

#endif  // _SBTRD_MUTEX_H__
