
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

#ifndef _SBCACHE_COUNTER_H__
#define _SBCACHE_COUNTER_H__

#include <limits.h>           // For ULONG_MAX

#include "SBcacheMisc.hpp"    // For SBcacheReaderWriterMutex

class SBcacheCounter {
public:
  // Constructor and destructor
  SBcacheCounter (VXIulong counter) : _mutex( ), _counter(counter) { }
  ~SBcacheCounter( ) { }

  // Creation method
  VXItrdResult Create( ) { return _mutex.Create (L"SBcacheCounter mutex"); }

  // Increment and test, returns an integer greater then, less then,
  // or equal to zero based on whether the resulting counter is greater
  // then, less then, or equal to the limit
  int IncrementTest (VXIulong increment, VXIulong limit) {
    int rc = -1;
    if ( _mutex.Lock( ) == VXItrd_RESULT_SUCCESS ) {
      _counter += increment;
      if ( _counter > limit )
	rc = 1;
      else if ( _counter == limit )
	rc = 0;
      _mutex.Unlock( );
    }
    return rc;
  }

  // See above
  int DecrementTest (VXIulong decrement, VXIulong limit) {
    int rc = -1;
    if ( _mutex.Lock( ) == VXItrd_RESULT_SUCCESS ) {
      _counter -= decrement;
      if ( _counter > limit )
	rc = 1;
      else if ( _counter == limit )
	rc = 0;
      _mutex.Unlock( );
    }
    return rc;
  }

  // Increment by 1 as a sequence number
  VXIulong IncrementSeqNum( ) {
    VXIulong res = 0;
    if ( _mutex.Lock( ) == VXItrd_RESULT_SUCCESS ) {
      res = _counter;
      if ( _counter == ULONG_MAX )
	_counter = 1;
      else
	_counter++;
      _mutex.Unlock( );
    }
    return res;
  }

  // Decrement
  VXIulong Decrement (VXIulong decrement) {
    VXIulong res = 0;
    if ( _mutex.Lock( ) == VXItrd_RESULT_SUCCESS ) {
      if ( _counter > decrement )
	_counter -= decrement;
      else
	_counter = 0;
      res = _counter;
      _mutex.Unlock( );
    }
    return res;
  }

  // Get the counter
  VXIulong Get( ) const {
    VXIulong counter = 0;
    if ( _mutex.Lock( ) == VXItrd_RESULT_SUCCESS ) {
      counter = _counter;
      _mutex.Unlock( );
    }
    return counter;
  }

  // Reset the counter
  void Reset (VXIulong counter) {
    if ( _mutex.Lock( ) == VXItrd_RESULT_SUCCESS ) {
      _counter = counter;
      _mutex.Unlock( );
    }
  }

private:
  // Disabled assignment operator and copy constructor
  SBcacheCounter (const SBcacheCounter &s);
  SBcacheCounter & operator= (const SBcacheCounter &s);

private:
  SBcacheMutex  _mutex;
  VXIulong      _counter;
};

#endif /* include guard */
