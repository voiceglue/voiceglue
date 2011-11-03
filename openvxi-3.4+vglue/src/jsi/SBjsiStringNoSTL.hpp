/*****************************************************************************
 *****************************************************************************
 *
 * SBjsiString, string class that is a subset of STL wstring
 *
 * The SBjsiString class stores a string in a grow-only buffer, a
 * functional subset of the STL wstring class. This header merely
 * exists to make it easy to eliminate the use of STL wstring which
 * does not exist on some Linux versions.
 *
 *****************************************************************************
 ****************************************************************************/

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

#ifndef _SBJSISTRINGNOSTL_HPP__
#define _SBJSISTRINGNOSTL_HPP__

// Non-STL implementation

#include <stdlib.h>
#include <wchar.h>

class SBjsiString
{
 public:
  // Constructors and destructor
  SBjsiString():
    data(NULL),
    allocated(0),
    dataLen(0)
  {}

  SBjsiString(const VXIchar *str):
    data(NULL),
    allocated(0),
    dataLen(0)
  {
    operator+=(str);
  }

  SBjsiString (const SBjsiString &str):
    data(NULL),
    allocated(0),
    dataLen(0)
  {
    operator+=(str.data);
  }

  virtual ~SBjsiString()
  {
    if ( data ) free (data);
  }

  // Assignment operators
  SBjsiString& operator=(const SBjsiString &str)
  {
    if (&str != this)
    {
      dataLen = 0;
      operator+=(str.data);
    }
    return *this;
  }

  SBjsiString& operator=(const VXIchar *str)
  {
    dataLen = 0;
    return operator+=(str);
    return *this;
  }

  // Clear operator
  void clear()
  {
    dataLen = 0;
  }

  // Operators for appending data to the string
  SBjsiString& operator+=(const SBjsiString& str)
  {
    return concat(str.data, str.datalen);
  }

  SBjsiString& operator+=(const VXIchar *str)
  {
    return concat(str, wcslen(str));
  }

  SBjsiString& operator+=(VXIchar c)
  {
    return concat(&c, 1);
  }

	bool operator==(const VXIchar *str)
	{
		return wcscmp( str, this->c_str() ) == 0;
	}
	
	bool operator==(const SBjsiString& str)
	{
		return wcscmp( str.c_str(), this->c_str() ) == 0;
	}	
		
  // Operators to access the string data
  unsigned int length( ) const { return dataLen; }
  const VXIchar *c_str( ) const { return data; }

  void replace(int p, int n, VXIchar *s)
  {
    if((s == NULL) return;
    if(p + n > dataLen) n = dataLen - p;
    ::wcsncpy(&data[p], s, n);
  }

 private:

  SBjsiString& concat(const VXIchar *str, int len)
  {
    if (datalen + len + 1 <= allocated || Grow(len + 1))
    {
      wcsncpy(data + datalen, str, len);
      datalen += len;
      data[datalen] = L'\0';
    }
    return *this;
  }

  // Grow the buffer
  bool Grow (unsigned int size)
  {
    if ( size < 128 ) size = 128;
    VXIchar *newData =
      (VXIchar *) realloc (data, (dataLen + size) * sizeof (VXIchar));
    if (!newData) return false;
    data = newData;
    allocated = dataLen + size;
    return true;
  }

 private:
  unsigned int  allocated;
  unsigned int  dataLen;
  VXIchar      *data;
};

#endif /* include guard */
