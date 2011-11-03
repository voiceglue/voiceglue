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

#ifndef _JSI_CHAR_CVT_H__
#define _JSI_CHAR_CVT_H__

#include "VXItypes.h"                 // for VXIchar

// SpiderMonkey jschar specifies that jschar must be able to store
// UTF-16 characters.  VXIchar should be the general wide character
// representation (wchar_t) of the platform.  As wchar_t may be other
// types, conversion functions are necessary.

// ------*---------*---------*---------*---------*---------*---------*---------

// The native Solaris and Linux wide character encoding is UTF-32.  This
// provides an imperfect conversion from UTF-16 to UTF-32, ignoring all
// surrogate pairs.

#if defined(__linux__) || \
    defined(SOLARIS) || defined(__SVR4) || defined(UNIXWARE) || \
    defined(_decunix_)
#define UTF16TO32

// ------*---------*---------*---------*---------*---------*---------*---------

// Windows uses UTF-16 (or UCS-2 which is nearly equivalent), so no conversion
// is necessary.
#elif defined(WIN32)
#define NOCONVERSION

// ------*---------*---------*---------*---------*---------*---------*---------

#else
#error Platform not supported.
#endif

// ------*---------*---------*---------*---------*---------*---------*---------

#if defined(NOCONVERSION)
#include <cstring>

inline bool CompareJSchar(const jschar * x, const VXIchar * y)
{ return wcscmp(x, y) == 0; }

struct VXIcharToJschar {
  const jschar * c_str() const                 { return cstr; }
  size_t length() const                        { return wcslen(cstr); }
  VXIcharToJschar(const VXIchar * x) : cstr(x) { }
  ~VXIcharToJschar()                           { }

private:
  const jschar * cstr;
  VXIcharToJschar(const VXIcharToJschar &);
  VXIcharToJschar& operator=(const VXIcharToJschar &);
};

struct JscharToVXIchar {
  const VXIchar * c_str() const               { return cstr; }
  size_t length() const                       { return wcslen(cstr); }
  JscharToVXIchar(const jschar * x) : cstr(x) { }
  ~JscharToVXIchar() { }

private:
  const VXIchar * cstr;
  JscharToVXIchar(const JscharToVXIchar &);
  JscharToVXIchar& operator=(const JscharToVXIchar &);
};

#endif /* NOCONVERSION */

// ------*---------*---------*---------*---------*---------*---------*---------

#if defined(UTF16TO32)
#include <ostream>

inline bool CompareJSchar(const jschar * x, const VXIchar * y)
{
  if (x == NULL && y == NULL) return true;
  if (x == NULL && *y == '\0') return true;
  if (y == NULL && *x == '\0') return true;
  if (y == NULL || x == NULL) return false;

  while (*x && *y && VXIchar(*x) == *y) ++x, ++y;
  if (*x || *y) return false;
  return true;
}


struct VXIcharToJschar {
  const jschar * c_str() const { return cstr; }
  size_t length() const { return len; }

  VXIcharToJschar(const VXIchar * x) : cstr(NULL), len(0)
  {
    if (x == NULL) return;
    len = wcslen(x);
    cstr = new jschar[len + 1];
    if (cstr == NULL) return;
    for (unsigned int i = 0; i < len + 1; ++i)
      // We throw out any surrogate characters (0xD800 - 0xDFFF)
      cstr[i] = ((x[i] ^ 0xD800) < 0x0100) ? jschar(0xBF) : jschar(x[i]);
  }
  ~VXIcharToJschar() { if (cstr) delete [] cstr; }

private:
  size_t len;
  jschar * cstr;
  VXIcharToJschar(const VXIcharToJschar &);
  VXIcharToJschar& operator=(const VXIcharToJschar &);
};


struct JscharToVXIchar {
  const VXIchar * c_str() const { return cstr; }
  size_t length() const { return len; }

  JscharToVXIchar(const jschar * x) : cstr(NULL), len(0)
  {
    if (x == NULL) return;
    len = 0;
    while (x[len]) len++;
    cstr = new VXIchar[len + 1];
    if (cstr == NULL) return;
    
    cstr[0] = 0; 
    unsigned int i = 0;
    if (x[0] == 0xFEFF) ++i;
    for (unsigned int j = 0; i < len + 1; ++i, ++j)
      // We throw out anything above 0xFFFF
      cstr[j] = (x[i] != 0 && (x[i] & ~jschar(0xFFFF))) ? VXIchar(0xBE)
                                                       : VXIchar(x[i]);
  }
  ~JscharToVXIchar() {if (cstr) delete [] cstr; }

private:
  size_t len;
  VXIchar * cstr;
  JscharToVXIchar(const JscharToVXIchar &);
  JscharToVXIchar& operator=(const JscharToVXIchar &);
};

#endif /* UTF16TO32 */

#endif /* include guard */
