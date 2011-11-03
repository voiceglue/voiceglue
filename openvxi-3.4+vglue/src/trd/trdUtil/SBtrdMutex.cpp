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

#define SBTRDUTIL_EXPORTS
#include "SBtrdMutex.hpp"             // Header for this class

#include <stdio.h>
#include <limits.h>                   // For ULONG_MAX

#include "VXIlog.h"                   // For logging

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

// Destructor
SBTRDUTIL_API_CLASS SBtrdMutex::~SBtrdMutex( )
{
  if ( _name )
    delete [] _name;

  if ( _mutex )
    VXItrdMutexDestroy (&_mutex);
}


// Creation method
SBTRDUTIL_API_CLASS VXItrdResult 
SBtrdMutex::Create (const wchar_t *name)
{
  if (( ! name ) || ( ! name[0] ))
    return VXItrd_RESULT_INVALID_ARGUMENT;
  
  // Save a copy of the name
  size_t len = wcslen (name);
  _name = new wchar_t[len];
  if ( ! _name )
    return VXItrd_RESULT_OUT_OF_MEMORY;
  
  // Create the mutex
  return VXItrdMutexCreate (&_mutex);
}


// Lock and unlock the mutex. Declared as const so that users can
// lock/unlock for read access within const methods.  NOTE: Not
// inlined, otherwise users of SBtrdUtil would have to manually link
// to SBtrd directly instead of just requiring the former.
SBTRDUTIL_API_CLASS VXItrdResult SBtrdMutex::Lock( ) const 
{ 
  return VXItrdMutexLock (_mutex);
}

SBTRDUTIL_API_CLASS VXItrdResult SBtrdMutex::Unlock( ) const
{ 
  return VXItrdMutexUnlock (_mutex);
}


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// CrossThreadMutex creation method
SBTRDUTIL_API_CLASS VXItrdResult
SBtrdReaderWriterMutex::CrossThreadMutex::Create (const wchar_t *name)
{
  VXItrdResult rc = SBtrdMutex::Create (name);
  if ( rc == VXItrd_RESULT_SUCCESS )
    rc = VXItrdTimerCreate (&_timer);
  return rc;
}


// CrossThreadMutex lock method
SBTRDUTIL_API_CLASS VXItrdResult
SBtrdReaderWriterMutex::CrossThreadMutex::Lock( ) const
{
  VXItrdResult rc;
  if ( (rc = SBtrdMutex::Lock( )) == VXItrd_RESULT_SUCCESS ) {
    while (( _locked ) && 
	   ( (rc = VXItrdTimerSleep (_timer, INT_MAX, NULL)) ==
	     VXItrd_RESULT_SUCCESS ))
      ; // keep waiting
    
    if ( rc == VXItrd_RESULT_SUCCESS ) {
      CrossThreadMutex *pThis = const_cast<CrossThreadMutex *>(this);
      pThis->_locked = true;
    }
    
    if ( SBtrdMutex::Unlock( ) != VXItrd_RESULT_SUCCESS )
      rc = VXItrd_RESULT_SYSTEM_ERROR;
  }

  return rc;
}


// CrossThreadMutex unlock method
SBTRDUTIL_API_CLASS VXItrdResult
SBtrdReaderWriterMutex::CrossThreadMutex::Unlock( ) const
{
  if ( ! _locked )
    return VXItrd_RESULT_FATAL_ERROR;

  CrossThreadMutex *pThis = const_cast<CrossThreadMutex *>(this);
  pThis->_locked = false;
  return VXItrdTimerWake (_timer);
}


// Creation method
SBTRDUTIL_API_CLASS VXItrdResult 
SBtrdReaderWriterMutex::Create (const wchar_t *name)
{
  // Need five underlying mutex. We'll use the mutex from our base
  // class for the write lock.
  VXItrdResult rc;
  if (( (rc = SBtrdMutex::Create (name)) != VXItrd_RESULT_SUCCESS ) ||
      ( (rc = _readerMutex.Create (name)) != VXItrd_RESULT_SUCCESS ) ||
      ( (rc = _writerMutex.Create (name)) != VXItrd_RESULT_SUCCESS ) ||
      ( (rc = _readerCountMutex.Create (name)) != VXItrd_RESULT_SUCCESS ) ||
      ( (rc = _writerCountMutex.Create (name)) != VXItrd_RESULT_SUCCESS )) {
    Error (0, NULL);
    return rc;
  }

  return VXItrd_RESULT_SUCCESS;
}


