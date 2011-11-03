
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

#define VXIVALUE_EXPORTS
#include "VXIvalue.h"                    // Header for this function

#ifndef NO_STL

#include <stdio.h>                       // For sprintf( )
#include <string.h>                      // For strlen( )
#include <wchar.h>
#include <string>                        // For std::basic_string
#include <vector>
#include <iostream>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static inline char GetDecimalSeparator( ) {
  // Get the locale dependant decimal separator using a Win32 API call
  char sepBuf[32];
  return (GetLocaleInfoA (LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
			  sepBuf, 32) > 0 ? sepBuf[0] : '\0');
}
#else
#include <locale.h>

static inline char GetDecimalSeparator( ) {
  // Get the locale dependant decimal separator using a POSIX call
  struct lconv *info = localeconv( );
  return (info ? info->decimal_point[0] : '\0');
}
#endif

// Definitions of VXIchar based C++ string types
typedef std::basic_string<VXIchar> myString;
#ifdef WIN32
typedef std::basic_string<VXIbyte> myUTF8String;
#else
// Portability: stl on UNIX is not always portable, therefore using
// std::basic_string<unsigned char> is not always compiled and linked.
// The simple solution for this issue is to represent unsigned char string 
// as a vector
typedef std::vector<VXIbyte> myUTF8String;
#endif

// Prototypes for local functions
static void appendValue(myString &result,
			const VXIValue *value,
			const VXIchar *fieldName);

// These functions ensure that the string has enough capacity to hold the extra characters.
// To ensure amortized linear time, we reserve twice as many bytes as required.
template <class T> static void ensureCapacity(std::basic_string<T>& buffer,
                                              typename std::basic_string<T>::size_type extraChars)
{
  typename std::basic_string<T>::size_type requiredCapacity = buffer.length() + extraChars;
  if (requiredCapacity > buffer.capacity())
  {
    buffer.reserve(2 * requiredCapacity);
  }
}

#ifndef WIN32
static void ensureCapacity(myUTF8String& buffer, unsigned int extraChars)
{
  unsigned int requiredCapacity = buffer.size() + extraChars;
  if (requiredCapacity > buffer.capacity())
  {
    buffer.reserve(2 * requiredCapacity);
  }
}
#endif

//
// VXIchar string to Unicode UTF-8 string conversion
//
static bool convertToUTF8(const VXIchar *input,
			  myUTF8String &output)
{
  //  firstByteMark
  //      A list of values to mask onto the first byte of an encoded sequence,
  //      indexed by the number of bytes used to create the sequence.
  static const char firstByteMark[7] =
    {  char(0x00), char(0x00), char(0xC0), char(0xE0),
       char(0xF0), char(0xF8), char(0xFC) };

  //  Get pointers to our start and end points of the input buffer.
  const VXIchar* srcPtr = input;
  const VXIchar* srcEnd = srcPtr + wcslen(input);
  output.resize (0);

  while (srcPtr < srcEnd) {
    VXIchar curVal = *srcPtr++;

    // Watchout for surrogates, if found truncate
    if ((curVal >= 0xD800) && (curVal <= 0xDBFF)) {
      break;
    }

    // Figure out how many bytes we need
    unsigned int encodedBytes;
    if (curVal < 0x80)                encodedBytes = 1;
    else if (curVal < 0x800)          encodedBytes = 2;
    else if (curVal < 0x10000)        encodedBytes = 3;
    else if (curVal < 0x200000)       encodedBytes = 4;
    else if (curVal < 0x4000000)      encodedBytes = 5;
    else if (curVal <= 0x7FFFFFFF)    encodedBytes = 6;
    else {
      // THIS SHOULD NOT HAPPEN!
      return false;
    }

    //  And spit out the bytes. We spit them out in reverse order
    //  here, so bump up the output pointer and work down as we go.
    char buffer[7] = { 0, 0, 0, 0, 0, 0, 0 };

    char * outPtr = buffer + encodedBytes;
    switch(encodedBytes) {
    case 6 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 5 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 4 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 3 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 2 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 1 : *--outPtr = char(curVal | firstByteMark[encodedBytes]);
    }

    ensureCapacity(output, encodedBytes);
    for (int i = 0; buffer[i] != 0; i++)
    {
#ifdef WIN32
      output += buffer[i];
#else
      output.push_back( buffer[i] );
#endif      
    }
  }
  return true;
}


