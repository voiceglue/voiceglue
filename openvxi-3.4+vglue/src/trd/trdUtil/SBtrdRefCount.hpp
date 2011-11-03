/* SBtrdRefCount, base class for managing reference counts */

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

#ifndef _SBTRD_REFCOUNT_H__
#define _SBTRD_REFCOUNT_H__

#include "VXIheaderPrefix.h"          // For SYMBOL_EXPORT_CPP_DECL
#include "VXItrd.h"                   // For error codes

#ifdef SBTRDUTIL_EXPORTS
#define SBTRDUTIL_API_CLASS SYMBOL_EXPORT_CPP_CLASS_DECL
#else
#define SBTRDUTIL_API_CLASS SYMBOL_IMPORT_CPP_CLASS_DECL
#endif

class SBtrdMutex;

class SBTRDUTIL_API_CLASS SBtrdRefCount {
 public:
  // Constructor and destructor, pass NULL for the mutex if mutex
  // protection is not required. Note that this class assumes the
  // passed mutex will be shared across many instances to avoid having
  // huge numbers of mutexes in the system, see above.
  SBtrdRefCount (SBtrdMutex *mutex) : _refCount(1), _mutex(mutex) { }
  virtual ~SBtrdRefCount( ) { }

  // Add a reference and release a reference. Note: Release sets the
  // pointer to NULL.
  static VXItrdResult AddRef  (SBtrdRefCount *obj);
  static VXItrdResult Release (SBtrdRefCount **obj);

  // Get a unique copy if this is being shared in preparation for a
  // write operation, don't want to change this under someone else's
  // feet. The passed pointer will be changed if this is being shared.
  // The passed mutex is used for future use by the new copy if a new
  // copy is made, if NULL is passed the mutex used for the original
  // copy will be shared.
  static VXItrdResult GetUniqueCopy (SBtrdRefCount **obj,
				     SBtrdMutex *mutex);
  
  // Copy constructor for deep copies of derrived class during
  // GetUniqueCopy( )
  SBtrdRefCount (const SBtrdRefCount &r) : _refCount(1), _mutex(r._mutex) { }

 protected:
  // Ability to specify the mutex after the constructor
  void SetRefCountMutex (SBtrdMutex *mutex) { _mutex = mutex; }

  // Used to allocate a copy of the real derrived class when we don't
  // know/care what the real derrived class is, must be implemented by
  // all derrived classes. Default implementation here is to not
  // support this, meaning GetUniqueCopy( ) is not supported.
  virtual SBtrdRefCount *AllocateCopy( ) const { return NULL; }

 private:
  // Disable the assignment operator, just doesn't make sense to do
  // this on the actual derrived class (need to have a wrapper class
  // that does so by copying the pointer to the derrived class and
  // adding a reference)
  SBtrdRefCount & operator= (const SBtrdRefCount &r);

 private:
  unsigned long  _refCount;
  SBtrdMutex    *_mutex;
};

#include "VXIheaderSuffix.h"

#endif  // _SBTRD_REFCOUNT_H__