// Obtain write access
SBTRDUTIL_API_CLASS VXItrdResult SBtrdReaderWriterMutex::Lock( ) const
{
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;

#ifdef FAKE_READER_WRITER_MUTEX
  rc = SBtrdMutex::Lock( );
#else
  Diag (0, L"SBtrdReaderWriterMutex::Lock", L"enter: rwMutex 0x%p", this);

  // Get the writer lock, writers have preference
  if ( _writerCountMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
    Error (1, NULL);
    return VXItrd_RESULT_SYSTEM_ERROR;
  }

  if ( _writerCount < ULONG_MAX ) {
    SBtrdReaderWriterMutex *pThis = const_cast<SBtrdReaderWriterMutex *>(this);
    ++(pThis->_writerCount);
    if ( _writerCount == 1 ) {
      Diag (0, L"SBtrdReaderWriterMutex::Lock", L"first writer 0x%p", this);
      if ( _readerMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
	Error (2, NULL);
	rc = VXItrd_RESULT_SYSTEM_ERROR;
      }
    }
  } else {
    Error (3, NULL);
    rc = VXItrd_RESULT_FATAL_ERROR;
  }

  if ( _writerCountMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
    Error (40, NULL);
    rc = VXItrd_RESULT_SYSTEM_ERROR;
  } else if (( rc == VXItrd_RESULT_SUCCESS ) &&
	     ( _writerMutex.Lock( ) != VXItrd_RESULT_SUCCESS )) {
    Error (4, NULL);
    rc = VXItrd_RESULT_SYSTEM_ERROR;
  }

  Diag (0, L"SBtrdReaderWriterMutex::Lock", L"exit: rwMutex 0x%p", this);
#endif

  return rc;
}


// Release write access
SBTRDUTIL_API_CLASS VXItrdResult SBtrdReaderWriterMutex::Unlock( ) const
{
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;

#ifdef FAKE_READER_WRITER_MUTEX
  rc = SBtrdMutex::Unlock( );
#else
  Diag (0, L"SBtrdReaderWriterMutex::Unlock", L"enter: rwMutex 0x%p", this);

  // Release the writer lock, preserve writer preference
  if (( _writerMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) ||
      ( _writerCountMutex.Lock( ) != VXItrd_RESULT_SUCCESS )) {
    Error (5, NULL);
    return VXItrd_RESULT_SYSTEM_ERROR;
  }

  if ( _writerCount > 0 ) {
    SBtrdReaderWriterMutex *pThis = const_cast<SBtrdReaderWriterMutex *>(this);
    --(pThis->_writerCount);
    if ( _writerCount == 0 ) {
      Diag (0, L"SBtrdReaderWriterMutex::Unlock", L"last writer 0x%p", this);
      if ( _readerMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
	Error (6, NULL);
	rc = VXItrd_RESULT_SYSTEM_ERROR;
      }
    }
  } else {
    Error (7, NULL);
    rc = VXItrd_RESULT_FATAL_ERROR;
  }
  
  if ( _writerCountMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
    Error (8, NULL);
    rc = VXItrd_RESULT_SYSTEM_ERROR;
  }

  Diag (0, L"SBtrdReaderWriterMutex::Unlock", L"exit: rwMutex 0x%p", this);
#endif

  return rc;
}