//
// Append escaped binary data
//
static void appendEscapedData(myString &result,
			      const VXIbyte *data,
			      VXIulong size)
{
  static const unsigned char isAcceptable[96] =
    /*0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xA 0xB 0xC 0xD 0xE 0xF */
    {
      /* 2x  !"#$%&'()*+,-./   */
      0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xF,0xE,0x0,0xF,0xF,0xC,
      /* 3x 0123456789:;<=>?   */
      0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0x8,0x0,0x0,0x0,0x0,0x0,
      /* 4x @ABCDEFGHIJKLMNO   */
      0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,
      /* 5X PQRSTUVWXYZ[\]^_   */
      0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0x0,0x0,0x0,0x0,0xF,
      /* 6x `abcdefghijklmno   */
      0x0,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,
      /* 7X pqrstuvwxyz{\}~DEL */
      0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0x0,0x0,0x0,0x0,0x0
    };

  static const VXIchar *hexChar = L"0123456789ABCDEF";
  static const int mask = 0x1;

  if ( ! data)
    return;

  VXIulong i;
  const VXIbyte *p;
  for (i = 0, p = data; i < size; i++, p++)
  {
    VXIbyte a = *p;
    if (a < 32 || a >= 128 || !(isAcceptable[a-32] & mask))
    {
      ensureCapacity(result, 3);
      result += '%';
      result += hexChar[a >> 4];
      result += hexChar[a & 15];
    }
    else
    {
      ensureCapacity(result, 1);
      result += *p;
    }
  }
}


//
// Append a string. Resulting byte sequence is a URL encoded Unicode
// UTF-8 string, meaning that the ASCII character set appears as
// normally done for URL encoded data, and any Latin-1 or Unicode
// characters will be translated into a URL encoded UTF-8 byte
// sequence where each character consumes 2 to 18 bytes (2 or more
// bytes in the UTF-8 representation where each of those bytes is
// encoded as 1 to 3 bytes depending on whether they need to get
// escaped for URL encoding).
//
static void appendEscapedString(myString &result,
				const VXIchar *str)
{
  if (!str)
    return;

  // Convert to UTF-8
  myUTF8String utf8;
  utf8.reserve (wcslen(str) * 4); // For efficiency, pre-allocate buffer
  if (! convertToUTF8 (str, utf8))
    return;
    
  // Now URL encode the bytes
#ifdef WIN32
  appendEscapedData (result, utf8.c_str(), utf8.length());
#else
  appendEscapedData (result, &utf8[0], utf8.size());  
#endif
}


static void appendKeyValuePair(myString &result,
			       const VXIchar *key,
			       const VXIchar *value)
{
  if ((key) && (value) && (*key))
  {
    if (result.length() > 0)
    {
      ensureCapacity(result, 1);
      result += L'&';
    }

    appendEscapedString(result, key);
    ensureCapacity(result, 1);
    result += L'=';

    if (*value)
      appendEscapedString(result, value);
  }
}


static void appendKeyValuePair(myString &result,
			       const VXIchar *key,
			       char *value)
{
  if ((key) && (value) && (*key))
  {
    if (result.length() > 0)
    {
      ensureCapacity(result, 1);
      result += L'&';
    }

    appendEscapedString(result, key);
    ensureCapacity(result, 1);
    result += L'=';
    
    if (*value)
      appendEscapedData (result,
			 reinterpret_cast<const unsigned char *>(value),
			 strlen(value));
  }
}


static void appendVector(myString &result,
			 const VXIVector *vxivector,
			 const VXIchar *fieldName)
{
  const VXIValue *value = NULL;

  VXIunsigned vectorLen = VXIVectorLength(vxivector);
  VXIunsigned i;

  for (i = 0; i < vectorLen; i++)
  {
    value = VXIVectorGetElement(vxivector, i);
    if ( value )
    {
      // sprintf( ) is more portable then swprintf( )
      char intStr[64];
      sprintf(intStr, "%d", i);
      myString fieldNameWithIndex(fieldName);
      if (! fieldNameWithIndex.empty())
        fieldNameWithIndex += L'.';
      for (const char *ptr = intStr; *ptr; ptr++)
        fieldNameWithIndex += static_cast<VXIchar>(*ptr);
      appendValue(result, value, fieldNameWithIndex.c_str());
    }
  }
}


static void appendMap(myString &result,
		      const VXIMap *vximap,
		      const VXIchar *fieldName)
{
  const VXIchar  *key = NULL;
  const VXIValue *value = NULL;

  VXIMapIterator *mapIterator = VXIMapGetFirstProperty(vximap, &key, &value);
  do
  {
    if ((key) && (value))
    {
      myString fieldNameWithIndex (fieldName);
      if (! fieldNameWithIndex.empty())
        fieldNameWithIndex += L'.';
      fieldNameWithIndex += key;

      appendValue(result, value, fieldNameWithIndex.c_str());
    }
  } while (VXIMapGetNextProperty(mapIterator, &key, &value) ==
           VXIvalue_RESULT_SUCCESS);

  VXIMapIteratorDestroy(&mapIterator);
}


static void appendContent(myString &result,
			  const VXIContent *vxicontent,
			  const VXIchar *fieldName)
{
  const VXIchar *type;
  const VXIbyte *data;
  VXIulong size;

  VXIContentValue(vxicontent, &type, &data, &size);

  if (result.length() > 0)
    result += L'&';

  appendEscapedString(result, fieldName);
  ensureCapacity(result, 1);
  result += L'=';
  appendEscapedData(result, data, size);
}


