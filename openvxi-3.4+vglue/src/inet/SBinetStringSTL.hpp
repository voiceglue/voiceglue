
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

#ifndef _SBINETSTRINGSTL_HPP__
#define _SBINETSTRINGSTL_HPP__

#include "SBinetInternal.h"

// Highly efficient STL wstring implementation, use a wrapper to
// ensure we don't go beyond a specific subset of functionality that
// will break the non-STL implementation
#include <string>
#include <cstring>
using namespace std;

class SBinetNString;

class SBinetString
{
 public:
  // Constructors and destructor
  SBinetString():
    details()
  {}

  SBinetString(const VXIchar *str):
    details(str)
  {}

  SBinetString(const SBinetString &str):
    details(str.details)
  {}

  SBinetString(const VXIchar *str, int len):
    details(str, len)
  {}

  SBinetString(const char *str):
    details()
  {
    details.reserve(strlen(str) + 1);
    while (*str) details += (unsigned char) *str++;
  }

  SBinetString(const char *str, int len):
    details()
  {
    details.reserve(len + 1);
    for (int i = 0; i < len; i++) details += (unsigned char) str[i];
  }

  SBinetString(const SBinetNString &str):
    details()
  {
    operator=(str);
  }


  virtual ~SBinetString()
  {}

  // Assignment operators
  SBinetString& operator=(const SBinetString &str)
  {
    if ( &str != this ) details = str.details;
    return *this;
  }

  SBinetString & operator=(const VXIchar *str)
  {
    details = str;
    return *this;
  }

  SBinetString& operator=(const char *str)
  {
    clear();
    return operator+=(str);
  }

  SBinetString& operator=(const SBinetNString& str)
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
  SBinetString& operator+=(const SBinetString & str)
  {
    details += str.details;
    return *this;
  }

  SBinetString& operator+=(const VXIchar *str)
  {
    details += str;
    return *this;
  }

  inline SBinetString& operator+=(const SBinetNString& str);

  SBinetString& operator+=(const char *str)
  {
    details.reserve(details.size() + strlen(str));
    while (*str) details += (unsigned char) *str++;
    return *this;
  }

  SBinetString & operator+=(VXIchar c)
  {
    details += c;
    return *this;
  }

  SBinetString& operator+=(char c)
  {
    details += (unsigned char) c;
    return *this;
  }

  SBinetString& append(const VXIchar *str, int len)
  {
    details.append(str, len);
    return *this;
  }

  SBinetString& append(const char *str, int len)
  {
    details.reserve(details.size() + len + 1);
    for (int i = 0; i < len; i++) details += (unsigned char) str[i];
    return *this;
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

class SBinetNString
{
 public:
  // Constructors and destructor
  SBinetNString(): details()
  {}

  SBinetNString (const char *str):
    details(str)
  {}

  SBinetNString(const SBinetNString &str):
    details(str.details)
  {}

  SBinetNString(const VXIchar *str):
    details()
  {
    details.reserve(wcslen(str) + 1);
    while (*str) details += SBinetW2C(*str++);
  }

  SBinetNString(const SBinetString& str):
    details()
  {
    operator=(str);
  }

  SBinetNString(const char *str, int len):
    details(str,len)
  {}

  SBinetNString(const VXIchar *str, int len):
    details()
  {
    details.reserve(len + 1);
    for (int i = 0; i < len; i++) details += SBinetW2C(str[i]);
  }

  virtual ~SBinetNString()
  {}

  // Assignment operators
  SBinetNString& operator=(const SBinetNString &str)
  {
    if (&str != this) details = str.details;
    return *this;
  }

  SBinetNString& operator=(const char *str)
  {
    details = str;
    return *this;
  }

  SBinetNString& operator=(const VXIchar *str)
  {
    clear();
    return operator+=(str);
  }

  SBinetNString& operator=(const SBinetString& str)
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
  SBinetNString& operator+=(const SBinetNString & str)
  {
    details += str.details;
    return *this;
  }

  SBinetNString& operator+=(const char *str)
  {
    details += str;
    return *this;
  }

  SBinetNString& operator+=(char c)
  {
    details += c;
    return *this;
  }

  SBinetNString& operator+=(const SBinetString & str)
  {
    int n = str.length();
    details.reserve(n + 1);
    for (int i = 0; i < n; i++) details += SBinetW2C(str[i]);
    return *this;
  }

  SBinetNString& operator+=(const VXIchar *str)
  {
    details.reserve(details.size() + wcslen(str) + 1);
    while (*str) details += SBinetW2C(*str++);
    return *this;
  }

  SBinetNString& operator+=(VXIchar c)
  {
    details += SBinetW2C(c);
    return *this;
  }

  SBinetNString& append(const char *str, int len)
  {
    details.append(str, len);
    return *this;
  }

  SBinetNString& append(const VXIchar *str, int len)
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

SBinetString& SBinetString::operator+=(const SBinetNString& str)
{
  int n = str.length();
  details.reserve(n + 1);
  for (int i = 0; i < n; i++) details += (unsigned char) str[i];
  return *this;
}

#endif  /* include guard */