// Obtain read access. Declared as const so that users can lock/unlock
// for read access within const methods.
SBTRDUTIL_API_CLASS VXItrdResult 
SBtrdReaderWriterMutex::StartRead( ) const
{
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;

#ifdef FAKE_READER_WRITER_MUTEX
  rc = SBtrdMutex::Lock( );
#else
  Diag (0, L"SBtrdReaderWriterMutex::StartRead", L"enter: rwMutex 0x%p", this);

  // Wait on mutexes, giving writers preference
  if (( SBtrdMutex::Lock( ) != VXItrd_RESULT_SUCCESS ) ||
      ( _readerMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) ||
      ( _readerCountMutex.Lock( ) != VXItrd_RESULT_SUCCESS )) {
    Error (9, NULL);
    return VXItrd_RESULT_SYSTEM_ERROR;
  }
  
  if ( _readerCount < ULONG_MAX ) {
    SBtrdReaderWriterMutex *pThis = const_cast<SBtrdReaderWriterMutex *>(this);
    ++(pThis->_readerCount);
    if ( _readerCount == 1 ) {
      Diag (0, L"SBtrdReaderWriterMutex::StartRead",L"first reader 0x%p",this);
      if ( _writerMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
	Error (10, NULL);
	rc = VXItrd_RESULT_SYSTEM_ERROR;
      }
    }
  } else {
    Error (11, NULL);
    rc = VXItrd_RESULT_FATAL_ERROR;
  }
  
  if (( _readerCountMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) ||
      ( _readerMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) ||
      ( SBtrdMutex::Unlock( ) != VXItrd_RESULT_SUCCESS )) {
    Error (12, NULL);
    rc = VXItrd_RESULT_SYSTEM_ERROR;
  }
  
  Diag (0, L"SBtrdReaderWriterMutex::StartRead", L"exit: rwMutex 0x%p", this);
#endif

  return rc;
}


// Release read access. Declared as const so that users can lock/unlock
// for read access within const methods.
SBTRDUTIL_API_CLASS VXItrdResult 
SBtrdReaderWriterMutex::EndRead( ) const
{
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;

#ifdef FAKE_READER_WRITER_MUTEX
  rc = SBtrdMutex::Unlock( );
#else
  Diag (0, L"SBtrdReaderWriterMutex::EndRead", L"enter: rwMutex 0x%p", this);

  // Release mutex, preserving writer preference
  if ( _readerCountMutex.Lock( ) != VXItrd_RESULT_SUCCESS ) {
    Error (13, NULL);
    return VXItrd_RESULT_SYSTEM_ERROR;
  }

  if ( _readerCount > 0 ) {
    SBtrdReaderWriterMutex *pThis = const_cast<SBtrdReaderWriterMutex *>(this);
    --(pThis->_readerCount);
    if ( _readerCount == 0 ) {
      Diag (0, L"SBtrdReaderWriterMutex::EndRead", L"last reader 0x%p", this);
      if ( _writerMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
	Error (14, NULL);
	rc = VXItrd_RESULT_SYSTEM_ERROR;
      }
    }
  } else {
    Error (15, NULL);
    rc = VXItrd_RESULT_FATAL_ERROR;
  }
  
  if ( _readerCountMutex.Unlock( ) != VXItrd_RESULT_SUCCESS ) {
    Error (16, NULL);
    rc = VXItrd_RESULT_SYSTEM_ERROR;
  }

  Diag (0, L"SBtrdReaderWriterMutex::EndRead", L"exit: rwMutex 0x%p", this);
#endif

  return rc;
}


// Error logging
SBTRDUTIL_API_CLASS void
SBtrdReaderWriterMutex::Error (VXIunsigned errorID, const VXIchar *format,
			       ...) const
{
  if ( _log ) {
    if ( format ) {
      va_list arguments;
      va_start(arguments, format);
      (*_log->VError)(_log, COMPANY_DOMAIN L"SBtrdUtil", errorID, format,
		      arguments);
      va_end(arguments);
    } else {
      (*_log->Error)(_log, COMPANY_DOMAIN L"SBtrdUtil", errorID, NULL);
    }
  }  
}


