
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

#ifndef _SBINETSTRINGNOSTL_HPP__
#define _SBINETSTRINGNOSTL_HPP__

// Non-STL implementation

#include <stdlib.h>
#include <wchar.h>

class SBinetString
{
 public:
  // Constructors and destructor
  SBinetString():
    data(NULL),
    allocated(0),
    dataLen(0)
  {}

  SBinetString(const VXIchar *str):
    data(NULL),
    allocated(0),
    dataLen(0)
  {
    operator+=(str);
  }

  SBinetString (const SBinetString &str):
    data(NULL),
    allocated(0),
    dataLen(0)
  {
    operator+=(str.data);
  }

  virtual ~SBinetString()
  {
    if ( data ) free (data);
  }

  // Assignment operators
  SBinetString& operator=(const SBinetString &str)
  {
    if (&str != this)
    {
      dataLen = 0;
      operator+=(str.data);
    }
    return *this;
  }

  SBinetString& operator=(const VXIchar *str)
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
  SBinetString& operator+=(const SBinetString& str)
  {
    return concat(str.data, str.datalen);
  }

  SBinetString& operator+=(const VXIchar *str)
  {
    return concat(str, wcslen(str));
  }

  SBinetString& operator+=(VXIchar c)
  {
    return concat(&c, 1);
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

  SBinetString& concat(const VXIchar *str, int len)
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
