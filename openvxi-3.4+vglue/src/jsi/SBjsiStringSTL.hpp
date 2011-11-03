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

#ifndef _SBJSISTRINGSTL_HPP__
#define _SBJSISTRINGSTL_HPP__

#include "SBjsiInternal.h"

// Highly efficient STL wstring implementation, use a wrapper to
// ensure we don't go beyond a specific subset of functionality that
// will break the non-STL implementation
#include <string>
#include <cstring>
using namespace std;

class SBjsiNString;

class SBjsiString
{
 public:
  // Constructors and destructor
  SBjsiString():
    details()
  {}

  SBjsiString(const VXIchar *str):
    details(str)
  {}

  SBjsiString(const SBjsiString &str):
    details(str.details)
  {}

  SBjsiString(const VXIchar *str, int len):
    details(str, len)
  {}

  SBjsiString(const char *str):
    details()
  {
    details.reserve(strlen(str) + 1);
    while (*str) details += (unsigned char) *str++;
  }

  SBjsiString(const char *str, int len):
    details()
  {
    details.reserve(len + 1);
    for (int i = 0; i < len; i++) details += (unsigned char) str[i];
  }

  SBjsiString(const SBjsiNString &str):
    details()
  {
    operator=(str);
  }


  virtual ~SBjsiString()
  {}

  // Assignment operators
  SBjsiString& operator=(const SBjsiString &str)
  {
    if ( &str != this ) details = str.details;
    return *this;
  }

  SBjsiString & operator=(const VXIchar *str)
  {
    details = str;
    return *this;
  }

  SBjsiString& operator=(const char *str)
  {
    clear();
    return operator+=(str);
  }

  SBjsiString& operator=(const SBjsiNString& str)
  {
    clear();
    return operator+=(str);
  }

  // Clear operator
  void clear()
  {
    details.resize(0);
  }

  // Operators for appending data to the string
  SBjsiString& operator+=(const SBjsiString & str)
  {
    details += str.details;
    return *this;
  }

  SBjsiString& operator+=(const VXIchar *str)
  {
    details += str;
    return *this;
  }

  inline SBjsiString& operator+=(const SBjsiNString& str);

  SBjsiString& operator+=(const char *str)
  {
    details.reserve(details.size() + strlen(str));
    while (*str) details += (unsigned char) *str++;
    return *this;
  }

  SBjsiString & operator+=(VXIchar c)
  {
    details += c;
    return *this;
  }

  SBjsiString& operator+=(char c)
  {
    details += (unsigned char) c;
    return *this;
  }

  SBjsiString& append(const VXIchar *str, int len)
  {
    details.append(str, len);
    return *this;
  }

  SBjsiString& append(const char *str, int len)
  {
    details.reserve(details.size() + len + 1);
    for (int i = 0; i < len; i++) details += (unsigned char) str[i];
    return *this;
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
  unsigned int length() const
  {
    return details.length();
  }

  const VXIchar *c_str() const
  {
    return details.c_str();
  }

  const VXIchar *data() const
  {
    return details.data();
  }

  const VXIchar operator[] (unsigned int i) const
  {
    return details[i];
  }

  VXIchar& operator[] (unsigned int i)
  {
    return details[i];
  }

  void resize(int n, VXIchar c = L'\0')
  {
    details.resize(n,c);
  }

  void replace(int p, int n, VXIchar *s)
  {
    details.replace(p,n,s);
  }

  // Operator to search the string for a character
  unsigned int find(VXIchar c) const
  {
    return details.find(c);
  }

 private:
  std::basic_string<VXIchar> details;
};

class SBjsiNString
{
 public:
  // Constructors and destructor
  SBjsiNString(): details()
  {}

  SBjsiNString (const char *str):
    details(str)
  {}

  SBjsiNString(const SBjsiNString &str):
    details(str.details)
  {}

  SBjsiNString(const VXIchar *str):
    details()
  {
    details.reserve(wcslen(str) + 1);
    while (*str) details += SBinetW2C(*str++);
  }

  SBjsiNString(const SBjsiString& str):
    details()
  {
    operator=(str);
  }

  SBjsiNString(const char *str, int len):
    details(str,len)
  {}

  SBjsiNString(const VXIchar *str, int len):
    details()
  {
    details.reserve(len + 1);
    for (int i = 0; i < len; i++) details += SBinetW2C(str[i]);
  }

  virtual ~SBjsiNString()
  {}

  // Assignment operators
  SBjsiNString& operator=(const SBjsiNString &str)
  {
    if (&str != this) details = str.details;
    return *this;
  }

  SBjsiNString& operator=(const char *str)
  {
    details = str;
    return *this;
  }

  SBjsiNString& operator=(const VXIchar *str)
  {
    clear();
    return operator+=(str);
  }

  SBjsiNString& operator=(const SBjsiString& str)
  {
    clear();
    return operator+=(str);
  }


  // Clear operator
  void clear()
  {
    details.resize(0);
  }

  // Operators for appending data to the string
  SBjsiNString& operator+=(const SBjsiNString & str)
  {
    details += str.details;
    return *this;
  }

  SBjsiNString& operator+=(const char *str)
  {
    details += str;
    return *this;
  }

  SBjsiNString& operator+=(char c)
  {
    details += c;
    return *this;
  }

  SBjsiNString& operator+=(const SBjsiString & str)
  {
    int n = str.length();
    details.reserve(n + 1);
    for (int i = 0; i < n; i++) details += SBinetW2C(str[i]);
    return *this;
  }

  SBjsiNString& operator+=(const VXIchar *str)
  {
    details.reserve(details.size() + wcslen(str) + 1);
    while (*str) details += SBinetW2C(*str++);
    return *this;
  }

  SBjsiNString& operator+=(VXIchar c)
  {
    details += SBinetW2C(c);
    return *this;
  }

  SBjsiNString& append(const char *str, int len)
  {
    details.append(str, len);
    return *this;
  }

  SBjsiNString& append(const VXIchar *str, int len)
  {
    details.reserve(details.size() + len + 1);
    for (int i = 0; i < len; i++)
      details += SBinetW2C(str[i]);
    return *this;
  }

  // Operators to access the string data
  unsigned int length() const
  {
    return details.length();
  }

  const char *c_str( ) const
  {
    return details.c_str( );
  }

  const char *data() const
  {
    return details.data();
  }

  char operator[] (unsigned int i) const
  {
    return details[i];
  }

  char& operator[] (unsigned int i)
  {
    return details[i];
  }

  void resize(int n, char c = '\0')
  {
    details.resize(n,c);
  }

  // Operator to search the string for a character
  unsigned int find(char c) const
  {
    return details.find(c);
  }

 private:
  string  details;
};

SBjsiString& SBjsiString::operator+=(const SBjsiNString& str)
{
  int n = str.length();
  details.reserve(n + 1);
  for (int i = 0; i < n; i++) details += (unsigned char) str[i];
  return *this;
}

#endif  /* include guard */