static void appendValue(myString &result,
			const VXIValue *value,
			const VXIchar *fieldName)
{
  char valueStr[128];
  
  switch (VXIValueGetType(value)) {
  case VALUE_MAP:
    // nested object
    // append the object name to the field name prefix
    appendMap(result, (const VXIMap *) value, fieldName);
    break;
  case VALUE_VECTOR:
    // nested vector
    // append the vector name to the field name prefix
    appendVector(result, (const VXIVector *) value, fieldName);
    break;
  case VALUE_CONTENT:
    appendContent(result, (const VXIContent *) value, fieldName);
    break;
  case VALUE_BOOLEAN:
    if (VXIBooleanValue((const VXIBoolean *) value) == TRUE)
      appendKeyValuePair(result, fieldName, L"true");
    else
      appendKeyValuePair(result, fieldName, L"false");
    break;
  case VALUE_INTEGER:
    // sprintf( ) is more portable then swprintf( )
    sprintf(valueStr, "%d", VXIIntegerValue((const VXIInteger *) value));
    appendKeyValuePair(result, fieldName, valueStr);
    break;
  case VALUE_ULONG:
    // sprintf( ) is more portable then swprintf( )
    sprintf(valueStr, "%u", (unsigned int) VXIULongValue((const VXIULong *) value));
    appendKeyValuePair(result, fieldName, valueStr);
    break;
  case VALUE_LONG:
    // sprintf( ) is more portable then swprintf( )
    sprintf(valueStr, "%d", (unsigned int) VXILongValue((const VXILong *) value));
    appendKeyValuePair(result, fieldName, valueStr);
    break;
  case VALUE_FLOAT:
    {
      // sprintf( ) is more portable then swprintf( ), but have to
      // watch for locale dependance where the integer and fractional
      // parts may be separated by a comma in some locales
      sprintf(valueStr, "%#.6g",
	      (double) VXIFloatValue((const VXIFloat *) value));
      char sep = GetDecimalSeparator( );
      // printf("decimalSeparator = '%c'\n", (sep ? sep : 'e'));
      if ((sep) && (sep != '.')) {
        char *ptr = strchr(valueStr, sep);
        if (ptr)
          *ptr = '.';
      }
      appendKeyValuePair(result, fieldName, valueStr);
    }
    break;
  case VALUE_DOUBLE:
    {
      // sprintf( ) is more portable then swprintf( ), but have to
      // watch for locale dependance where the integer and fractional
      // parts may be separated by a comma in some locales
      sprintf(valueStr, "%#.6g",
	      (double) VXIDoubleValue((const VXIDouble *) value));
      char sep = GetDecimalSeparator( );
      // printf("decimalSeparator = '%c'\n", (sep ? sep : 'e'));
      if ((sep) && (sep != '.')) {
        char *ptr = strchr(valueStr, sep);
        if (ptr)
          *ptr = '.';
      }
      appendKeyValuePair(result, fieldName, valueStr);
    }
    break;
  case VALUE_STRING:
    appendKeyValuePair(result, fieldName,
		       VXIStringCStr((const VXIString *) value));
    break;
  case VALUE_PTR:
    {
      // Can't rely on the C library to give consistant enough results.
      // sprintf( ) is more portable then swprintf( )
      sprintf(valueStr, "%p", VXIPtrValue((const VXIPtr *) value));
      myString finalValue;
      if ( strncmp(valueStr, "0x", 2) != 0 )
	finalValue += L"0x";
      for (const char *ptr = valueStr; *ptr; ptr++) {
	switch (*ptr) {
	case 'a': finalValue += L'A'; break;
	case 'b': finalValue += L'B'; break;
	case 'c': finalValue += L'C'; break;
	case 'd': finalValue += L'D'; break;
	case 'e': finalValue += L'E'; break;
	case 'f': finalValue += L'F'; break;
	default:  finalValue += static_cast<VXIchar>(*ptr);
	}
      }
      appendKeyValuePair(result, fieldName, finalValue.c_str());
    }
    break;
  default:
    appendKeyValuePair(result, fieldName, L"errorInvalidType");
    break;
  }
}


#endif /* #ifndef NO_STL */


/**
 * Generic Value to string conversion
 *
 * This converts any VXIValue type to a string.
 *
 * @param   v       Value to convert to a string
 * @param   name    Name to use for labeling the VXIValue data
 * @param   format  Format to use for the string, see above
 * @return          VXIString, NULL on error
 */
VXIVALUE_API VXIString *VXIValueToString(const VXIValue      *v,
					 const VXIchar       *name,
					 VXIValueStringFormat format)
{
  if (( ! v ) || ( ! name ))
    return NULL;

#ifdef NO_STL
  return NULL;
#else
  myString str;
  str.reserve(1024); // For efficiency, preallocate 1024 char buffer
  appendValue (str, v, name);
  return (str.length( ) > 0 ? VXIStringCreate (str.c_str()) : NULL);
#endif
}
