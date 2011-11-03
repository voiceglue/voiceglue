
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

#ifndef __XMLChar_Converter_HPP__
#define __XMLChar_Converter_HPP__

#include "VXItypes.h"                 // for VXIchar

#ifndef HAVE_XERCES
#error Need Apache Xerces to build the VoiceXML interpreter
#endif

#include <util/XercesDefs.hpp>        // for XMLCh
#include <util/XMLString.hpp>         // for XMLString
using namespace xercesc;

// Xerces specifies that XMLCh must be able to store UTF-16 characters.
// VXIchar should be the general wide character representation (wchar_t) of the
// platform.  As wchar_t may be other types, conversion functions are
// necessary.

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
#elif defined(XML_WIN32)
#define NOCONVERSION

// ------*---------*---------*---------*---------*---------*---------*---------

#else
#error Platform not supported.
#endif

// ------*---------*---------*---------*---------*---------*---------*---------

#if defined(NOCONVERSION)
#include <cstring>

inline bool Compare(const XMLCh * x, const VXIchar * y)
{ return wcscmp(x, y) == 0; }

struct VXIcharToXMLCh {
  const XMLCh * c_str() const                 { return cstr; }
  VXIcharToXMLCh(const VXIchar * x) : cstr(x) { if (cstr != NULL && cstr[0] == 0xFEFF) ++cstr; }
  ~VXIcharToXMLCh()                           { }

private:
  const XMLCh * cstr;
  VXIcharToXMLCh(const VXIcharToXMLCh &);
  VXIcharToXMLCh& operator=(const VXIcharToXMLCh &);
};

struct XMLChToVXIchar {
  const VXIchar * c_str() const             { return cstr; }
  XMLChToVXIchar(const XMLCh * x) : cstr(x) { }
  ~XMLChToVXIchar() { }

private:
  const VXIchar * cstr;
  XMLChToVXIchar(const XMLChToVXIchar &);
  XMLChToVXIchar& operator=(const XMLChToVXIchar &);
};

#endif /* NOCONVERSION */

// ------*---------*---------*---------*---------*---------*---------*---------

#if defined(UTF16TO32)
#include <ostream>

inline bool Compare(const XMLCh * x, const VXIchar * y)
{
  if (x == NULL && y == NULL) return true;
  if (x == NULL && *y == '\0') return true;
  if (y == NULL && *x == '\0') return true;
  if (y == NULL || x == NULL) return false;

  while (*x && *y && VXIchar(*x) == *y) ++x, ++y;
  if (*x || *y) return false;
  return true;
}


struct VXIcharToXMLCh {
  const XMLCh * c_str() const { return cstr; }

  VXIcharToXMLCh(const VXIchar * x) : cstr(NULL)
  {
    if (x == NULL) return;
    unsigned int len = wcslen(x) + 1;
    cstr = new XMLCh[len];
    if (cstr == NULL) return;
    for (unsigned int i = 0; i < len; ++i)
      // We throw out any surrogate characters (0xD800 - 0xDFFF)
      cstr[i] = ((x[i] ^ 0xD800) < 0x0100) ? XMLCh(0xBF) : XMLCh(x[i]);
  }
  ~VXIcharToXMLCh() { delete [] cstr; }

private:
  XMLCh * cstr;
  VXIcharToXMLCh(const VXIcharToXMLCh &);
  VXIcharToXMLCh& operator=(const VXIcharToXMLCh &);
};


struct XMLChToVXIchar {
  const VXIchar * c_str() const { return cstr; }

  XMLChToVXIchar(const XMLCh * x) : cstr(NULL)
  {
    if (x == NULL) return;
    unsigned int len = XMLString::stringLen(x) + 1;
    cstr = new VXIchar[len];
    if (cstr == NULL) return;
    
    unsigned int i = 0;
    if (x[0] == 0xFEFF) ++i;
    for (unsigned j = 0; i < len; ++i, ++j)
      // We throw out anything above 0xFFFF
      cstr[j] = (x[i] != 0 && (x[i] & ~XMLCh(0xFFFF))) ? VXIchar(0xBE)
                                                       : VXIchar(x[i]);
  }
  ~XMLChToVXIchar() { delete [] cstr; }

private:
  VXIchar * cstr;
  XMLChToVXIchar(const XMLChToVXIchar &);
  XMLChToVXIchar& operator=(const XMLChToVXIchar &);
};

#endif /* UTF16TO32 */

// ------*---------*---------*---------*---------*---------*---------*---------

inline std::basic_ostream<VXIchar>& operator<<(std::basic_ostream<VXIchar>& os,
					       const XMLChToVXIchar & val)
{ return os << val.c_str(); }

class xmlcharstring {
public:
  // constructors
  xmlcharstring(const XMLCh* s) : data(NULL), len(0) {
	  storeit(s);
  }
  xmlcharstring() : data(NULL), len(0) {}
  xmlcharstring(const xmlcharstring &x) : data(NULL), len(0) {
    if( !x.empty() ) storeit(x.c_str());
  }
  
  // destructor
  ~xmlcharstring() {
    clear();
  }
  
  // members
  bool empty(void) const { return (len == 0); }
  size_t length(void) const { return len; }
  const XMLCh* c_str(void) const { return (data ? data : (const XMLCh*)""); }
  void clear(void) {
    if( data ) ::free(data);
    data = NULL;
    len = 0;
  }   
  const XMLCh operator[](unsigned int i) const { return data[i]; }    
    
  // copy assignment
  xmlcharstring & operator=(const XMLCh* s) {
    clear();
    storeit(s);
    return *this;     
  }
  xmlcharstring & operator=(const xmlcharstring &x) {
    if( this != &x ) {
      clear();
      if( !x.empty() ) storeit(x.c_str());
    }
    return *this;
  }
  
  // operators
  xmlcharstring & operator+(const XMLCh* s) {
    storeit(s, true); // store and preserve old content
    return *this;     
  }  
  xmlcharstring & operator+=(const XMLCh* s) {
    storeit(s, true);
    return *this;
  } 
  xmlcharstring & operator+(const xmlcharstring & x) {
    if( !x.empty() ) storeit(x.c_str(), true);
    return *this;
  } 
  xmlcharstring & operator+=(const xmlcharstring & x) {
    if( !x.empty() ) storeit(x.c_str(), true);
    return *this;
  } 
  
  // comparison
  bool operator==(const XMLCh* s) {
    return (XMLString::compareString(data, s) == 0);
  }
  bool operator==(const xmlcharstring & x) {
    return operator==(x.c_str());
  }
  bool operator>(const XMLCh* s) {
    return (XMLString::compareString(data, s) > 0);
  }
  bool operator>(const xmlcharstring & x) {
    return operator>(x.c_str());
  }
  bool operator<(const XMLCh* s) {
    return !operator>(s);
  }
  bool operator<(const xmlcharstring & x) {
    return !operator>(x);
  }
  
private:
  void storeit(const XMLCh* s, bool preserve = false) {
    if( s ) {
      if( !preserve ) {
        clear();
        len = XMLString::stringLen(s);
        data = (XMLCh*)::malloc(sizeof(XMLCh)*(len+1)); 
        XMLString::copyString(data, s);        
      }
      else {
        size_t plen = len;
        len += XMLString::stringLen(s);
        XMLCh* ptr = (XMLCh*)::realloc(data, sizeof(XMLCh)*(len+1)); 
        data = ptr;
        XMLString::copyString(&data[plen], s);        
      }
    }
  }
  
private:
  XMLCh* data;
  size_t len;     
};

#endif
