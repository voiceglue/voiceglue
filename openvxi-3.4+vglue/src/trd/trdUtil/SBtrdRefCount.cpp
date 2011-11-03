/* SBtrdRefCount, class for managing reference counts */

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

#include <limits.h>                        // For ULONG_MAX

#define SBTRDUTIL_EXPORTS
#include "SBtrdMutex.hpp"                  // For the GlobalMutex class
#include "SBtrdRefCount.hpp"               // For this class

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8


// Add a reference to an object
SBTRDUTIL_API_CLASS VXItrdResult 
SBtrdRefCount::AddRef (SBtrdRefCount *obj)
{
  if ( obj == NULL ) return VXItrd_RESULT_INVALID_ARGUMENT;

  // Do optional mutex locking
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;
  if (( obj->_mutex ) && 
      ((rc = obj->_mutex->Lock( )) != VXItrd_RESULT_SUCCESS ))
    return rc;

  // Increment the reference count
  if ( obj->_refCount < ULONG_MAX )
    (obj->_refCount)++;
  else
    rc = VXItrd_RESULT_FATAL_ERROR;

  // Release the mutex lock
  if ( obj->_mutex ) {
    VXItrdResult rc2 =  obj->_mutex->Unlock( );
    if (rc2 != VXItrd_RESULT_SUCCESS )
      rc = rc2;
  }

  return rc;
}


// Release a reference. Note: this sets the passed pointer to NULL.
SBTRDUTIL_API_CLASS VXItrdResult 
SBtrdRefCount::Release (SBtrdRefCount **obj)
{
  if (( ! obj ) || ( ! *obj )) return VXItrd_RESULT_INVALID_ARGUMENT;

  // Do optional mutex locking
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;
  if (( (*obj)->_mutex ) && 
      ((rc = (*obj)->_mutex->Lock( )) != VXItrd_RESULT_SUCCESS ))
    return rc;

  // Decrement the reference count
  unsigned long count = 999;
  if ( (*obj)->_refCount > 0 )
    count = --((*obj)->_refCount);
  else
    rc = VXItrd_RESULT_FATAL_ERROR;

  // Release the mutex lock
  if ( (*obj)->_mutex ) {
    VXItrdResult rc2 =  (*obj)->_mutex->Unlock( );
    if (rc2 != VXItrd_RESULT_SUCCESS )
      rc = rc2;
  }

  // Invalidate the pointer and return
  if ( count == 0 )
    delete *obj;
  *obj = NULL;

  return rc;
}


// Get a unique copy if this is being shared in preparation for a
// write operation, don't want to change this under someone else's
// feet. The passed pointer will be changed if this is being shared.
SBTRDUTIL_API_CLASS VXItrdResult 
SBtrdRefCount::GetUniqueCopy (SBtrdRefCount **obj,
			      SBtrdMutex *mutex)
{
  if (( ! obj ) || ( ! *obj )) return VXItrd_RESULT_INVALID_ARGUMENT;

  // Do optional mutex locking
  VXItrdResult rc = VXItrd_RESULT_SUCCESS;
  if (( (*obj)->_mutex ) && 
      ((rc = (*obj)->_mutex->Lock( )) != VXItrd_RESULT_SUCCESS ))
    return rc;

  // Do the split if the ref count is greater then 1
  SBtrdRefCount *newObj = NULL;
  if ( (*obj)->_refCount > 1 ) {
    if ( (newObj = (*obj)->AllocateCopy( )) != NULL ) {
      (*obj)->_refCount--;
      if ( mutex )
	newObj->_mutex = mutex;
    } else {
      rc = VXItrd_RESULT_OUT_OF_MEMORY;
    }
  }

  // Release the mutex lock
  if ( (*obj)->_mutex ) {
    VXItrdResult rc2 =  (*obj)->_mutex->Unlock( );
    if (rc2 != VXItrd_RESULT_SUCCESS )
      rc = rc2;
  }
  
  // Return the pointer now that we've unlocked the mutex
  if ( newObj != NULL )
    *obj = newObj;

  return rc;
}