// Diagnostic logging
SBTRDUTIL_API_CLASS void
SBtrdReaderWriterMutex::Diag (VXIunsigned tag, const VXIchar *subtag, 
			      const VXIchar *format, ...) const
{
  if ( _log ) {
    if ( format ) {
      va_list arguments;
      va_start(arguments, format);
      (*_log->VDiagnostic)(_log, tag + _diagTagBase, subtag, format, 
			   arguments);
      va_end(arguments);
    } else {
      (*_log->Diagnostic)(_log, tag + _diagTagBase, subtag, NULL);
    }
#if 0
  } else {
    VXIchar temp[1024];
    va_list arguments;
    va_start(arguments, format);
    wcscpy (temp, subtag);
    wcscat (temp, L"|");
    wcscat (temp, format);
    wcscat (temp, L"\n");
    vfwprintf(stderr, temp, arguments);
    va_end(arguments);
#endif
  }
}


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Creation method, the name is simply for logging purposes
SBTRDUTIL_API_CLASS VXItrdResult 
SBtrdMutexPool::Create (const wchar_t *name, unsigned int size)
{
  if (( ! name ) || ( ! name[0] ) || ( size == 0 ))
    return VXItrd_RESULT_INVALID_ARGUMENT;

  // Create the mutex pool
  _pool = new SBtrdMutex [size];
  if ( ! _pool )
    return VXItrd_RESULT_OUT_OF_MEMORY;

  // Initialize the mutexes
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;
  for (unsigned int i = 0; i < size; i++) {
    if ( (rc = _pool[i].Create (name)) != VXItrd_RESULT_SUCCESS ) {
      delete [] _pool;
      return rc;
    }
  }
  
  _size = size;
  return VXItrd_RESULT_SUCCESS;
}


// Obtain a mutex out of the pool, the mutex pool continues to own
// this mutex so you must not destroy it, and this mutex may be
// returned multiple times
SBTRDUTIL_API_CLASS SBtrdMutex *SBtrdMutexPool::GetMutex( )
{
  if ( _pool == NULL )
    return NULL;

  // Lock using mutex 0
  if ( _pool[0].Lock( ) != VXItrd_RESULT_SUCCESS )
    return NULL;

  // Retrieve the appropriate mutex and increment the index for
  // round-robin allocation
  SBtrdMutex *mutex = &_pool[_curIndex];
  if ( _curIndex < _size - 1 )
    _curIndex++;
  else
    _curIndex = 0;

  // Unlock using mutex 0
  if ( _pool[0].Unlock( ) != VXItrd_RESULT_SUCCESS )
    return NULL;

  return mutex;
}
// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Creation method, the name is simply for logging purposes
SBTRDUTIL_API_CLASS VXItrdResult 
SBtrdReaderWriterMutexPool::Create (const wchar_t *name, unsigned int size)
{
  if (( ! name ) || ( ! name[0] ) || ( size == 0 ))
    return VXItrd_RESULT_INVALID_ARGUMENT;

  // Create the allocation mutex
  _allocationMutex = new SBtrdMutex;
  if ( ! _allocationMutex )
    return VXItrd_RESULT_OUT_OF_MEMORY;
  
  // Create the mutex pool
  _pool = new SBtrdReaderWriterMutex [size];
  if ( ! _pool )
    return VXItrd_RESULT_OUT_OF_MEMORY;

  // Initialize the mutexes
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;
  if ( (rc = _allocationMutex->Create (name)) != VXItrd_RESULT_SUCCESS ) {
    delete _allocationMutex;
    return rc;
  }

  for (unsigned int i = 0; i < size; i++) {
    if ( (rc = _pool[i].Create (name)) != VXItrd_RESULT_SUCCESS ) {
      delete [] _pool;
      return rc;
    }
  }
  
  _size = size;
  return VXItrd_RESULT_SUCCESS;
}


// Obtain a mutex out of the pool, the mutex pool continues to own
// this mutex so you must not destroy it, and this mutex may be
// returned multiple times
SBTRDUTIL_API_CLASS SBtrdReaderWriterMutex *
SBtrdReaderWriterMutexPool::GetMutex( )
{
  if ( _pool == NULL )
    return NULL;

  // Lock for exlusive access
  if ( _allocationMutex->Lock( ) != VXItrd_RESULT_SUCCESS )
    return NULL;

  // Retrieve the appropriate mutex and increment the index for
  // round-robin allocation
  SBtrdReaderWriterMutex *mutex = &_pool[_curIndex];
  if ( _curIndex < _size - 1 )
    _curIndex++;
  else
    _curIndex = 0;

  // Unlock
  if ( _allocationMutex->Unlock( ) != VXItrd_RESULT_SUCCESS )
    return NULL;

  return mutex;
}
